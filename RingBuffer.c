/****************************************************************************//*
* \file      CFW_RingBuffer.c
* 
*******************************************************************************/
#include "string.h"
#include "IncIf.h"
#include "csm_stack_datatypes.h"
#include  "CFW_RingBuffer.h"

/*******************************************************************************
* Local Function Prototypes
*******************************************************************************/
static GLB_INLINE void  CFW_RingBuffer_vAppendToTail(CFW_RingBuffer*, const BYTE*, UINT);
static GLB_INLINE void  CFW_RingBuffer_vIncReadPointer(CFW_RingBuffer*);
static GLB_INLINE void  CFW_RingBuffer_vIncWritePointer(CFW_RingBuffer*);
static GLB_INLINE WORD CFW_RingBuffer_uwFree(CFW_RingBuffer* self, BYTE ubSize);


/****************************************************************************//*
* Function          : CFW_RingBuffer_blInit
*------------------------------------------------------------------------------
* Description       : Initialises a new ring buffer.
*-------------------------------------------------------------------------------
* History           : 26-Feb-2015, Intial Version
*******************************************************************************/
BOOL CFW_RingBuffer_blInit(CFW_RingBuffer* self, BYTE *pubBuf, WORD uwSizeOfBuffer )
{
  BOOL blReturn = FALSE;
  memset(self,0,sizeof(CFW_RingBuffer));

  if( (NULL != pubBuf) && (0 != uwSizeOfBuffer) )
  {
    self->pubStartPtr = (const BYTE*)pubBuf;
    self->pubEndPtr   = (const BYTE*)pubBuf + (uwSizeOfBuffer-1);
    self->pubWritePtr = pubBuf;
    self->pubReadPtr  = pubBuf;
    blReturn          = TRUE;
  }
  else
  {

  }
  return( blReturn );
}

/****************************************************************************//*
* Function          : CFW_RingBuffer_vWrite
*------------------------------------------------------------------------------
* Description       : Writes the passed message into the buffer.
*-------------------------------------------------------------------------------
* History           : 26-Feb-2015, Intial Version
*******************************************************************************/
GLB_INLINE void CFW_RingBuffer_vWrite(CFW_RingBuffer* self, const BYTE *pubData, BYTE ubSize)
{
  if( (ubSize != 0) /*&& (ubSize <= 0xFE)*/ ) /* MAX & MIN size of message */
  {
//    BYTE ubBytesToWrite = ubSize+1;
//  int_no_of_Debug[0]++;
//    if( ubBytesToWrite < CFW_RingBuffer_uwSizeOf(self) )
//    {
//      if( ubBytesToWrite > CFW_RingBuffer_uwGetFreeBytes(self) )
//      {
//        self->uwOverWrittenBytes += CFW_RingBuffer_uwFree(self,ubBytesToWrite);
//      }
      CFW_RingBuffer_vAppendToTail(self,pubData,ubSize);
//    }
  }
//  else
//  {
//    volatile int i = 0;
//  i = 1;
//  }
}

/****************************************************************************//*
* Function          : CFW_RingBuffer_blRead
*------------------------------------------------------------------------------
* Description       : Packs the passed buffer with as many messages as can fit
*                     into the buffer.
*------------------------------------------------------------------------------
* History           : 26-Feb-2015, Intial Version
*******************************************************************************/
BYTE CFW_RingBuffer_ubRead(CFW_RingBuffer* self, BYTE *pubDestBuf, WORD *puwSize)
{
  WORD uwSizeOfDestBuffer = *puwSize;
  BYTE ubMessageCount     = 0;
  UINT umcMsgLength       = *self->pubReadPtr; /* length of first msg in Q   */
  *puwSize                 = 0;                 /* No. of bytes read so far   */

  while(   (TRUE != CFW_RingBuffer_blIsEmpty(self))  /* as long as the Q */
        && (umcMsgLength <= uwSizeOfDestBuffer))/* is not empty and there is  */
  {                                             /* enough space in the buffer.*/
                                                /*                            */
    if( umcMsgLength == 0 )
    {

      /* This should never happen (unless the buffer is empty) */
      /* This check is necessary till the buffer corruption    */
      /* problem is analyzed and corrected                     */
      CFW_RingBuffer_vReset(self);  /* ## BP ## */
      *puwSize = 0;
      return 0;
    }

    *puwSize += umcMsgLength;                   /* Update number of msgs read.*/
    uwSizeOfDestBuffer -= umcMsgLength;         /* reduce the buffer size.    */
    while( 0U < umcMsgLength-- )                /*                            */
    {                                           /*                            */
      *(pubDestBuf++) = *(self->pubReadPtr);    /*  Copy the message          */
      CFW_RingBuffer_vIncReadPointer(self);/*                            */
    }                                           /*                            */
    ubMessageCount++;                           /* update number of msgs read */
    umcMsgLength = *self->pubReadPtr;           /* Get size of next msg in  Q */
  }
  return ubMessageCount;
}

/****************************************************************************//*
* Function          : CFW_RingBuffer_blReadSingle
*------------------------------------------------------------------------------
* Description       : Reads a single message out of the buffer
*------------------------------------------------------------------------------
* History           : 26-Feb-2015, Intial Version
*******************************************************************************/
BYTE CFW_RingBuffer_ubReadSingle(CFW_RingBuffer* self, BYTE *pubDestBuf, WORD *puwSize)
{
  WORD uwSizeOfDestBuffer = *puwSize;
  UINT umcMsgLength       = *self->pubReadPtr; /* length of top message in Q */
  *puwSize                 = 0;                 /* 0 bytes read so far        */

  if(   ( CFW_RingBuffer_blIsEmpty(self) != TRUE )
     && ( umcMsgLength <= uwSizeOfDestBuffer)            )
  {
    if( umcMsgLength == 0 )
    {
      /* This should never happen (unless the buffer is empty) */
      /* This check is necessary till the buffer corruption    */
      /* problem is analyzed and corrected                     */
      CFW_RingBuffer_vReset(self);  /* ## BP ## */
      *puwSize = 0;
      return 0;
    }

    CFW_RingBuffer_vIncReadPointer(self);
    umcMsgLength--;                          /* Discard the length information */
    *puwSize = (BYTE)umcMsgLength;
    while( 0U < umcMsgLength-- )
    {
      *(pubDestBuf++) = *(self->pubReadPtr);
      CFW_RingBuffer_vIncReadPointer(self);
    }
  }
  return((*puwSize == 0)?(BYTE)0:(BYTE)1);
}

/****************************************************************************//*
* Function          : CFW_RingBuffer_uwGetFreeBytes
*------------------------------------------------------------------------------
* Description       : returns free space in buffer
*------------------------------------------------------------------------------
* History           : 26-Feb-2015, Intial Version
*******************************************************************************/
GLB_INLINE WORD CFW_RingBuffer_uwGetFreeBytes(const CFW_RingBuffer* self)
{
  WORD uwFspace;
  if( (self->pubWritePtr >= self->pubReadPtr)  )
  {
    uwFspace  = (WORD)(self->pubEndPtr  - self->pubWritePtr);
    uwFspace += (WORD)(self->pubReadPtr - self->pubStartPtr);
  }
  else
  {
    uwFspace = (WORD)(self->pubReadPtr - self->pubWritePtr );
  }
  return uwFspace;
}

/****************************************************************************//*
* Function          : CFW_RingBuffer_vAppendToTail
*------------------------------------------------------------------------------
* Description       :  local function that adds a message at the tail of the q
*------------------------------------------------------------------------------
* History           : 26-Feb-2015, Intial Version
*******************************************************************************/
static GLB_INLINE void CFW_RingBuffer_vAppendToTail(CFW_RingBuffer* self,
                                                         const BYTE* pubSrcBuf,
                                                         UINT umBytesToWrite)
{
  BYTE * pubEnd;

  *(self->pubWritePtr) = (BYTE)(umBytesToWrite+1U); /* 1 byte Msg length */
  CFW_RingBuffer_vIncWritePointer(self);

  pubEnd = self->pubWritePtr + umBytesToWrite;

  if(pubEnd <= self->pubEndPtr)
  {/* End location yet to come, it is safe to copy as a DWORD */
    WORD wlen4byte = umBytesToWrite/4;  /* Exactly how many DWOR can be copied */
    WORD wlen1byte = umBytesToWrite%4;  /* Rest of data will be copied byte by byte */
    while (wlen4byte)
    {
      (*((DWORD *)self->pubWritePtr)) = (*((DWORD *) (pubSrcBuf)));
      self->pubWritePtr = self->pubWritePtr + 4;
      pubSrcBuf = pubSrcBuf + 4;
    --wlen4byte;
    }
    while (wlen1byte)
    {
      (*self->pubWritePtr++) =  (*pubSrcBuf++);
    --wlen1byte;
    }
  }
  else
  {/* Copy byte by byte as when the array last location can come??? */
    while(umBytesToWrite--)
    {
      *(self->pubWritePtr) = *(pubSrcBuf++);
      CFW_RingBuffer_vIncWritePointer(self);
    }
  }
}

/****************************************************************************//*
* Function          : CFW_RingBuffer_vIncWritePointer
*------------------------------------------------------------------------------
* Description       : Increments the write pointer. Wraps around if the pointer
*                     is beyond the allocated memory
*------------------------------------------------------------------------------
* History           : 26-Feb-2015, Intial Version
*******************************************************************************/
static GLB_INLINE void CFW_RingBuffer_vIncWritePointer(CFW_RingBuffer *self)
{
  self->pubWritePtr++;
  if( self->pubWritePtr > self->pubEndPtr )
  {
    /* MISRA Rule 45 not removed; The pointer cast is intentional and necessary */
    self->pubWritePtr = (BYTE*)(self->pubStartPtr);
  }
}

/****************************************************************************//*
* Function          : CFW_RingBuffer_vIncReadPointer
*------------------------------------------------------------------------------
* Description       : Increments the read pointer. Wraps around if the pointer
*                     is beyond the allocated memory
*------------------------------------------------------------------------------
* History           : 26-Feb-2015, Intial Version
*******************************************************************************/
static GLB_INLINE void CFW_RingBuffer_vIncReadPointer(CFW_RingBuffer *self)
{
  self->pubReadPtr++;
  if( self->pubReadPtr > self->pubEndPtr )
  {
    /* MISRA Rule 45 not removed; The pointer cast is intentional and necessary */
    self->pubReadPtr = (BYTE*)(self->pubStartPtr);
  }
}

/****************************************************************************//*
* Function          : CFW_RingBuffer_uwFree
*------------------------------------------------------------------------------

* Description       : Deletes the oldest message in the buffer
*------------------------------------------------------------------------------
* History           : 26-Feb-2015, Intial Version
*******************************************************************************/
static GLB_INLINE WORD CFW_RingBuffer_uwFree(CFW_RingBuffer* self, BYTE ubSize)
{
  WORD uwFreed = 0;
  while( uwFreed < (WORD)ubSize )
  {
    BYTE ubSizeOfMessage = *(self->pubReadPtr);
    if( 0 == ubSizeOfMessage )
    {
      /* This should never happen (unless the buffer is empty) */
      CFW_RingBuffer_vReset(self);
      uwFreed = ubSize;
    }
    else
    {
      self->pubReadPtr += ubSizeOfMessage;
      if( self->pubReadPtr > self->pubEndPtr )
      {
        /* MISRA Rule 45 not removed; The pointer cast is intentional and necessary */
        self->pubReadPtr = (BYTE*)(self->pubStartPtr + (self->pubReadPtr - self->pubEndPtr-1) );
      }
      uwFreed += ubSizeOfMessage;
    }
  }
  return uwFreed;
}



/** END OF FILE ***************************************************************/
