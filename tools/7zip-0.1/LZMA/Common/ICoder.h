// ICoder.h

#ifndef __ICODER_H
#define __ICODER_H

enum EEnum
{
  kDictionarySize = 0x400,
  kUsedMemorySize,
  kOrder,
  kPosStateBits = 0x440,
  kLitContextBits,
  kLitPosBits,
  kNumFastBytes = 0x450,
  kMatchFinder,
  kNumPasses = 0x460, 
  kAlgorithm = 0x470
};


#endif
