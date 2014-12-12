// MatchFinders/IMatchFinder.h

#ifndef __IMATCHFINDER_H
#define __IMATCHFINDER_H

#include "../Common/IStream.h"

class IInWindowStream
{
  virtual int Init(ISequentialInStream *inStream) = 0;
  virtual void ReleaseStream() = 0;
  virtual int MovePos() = 0;
  virtual BYTE GetIndexByte(UINT32 index) = 0;
  virtual UINT32 GetMatchLen(UINT32 index, UINT32 distance, UINT32 limit) = 0;
  virtual UINT32 GetNumAvailableBytes() = 0;
  virtual const BYTE *GetPointerToCurrentPos() = 0;
};
 
class IMatchFinder: public IInWindowStream
{
  virtual void Create(UINT32 historySize, UINT32 keepAddBufferBefore, UINT32 matchMaxLen, UINT32 keepAddBufferAfter) = 0;
  virtual UINT32 GetLongestMatch(UINT32 *distances) = 0;
  virtual void DummyLongestMatch() = 0;
};

class IMatchFinderCallback
{
  virtual void BeforeChangingBufferPos() = 0;
  virtual void AfterChangingBufferPos() = 0;
};

class IMatchFinderSetCallback
{
  virtual void SetCallback(IMatchFinderCallback *callback) = 0;
};


#endif
