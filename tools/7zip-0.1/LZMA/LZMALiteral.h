// LiteralCoder.h

#ifndef __LITERALCODER_H
#define __LITERALCODER_H

#include "RangeCoder/RangeCoderBit.h"
#include "RangeCoder/RangeCoderOpt.h"

class CEncoder2
{
  CBitEncoder<kNumMoveBits> _encoders[3][1 << 8];

public:
  void Init()
  {
    for (int i = 0; i < 3; i++)
      for (int j = 1; j < (1 << 8); j++)
        _encoders[i][j].Init();
  }

  void Encode(CRangeEncoder *rangeEncoder, bool matchMode, BYTE matchByte, BYTE symbol)
  {
    UINT32 context = 1;
    bool same = true;
    for (int i = 7; i >= 0; i--)
    {
      UINT32 bit = (symbol >> i) & 1;
      UINT32 state;
//Ramesh ??
//      UINT state;

      if (matchMode && same)
      {
        UINT32 matchBit = (matchByte >> i) & 1;
        state = 1 + matchBit;
        same = (matchBit == bit);
      }
      else
        state = 0;
      _encoders[state][context].Encode(rangeEncoder, bit);
      context = (context << 1) | bit;
    }
  }

  UINT32 GetPrice(bool matchMode, BYTE matchByte, BYTE symbol) const
  {
    UINT32 price = 0;
    UINT32 context = 1;
    int i = 7;

    if (matchMode)
    {
      for (; i >= 0; i--)
      {
        UINT32 matchBit = (matchByte >> i) & 1;   
        UINT32 bit = (symbol >> i) & 1;
        price += _encoders[1 + matchBit][context].GetPrice(bit);
        context = (context << 1) | bit;
        if (matchBit != bit)
        {
          i--;
          break; 
        }
      }
    }

    for (; i >= 0; i--)
    {
      UINT32 bit = (symbol >> i) & 1;
      price += _encoders[0][context].GetPrice(bit);
      context = (context << 1) | bit;
    }

    return price;
  }

};


class CEncoderLiteral
{
  CEncoder2 *_coders;
  UINT32 _numPrevBits;
  UINT32 _numPosBits;
  UINT32 _posMask;

public:
  CEncoderLiteral(): _coders(0) {}

  ~CEncoderLiteral()  
  { 
     Free(); 
  }

  void Free()
  { 
    delete []_coders;
    _coders = 0;
  }

  void Create(UINT32 numPosBits, UINT32 numPrevBits)
  {
    Free();
    _numPosBits = numPosBits;
    _posMask = (1 << numPosBits) - 1;
    _numPrevBits = numPrevBits;
    UINT32 numStates = 1 << (_numPrevBits + _numPosBits);
    _coders = new CEncoder2[numStates];
  }

  void Init()
  {
    UINT32 numStates = 1 << (_numPrevBits + _numPosBits);
    for (UINT32 i = 0; i < numStates; i++)
      _coders[i].Init();
  }

  UINT32 GetState(UINT32 pos, BYTE prevByte) const
  { 
    return ((pos & _posMask) << _numPrevBits) + (prevByte >> (8 - _numPrevBits)); 
  }

  void Encode(CRangeEncoder *rangeEncoder, UINT32 pos, BYTE prevByte, bool matchMode, BYTE matchByte, BYTE symbol)
  { 
    _coders[GetState(pos, prevByte)].Encode(rangeEncoder, matchMode, matchByte, symbol); 
  }

  UINT32 GetPrice(UINT32 pos, BYTE prevByte, bool matchMode, BYTE matchByte, BYTE symbol) const
  { 
    return _coders[GetState(pos, prevByte)].GetPrice(matchMode, matchByte, symbol); 
  }

};

#endif
