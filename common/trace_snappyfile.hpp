/**************************************************************************
 *
 * Copyright 2011 Zack Rusin
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


#ifndef TRACE_SNAPPYFILE_HPP
#define TRACE_SNAPPYFILE_HPP

#include <assert.h>

#include "trace_file.hpp"

#include <string>
#include <fstream>

namespace snappy {
    class File;
}

namespace Trace {

#define SNAPPY_CHUNK_SIZE (1 * 1024 * 1024)

#define SNAPPY_BYTE1 'a'
#define SNAPPY_BYTE2 't'


class SnappyFile : public File {
public:
    SnappyFile(const std::string &filename = std::string(),
               File::Mode mode = File::Read);
    virtual ~SnappyFile();

    virtual bool supportsOffsets() const;
    virtual File::Offset currentOffset();
    virtual void setCurrentOffset(const File::Offset &offset);
protected:
    virtual bool rawOpen(const std::string &filename, File::Mode mode);
    virtual bool rawWrite(const void *buffer, size_t length);
    virtual bool rawRead(void *buffer, size_t length);
    virtual int rawGetc();
    virtual void rawClose();
    virtual void rawFlush();
    virtual bool rawSkip(size_t length);
    virtual int rawPercentRead();

private:
    inline size_t usedCacheSize() const
    {
        assert(m_cachePtr >= m_cache);
        return m_cachePtr - m_cache;
    }
    inline size_t freeCacheSize() const
    {
        assert(m_cacheSize >= usedCacheSize());
        if (m_cacheSize > 0) {
            return m_cacheSize - usedCacheSize();
        } else {
            return 0;
        }
    }
    inline bool endOfData() const
    {
        return m_stream.eof() && freeCacheSize() == 0;
    }
    void flushWriteCache();
    void flushReadCache(size_t skipLength = 0);
    void createCache(size_t size);
    void writeCompressedLength(size_t length);
    size_t readCompressedLength();
private:
    std::fstream m_stream;
    char *m_cache;
    char *m_cachePtr;
    size_t m_cacheSize;

    char *m_compressedCache;

    File::Offset m_currentOffset;
    std::streampos m_endPos;
};

}

#endif // TRACE_SNAPPYFILE_HPP
