// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\euser\cbase\ub_buf.cpp
// 
//

#include "ub_std.h"

class TBufSegLink : public TDblQueLink
	{
public:
	inline TBufSegLink() : iLen(0) {}
	inline TBufSegLink *Next() const {return((TBufSegLink *)iNext);}
	inline TBufSegLink *Prev() const {return((TBufSegLink *)iPrev);}
public:
	TInt iLen;
	};

EXPORT_C CBufBase::CBufBase(TInt anExpandSize)
//
// Constructor
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(anExpandSize>=0,Panic(EBufExpandSizeNegative));
//	iSize=0;
	iExpandSize=anExpandSize;
	}

EXPORT_C CBufBase::~CBufBase()
/**
Destructor
*/
	{
	}

EXPORT_C void CBufBase::Reset()
/**
Deletes all data in the buffer.

Its behaviour is the same as calling Delete(0,Size()). 
The buffer is compressed before the function returns.
*/
	{

	if (iSize)
		Delete(0,iSize);
	Compress();
	}

EXPORT_C void CBufBase::Read(TInt aPos,TDes8 &aDes) const
//
// Read up to aDes.MaxLength() bytes.
//
/**
Reads data from the buffer into a descriptor.

Data, starting at the specified buffer position is written to the descriptor, 
filling the descriptor.

@param aPos Buffer position from which data is read: must be in range zero 
            to Size(). 
@param aDes On return, contains the data read from the buffer; its MaxLength() 
            specifies the amount of data to be read.
*/
	{

	Read(aPos,aDes,aDes.MaxLength());
	}

EXPORT_C void CBufBase::Read(TInt aPos,TDes8 &aDes,TInt aLength) const
/**
Reads the specified number of bytes of data from the buffer into a descriptor.

@param aPos    Buffer position from which data is read: must be in range zero 
               to (Size() minus the length of the data to be read). 
@param aDes    On return, contains data read from the buffer.
@param aLength The length of the data to be read.
*/
	{

	aDes.SetLength(aLength);
	Read(aPos,(TAny *)aDes.Ptr(),aLength);
	}

EXPORT_C void CBufBase::Read(TInt aPos,TAny *aPtr,TInt aLength) const
/**
Reads the specified number of bytes of data from the buffer into a specified 
address.

@param aPos    Buffer position from which data is read: must be in range zero 
               to (Size() minus the length of the data to be read). 
@param aPtr    The address into which the data should be read.
@param aLength The length of the data to be read.
*/
	{

	if (aLength==0)
		return;
	__ASSERT_ALWAYS(aLength>0,Panic(EBufReadLengthNegative));
	__ASSERT_ALWAYS((aPos+aLength)<=iSize,Panic(EBufReadBeyondEnd));
	TUint8 *pT=(TUint8 *)aPtr;
	while (aLength)
		{
		TPtr8 p=((CBufBase *)this)->Ptr(aPos);
		TInt s=Min(p.Length(),aLength);
		pT=Mem::Copy(pT,p.Ptr(),s);
		aLength-=s;
		aPos+=s;
		}
	}

EXPORT_C void CBufBase::Write(TInt aPos,const TDesC8 &aDes)
//
// Write aDes.Length() characters to the buffer. Does not cause any expansion.
//
/**
Writes data from a descriptor to the buffer.

The data in the descriptor overwrites the data in the buffer from the insertion 
point onwards.

No new space is allocated; this function cannot fail (provided the parameters 
are specified within the bounds of the buffer and descriptor).

No shuffling occurs; new data is written to the memory locations occupied 
by the data it overwrites.

@param aPos Buffer position at which data will begin to be written; must be 
            in range zero to (Size() minus the length of the data
            to be written). 
@param aDes Contains the data to be written. The length of data to be written
            is the descriptor length.
*/
	{

	Write(aPos,aDes.Ptr(),aDes.Length());
	}

EXPORT_C void CBufBase::Write(TInt aPos,const TDesC8 &aDes,TInt aLength)
//
// Write aDes.Length() characters to the buffer. Does not cause any expansion.
//
/**
Writes the specified number of bytes of data from a descriptor to the buffer.

The data in the descriptor overwrites the data in the buffer from the insertion 
point onwards.

No new space is allocated; this function cannot fail (provided the parameters 
are specified within the bounds of the buffer and descriptor).

No shuffling occurs; new data is written to the memory locations occupied 
by the data it overwrites.

@param aPos    Buffer position at which data will begin to be written; must be 
               in range zero to (Size() minus the length of the data to
               be written). 
@param aDes    Contains the data to be written.
@param aLength The length of the data to be written.
*/
	{

	Write(aPos,aDes.Ptr(),aLength);
	}

EXPORT_C void CBufBase::Write(TInt aPos,const TAny *aPtr,TInt aLength)
/**
Writes the specified number of bytes of data from the specified address to the 
buffer.

The data in the buffer is overwritten from the insertion point onwards.

No new space is allocated; this function cannot fail (provided the parameters 
are specified within the bounds of the buffer and descriptor).

No shuffling occurs: new data is written to the memory locations occupied 
by the data it overwrites.

@param aPos    Buffer position at which data will begin to be written; must be 
               in range zero to (Size() minus the length of the data to
               be written). 
@param aPtr    The address of the data to be written.
@param aLength The length of the data to be written.

@panic E32USER-CBase 7, if aLength is not positive
@panic E32USER-CBase 5, if aPos + aLength is greater than the number of
                        data bytes in the buffer, i.e. if the target appears
                        to be outside the buffer.
*/
	{

	if (aLength==0)
		return;
	__ASSERT_ALWAYS(aLength>0,Panic(EBufWriteLengthNegative));
	__ASSERT_ALWAYS((aPos+aLength)<=iSize,Panic(EBufWriteBeyondEnd));
	const TUint8 *pS=(const TUint8 *)aPtr;
	while (aLength)
		{
		TPtr8 p=Ptr(aPos);
		TInt s=Min(p.Length(),aLength);
		Mem::Copy((TAny *)p.Ptr(),pS,s);
		pS+=s;
		aLength-=s;
		aPos+=s;
		}
	}

EXPORT_C void CBufBase::InsertL(TInt aPos,const TDesC8 &aDes)
//
// Insert aDes.Length() bytes into the buffer.
//
/**
Inserts data into the buffer.

Data at and beyond the insertion position is moved to make way for the inserted 
data. Data before the insertion position remains in place.

Notes:

1. Insertion may require more buffer space to be allocated.

2. In the case of flat buffers, the buffer is extended by a ReAllocL() of the 
   buffer's heap cell, to the smallest multiple of the granularity that will 
   contain the data required. If this reallocation fails, the insertion is
   impossible and a leave occurs.

3. In the case of segmented buffers, a reallocation is performed if the segment 
   containing the insertion position has insufficient space, and
   immediately-neighbouring segments cannot be used to contain the new data.
   As many new segments as are necessary to contain the inserted data are
   allocated. Each new segment's length is the buffer's granularity.
   If extension or new allocation fails, a leave occurs.

4. Insertion may also require data to be shuffled. In the case of flat buffers, 
   data beyond the insertion point is shuffled up to create a gap; the new data 
   is then inserted into this gap. In the case of segmented buffers, shuffling 
   is minimised by inserting the new data into newly-allocated buffers, and
   shuffling only immediately-neighbouring buffers if possible. This may result
   in some wastage of space, but is much more time-efficient for large amounts
   of data.

@param aPos Buffer position before which the data will be inserted; must be 
            in range zero to Size().
@param aDes The data to be inserted; the length of the data is the descriptor
            length.

@leave KErrNoMemory If the insertion requires a bigger buffer, and the
       necessary allocation or re-allocation fails.
*/
	{

	InsertL(aPos,aDes.Ptr(),aDes.Length());
	}

EXPORT_C void CBufBase::InsertL(TInt aPos,const TDesC8 &aDes,TInt aLength)
//
// Insert aLength bytes into the buffer.
//
/**
Inserts the specified number of bytes of data from a descriptor into
the buffer.

aLength bytes of data from aDes are inserted into the buffer at aPos. Data at
and beyond the insertion position is moved to make way for the inserted data. 
Data before the insertion position remains in place.

Notes:

1. Insertion may require more buffer space to be allocated.

2. In the case of flat buffers, the buffer is extended by a ReAllocL() of the 
   buffer's heap cell, to the smallest multiple of the granularity that will 
   contain the data required. If this reallocation fails, the insertion is
   impossible and a leave occurs.

3. In the case of segmented buffers, a reallocation is performed if the segment 
   containing the insertion position has insufficient space, and
   immediately-neighbouring segments cannot be used to contain the new data.
   As many new segments as are necessary to contain the inserted data are
   allocated. Each new segment's length is the buffer's granularity.
   If extension or new allocation fails, a leave occurs.

4. Insertion may also require data to be shuffled. In the case of flat buffers, 
   data beyond the insertion point is shuffled up to create a gap: the new data 
   is then inserted into this gap. In the case of segmented buffers, shuffling 
   is minimised by inserting the new data into newly-allocated buffers,
   and shuffling  only immediately-neighbouring buffers if possible.
   This may result in some  wastage of space, but is much more time-efficient
   for large amounts of data.
   
@param aPos    Buffer position before which the data will be inserted; must be 
               in range zero to Size().
@param aDes    The data to be inserted.
@param aLength The length of data to be inserted. 

@leave KErrNoMemory If the insertion requires a bigger buffer, and the
       necessary allocation or re-allocation fails.
*/
	{

	InsertL(aPos,aDes.Ptr(),aLength);
	}

EXPORT_C void CBufBase::InsertL(TInt aPos,const TAny *aPtr,TInt aLength)
/**
Inserts bytes of data from the specified address into the buffer.

Inserts aLength bytes of data found at address aPtr into the buffer at aPos. 
Data at and beyond the insertion position is moved to make way for the inserted 
data. Data before the insertion position remains in place.

Notes:

1. Insertion may require more buffer space to be allocated.

2. In the case of flat buffers, the buffer is extended by a ReAllocL() of the 
   buffer's heap cell, to the smallest multiple of the granularity that will 
   contain the data required. If this reallocation fails, the insertion is
   impossible and a leave occurs.

2. In the case of segmented buffers, a reallocation is performed if the segment 
   containing the insertion position has insufficient space, and
   immediately-neighbouring segments cannot be used to contain the new data.
   As many new segments as are necessary to contain the inserted data are
   allocated. Each new segment's length is the buffer's granularity.
   If extension or new allocation fails, a leave occurs.

4. Insertion may also require data to be shuffled. In the case of flat buffers, 
   data beyond the insertion point is shuffled up to create a gap: the new data 
   is then inserted into this gap. In the case of segmented buffers, shuffling 
   is minimised by inserting the new data into newly-allocated buffers, and
   shuffling only immediately-neighbouring buffers if possible. This may result
   in some wastage of space, but is much more time-efficient for large amounts
   of data.

@param aPos    Buffer position before which the data will be inserted: must be 
               in range zero to Size().
@param aPtr    The address of the data to be inserted. 
@param aLength The length of the data to be inserted.
 
@leave KErrNoMemory If the insertion requires a bigger buffer, and the
       necessary allocation or re-allocation fails.
*/
	{

	if (aLength==0)
		return;
	__ASSERT_ALWAYS(aLength>0,Panic(EBufInsertLengthNegative));
	__ASSERT_ALWAYS(aPtr,Panic(EBufInsertBadPtr));
	DoInsertL(aPos,aPtr,aLength);
	}

EXPORT_C void CBufBase::ExpandL(TInt aPos,TInt aLength)
/**
Inserts an uninitialised region into the buffer.

Data at and beyond the insertion position is moved to make way for the inserted
region. Data before the insertion position remains in place.

Note:

1. The inserted region is not initialised. After using ExpandL(), you should 
   then use a series of Write()s to fill this region with data.

2. Use ExpandL() followed by a series of Write()s when you know the amount of 
   data to be inserted, in advance. It is more efficient than a series of
   InsertL()s. In addition, once the result of the ExpandL() has been checked,
   it is guaranteed that the Write()s do not leave, which can sometimes be
   useful.

@param aPos    Buffer position before which the region will be inserted; must 
               be in range zero to Size(). 
@param aLength The length of the region to be inserted.
*/
	{

	if (aLength==0)
		return;
	__ASSERT_ALWAYS(aLength>0,Panic(EBufInsertLengthNegative));
	DoInsertL(aPos,NULL,aLength);
	}

EXPORT_C void CBufBase::ResizeL(TInt aSize)
/**
Re-sizes the buffer to the specified size.

The new size can be larger or smaller than the existing size.

If the new size is larger than the existing size, the buffer is expanded by 
adding uninitialised data to the end of it.

If the new size is smaller than the existing size, the buffer is reduced; 
any data at the end of the buffer is lost.

Notes:

1. If the new size is larger than the existing size, the function is equivalent 
   to Delete(aSize,Size()-aSize).

2. If the new size is smaller than the existing size, the function is equivalent 
   to ExpandL((Size(),aSize-Size()).

3. The motivations for using ResizeL() are the same as those for using Delete() 
   and ExpandL().

@param aSize The new size of the buffer; this value must be greater than or 
             equal to zero.
*/
	{

	TInt excess=iSize-aSize;
	if (excess>0)
		Delete(aSize,excess);
	else
		ExpandL(iSize,-excess);
	}

EXPORT_C CBufFlat *CBufFlat::NewL(TInt anExpandSize)
/**
Allocates and constructs a flat buffer.

If there is insufficient memory available to allocate the flat buffer, the 
function leaves.

@param anExpandSize The granularity of buffer expansion. Additional space, 
                    when required, is always allocated in multiples of
                    this number. Note: although a value of zero is permitted
                    by this interface, it has no meaning, and risks raising
                    panics later during execution. We suggest that you pass
                    a positive value.
                                        
@return A pointer to the flat buffer object.

@panic E32USER-CBase 3 if the granularity is negative.
*/
	{

	return(new(ELeave) CBufFlat(anExpandSize));
	}

EXPORT_C CBufFlat::CBufFlat(TInt anExpandSize)
//
// Constructor
//
/**
@internalComponent
*/
	: CBufBase(anExpandSize)
	{

//	iMaxSize=0;
//	iPtr=NULL;
	}

EXPORT_C CBufFlat::~CBufFlat()
/** 
Destructor.

Frees all resources owned by the object, prior to its destruction.
Specifically, it frees the allocated cell used as a buffer.
*/
	{

	User::Free(iPtr);
	}

EXPORT_C void CBufFlat::Compress()
/**
Compresses the buffer so as to occupy minimal space.

This frees any unused memory at the end of the buffer.

@see CBufBase::Compress
*/
	{

	SetReserveL(iSize);
	}

EXPORT_C void CBufFlat::SetReserveL(TInt aSize)
/**
Specifies a minimum amount of space which the flat buffer should occupy.

If the required size is zero, the heap cell is deleted. If it is different 
from the current size, the heap cell is rellocated accordingly.

@param aSize The size of the buffer required. If there is no data in the
             buffer, i.e. Size() returns zero, then this value 
             can be zero, which causes the buffer's allocated heap cell
             to be deleted.

@panic E32USER-CBase 10, if aSize is negative.
@panic E32USER-CBase 11, if there is data in the buffer, and aSize is less than
       the value returned by Size().
*/
	{

	__ASSERT_ALWAYS(aSize>=0,Panic(EBufFlatReserveNegative));
	__ASSERT_ALWAYS(aSize>=iSize,Panic(EBufFlatReserveSetTooSmall));
    if (!aSize)
        {
        User::Free(iPtr);
        iPtr=NULL;
        }
    else
        iPtr=(TUint8 *)User::ReAllocL(iPtr,aSize);
    iMaxSize=aSize;
	}

EXPORT_C void CBufFlat::DoInsertL(TInt aPos,const TAny *aPtr,TInt aLength)
//
// Insert into the buffer. Can cause expansion.
//
	{

	__ASSERT_ALWAYS(aPos>=0 && aPos<=iSize,Panic(EBufFlatPosOutOfRange));
	TInt len=iSize+aLength;
	if (len>iMaxSize)
		{
		TInt r=len-iMaxSize;
		r=((r/iExpandSize)+1)*iExpandSize;
		SetReserveL(iMaxSize+r);
		}
	Mem::Copy(iPtr+aPos+aLength,iPtr+aPos,iSize-aPos);
	if (aPtr)
		Mem::Copy(iPtr+aPos,aPtr,aLength);
	iSize+=aLength;
	}

EXPORT_C void CBufFlat::Delete(TInt aPos,TInt aLength)
/**
Deletes data from the buffer.

During deletion, any data beyond the deleted data is shuffled up so that
the buffer contents are contiguous. No memory is freed.

@param aPos    Buffer position where the deletion will begin; must be in the 
               range zero to (Size() minus the length of the data
               to be deleted). 
@param aLength The number of bytes to be deleted.

@panic E32USER-CBase 12, if aPos is negative or is greater than the
       current size of the buffer.
@panic E32USER-CBase 13, if aPos + aLength is greater than the
       current size of the buffer.
       
@see CBufBase::Delete
*/
	{

	__ASSERT_ALWAYS(aPos>=0 && aPos<=iSize,Panic(EBufFlatPosOutOfRange));
	__ASSERT_ALWAYS((aPos+aLength)<=iSize,Panic(EBufFlatDeleteBeyondEnd));
	Mem::Copy(iPtr+aPos,iPtr+aPos+aLength,iSize-aLength-aPos);
	iSize-=aLength;
	}

EXPORT_C TPtr8 CBufFlat::Ptr(TInt aPos)
/**
Gets a pointer descriptor to represent the data starting at the specified
data byte through to the end of the contiguous region containing that byte.

Calculation of the pointer and length involves only a few machine instructions
and is independent of the data contained in the buffer.

@param aPos Buffer position: must be in range zero to Size().
	 
@return Descriptor representing the data starting at aPos to the end of
        the buffer.      	
*/
	{

	__ASSERT_ALWAYS(aPos>=0 && aPos<=iSize,Panic(EBufFlatPosOutOfRange));
	TInt len=iSize-aPos;
	return(TPtr8(iPtr+aPos,len,len));
	}

EXPORT_C TPtr8 CBufFlat::BackPtr(TInt aPos)
//
// Return a pointer to the buffer which has the maximum amount of data
// before aPos, and the amount of data remaining.  
//
/**
Gets a pointer descriptor to represent the data starting at the beginning
of the contiguous region containing that byte through to the byte immediately
preceding the specified byte.

The descriptor always points to the beginning of the buffer containing
the specified byte. Calculation of the pointer and length involves only a few
machine instructions and is independent of the data contained in the buffer.

@param aPos Buffer position: must be in range zero to Size().

@return Descriptor representing the back contiguous region. 

@see CBufBase::BackPtr
*/
	{

	__ASSERT_ALWAYS(aPos>=0 && aPos<=iSize,Panic(EBufFlatPosOutOfRange));
	return(TPtr8(iPtr,aPos,aPos));
	}

void CBufSeg::InsertIntoSegment(TBufSegLink *aSeg,TInt anOffset,const TAny *aPtr,TInt aLength)
//
// Insert into the segment.
//
	{

    if (aLength)
        {
        TUint8 *pS=((TUint8 *)(aSeg+1))+anOffset;
        Mem::Copy(pS+aLength,pS,aSeg->iLen-anOffset);
		if (aPtr)
			Mem::Copy(pS,aPtr,aLength);
        aSeg->iLen+=aLength;
        }
	}

void CBufSeg::DeleteFromSegment(TBufSegLink *aSeg,TInt anOffset,TInt aLength)
//
// Delete from the segment.
//
	{

    if (aLength)
        {
        TUint8 *pS=((TUint8 *)(aSeg+1))+anOffset;
        Mem::Copy(pS,pS+aLength,aSeg->iLen-anOffset-aLength);
        aSeg->iLen-=aLength;
        }
	}

void CBufSeg::FreeSegment(TBufSegLink *aSeg)
//
// Free an entire segment.
//
	{

    aSeg->Deque();
    User::Free(aSeg);
	}

void CBufSeg::SetSBO(TInt aPos)
//
// Set a segment-base-offset struct (SBO) to a new pos.
// If the initial psbo->seg is not NULL, it assumes that it is a valid
// SBO for a different position and counts relative to the initial SBO
// to set the desired position. If the initial psbo->seg is NULL, it starts
// scanning from the beginning ie pos=0.
// When the position is between segments A and B, there are two equivalent
// positions: (1) at the beginning of B and (2) at the end of A.
// Option (1) is suitable for referencing the data and deleting.
// Option (2) is best for insertion when A is not full.
// This function uses option (1) and will always set the SBO to the
// beginning of the next segment. It does however set to the end of the
// last segment when pos is equal to the number of bytes in the buffer.
//
	{

    __ASSERT_ALWAYS(aPos>=0 && aPos<=iSize,Panic(EBufSegPosOutOfRange));
    if (aPos==iSize)
        { // Positioning to end is treated as a special case
        iSeg=0;
        if (iSize)
            iBase=aPos-(iOffset=(iSeg=iQue.Last())->iLen);
        return;
        }
    TInt base=iBase;
	TBufSegLink *next;
    if ((next=iSeg)==NULL)
        { // anSbo is not valid - set to pos=0
        next=iQue.First();
        base=0;
        }
    if (aPos<base)
        { // Look to the left
        do
            {
            next=next->Prev();
            base-=next->iLen;
            } while (aPos<base);
        }
    else
        { // Look to the right
		TBufSegLink *nn;
        while (aPos>=(base+next->iLen) && !iQue.IsHead(nn=next->Next()))
            {
            base+=next->iLen;
            next=nn;
            }
        }
    iSeg=next;
    iBase=base;
    iOffset=aPos-base;
	__ASSERT_DEBUG(iOffset<=iExpandSize,Panic(EBufSegSetSBO));
	}

void CBufSeg::AllocSegL(TBufSegLink *aSeg,TInt aNumber)
//
// Allocate a number of segments.
//
	{

	for (TInt i=0;i<aNumber;i++)
		{
		TBufSegLink *pL=(TBufSegLink *)User::Alloc(sizeof(TBufSegLink)+iExpandSize);
		if (pL==NULL)
			{ // alloc failed - tidy up
			while (i--)
				FreeSegment(aSeg->Next());
			User::Leave(KErrNoMemory);
			}
		new(pL) TBufSegLink;
		pL->Enque(aSeg);
		}
	}

EXPORT_C CBufSeg *CBufSeg::NewL(TInt anExpandSize)
/**
Allocates and constructs a segmented buffer.

If there is insufficient memory available to allocate the segmented buffer, 
the function leaves.

@param anExpandSize The granularity of the buffer. Each segment contains (in 
                    addition to 16 bytes of overhead) this number of bytes for
                    data. Note: although a value of zero is permitted by this
                    interface, it has no meaning, and risks raising panics later
                    during execution. We suggest that you pass a positive value. 
                    
@return If successful, a pointer to the segmented buffer object.

@panic E32USER-CBase 3 if the granularity is negative.
*/
	{

	return(new(ELeave) CBufSeg(anExpandSize));
	}

EXPORT_C CBufSeg::CBufSeg(TInt anExpandSize)
//
// Constructor
//
	: CBufBase(anExpandSize)
	{

//	iSeg=NULL;
	}

EXPORT_C CBufSeg::~CBufSeg()
/**
Destructor.

Frees all resources owned by the object, prior to its destruction.

Specifically, it frees all segments allocated to the buffer.
*/
	{

	Delete(0,iSize);
	}

EXPORT_C void CBufSeg::Compress()
/**
Compresses the buffer so as to occupy minimal space.

Fills any space in each segment of the buffer by moving contents from the next
segment to the current one.  Where this activity results in empty segments,
it frees the memory associated with these segments.

@see CBufBase::Compress
*/
	{

    if (!iSize)
        return;
    iSeg=NULL; // Invalidate current position
    TBufSegLink *p1=iQue.First();
    TBufSegLink *p2;
    while (!iQue.IsHead(p2=p1->Next()))
        {
        TInt rem=iExpandSize-p1->iLen;
        if (rem==0)
            {
            p1=p2;
            continue; // Full
            }
        if (rem>=p2->iLen)
            { // Zap the next segment
            InsertIntoSegment(p1,p1->iLen,p2+1,p2->iLen);
            FreeSegment(p2);
            continue;
            }
        InsertIntoSegment(p1,p1->iLen,p2+1,rem);  // Make full
        DeleteFromSegment(p2,0,rem);
        p1=p2;
        }
	}

EXPORT_C void CBufSeg::DoInsertL(TInt aPos,const TAny *aPtr,TInt aLength)
//
// Insert data at the specified position. This is quite tricky.
// In general, the data to be copied may be broken down into the
// following elements:
//     s1 bytes into the current segment (p1)
//     nseg-1 segments of self->sgbuf.hd.len (ie full segments)
//     s2 bytes into segment nseg
//     s3 bytes into the next segment (p2)
// where p2 is the next segment before the insertion of nseg new segments.
// In addition, any remaining data to the right of the insertion point must
// be moved appropriately. In general, r1 bytes must be moved into segment
// nseg (r2 bytes) and segment p2 (r3 bytes) where r1=r2+r3.
//
	{

    SetSBO(aPos);
    TInt slen=iExpandSize;
    TInt ofs=iOffset;	
	TInt ll=0;
    TInt s1=0;
    TInt r1=0;
    TBufSegLink *p1=(TBufSegLink *)(&iQue); 
    TBufSegLink *p2=p1->Next(); 
	TUint8 *pR=NULL;
    if (iSize)	
        {
        p1=iSeg;	
     	if (!iQue.IsHead(p2=p1->Prev()) && ofs==0 && p2->iLen<slen)
        	{  
        	iSeg=p1=p2;     
        	iOffset=ofs=p1->iLen;
        	iBase-=ofs;     
        	}
        s1=slen-ofs; 
        if (s1>aLength)
            s1=aLength; 
		TInt r2=slen-p1->iLen; 
        if (aLength>r2)	
            { 
            pR=((TUint8 *)(p1+1))+ofs; 
            r1=aLength-r2; 
			r2=p1->iLen-ofs; 
            if (r1>r2) 
                r1=r2; 
            else
                pR+=(r2-r1); 
            }
		p2=p1->Next();
        ll=slen-p1->iLen;
		if (!iQue.IsHead(p2))
		  	ll+=slen-p2->iLen;
        }
    TUint8 *pB=((TUint8 *)aPtr)+s1; 
    TInt lrem=aLength-s1;
    TBufSegLink *pP=p1;
    if (aLength>ll)
        {// Need some more segments
		TInt nseg=(slen-1+aLength-ll)/slen;
        AllocSegL(p1,nseg); // Could leave
        while (nseg--)
            { // Copy into allocated segments
            pP=pP->Next();
            TInt gap=slen;
            if (lrem<gap)
                gap=lrem;
			InsertIntoSegment(pP,0,aPtr==NULL ? NULL : pB,gap);
            pB+=gap;
            lrem-=gap;
            }
        }
    if (lrem) 
        {	
		InsertIntoSegment(p2,0,aPtr==NULL ? NULL : pB,lrem); 
        InsertIntoSegment(p2,lrem,pR,r1); 
        }
    else 
        { 
        TInt r2=0;
        if (pP!=p1)
            {
            r2=slen-pP->iLen; 
            if (r2>r1)
                r2=r1;	
            }
        InsertIntoSegment(pP,pP->iLen,pR,r2); // Moved from p1 
        InsertIntoSegment(p2,0,pR+r2,r1-r2); // Also moved from p1
        }
    p1->iLen-=r1;
	InsertIntoSegment(p1,ofs,aPtr,s1);
    iSize+=aLength;
	}

EXPORT_C void CBufSeg::Delete(TInt aPos,TInt aLength)
/**
Deletes data from the buffer.

During deletion, shuffling is minimised by deleting intermediate segments
and allowing segments to contain less data than the buffer granularity.

@param aPos    Buffer position where the deletion will begin; must be in the 
               range zero to (Size() minus the length of the data
               to be deleted). 
@param aLength The number of bytes to be deleted.

@see CBufBase::Delete
*/
	{

    if (aLength==0)
        return;
    SetSBO(aPos);
    TInt ofs=iOffset;
    __ASSERT_ALWAYS((iBase+ofs+aLength)<=iSize,Panic(EBufSegDeleteBeyondEnd));
    iSize-=aLength;
    TBufSegLink *p1=iSeg;
	TBufSegLink *p2;
    TInt rem=p1->iLen-ofs;
    FOREVER
        {
        p2=p1->Next();
        TInt gap=aLength;
        if (gap>rem)
            gap=rem;
        DeleteFromSegment(p1,ofs,gap);
        if (p1->iLen==0)
            {
            iSeg=NULL;
            FreeSegment(p1);
            }
        p1=p2;
        if ((aLength-=gap)==0)
            break;
        rem=p1->iLen;
        ofs=0;
        }
    if (iSize)
        {
        p1=p2->Prev();
        if (!iQue.IsHead(p1) && !iQue.IsHead(p2))
            {
            if ((p1->iLen+p2->iLen)<=iExpandSize)
                { // Join to the right
                InsertIntoSegment(p1,p1->iLen,p2+1,p2->iLen);
                FreeSegment(p2);
                }
            }
        }
    SetSBO(aPos);
	}

EXPORT_C TPtr8 CBufSeg::Ptr(TInt aPos)
/**
Gets a pointer descriptor to represent the data starting at the specified
data byte through to the end of the contiguous region containing that byte.

The time needed for calculation of the pointer depends on how many segments
there are in the buffer, and how near the target segment is to the segment
which was last used in the buffer.

@param aPos Buffer position: must be in range zero to Size().
	 
@return Descriptor representing the data starting at aPos to the end of
        the contiguous region containing that byte.     	
*/
	{

    if (iSize==0)
		return(TPtr8(NULL,0,0));
    SetSBO(aPos);
	TInt len=iSeg->iLen-iOffset;
    return(TPtr8(((TUint8 *)(iSeg+1))+iOffset,len,len));
	}

EXPORT_C TPtr8 CBufSeg::BackPtr(TInt aPos)
//
// Return a pointer to the buffer which has the maximum amount of data
// before aPos, and the amount of data remaining.  
//
/**
Gets a pointer descriptor to represent the data starting at the beginning
of the contiguous region containing that byte through to the byte immediately
preceding the specified byte.

The descriptor always points to the beginning of the segment containing the
specified byte. The time needed for calculation of the pointer depends on how
many segments there are in the buffer, and how near the target segment is to
the segment which was last used in the buffer.

@param aPos Buffer position: must be in range zero to Size().

@return Descriptor representing the back contiguous region. 

@see CBufBase::BackPtr
*/


	{

    if (aPos==0)
		return(TPtr8(NULL,0,0));
    SetSBO(aPos);
    if (iOffset)
        return(TPtr8((TUint8 *)(iSeg+1),iOffset,iOffset));
    TBufSegLink *pL=iSeg->Prev();
	TInt len=pL->iLen;
	return(TPtr8((TUint8 *)(pL+1),len,len));
	}

