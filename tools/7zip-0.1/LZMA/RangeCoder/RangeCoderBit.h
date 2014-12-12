// Compress/RangeCoder/RangeCoderBit.h

#ifndef _RANGECODER_BIT_H
#define _RANGECODER_BIT_H

#include "RangeCoder.h"

const int kNumBitModelTotalBits  = 11;
const UINT32 kBitModelTotal = (1 << kNumBitModelTotalBits);

const int kNumMoveReducingBits = 2;

const int kNumBitPriceShiftBits = 6;
const UINT32 kBitPrice = 1 << kNumBitPriceShiftBits;

class CPriceTables
{
public:
  UINT32 StatePrices[kBitModelTotal >> kNumMoveReducingBits];

  CPriceTables()
  {
    const int kNumBits = (kNumBitModelTotalBits - kNumMoveReducingBits);
    for(int i = kNumBits - 1; i >= 0; i--)
    {
      UINT32 start = 1 << (kNumBits - i - 1);
      UINT32 end = 1 << (kNumBits - i);
      for (UINT32 j = start; j < end; j++)
        StatePrices[j] = (i << kNumBitPriceShiftBits) +
            (((end - j) << kNumBitPriceShiftBits) >> (kNumBits - i - 1));
    }
  }
};

CPriceTables g_PriceTables;


/////////////////////////////
// CBitModel

template <int aNumMoveBits>
class CBitModel
{
public:
  UINT32 Probability;
  void UpdateModel(UINT32 symbol)
  {
    if (symbol == 0)
      Probability += (kBitModelTotal - Probability) >> aNumMoveBits;
    else
      Probability -= (Probability) >> aNumMoveBits;
  }
public:
  void Init() { Probability = kBitModelTotal / 2; }
};


template <int aNumMoveBits>
class CBitEncoder: public CBitModel<aNumMoveBits>
{
public:
  void Encode(CRangeEncoder *encoder, UINT32 symbol)
  {
    encoder->EncodeBit(CBitEncoder<aNumMoveBits>::Probability, kNumBitModelTotalBits, symbol);
    CBitEncoder<aNumMoveBits>::UpdateModel(symbol);
  }
  UINT32 GetPrice(UINT32 symbol) const
  {
    return g_PriceTables.StatePrices[
      (((CBitEncoder<aNumMoveBits>::Probability - symbol) ^ ((-(int)symbol))) & (kBitModelTotal - 1)) >> kNumMoveReducingBits];
  }
};


#endif
