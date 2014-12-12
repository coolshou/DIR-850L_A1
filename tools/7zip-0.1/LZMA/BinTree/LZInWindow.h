// LZInWindow.h

#ifndef __LZ_IN_WINDOW_H
#define __LZ_IN_WINDOW_H

#include <string.h>
#include "../Common/IStream.h"

class CLZInWindow
{
  BYTE  *_bufferBase; 			// pointer to buffer with data
  ISequentialInStream _stream;
  UINT32 _posLimit;  			// offset (from _buffer) of first byte when new block reading must be done
  bool _streamEndWasReached; 		// if (true) then _streamPos shows real end of stream
  const BYTE *_pointerToLastSafePosition;

protected:
  BYTE  *_buffer;   			// Pointer to virtual Buffer begin
  UINT32 _blockSize;  			// Size of Allocated memory block
  UINT32 _pos;             		// offset (from _buffer) of curent byte
  UINT32 _keepSizeBefore;  		// how many BYTEs must be kept in buffer before _pos
  UINT32 _keepSizeAfter;   		// how many BYTEs must be kept buffer after _pos
  UINT32 _keepSizeReserv;  		// how many BYTEs must be kept as reserv
  UINT32 _streamPos;   			// offset (from _buffer) of first not read byte from Stream

  virtual void BeforeMoveBlock() {};
  virtual void AfterMoveBlock() {};

  void MoveBlock()
  {
    BeforeMoveBlock();
    UINT32 offset = (_buffer + _pos - _keepSizeBefore) - _bufferBase;
    UINT32 numBytes = (_buffer + _streamPos) -  (_bufferBase + offset);
    memmove(_bufferBase, _bufferBase + offset, numBytes);
    _buffer -= offset;
    AfterMoveBlock();
  }

  virtual int ReadBlock()
  {
    if(_streamEndWasReached)
      return (0);

    while(true)
    {
      UINT32 size = (_bufferBase + _blockSize) - (_buffer + _streamPos);
      if(size == 0)  
        return (0);
      UINT32 numReadBytes;
      RINOK(_stream.ReadPart(_buffer + _streamPos, size, &numReadBytes));
      if(numReadBytes == 0)
      {
        _posLimit = _streamPos;
        const BYTE *pointerToPostion = _buffer + _posLimit;
        if(pointerToPostion > _pointerToLastSafePosition)
          _posLimit = _pointerToLastSafePosition - _buffer;
        _streamEndWasReached = true;
        return (0);
      }
      _streamPos += numReadBytes;
      if(_streamPos >= _pos + _keepSizeAfter)
      {
        _posLimit = _streamPos - _keepSizeAfter;
        return (0);
      }
    }

  }

  void Free()
  {
    delete []_bufferBase;
    _bufferBase = 0;
  }

public:
  CLZInWindow(): _bufferBase(0) {}

  virtual ~CLZInWindow()
  {
    Free();
  }

  void Create(UINT32 keepSizeBefore, UINT32 keepSizeAfter, UINT32 keepSizeReserv = (1<<17))
  {
    _keepSizeBefore = keepSizeBefore;
    _keepSizeAfter = keepSizeAfter;
    _keepSizeReserv = keepSizeReserv;
    _blockSize = keepSizeBefore + keepSizeAfter + keepSizeReserv;
    Free();
    _bufferBase = new BYTE[_blockSize];
    _pointerToLastSafePosition = _bufferBase + _blockSize - keepSizeAfter;
  }

  int Init(BYTE *ptr, UINT32 inSize)
  {
    _stream.Init(ptr, inSize);
    _buffer = _bufferBase;
    _pos = 0;
    _streamPos = 0;  
    _streamEndWasReached = false;
    return ReadBlock();
  }

  BYTE *GetBuffer() const 
  { 
    return _buffer; 
  }

  const BYTE *GetPointerToCurrentPos() const 
  { 
    return _buffer + _pos; 
  }

  int MovePos()
  {
    _pos++;
    if (_pos > _posLimit)
    {
      const BYTE *pointerToPostion = _buffer + _pos;
      if(pointerToPostion > _pointerToLastSafePosition)
        MoveBlock();
      return ReadBlock();
    }
    else
      return (0);
  }

  BYTE GetIndexByte(UINT32 index)const
  {  
    return _buffer[_pos + index]; 
  }

  UINT32 GetMatchLen(UINT32 index, UINT32 back, UINT32 limit) const
  {  
    if(_streamEndWasReached)
      if ((_pos + index) + limit > _streamPos)
        limit = _streamPos - (_pos + index);
    back++;
    BYTE *pby = _buffer + _pos + index;
    UINT32 i;
    for(i = 0; i < limit && pby[i] == pby[i - back]; i++);
    return i;
  }

  UINT32 GetNumAvailableBytes() const 
  { 
    return _streamPos - _pos; 
  }

  void ReduceOffsets(UINT32 subValue)
  {
    _buffer += subValue;
    _posLimit -= subValue;
    _pos -= subValue;
    _streamPos -= subValue;
  }

};

#endif
