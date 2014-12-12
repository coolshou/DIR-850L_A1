// IStream.h

#ifndef __ISTREAMS_H
#define __ISTREAMS_H


class ISequentialInStream
{
 public:
   void Init(BYTE *ptr, UINT32 inSize) 
   {
      local_ptr	= ptr;
      start_ptr = ptr;
      _size	= inSize;
      _pos	= 0; 
   }
         
   UINT32 Read(void *data, UINT32 size, UINT32 *processedSize) 
   {
      UINT32 numBytesToRead = MyMin(_pos + size, _size) - _pos;
      BYTE *p = (BYTE *)data;

      for(UINT32 i=0; i<numBytesToRead; i++) 
      {
         *p++ = *local_ptr++;  
      }
      _pos += numBytesToRead;
      *processedSize = numBytesToRead;

       return 0;
   }
          
   UINT32 ReadPart(void *data, UINT32 size, UINT32 *processedSize) 
   {
      return Read(data,size,processedSize);
   } 
           
   UINT64 GetProcessedSize(void) 
   {
      return ((UINT64)(local_ptr - start_ptr));
   }
         
 private:
   BYTE *local_ptr;
   BYTE *start_ptr;
   UINT32 _size;
   UINT32 _pos;
};


class ISequentialOutStream
{
 public:
   void Init(BYTE *ptr) 
   {
      local_ptr = ptr;
      start_ptr = ptr;
   }

   UINT32 Write(void *data, UINT32 size, UINT32 *processedSize) 
   {
      BYTE *p=(BYTE *)data;
      for(UINT32 i=0;i<size;i++) 
      {
        *local_ptr++ = *p++;
      }
      *processedSize=size;
      return 0;
   }

   UINT64 GetProcessedSize(void) 
   {
      return ((UINT64)(local_ptr-start_ptr));
   }

   UINT32 Flush(void) 
   {
      return 0;
   }

 private:
        BYTE *local_ptr;
        BYTE *start_ptr;
};


#endif
