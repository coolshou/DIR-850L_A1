// BinTree.h

#include "LZInWindow.h"
 
typedef UINT32 CIndex;
const UINT32 kMaxValForNormalize = (UINT32(1) << 31) - 1;

struct CPair
{
  CIndex Left;
  CIndex Right;
};

class CInTree: public CLZInWindow
{
  UINT32 _cyclicBufferPos;
  UINT32 _cyclicBufferSize;
  UINT32 _historySize;
  UINT32 _matchMaxLen;

  CIndex *_hash;
  CIndex *_hash2;
  CIndex *_hash3;
  
  CPair *_son;

  UINT32 _cutValue;

  void NormalizeLinks(CIndex *array, UINT32 numItems, UINT32 subValue);
  void Normalize();
  void FreeMemory();

public:
  CInTree();
  ~CInTree();
  int Create(UINT32 sizeHistory,UINT32 keepAddBufferBefore,UINT32 matchMaxLen,UINT32 keepAddBufferAfter,UINT32 sizeReserv = (1<<17));
//  int Init(ISequentialInStream *stream);
  int Init(BYTE *ptr, UINT32 inSize);
  void SetCutValue(UINT32 cutValue) 
  { 
    _cutValue = cutValue; 
  }
  UINT32 GetLongestMatch(UINT32 *distances);
  void DummyLongestMatch();

  int MovePos()
  {
    _cyclicBufferPos++;
    if (_cyclicBufferPos >= _cyclicBufferSize)
      _cyclicBufferPos = 0;
    RINOK(CLZInWindow::MovePos());
    if (_pos == kMaxValForNormalize)
      Normalize();
    return (0);
  }

};
