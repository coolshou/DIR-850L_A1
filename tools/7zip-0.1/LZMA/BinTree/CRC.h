// CRC.h

#ifndef __COMMON_CRC_H
#define __COMMON_CRC_H

#include "../Types.h"

#define UPDATE valueLoc = Table[(BYTE)valueLoc] ^ (valueLoc >> 8)
#define UPDATE4 UPDATE; UPDATE; UPDATE; UPDATE;

static const UINT32 kCRCPoly = 0xEDB88320;

class CCRC
{
  UINT32 _value;

public:
  static UINT32 Table[256];
  CCRC():  _value(0xFFFFFFFF){};

  void Init() 
  { 
    _value = 0xFFFFFFFF; 
  }

  void Update(const void *data, UINT32 size)
  {
    UINT32 valueLoc = _value;
    const BYTE *byteBuffer = (const BYTE *)data;


    for(; (UINT32(*byteBuffer) & 3) != 0 && size > 0; size--, byteBuffer++)
      valueLoc = Table[(((BYTE)(valueLoc)) ^ (*byteBuffer))] ^ (valueLoc >> 8);

    const UINT32 kBlockSize = 4;
    while (size >= kBlockSize)
    {
      size -= kBlockSize;
      valueLoc ^= *(const UINT32 *)byteBuffer;
      UPDATE4
      byteBuffer += kBlockSize;
    }
    for(UINT32 i = 0; i < size; i++)
      valueLoc = Table[(((BYTE)(valueLoc)) ^ (byteBuffer)[i])] ^ (valueLoc >> 8);
    _value = valueLoc;
  }

  UINT32 GetDigest() const 
  { 
    return _value ^ 0xFFFFFFFF; 
  }

  static UINT32 CalculateDigest(const void *data, UINT32 size)
  {
    CCRC crc;
    crc.Update(data, size);
    return crc.GetDigest();
  }

  static bool VerifyDigest(UINT32 digest, const void *data, UINT32 size)
  {
    return (CalculateDigest(data, size) == digest);
  }

};

UINT32 CCRC::Table[256];

class CCRCTableInit
{
public:
CCRCTableInit()
{
  for (UINT32 i = 0; i < 256; i++)
  {
    UINT32 r = i;
    for (int j = 0; j < 8; j++)
      if (r & 1)
        r = (r >> 1) ^ kCRCPoly;
      else
        r >>= 1;
    CCRC::Table[i] = r;
  }
}
} g_CRCTableInit;


#endif

