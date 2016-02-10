/**************************************************************************
 *
 * Copyright 2010-2011 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

#ifndef _WIN32

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <signal.h>

#ifdef __linux__
#include <linux/limits.h> // PATH_MAX
#include <malloc.h>
#endif /* __linux__ */

#ifdef __APPLE__
#include <sys/syslimits.h> // PATH_MAX
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld.h>
#endif /* __APPLE__ */

#ifdef ANDROID
#include <android/log.h>
#endif

#ifndef PATH_MAX
#warning PATH_MAX undefined
#define PATH_MAX 4096
#endif

#ifdef __QNXNTO__
#define SA_RESTART 0 // QNX does not have SA_RESTART
#endif

#include "os.hpp"
#include "os_string.hpp"
#include "os_backtrace.hpp"


namespace os {


String
getProcessName(void)
{
    String path;
    size_t size = PATH_MAX;
    char *buf = path.buf(size);

    // http://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe
#ifdef __APPLE__
    uint32_t len = size;
    if (_NSGetExecutablePath(buf, &len) != 0) {
        // grow buf and retry
        buf = path.buf(len);
        _NSGetExecutablePath(buf, &len);
    }
    len = strlen(buf);
#else
    ssize_t len;

#ifdef ANDROID
    // On Android, we are almost always interested in the actual process title
    // rather than path to the VM kick-off executable
    // (/system/bin/app_process).
    len = 0;
#else
    len = readlink("/proc/self/exe", buf, size - 1);
#endif

    if (len <= 0) {
        // /proc/self/exe is not available on setuid processes, so fallback to
        // /proc/self/cmdline.
        int fd = open("/proc/self/cmdline", O_RDONLY);
        if (fd >= 0) {
            // buf already includes trailing zero
            len = read(fd, buf, size);
            close(fd);
            if (len >= 0) {
                len = strlen(buf);
            }
        }
    }

#ifdef __GLIBC__
    // fallback to `program_invocation_name`
    if (len <= 0) {
        len = strlen(program_invocation_name);
        buf = path.buf(len + 1);
        strcpy(buf, program_invocation_name);
    }
#endif

    // fallback to process ID
    if (len <= 0) {
        len = snprintf(buf, size, "%i", (int)getpid());
        if (len >= size) {
            len = size - 1;
        }
    }
#endif
    path.truncate(len);

    return path;
}

String
getCurrentDir(void)
{
    String path;
    size_t size = PATH_MAX;
    char *buf = path.buf(size);

    if (getcwd(buf, size)) {
        buf[size - 1] = 0;
        path.truncate();
    } else {
        path.truncate(0);
    }
    
    return path;
}

String
getConfigDir(void)
{
    String path;

#ifdef __APPLE__
    // Library/Preferences
    const char *homeDir = getenv("HOME");
    assert(homeDir);
    if (homeDir) {
        path = homeDir;
        path.join("Library/Preferences");
    }
#else
    // http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
    const char *configHomeDir = getenv("XDG_CONFIG_HOME");
    if (configHomeDir) {
        path = configHomeDir;
    } else {
        const char *homeDir = getenv("HOME");
        if (!homeDir) {
            struct passwd *user = getpwuid(getuid());
            if (user != NULL) {
                homeDir = user->pw_dir;
            }
        }
        assert(homeDir);
        if (homeDir) {
            path = homeDir;
#if !defined(ANDROID)
            path.join(".config");
#endif
        }
    }
#endif

    return path;
}

bool
createDirectory(const String &path)
{
    return mkdir(path, 0777) == 0;
}

bool
String::exists(void) const
{
    struct stat st;
    int err;

    err = stat(str(), &st);
    if (err) {
        return false;
    }

    return true;
}

int execute(char * const * args)
{
    pid_t pid = fork();
    if (pid == 0) {
        // child
        execvp(args[0], args);
        fprintf(stderr, "error: failed to execute:");
        for (unsigned i = 0; args[i]; ++i) {
            fprintf(stderr, " %s", args[i]);
        }
        fprintf(stderr, "\n");
        exit(-1);
    } else {
        // parent
        if (pid == -1) {
            fprintf(stderr, "error: failed to fork\n");
            return -1;
        }
        int status = -1;
        int ret;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            ret = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            // match shell return code
            ret = WTERMSIG(status) + 128;
        } else {
            ret = 128;
        }
        return ret;
    }
}

static volatile bool logging = false;

#ifndef HAVE_EXTERNAL_OS_LOG
void
log(const char *format, ...)
{
    logging = true;
    va_list ap;
    va_start(ap, format);
    fflush(stdout);
#ifdef ANDROID
    __android_log_vprint(ANDROID_LOG_DEBUG, "apitrace", format, ap);
#else
    static FILE *log = NULL;
    if (!log) {
        // Duplicate stderr file descriptor, to prevent applications from
        // redirecting our debug messages to somewhere else.
        //
        // Another alternative would be to log to /dev/tty when available.
        log = fdopen(dup(STDERR_FILENO), "at");
    }
    vfprintf(log, format, ap);
    fflush(log);
#endif
    va_end(ap);
    logging = false;
}
#endif /* !HAVE_EXTERNAL_OS_LOG */

#if defined(__APPLE__)
long long timeFrequency = 0LL;
#endif

void
abort(void)
{
    _exit(1);
}


void
breakpoint(void)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    asm("int3");
#else
    kill(getpid(), SIGTRAP);
#endif
}


static void (*gCallback)(void) = NULL;

#define NUM_SIGNALS 16

struct sigaction old_actions[NUM_SIGNALS];


/*
 * See also:
 * - http://sourceware.org/git/?p=glibc.git;a=blob;f=debug/segfault.c
 * - http://ggi.cvs.sourceforge.net/viewvc/ggi/ggi-core/libgg/gg/cleanup.c?view=markup
 */
static void
signalHandler(int sig, siginfo_t *info, void *context)
{
    /*
     * There are several signals that can happen when logging to stdout/stderr.
     * For example, SIGPIPE will be emitted if stderr is a pipe with no
     * readers.  Therefore ignore any signal while logging by returning
     * immediately, to prevent deadlocks.
     */
    if (logging) {
        return;
    }

    static int recursion_count = 0;

    log("apitrace: warning: caught signal %i\n", sig);

    if (recursion_count) {
        log("apitrace: warning: recursion handling signal %i\n", sig);
    } else {
        ++recursion_count;
        if (gCallback)
            gCallback();
        os::dump_backtrace();
        --recursion_count;
    }

    struct sigaction *old_action;
    if (sig >= NUM_SIGNALS) {
        /* This should never happen */
        log("error: unexpected signal %i\n", sig);
        raise(SIGKILL);
    }
    old_action = &old_actions[sig];

    if (old_action->sa_flags & SA_SIGINFO) {
        // Handler is in sa_sigaction
        old_action->sa_sigaction(sig, info, context);
    } else {
        if (old_action->sa_handler == SIG_DFL) {
            log("apitrace: info: taking default action for signal %i\n", sig);

#if 1
            struct sigaction dfl_action;
            dfl_action.sa_handler = SIG_DFL;
            sigemptyset (&dfl_action.sa_mask);
            dfl_action.sa_flags = 0;
            sigaction(sig, &dfl_action, NULL);

            raise(sig);
#else
            raise(SIGKILL);
#endif
        } else if (old_action->sa_handler == SIG_IGN) {
            /* ignore */
        } else {
            /* dispatch to handler */
            old_action->sa_handler(sig);
        }
    }
}

void
setExceptionCallback(void (*callback)(void))
{
    assert(!gCallback);
    if (!gCallback) {
        gCallback = callback;

        struct sigaction new_action;
        new_action.sa_sigaction = signalHandler;
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = SA_SIGINFO | SA_RESTART;


        for (int sig = 1; sig < NUM_SIGNALS; ++sig) {
            // SIGKILL and SIGSTOP can't be handled.
            if (sig == SIGKILL || sig == SIGSTOP) {
                continue;
            }

            /*
             * SIGPIPE can be emitted when writing to stderr that is redirected
             * to a pipe without readers.  It is also very unlikely to ocurr
             * inside graphics APIs, and most applications where it can occur
             * normally already ignore it.  In summary, it is unlikely that a
             * SIGPIPE will cause abnormal termination, which it is likely that
             * intercepting here will cause problems, so simple don't intercept
             * it here.
             */
            if (sig == SIGPIPE) {
                continue;
            }

            if (sigaction(sig,  NULL, &old_actions[sig]) >= 0) {
                sigaction(sig,  &new_action, NULL);
            }
        }
    }
}

void
resetExceptionCallback(void)
{
    gCallback = NULL;
}

#ifdef __linux__

struct MapRegion {
    uintptr_t start;
    uintptr_t stop;
    bool read;
    bool write;
    bool execute;
    bool shared;
};


/**
 * Reader for Linux's /proc/[pid]/maps.
 *
 * See also:
 * - http://www.kernel.org/doc/Documentation/filesystems/proc.txt
 */
class MapsReader
{
protected:
    int fd;
    char buf[512];
    int pos;
    int len;
    char lookahead;

    /**
     * Read some data into buf.
     */
    void slurp(void) {
        pos = 0;
        len = 0;
        lookahead = 0;

        if (fd >= 0) {
            ssize_t nread = read(fd, buf, sizeof buf);
            if (nread > 0) {
                len = nread;
                lookahead = buf[0];
            }
        }
    }

    /**
     * Consume one byte.
     */
    inline void consume(void) {
        ++pos;
        if (pos < len) {
            lookahead = buf[pos];
        } else {
            slurp();
        }
    }

public:
    MapsReader() {
        fd = open("/proc/self/maps", O_RDONLY);
        slurp();
    }

    ~MapsReader() {
        close(fd);
    }

    bool read_region(MapRegion &region) {
        if (lookahead == 0) {
            // EOF
            return false;
        }

        // address range
        if (!read_hex(region.start)) {
            return false;
        }
        if (lookahead != '-') {
            return false;
        }
        consume();
        if (!read_hex(region.stop)) {
            return false;
        }

        if (!read_space()) {
            return false;
        }

        // permissions
        if (!read_flag('r', '-', region.read)) {
            return false;
        }
        if (!read_flag('w', '-', region.write)) {
            return false;
        }
        if (!read_flag('x', '-', region.execute)) {
            return false;
        }
        if (!read_flag('s', 'p', region.shared)) {
            return false;
        }

        if (!read_space()) {
            return false;
        }

        // rest
        while (lookahead != '\n') {
            if (lookahead == 0) {
                return false;
            }
            consume();
        }
        consume();

        return true;
    }

protected:
    bool read_hex(uintptr_t &val) {
        bool success = false;
        val = 0;
        while (true) {
            if (lookahead >= '0' && lookahead <= '9') {
                val <<= 4;
                val |= lookahead - '0';
            } else if (lookahead >= 'A' && lookahead <= 'F') {
                val <<= 4;
                val |= lookahead - 'A' + 10;
            } else if (lookahead >= 'a' && lookahead <= 'f') {
                val <<= 4;
                val |= lookahead - 'a' + 10;
            } else {
                break;
            }
            consume();
            success = true;
        }
        return success;
    }

    bool read_flag(char t, char f, bool &flag) {
        if (lookahead == t) {
            flag = true;
            consume();
            return true;
        }

        if (lookahead == f) {
            flag = false;
            consume();
            return true;
        }

        return false;
    }

    inline bool read_space(void) {
        bool success = false;
        while (lookahead == ' ') {
            consume();
            success = true;
        }
        return success;
    }
};


/**
 * Data segment sub-break.
 *
 * glibc's malloc request memory from the kernel by changing the program break
 * to satisfy mallocs smaller than a certain size.  This means that the heap
 * region often varies in size (typically growing), which is very difficult to
 * replay.
 *
 * To avoid this, we malloc these sub-breaks which split the heap, such that
 * the address of interest are enclosed by them.  Therefore, we turn the
 * ever-growing heap region into a growing collection of disjoint and fixed
 * sized regions, at the expense of fragmenting the heap.
 */
union SubBreak
{
    SubBreak *next;

    /**
     * It must not be bigger than M_MMAP_THRESHOLD.
     */
    const char padding[1024];
};


/**
 * Singly-linked list of the sub-breaks.
 */
static SubBreak *subBreaks = NULL;


bool queryVirtualAddress(const void *address, MemoryInfo *info)
{
    MapsReader reader;
    MapRegion region;

    // Prevent the data segment from shrinking
    mallopt(M_TRIM_THRESHOLD, -1);

    // Use mmap more frequently
    mallopt(M_MMAP_THRESHOLD, 64*1024);

    void *brk = sbrk(0);

    while (reader.read_region(region)) {
        if (region.start <= (uintptr_t)address && (uintptr_t)address < region.stop) {
            // Found an enclosing region

            if (!region.read) {
                return false;
            }

            info->start = (const void *)region.start;
            info->stop  = (const void *)region.stop;

            // The 
            if (info->stop <= brk) {

                // Find a sub-break that encloses the address
                SubBreak *subBreak = subBreaks;
                while (subBreak) {
                    if (address <= (const void *)subBreak) {
                        break;
                        return true;
                    }

                    subBreak = subBreak->next;
                }

                if (!subBreak) {
                    // The address is higher than all existing sub-breaks

                    SubBreak *tmpBreaks = NULL;

                    // Keep allocating sub-breaks until reach an address that
                    // encloses the address
                    while (true) {
                        subBreak = (SubBreak *)malloc(sizeof *subBreak);
                        if (!subBreak) {
                            break;
                        }

                        if (address < (const void *)subBreak) {
                            break;
                        }

                        subBreak->next = tmpBreaks;
                        tmpBreaks = subBreak;
                    }

                    // Add the new sub-break to the list
                    if (subBreak) {
                        //DebugMessage("   subbreak = %p, break = %p\n", subBreak, sbrk(0));
                        subBreak->next = subBreaks;
                        subBreaks = subBreak;
                    }

                    // Free all temporary malloced memory below the address
                    while (tmpBreaks) {
                        SubBreak *nextTmpBreak = tmpBreaks->next;
                        free(tmpBreaks);
                        tmpBreaks = nextTmpBreak;
                    }
                }

                // Adjust the range to the enclosing sub-break
                if (subBreak) {
                    info->stop = (const void *)subBreak;
                    if (subBreak->next) {
                        info->start = (const void *)(subBreak + 1);
                    }
                    return true;
                }
            }

            // Concatenate the next region. Necessary because when program or
            // shared objects are loaded, the data segments are made part of
            // file mapping, part anynomous.
            while (reader.read_region(region) &&
                   region.read && !region.execute &&
                   region.start == (uintptr_t)info->stop) {
                info->stop  = (const void *)region.stop;
            }

            return true;
        }
    }

    return false;
}


#endif /* __linux__ */


#ifdef __APPLE__


bool queryVirtualAddress(const void *address, MemoryInfo *info)
{
    /**
     * http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/osfmk/man/vm_region.html?txt
     * http://www.gnu.org/software/hurd/gnumach-doc/Memory-Attributes.html#Memory-Attributes
     * http://stackoverflow.com/questions/1627998/retrieving-the-memory-map-of-its-own-process-in-os-x-10-5-10-6
     */

    vm_map_t task = mach_task_self();
    vm_address_t vmaddr = (vm_address_t)address;
    vm_size_t vmsize = 0;
    vm_region_flavor_t flavor = VM_REGION_BASIC_INFO_64;
    vm_region_basic_info_data_64_t vminfo;
    mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object_name;

    kern_return_t kr;
    kr = vm_region_64(task, &vmaddr, &vmsize, flavor,
                      (vm_region_info_t)&vminfo, &info_count, &object_name);
    if (kr != KERN_SUCCESS) {
        return false;
    }

    info->start = (const void *)vmaddr;
    info->stop  = (const void *)(vmaddr + vmsize);

    return true;
}


#endif /* __APPLE__ */

#ifdef __ANDROID__
#include "os_memory.hpp"
#include <cassert>
#include <cstdio>

#include <fcntl.h>
#include <unistd.h>

char statmBuff[256];
static __uint64_t pageSize = sysconf(_SC_PAGESIZE);

static long size, resident;

static inline void parseStatm()
{
    int fd = open("/proc/self/statm", O_RDONLY, 0);
    int sz = read(fd, statmBuff, 255);
    close(fd);
    statmBuff[sz] = 0;
    sz = sscanf(statmBuff, "%ld %ld",
               &size, &resident);
    assert(sz == 2);
}

long long getVsize()
{
    parseStatm();
    return pageSize * size;
}

long long getRss()
{
    parseStatm();
    return pageSize * resident;
}
#endif

} /* namespace os */

#endif // !defined(_WIN32)
