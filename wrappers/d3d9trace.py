##########################################################################
#
# Copyright 2008-2009 VMware, Inc.
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
##########################################################################/


from dlltrace import DllTracer
from specs.d3d9 import d3d9, D3DSHADER9


class D3D9Tracer(DllTracer):

    def serializeArgValue(self, function, arg):
        # Dump shaders as strings
        if arg.type is D3DSHADER9:
            print '    DumpShader(trace::localWriter, %s);' % (arg.name)
            return

        DllTracer.serializeArgValue(self, function, arg)

    def enumWrapperInterfaceVariables(self, interface):
        variables = DllTracer.enumWrapperInterfaceVariables(self, interface)
        
        if interface.getMethodByName('Lock') is not None or \
           interface.getMethodByName('LockRect') is not None or \
           interface.getMethodByName('LockBox') is not None:
            variables += [
                ('size_t', '_LockedSize', '0'),
                ('VOID *', 'm_pbData', '0'),
            ]

        return variables

    def implementWrapperInterfaceMethodBody(self, interface, base, method):
        if method.name in ('Unlock', 'UnlockRect', 'UnlockBox'):
            print '    if (_LockedSize && m_pbData) {'
            self.emit_memcpy('(LPBYTE)m_pbData', '(LPBYTE)m_pbData', '_LockedSize')
            print '    }'

        DllTracer.implementWrapperInterfaceMethodBody(self, interface, base, method)

        if method.name in ('Lock', 'LockRect', 'LockBox'):
            # FIXME: handle recursive locks
            print '    if (SUCCEEDED(_result) && !(Flags & D3DLOCK_READONLY)) {'
            print '        _getLockInfo(_this, %s, m_pbData, _LockedSize);' % ', '.join(method.argNames()[:-1])
            print '    } else {'
            print '        m_pbData = NULL;'
            print '        _LockedSize = 0;'
            print '    }'


if __name__ == '__main__':
    print '#define INITGUID'
    print
    print '#include "trace_writer_local.hpp"'
    print '#include "os.hpp"'
    print
    print '#include "d3d9imports.hpp"'
    print '#include "d3dsize.hpp"'
    print '#include "d3d9shader.hpp"'
    print
    print '''
static inline size_t
_declCount(const D3DVERTEXELEMENT9 *pVertexElements) {
    size_t count = 0;
    if (pVertexElements) {
        while (pVertexElements[count++].Stream != 0xff)
            ;
    }
    return count;
}
'''
    tracer = D3D9Tracer('d3d9.dll')
    tracer.traceApi(d3d9)

