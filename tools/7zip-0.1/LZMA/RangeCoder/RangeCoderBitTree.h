// Compress/RangeCoder/RangeCoderBitTree.h

#ifndef __RANGECODER_BIT_TREE_H
#define __RANGECODER_BIT_TREE_H

#include "RangeCoderBit.h"
#include "RangeCoderOpt.h"


//////////////////////////
// CBitTreeEncoder

template <int numMoveBits, UINT32 NumBitLevels>
class CBitTreeEncoder
{
  CBitEncoder<numMoveBits> Models[1 << NumBitLevels];
public:
  void Init()
  {
    for(UINT32 i = 1; i < (1 << NumBitLevels); i++)
      Models[i].Init();
  }
  void Encode(CRangeEncoder *rangeEncoder, UINT32 symbol)
  {
    UINT32 modelIndex = 1;
    for (UINT32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UINT32 bit = (symbol >> bitIndex ) & 1;
      Models[modelIndex].Encode(rangeEncoder, bit);
      modelIndex = (modelIndex << 1) | bit;
    }
  };
  UINT32 GetPrice(UINT32 symbol) const
  {
    UINT32 price = 0;
    UINT32 modelIndex = 1;
    for (UINT32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UINT32 bit = (symbol >> bitIndex ) & 1;
      price += Models[modelIndex].GetPrice(bit);
      modelIndex = (modelIndex << 1) + bit;
    }
    return price;
  }
};


////////////////////////////////
// CReverseBitTreeEncoder

template <int numMoveBits>
class CReverseBitTreeEncoder2
{
  CBitEncoder<numMoveBits> *Models;
  UINT32 NumBitLevels;
public:
  CReverseBitTreeEncoder2(): Models(0) { }
  ~CReverseBitTreeEncoder2() { delete []Models; }
  void Create(UINT32 numBitLevels)
  {
    NumBitLevels = numBitLevels;
    Models = new CBitEncoder<numMoveBits>[1 << numBitLevels];
  }
  void Init()
  {
    UINT32 numModels = 1 << NumBitLevels;
    for(UINT32 i = 1; i < numModels; i++)
      Models[i].Init();
  }
  void Encode(CRangeEncoder *rangeEncoder, UINT32 symbol)
  {
    UINT32 modelIndex = 1;
    for (UINT32 i = 0; i < NumBitLevels; i++)
    {
      UINT32 bit = symbol & 1;
      Models[modelIndex].Encode(rangeEncoder, bit);
      modelIndex = (modelIndex << 1) | bit;
      symbol >>= 1;
    }
  }
  UINT32 GetPrice(UINT32 symbol) const
  {
    UINT32 price = 0;
    UINT32 modelIndex = 1;
    for (UINT32 i = NumBitLevels; i > 0; i--)
    {
      UINT32 bit = symbol & 1;
      symbol >>= 1;
      price += Models[modelIndex].GetPrice(bit);
      modelIndex = (modelIndex << 1) | bit;
    }
    return price;
  }
};


#endif
