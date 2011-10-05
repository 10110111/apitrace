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


#include "trace_snappyfile.hpp"

#include <snappy.h>

#include <iostream>

#include <assert.h>
#include <string.h>

using namespace Trace;

/*
 * Snappy file format.
 * -------------------
 *
 * Snappy at its core is just a compressoin algorithm so we're
 * creating a new file format which uses snappy compression
 * to hold the trace data.
 *
 * The file is composed of a number of chunks, they are:
 * chunk {
 *     uint32 - specifying the length of the compressed data
 *     compressed data, in little endian
 * }
 * File can contain any number of such chunks.
 * The default size of an uncompressed chunk is specified in
 * SNAPPY_CHUNK_SIZE.
 *
 * Note:
 * Currently the default size for a a to-be-compressed data is
 * 1mb, meaning that the compressed data will be <= 1mb.
 * The reason it's 1mb is because it seems
 * to offer a pretty good compression/disk io speed ratio
 * but that might change.
 *
 */

SnappyFile::SnappyFile(const std::string &filename,
                              File::Mode mode)
    : File(),
      m_cache(0),
      m_cachePtr(0),
      m_cacheSize(0)
{
    size_t maxCompressedLength =
        snappy::MaxCompressedLength(SNAPPY_CHUNK_SIZE);
    m_compressedCache = new char[maxCompressedLength];
}

SnappyFile::~SnappyFile()
{
    delete [] m_compressedCache;
    delete [] m_cache;
}

bool SnappyFile::rawOpen(const std::string &filename, File::Mode mode)
{
    std::ios_base::openmode fmode = std::fstream::binary;
    if (mode == File::Write) {
        fmode |= (std::fstream::out | std::fstream::trunc);
        createCache(SNAPPY_CHUNK_SIZE);
    } else if (mode == File::Read) {
        fmode |= std::fstream::in;
    }

    m_stream.open(filename.c_str(), fmode);

    //read in the initial buffer if we're reading
    if (m_stream.is_open() && mode == File::Read) {
        m_stream.seekg(0, std::ios::end);
        m_endPos = m_stream.tellg();
        m_stream.seekg(0, std::ios::beg);

        // read the snappy file identifier
        unsigned char byte1, byte2;
        m_stream >> byte1;
        m_stream >> byte2;
        assert(byte1 == SNAPPY_BYTE1 && byte2 == SNAPPY_BYTE2);

        flushReadCache();
    } else if (m_stream.is_open() && mode == File::Write) {
        // write the snappy file identifier
        m_stream << SNAPPY_BYTE1;
        m_stream << SNAPPY_BYTE2;
    }
    return m_stream.is_open();
}

bool SnappyFile::rawWrite(const void *buffer, size_t length)
{
    if (freeCacheSize() > length) {
        memcpy(m_cachePtr, buffer, length);
        m_cachePtr += length;
    } else if (freeCacheSize() == length) {
        memcpy(m_cachePtr, buffer, length);
        m_cachePtr += length;
        flushWriteCache();
    } else {
        size_t sizeToWrite = length;

        while (sizeToWrite >= freeCacheSize()) {
            size_t endSize = freeCacheSize();
            size_t offset = length - sizeToWrite;
            memcpy(m_cachePtr, (const char*)buffer + offset, endSize);
            sizeToWrite -= endSize;
            m_cachePtr += endSize;
            flushWriteCache();
        }
        if (sizeToWrite) {
            size_t offset = length - sizeToWrite;
            memcpy(m_cachePtr, (const char*)buffer + offset, sizeToWrite);
            m_cachePtr += sizeToWrite;
        }
    }

    return true;
}

bool SnappyFile::rawRead(void *buffer, size_t length)
{
    if (endOfData()) {
        return false;
    }

    if (freeCacheSize() >= length) {
        memcpy(buffer, m_cachePtr, length);
        m_cachePtr += length;
    } else {
        size_t sizeToRead = length;
        size_t offset = 0;
        while (sizeToRead) {
            size_t chunkSize = std::min(freeCacheSize(), sizeToRead);
            offset = length - sizeToRead;
            memcpy((char*)buffer + offset, m_cachePtr, chunkSize);
            m_cachePtr += chunkSize;
            sizeToRead -= chunkSize;
            if (sizeToRead > 0) {
                flushReadCache();
            }
            if (!m_cacheSize) {
                break;
            }
        }
    }

    return true;
}

int SnappyFile::rawGetc()
{
    int c = 0;
    if (!rawRead(&c, 1))
        return -1;
    return c;
}

void SnappyFile::rawClose()
{
    if (m_mode == File::Write) {
        flushWriteCache();
    }
    m_stream.close();
    delete [] m_cache;
    m_cache = NULL;
    m_cachePtr = NULL;
}

void SnappyFile::rawFlush()
{
    assert(m_mode == File::Write);
    flushWriteCache();
    m_stream.flush();
}

void SnappyFile::flushWriteCache()
{
    size_t inputLength = usedCacheSize();

    if (inputLength) {
        size_t compressedLength;

        ::snappy::RawCompress(m_cache, inputLength,
                              m_compressedCache, &compressedLength);

        writeCompressedLength(compressedLength);
        m_stream.write(m_compressedCache, compressedLength);
        m_cachePtr = m_cache;
    }
    assert(m_cachePtr == m_cache);
}

void SnappyFile::flushReadCache(size_t skipLength)
{
    //assert(m_cachePtr == m_cache + m_cacheSize);
    m_currentOffset.chunk = m_stream.tellg();
    size_t compressedLength;
    compressedLength = readCompressedLength();

    if (compressedLength) {
        m_stream.read((char*)m_compressedCache, compressedLength);
        ::snappy::GetUncompressedLength(m_compressedCache, compressedLength,
                                        &m_cacheSize);
        createCache(m_cacheSize);
        if (skipLength < m_cacheSize) {
            ::snappy::RawUncompress(m_compressedCache, compressedLength,
                                    m_cache);
        }
    } else {
        createCache(0);
    }
}

void SnappyFile::createCache(size_t size)
{
    // TODO: only re-allocate if the current buffer is not big enough

    if (m_cache) {
        delete [] m_cache;
    }

    if (size) {
        m_cache = new char[size];
    } else {
        m_cache = NULL;
    }

    m_cachePtr = m_cache;
    m_cacheSize = size;
}

void SnappyFile::writeCompressedLength(size_t length)
{
    unsigned char buf[4];
    buf[0] = length & 0xff; length >>= 8;
    buf[1] = length & 0xff; length >>= 8;
    buf[2] = length & 0xff; length >>= 8;
    buf[3] = length & 0xff; length >>= 8;
    assert(length == 0);
    m_stream.write((const char *)buf, sizeof buf);
}

size_t SnappyFile::readCompressedLength()
{
    unsigned char buf[4];
    size_t length;
    m_stream.read((char *)buf, sizeof buf);
    if (m_stream.fail()) {
        length = 0;
    } else {
        length  =  (size_t)buf[0];
        length |= ((size_t)buf[1] <<  8);
        length |= ((size_t)buf[2] << 16);
        length |= ((size_t)buf[3] << 24);
    }
    return length;
}

bool SnappyFile::supportsOffsets() const
{
    return true;
}

File::Offset SnappyFile::currentOffset()
{
    m_currentOffset.offsetInChunk = m_cachePtr - m_cache;
    return m_currentOffset;
}

void SnappyFile::setCurrentOffset(const File::Offset &offset)
{
    // to remove eof bit
    m_stream.clear();
    // seek to the start of a chunk
    m_stream.seekg(offset.chunk, std::ios::beg);
    // load the chunk
    flushReadCache();
    assert(m_cacheSize >= offset.offsetInChunk);
    // seek within our cache to the correct location within the chunk
    m_cachePtr = m_cache + offset.offsetInChunk;

}

bool SnappyFile::rawSkip(size_t length)
{
    if (endOfData()) {
        return false;
    }

    if (freeCacheSize() >= length) {
        m_cachePtr += length;
    } else {
        size_t sizeToRead = length;
        while (sizeToRead) {
            size_t chunkSize = std::min(freeCacheSize(), sizeToRead);
            m_cachePtr += chunkSize;
            sizeToRead -= chunkSize;
            if (sizeToRead > 0) {
                flushReadCache(sizeToRead);
            }
            if (!m_cacheSize) {
                break;
            }
        }
    }

    return true;
}

int SnappyFile::rawPercentRead()
{
    return 100 * (double(m_stream.tellg()) / double(m_endPos));
}
