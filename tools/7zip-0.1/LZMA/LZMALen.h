// LenCoder.h

#ifndef __LENCODER_H
#define __LENCODER_H

#include "RangeCoder/RangeCoderBitTree.h"

const int kNumMoveBits = 5;

const int kNumPosStatesBitsMax = 4;
const UINT32 kNumPosStatesMax = (1 << kNumPosStatesBitsMax);

const int kNumPosStatesBitsEncodingMax = 4;
const UINT32 kNumPosStatesEncodingMax = (1 << kNumPosStatesBitsEncodingMax);

const int kNumLenBits = 3;
const UINT32 kNumLowSymbols = 1 << kNumLenBits;
const int kNumMidBits = 3;
const UINT32 kNumMidSymbols = 1 << kNumMidBits;

const int kNumHighBits = 8;

const UINT32 kNumSymbolsTotal = kNumLowSymbols + kNumMidSymbols + (1 << kNumHighBits);

class CEncoderLength
{
  CBitEncoder<kNumMoveBits> _choice;
  CBitTreeEncoder<kNumMoveBits, kNumLenBits>  _lowCoder[kNumPosStatesEncodingMax];
  CBitEncoder<kNumMoveBits>  _choice2;
  CBitTreeEncoder<kNumMoveBits, kNumMidBits>  _midCoder[kNumPosStatesEncodingMax];
  CBitTreeEncoder<kNumMoveBits, kNumHighBits>  _highCoder;

protected:
  UINT32 _numPosStates;

public:
  void Create(UINT32 numPosStates)
  { 
    _numPosStates = numPosStates; 
  }

  void Init()
  {
    _choice.Init();
    for (UINT32 posState = 0; posState < _numPosStates; posState++)
    {
      _lowCoder[posState].Init();
      _midCoder[posState].Init();
    }
    _choice2.Init();
    _highCoder.Init();
  }

  void Encode(CRangeEncoder *rangeEncoder, UINT32 symbol, UINT32 posState)
  {
    if(symbol < kNumLowSymbols)
    {
      _choice.Encode(rangeEncoder, 0);
      _lowCoder[posState].Encode(rangeEncoder, symbol);
    }
    else
    {
      symbol -= kNumLowSymbols;
      _choice.Encode(rangeEncoder, 1);
      if(symbol < kNumMidSymbols)
      {
        _choice2.Encode(rangeEncoder, 0);
        _midCoder[posState].Encode(rangeEncoder, symbol);
      }
      else
      {
        _choice2.Encode(rangeEncoder, 1);
        _highCoder.Encode(rangeEncoder, symbol - kNumMidSymbols);
      }
    }
  }

  UINT32 GetPrice(UINT32 symbol, UINT32 posState) const
  {
    UINT32 price = 0;
    if(symbol < kNumLowSymbols)
    {
      price += _choice.GetPrice(0);
      price += _lowCoder[posState].GetPrice(symbol);
    }
    else
    {
      symbol -= kNumLowSymbols;
      price += _choice.GetPrice(1);
      if(symbol < kNumMidSymbols)
      { 
        price += _choice2.GetPrice(0);
        price += _midCoder[posState].GetPrice(symbol);
      }
      else
      {
        price += _choice2.GetPrice(1);   
        price += _highCoder.GetPrice(symbol - kNumMidSymbols);
      }
    }

    return price;
  }

};

const UINT32 kNumSpecSymbols = kNumLowSymbols + kNumMidSymbols;

class CPriceTableEncoder: public CEncoderLength
{
  UINT32 _prices[kNumSymbolsTotal][kNumPosStatesEncodingMax];
  UINT32 _tableSize;
  UINT32 _counters[kNumPosStatesEncodingMax];

public:
  void SetTableSize(UINT32 tableSize)
  { 
    _tableSize = tableSize;  
  }

  UINT32 GetPrice(UINT32 symbol, UINT32 posState) const
  { 
    return _prices[symbol][posState]; 
  }

  void UpdateTable(UINT32 posState)
  {
    for (UINT32 len = 0; len < _tableSize; len++)
      _prices[len][posState] = CEncoderLength::GetPrice(len , posState);
    _counters[posState] = _tableSize;
  }

  void UpdateTables()
  {
    for (UINT32 posState = 0; posState < _numPosStates; posState++)
      UpdateTable(posState);
  }

  void Encode(CRangeEncoder *rangeEncoder, UINT32 symbol, UINT32 posState)
  {
    CEncoderLength::Encode(rangeEncoder, symbol, posState);
    if (--_counters[posState] == 0)
      UpdateTable(posState);
  }

};


#endif
