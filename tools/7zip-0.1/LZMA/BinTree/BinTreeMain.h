// BinTreemain.h

#include "CRC.h"

static const UINT32 kHash2Size = 1 << 10;
static const UINT32 kNumHashDirectBytes = 0;
static const UINT32 kNumHashBytes = 4;
static const UINT32 kHash3Size = 1 << 18;
static const UINT32 kHashSize = 1 << 20;

CInTree::CInTree():
_hash(0),
_hash2(0),
_hash3(0),
_son(0),
_cutValue(0xFF)
{
}

void CInTree::FreeMemory()
{
  delete []_son;
  delete []_hash;
  _son = 0;
  _hash = 0;
  CLZInWindow::Free();
}


CInTree::~CInTree()
{ 
  FreeMemory();
}

int CInTree::Create(UINT32 sizeHistory, UINT32 keepAddBufferBefore,UINT32 matchMaxLen, UINT32 keepAddBufferAfter, UINT32 sizeReserv)
{
  FreeMemory();

  CLZInWindow::Create(sizeHistory + keepAddBufferBefore,matchMaxLen + keepAddBufferAfter, sizeReserv);
    
  if (_blockSize + 256 > kMaxValForNormalize)
    return (-1);
    
  _historySize = sizeHistory;
  _matchMaxLen = matchMaxLen;

  _cyclicBufferSize = sizeHistory + 1;
    
  UINT32 size = kHashSize;
  size += kHash2Size;
  size += kHash3Size;
    
  _son = new CPair[_cyclicBufferSize + 1];
  _hash = new CIndex[size + 1];
    
  _hash2 = &_hash[kHashSize]; 
  _hash3 = &_hash2[kHash2Size]; 

  return (0);
}

static const UINT32 kEmptyHashValue = 0;

int CInTree::Init(BYTE *ptr, UINT32 inSize)
{
  RINOK(CLZInWindow::Init(ptr, inSize));
  UINT32 i;
  for(i = 0; i < kHashSize; i++)
    _hash[i] = kEmptyHashValue;

  for(i = 0; i < kHash2Size; i++)
    _hash2[i] = kEmptyHashValue;
  for(i = 0; i < kHash3Size; i++)
    _hash3[i] = kEmptyHashValue;

  _cyclicBufferPos = 0;

  ReduceOffsets(0 - 1);

  return (0);
}


inline UINT32 Hash(const BYTE *pointer, UINT32 &hash2Value, UINT32 &hash3Value)
{
  UINT32 temp = CCRC::Table[pointer[0]] ^ pointer[1];
  hash2Value = temp & (kHash2Size - 1);
  hash3Value = (temp ^ (UINT32(pointer[2]) << 8)) & (kHash3Size - 1);
  return (temp ^ (UINT32(pointer[2]) << 8) ^ (CCRC::Table[pointer[3]] << 5)) & (kHashSize - 1);
}


UINT32 CInTree::GetLongestMatch(UINT32 *distances)
{
  UINT32 currentLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    currentLimit = _matchMaxLen;
  else
  {
    currentLimit = _streamPos - _pos;
    if(currentLimit < kNumHashBytes)
      return 0; 
  }

  UINT32 matchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  BYTE *cur = _buffer + _pos;
  
  UINT32 matchHashLenMax = 0;

  UINT32 hash2Value;
  UINT32 hash3Value;
  UINT32 hashValue = Hash(cur, hash2Value, hash3Value);

  UINT32 curMatch = _hash[hashValue];
  UINT32 curMatch2 = _hash2[hash2Value];
  UINT32 curMatch3 = _hash3[hash3Value];
  _hash2[hash2Value] = _pos;
  bool matchLen2Exist = false;
  UINT32 len2Distance = 0;
  if(curMatch2 >= matchMinPos)
  {
    if (_buffer[curMatch2] == cur[0])
    {
      len2Distance = _pos - curMatch2 - 1;
      matchHashLenMax = 2;
      matchLen2Exist = true;
    }
  }

  _hash3[hash3Value] = _pos;
  UINT32 matchLen3Exist = false;
  UINT32 len3Distance = 0;
  if(curMatch3 >= matchMinPos)
  {
    if (_buffer[curMatch3] == cur[0])
    {
      len3Distance = _pos - curMatch3 - 1;
      matchHashLenMax = 3;
      matchLen3Exist = true;
      if (matchLen2Exist)
      {
        if (len3Distance < len2Distance)
          len2Distance = len3Distance;
      }
      else
      {
        len2Distance = len3Distance;
        matchLen2Exist = true;
      }
    }
  }

  _hash[hashValue] = _pos;

  if(curMatch < matchMinPos)
  {
    _son[_cyclicBufferPos].Left = kEmptyHashValue; 
    _son[_cyclicBufferPos].Right = kEmptyHashValue; 

    distances[2] = len2Distance;
    distances[3] = len3Distance;

    return matchHashLenMax;
  }
  CIndex *ptrLeft = &_son[_cyclicBufferPos].Right;
  CIndex *ptrRight = &_son[_cyclicBufferPos].Left;

  UINT32 maxLen, minSameLeft, minSameRight, minSame;
  maxLen = minSameLeft = minSameRight = minSame = kNumHashDirectBytes;

  if (matchLen2Exist)
    distances[2] = len2Distance;
  else
    if (kNumHashDirectBytes >= 2)
      distances[2] = _pos - curMatch - 1;

  distances[maxLen] = _pos - curMatch - 1;
  
  for(UINT32 count = _cutValue; count > 0; count--)
  {
    BYTE *pby1 = _buffer + curMatch;
    UINT32 currentLen;
    for(currentLen = minSame; currentLen < currentLimit; currentLen++/*, dwComps++*/)
      if (pby1[currentLen] != cur[currentLen])
        break;
    while (currentLen > maxLen)
      distances[++maxLen] = _pos - curMatch - 1;
    
    UINT32 delta = _pos - curMatch;
    UINT32 cyclicPos = (delta <= _cyclicBufferPos) ? (_cyclicBufferPos - delta): (_cyclicBufferPos - delta + _cyclicBufferSize);

    if (currentLen != currentLimit)
    {
      if (pby1[currentLen] < cur[currentLen])
      {
        *ptrRight = curMatch;
        ptrRight = &_son[cyclicPos].Right;
        curMatch = _son[cyclicPos].Right;
        if(currentLen > minSameLeft)
        {
          minSameLeft = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
      else
      {
        *ptrLeft = curMatch;
        ptrLeft = &_son[cyclicPos].Left;
        curMatch = _son[cyclicPos].Left;
        if(currentLen > minSameRight)
        {
          minSameRight = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
    }
    else
    {
      if(currentLen < _matchMaxLen)
      {
        *ptrLeft = curMatch;
        ptrLeft = &_son[cyclicPos].Left;
        curMatch = _son[cyclicPos].Left;
        if(currentLen > minSameRight)
        {
          minSameRight = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
      else
      {
        *ptrLeft = _son[cyclicPos].Right;
        *ptrRight = _son[cyclicPos].Left;

        if (matchLen2Exist && len2Distance < distances[2])
          distances[2] = len2Distance;
        if (matchLen3Exist && len3Distance < distances[3])
          distances[3] = len3Distance;

        return maxLen;
      }
    }
    if(curMatch < matchMinPos)
      break;
  }
  *ptrLeft = kEmptyHashValue;
  *ptrRight = kEmptyHashValue;
  if (matchLen2Exist)
  {
    if (maxLen < 2)
    {
      distances[2] = len2Distance;
      maxLen = 2;
    }
    else if (len2Distance < distances[2])
      distances[2] = len2Distance;
  }
  if (matchLen3Exist)
  {
    if (maxLen < 3)
    {
      distances[3] = len3Distance;
      maxLen = 3;
    }
    else if (len3Distance < distances[3])
      distances[3] = len3Distance;
  }

  return maxLen;
}


void CInTree::DummyLongestMatch()
{
  UINT32 currentLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    currentLimit = _matchMaxLen;
  else
  {
    currentLimit = _streamPos - _pos;
    if(currentLimit < kNumHashBytes)
      return; 
  }
  UINT32 matchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  BYTE *cur = _buffer + _pos;
  
  UINT32 hash2Value;
  UINT32 hash3Value;
  UINT32 hashValue = Hash(cur, hash2Value, hash3Value);
  _hash3[hash3Value] = _pos;
  _hash2[hash2Value] = _pos;

  UINT32 curMatch = _hash[hashValue];
  _hash[hashValue] = _pos;

  if(curMatch < matchMinPos)
  {
    _son[_cyclicBufferPos].Left = kEmptyHashValue; 
    _son[_cyclicBufferPos].Right = kEmptyHashValue; 
    return;
  }
  CIndex *ptrLeft = &_son[_cyclicBufferPos].Right;
  CIndex *ptrRight = &_son[_cyclicBufferPos].Left;

  UINT32 maxLen, minSameLeft, minSameRight, minSame;
  maxLen = minSameLeft = minSameRight = minSame = kNumHashDirectBytes;
  for(UINT32 count = _cutValue; count > 0; count--)
  {
    BYTE *pby1 = _buffer + curMatch;
    UINT32 currentLen;
    for(currentLen = minSame; currentLen < currentLimit; currentLen++/*, dwComps++*/)
      if (pby1[currentLen] != cur[currentLen])
        break;

    UINT32 delta = _pos - curMatch;
    UINT32 cyclicPos = (delta <= _cyclicBufferPos) ? (_cyclicBufferPos - delta) : (_cyclicBufferPos - delta + _cyclicBufferSize);
    
    if (currentLen != currentLimit)
    {
      if (pby1[currentLen] < cur[currentLen])
      {
        *ptrRight = curMatch;
        ptrRight = &_son[cyclicPos].Right;
        curMatch = _son[cyclicPos].Right;
        if(currentLen > minSameLeft)
        {
          minSameLeft = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
      else 
      {
        *ptrLeft = curMatch;
        ptrLeft = &_son[cyclicPos].Left;
        curMatch = _son[cyclicPos].Left;
        if(currentLen > minSameRight)
        {
          minSameRight = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
    }
    else
    {
      if(currentLen < _matchMaxLen)
      {
        *ptrLeft = curMatch;
        ptrLeft = &_son[cyclicPos].Left;
        curMatch = _son[cyclicPos].Left;
        if(currentLen > minSameRight)
        {
          minSameRight = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
      else
      {
        *ptrLeft = _son[cyclicPos].Right;
        *ptrRight = _son[cyclicPos].Left;
        return;
      }
    }
    if(curMatch < matchMinPos)
      break;
  }
  *ptrLeft = kEmptyHashValue;
  *ptrRight = kEmptyHashValue;
}


void CInTree::NormalizeLinks(CIndex *array, UINT32 numItems, UINT32 subValue)
{
  for (UINT32 i = 0; i < numItems; i++)
  {
    UINT32 value = array[i];
    if (value <= subValue)
      value = kEmptyHashValue;
    else
      value -= subValue;
    array[i] = value;
  }
}


void CInTree::Normalize()
{
  UINT32 startItem = _pos - _historySize;
  UINT32 subValue = startItem - 1;
  NormalizeLinks((CIndex *)_son, _cyclicBufferSize * 2, subValue);
  
  NormalizeLinks(_hash, kHashSize, subValue);

  NormalizeLinks(_hash2, kHash2Size, subValue);
  NormalizeLinks(_hash3, kHash3Size, subValue);

  ReduceOffsets(subValue);
}
