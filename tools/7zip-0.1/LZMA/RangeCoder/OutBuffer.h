// OutBuffer.h

#ifndef __OUTBUFFER_H
#define __OUTBUFFER_H

#include "../Common/IStream.h"


class COutBuffer
{
  BYTE *_buffer;
  UINT32 _pos;
  UINT32 _bufferSize;
  ISequentialOutStream _stream;
  UINT32 _processedSize;

  void WriteBlock()
  {
    Flush();
  }

public:

  COutBuffer(UINT32 bufferSize = (1 << 20)):
    _bufferSize(bufferSize)
  {
    _buffer = new BYTE[_bufferSize];
  }

  ~COutBuffer()
  {
    delete []_buffer;
  }

  void Init(BYTE *ptr)
  {
    _stream.Init(ptr);
    _processedSize = 0;
    _pos = 0;
  }
  
  int Flush()
  {
    if (_pos == 0)
      return (0);
    UINT32 processedSize;
    UINT32 result = _stream.Write(_buffer, _pos, &processedSize);
    if (result != 0)
      return result;   
    if (_pos != processedSize)
      return (-1);
    _processedSize += processedSize;
    _pos = 0;

    return (0);
  }

  void WriteByte(BYTE b)
  {
    _buffer[_pos++] = b;
    if(_pos >= _bufferSize)
      WriteBlock();
  }

  void WriteBytes(const void *data, UINT32 size)
  {
    for (UINT32 i = 0; i < size; i++)
      WriteByte(((const BYTE *)data)[i]);
  }

  UINT32 GetProcessedSize() const 
  { 
    return _processedSize + _pos; 
  }
};

#endif
