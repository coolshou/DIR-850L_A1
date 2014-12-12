#ifndef FRAMETYPE_H_INCLUDED
#define FRAMETYPE_H_INCLUDED

char
FType_Type(unsigned int const frameNum);

unsigned int
FType_FutureRef(unsigned int const currFrameNum);

int	FType_PastRef _ANSI_ARGS_((int currFrameNum));

void SetFramePattern(const char * const pattern);

void
ComputeFrameTable(unsigned int const numFrames);

#endif
