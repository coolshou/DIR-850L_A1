// BinTreeMFMain.h

#include "BinTreeMain.h"


int CMatchFinderBinTree::Init(BYTE *ptr, UINT32 inSize)
{ 
  return _matchFinder.Init(ptr, inSize); 
}

void CMatchFinderBinTree::ReleaseStream()
{ 
  // _matchFinder.ReleaseStream(); 
}

int CMatchFinderBinTree::MovePos()
{ 
  return _matchFinder.MovePos(); 
}

BYTE CMatchFinderBinTree::GetIndexByte(UINT32 index)
{ 
  return _matchFinder.GetIndexByte(index); 
}

UINT32 CMatchFinderBinTree::GetMatchLen(UINT32 index, UINT32 back, UINT32 limit)
{ 
  return _matchFinder.GetMatchLen(index, back, limit); 
}

UINT32 CMatchFinderBinTree::GetNumAvailableBytes()
{ 
  return _matchFinder.GetNumAvailableBytes(); 
}
  
int CMatchFinderBinTree::Create(UINT32 sizeHistory,UINT32 keepAddBufferBefore, UINT32 matchMaxLen,UINT32 keepAddBufferAfter)
{ 
  UINT32 windowReservSize = (sizeHistory + keepAddBufferBefore + matchMaxLen + keepAddBufferAfter) / 2 + 256;
  return _matchFinder.Create(sizeHistory,keepAddBufferBefore,matchMaxLen,keepAddBufferAfter,windowReservSize); 
}

UINT32 CMatchFinderBinTree::GetLongestMatch(UINT32 *distances)
{ 
  return _matchFinder.GetLongestMatch(distances); 
}

void CMatchFinderBinTree::DummyLongestMatch()
{ 
  _matchFinder.DummyLongestMatch(); 
}

const BYTE * CMatchFinderBinTree::GetPointerToCurrentPos()
{
  return _matchFinder.GetPointerToCurrentPos();
}
