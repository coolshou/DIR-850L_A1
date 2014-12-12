// Types.h

#ifndef _TYPES_H
#define _TYPES_H

typedef unsigned char   UINT8;
typedef unsigned short  UINT16;
typedef unsigned int	UINT32;
typedef short INT16;
typedef UINT8 BYTE;
typedef unsigned long long UINT64;
typedef unsigned int * UINT32PTR;

#define RINOK(x) { int __result_ = (x); if(__result_ != 0) return __result_; }

template <class T> inline T MyMin(T a, T b)
  {  return a < b ? a : b; }
template <class T> inline T MyMax(T a, T b)
  {  return a > b ? a : b; }

template <class T> inline int MyCompare(T a, T b)
  {  return a < b ? -1 : (a == b ? 0 : 1); }

inline int BoolToInt(bool value)
  { return (value ? 1: 0); }

inline bool IntToBool(int value)
  { return (value != 0); }

#endif

