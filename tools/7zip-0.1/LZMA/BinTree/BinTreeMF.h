#include "../Common/ICoder.h"
#include "BinTree.h"


class CMatchFinderBinTree
{
public:
  int Init(BYTE *ptr, UINT32 inSize);
  void ReleaseStream();
  int MovePos();
  BYTE GetIndexByte(UINT32 index);
  UINT32 GetMatchLen(UINT32 index, UINT32 back, UINT32 limit);
  UINT32 GetNumAvailableBytes();
  const BYTE *GetPointerToCurrentPos();

  int Create(UINT32 sizeHistory, UINT32 keepAddBufferBefore, UINT32 matchMaxLen, UINT32 keepAddBufferAfter);
  UINT32 GetLongestMatch(UINT32 *distances);
  void DummyLongestMatch();

private:
  CInTree _matchFinder;

public:
  void SetCutValue(UINT32 cutValue) 
  { 
    _matchFinder.SetCutValue(cutValue); 
  }
};
