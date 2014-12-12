// Compress/RangeCoder.h
// This code is based on Eugene Shelwien's Rangecoder code

#ifndef _RANGECODER_H
#define _RANGECODER_H

#include "../Types.h"
#include "OutBuffer.h"


const UINT32 kNumTopBits = 24;
const UINT32 kTopValue = (1 << kNumTopBits);

class CRangeEncoder
{
  COutBuffer Stream;
  UINT64 Low;
  UINT32 Range;
  UINT32 _ffNum;
  BYTE _cache;

public:
  void Init(BYTE *ptr)
  {
    Stream.Init(ptr);
    Low = 0;
    Range = UINT32(-1);
    _ffNum = 0;
    _cache = 0;
  }

  int FlushStream()
  { 
    return Stream.Flush();  
  }

  void Encode(UINT32 start, UINT32 size, UINT32 total)
  {
    Low += start * (Range /= total);
    Range *= size;
    while (Range < kTopValue)
    {
      Range <<= 8;
      ShiftLow();
    }
  }

  void ShiftLow()
  {
    if (Low < (UINT32)0xFF000000 || UINT32(Low >> 32) == 1) 
    {
      Stream.WriteByte(_cache + BYTE(Low >> 32));            
      for (;_ffNum != 0; _ffNum--) 
        Stream.WriteByte(0xFF + BYTE(Low >> 32));
      _cache = BYTE(UINT32(Low) >> 24);                      
    } 
    else 
      _ffNum++;                               
    Low = UINT32(Low) << 8;                           
  }
  

  void FlushData()
  {
    for(int i = 0; i < 5; i++)
      ShiftLow();
  }

  void EncodeDirectBits(UINT32 value, UINT32 numTotalBits)
  {
    for (int i = numTotalBits - 1; i >= 0; i--)
    {
      Range >>= 1;
      if (((value >> i) & 1) == 1)
        Low += Range;
      if (Range < kTopValue)
      {
        Range <<= 8;
        ShiftLow();
      }
    }
  }

  void EncodeBit(UINT32 size0, UINT32 numTotalBits, UINT32 symbol)
  {
    UINT32 newBound = (Range >> numTotalBits) * size0;
    if (symbol == 0)
      Range = newBound;
    else
    {
      Low += newBound;
      Range -= newBound;
    }
    while (Range < kTopValue)
    {
      Range <<= 8;
      ShiftLow();
    }
  }

  UINT64 GetProcessedSize() {  return Stream.GetProcessedSize() + _ffNum; }
};


#endif
