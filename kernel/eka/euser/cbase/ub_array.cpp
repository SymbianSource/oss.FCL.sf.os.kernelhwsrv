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
// e32\euser\cbase\ub_array.cpp
// 
//

#include "ub_std.h"

struct SVarRec
	{
	TInt len;
	TAny *data;
	};

NONSHARABLE_CLASS(TSwapArray) : public TSwap
	{
public:
	inline TSwapArray(CBufBase *aBase,TInt aRecordLength);
	TUint8 *At(TInt anIndex) const;
	virtual void Swap(TInt aLeft,TInt aRight) const;
private:
	CBufBase *iBase;
	TInt iLength;
	};
inline TSwapArray::TSwapArray(CBufBase *aBase,TInt aRecordLength)
	: iBase(aBase),iLength(aRecordLength)
	{}

TUint8 *TSwapArray::At(TInt anIndex) const
//
// Return a pointer to the array element
//
	{
	
	return((TUint8 *)iBase->Ptr(anIndex*iLength).Ptr());
	}

void TSwapArray::Swap(TInt aLeft,TInt aRight) const
//
// Swap two elements of the array.
//
	{
	
	Mem::Swap(At(aLeft),At(aRight),iLength);
	}




EXPORT_C TKeyArrayFix::TKeyArrayFix(TInt anOffset,TKeyCmpText aType)
	: TKey(anOffset,aType)
/**
Constructs the characteristics of a descriptor key.

No length value is passed as this is taken from the descriptor type key.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of comparison to be made 
                between two descriptor keys.
@see TKeyCmpText
*/
	{}




EXPORT_C TKeyArrayFix::TKeyArrayFix(TInt anOffset,TKeyCmpText aType,TInt aLength)
	: TKey(anOffset,aType,aLength)
/**
Constructs the characteristics of a text key.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of comparison to be made 
                between two text keys.
@param aLength The length of the text key.

@see TKeyCmpText
*/
	{}




EXPORT_C TKeyArrayFix::TKeyArrayFix(TInt anOffset,TKeyCmpNumeric aType)
	: TKey(anOffset,aType)
/**
Constructs the characteristics of a numeric key.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of the numeric key.

@see TKeyCmpNumeric
*/
	{}




EXPORT_C void TKeyArrayFix::Set(CBufBase *aBase,TInt aRecordLength)
//
// Set the base and record length
//
	{

	iBase=aBase;
	iRecordLength=aRecordLength;
	}

EXPORT_C TAny *TKeyArrayFix::At(TInt anIndex) const
//
// Return an address in the array.
//
	{

	if (anIndex==KIndexPtr)
		return((TUint8 *)iPtr+iKeyOffset); 			
	return((TAny *)(iBase->Ptr(anIndex*iRecordLength).Ptr()+iKeyOffset));
	}




EXPORT_C TKeyArrayVar::TKeyArrayVar(TInt anOffset,TKeyCmpText aType)
	: TKey(anOffset,aType)
/**
Constructs the characteristics of a descriptor key.

No length value is passed as this is taken from the descriptor type key.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of comparison to be made 
                between two descriptor keys.
                
@see TKeyCmpText
*/
	{}




EXPORT_C TKeyArrayVar::TKeyArrayVar(TInt anOffset,TKeyCmpText aType,TInt aLength)
	: TKey(anOffset,aType,aLength)
/**
Constructs the characteristics of a text key.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of comparison to be made 
                between two text keys.
@param aLength  The length of the text key.

@see TKeyCmpText
*/
	{}




EXPORT_C TKeyArrayVar::TKeyArrayVar(TInt anOffset,TKeyCmpNumeric aType)
	: TKey(anOffset,aType)
/**
Constructs the characteristics of a numeric key.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of the numeric key.

@see TKeyCmpNumeric
*/
	{}




EXPORT_C void TKeyArrayVar::Set(CBufBase *aBase)
//
// Set the private variable iBase to aBase.
//
	{

	iBase=aBase;
	}

EXPORT_C TAny *TKeyArrayVar::At(TInt anIndex) const
//
// Return an address in the array.
//
	{

	if (anIndex==KIndexPtr)
		return((TUint8 *)iPtr+iKeyOffset);   
	SVarRec *pV=(SVarRec *)iBase->Ptr(anIndex*sizeof(SVarRec)).Ptr();
	return(((TUint8 *)pV->data)+iKeyOffset);
	}




EXPORT_C TKeyArrayPak::TKeyArrayPak(TInt anOffset,TKeyCmpText aType)
	: TKeyArrayVar(anOffset,aType)
/**
Constructs the characteristics of a descriptor key.

No length value is passed as this is taken from the descriptor type key.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of comparison to be made 
                between two descriptor keys.
                
@see TKeyCmpText
*/
	{}




EXPORT_C TKeyArrayPak::TKeyArrayPak(TInt anOffset,TKeyCmpText aType,TInt aLength)
	: TKeyArrayVar(anOffset,aType,aLength)
/**
Constructs the characteristics of a text key.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of comparison to be made 
                between two text keys.
@param aLength  The length of the text key.

@see TKeyCmpText
*/
	{}




EXPORT_C TKeyArrayPak::TKeyArrayPak(TInt anOffset,TKeyCmpNumeric aType)
	: TKeyArrayVar(anOffset,aType)
/**
Constructs the characteristics of a numeric key.

@param anOffset The offset of the key from the start of an array element.
@param aType    An enumeration which defines the type of the numeric key.

@see TKeyCmpNumeric
*/
	{}




EXPORT_C void TKeyArrayPak::Set(CBufBase *aBase)
//
// Set the private variable iBase to aBase.
//
	{

	iBase=aBase;
	iCacheIndex=0;
	iCacheOffset=0;
	}

EXPORT_C TAny *TKeyArrayPak::At(TInt anIndex) const
//
// Return a pointer to the data in the record with index anIndex.
//
	{
//
// When anIndex is equal to KIndexPtr (HighValues) this means that we should return the address of
// the iPtr+iKeyOffset which will have been set up by the TKeyArrayPak constructor.
//
	if (anIndex==KIndexPtr)
		return((TUint8 *)iPtr+iKeyOffset); 
//
// Otherwise get the offset into the buffer of the record with index anIndex.
//
	TInt offset=0;
 	TInt curIndex=0;
	if (iCacheIndex<=anIndex)
		{
		curIndex=iCacheIndex;
		offset=iCacheOffset;
		}
	TAny *pRecord=(TAny *)iBase->Ptr(offset).Ptr();
	while (curIndex<anIndex)
		{
		TInt lenData=(*(TInt *)pRecord);
		offset+=Align4(lenData)+sizeof(TUint);
		pRecord=(TAny *)iBase->Ptr(offset).Ptr();
		curIndex++;
		}
	(TInt &)iCacheIndex=anIndex;
	(TInt &)iCacheOffset=offset;
	TAny *pData=(TAny *)((TInt *)pRecord + 1);
 	return((TUint8 *)pData+iKeyOffset);
	}

EXPORT_C CArrayFixBase::CArrayFixBase(TBufRep aRep,TInt aRecordLength,TInt aGranularity)
//
// Constructor
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(aRecordLength>0,Panic(EArrayFixInvalidLength));
	__ASSERT_ALWAYS(aGranularity>0,Panic(EArrayFixInvalidGranularity));
//	iCount=0;
//	iBase=NULL;
	iLength=aRecordLength;
	iGranularity=aGranularity;
	iCreateRep=aRep;
	}

EXPORT_C CArrayFixBase::~CArrayFixBase()
/**
Destructor.

This frees all resources owned by the object, prior to its destruction.
*/
	{

	delete iBase;
	}

EXPORT_C void CArrayFixBase::Compress()
/**
Compresses the array.

The function removes the excess space from the array buffer. The effect is 
to reduce the memory allocated to the array buffer so that it is just
sufficient to contain the elements of the array. This applies to both flat
and segmented array buffers.

If the array is empty, then the memory allocated to the array buffer is freed.
*/
	{

	if (iBase)
		iBase->Compress();
	}

EXPORT_C void CArrayFixBase::Reset()
/**
Deletes all elements from the array and frees the memory allocated 
to the array buffer.
*/
	{

	iCount=0;
	if (iBase)
		iBase->Reset();
	}

EXPORT_C TInt CArrayFixBase::Sort(TKeyArrayFix &aKey)
/**
Sorts the elements of the array into key sequence.

@param aKey The key object defining the properties of the key. 

@return KErrNone if the sort completes successfully.
        KErrGeneral if the stack overflows
*/
	{

	if (iCount==0)
		return KErrNone;
	TSwapArray aSwap(iBase,iLength);
	SetKey(aKey);
	return(User::QuickSort(iCount,aKey,aSwap));
	}

EXPORT_C TAny *CArrayFixBase::At(TInt anIndex) const
//
// Index into the array.
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount,Panic(EArrayIndexOutOfRange));
	return((TAny *)iBase->Ptr(anIndex*iLength).Ptr());
	}

EXPORT_C TAny *CArrayFixBase::End(TInt anIndex) const
//
// Return a pointer past contiguous elements starting at anIndex.
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<=iCount,Panic(EArrayIndexOutOfRange));
	TPtr8 p=iBase->Ptr(anIndex*iLength);
	return((TAny *)(p.Ptr()+p.Length()));
	}

EXPORT_C TAny *CArrayFixBase::Back(TInt anIndex) const
//
// Return a pointer to contiguous elements before anIndex.
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<=iCount,Panic(EArrayIndexOutOfRange));
	TPtr8 p=iBase->BackPtr(anIndex*iLength);
	return((TAny *)p.Ptr());
	}

EXPORT_C void CArrayFixBase::Delete(TInt anIndex)
/**
Deletes a single element from the array at a specified position.

Deleting elements from the array does not cause the array buffer to be
automatically compressed. Call CArrayFixBase::Compress() to return excess space
to the heap.

@param anIndex The position within the array at which to delete the element, 
               This is a value relative to zero. 

@panic E32USER-CBase 21, if anIndex is negative or is greater 
                         than or equal to the number of elements currently
                         in the array.
@see CArrayFixBase::Compress
*/
	{

	Delete(anIndex,1);
	}

EXPORT_C void CArrayFixBase::Delete(TInt anIndex,TInt aCount)
/**
Deletes one or more contiguous elements from the array, starting at a specific 
position.

Deleting elements from the array does not cause the array buffer to be
automatically compressed. Call CArrayFixBase::Compress() to return excess space
to the heap.

@param anIndex The position within the array from where deletion of elements 
               is to start. The position is relative to zero, i.e. zero implies
               that elements, starting with the first, are deleted from the
               array.
                
@param aCount  The number of contiguous elements to be deleted from the array. 
  
@panic E32USER-CBase 21, if anIndex is negative, or is greater than or equal to
                         the number of elements currently in the array.
@panic E32USER-CBase 22, if aCount is negative.
@panic E32USER-CBase 29, if the sum of anIndex and aCount is greater than or equal
                         to the number of elements currently in the array.
*/
	{

	if (aCount==0)
		return;
	__ASSERT_ALWAYS(aCount>0,Panic(EArrayCountNegative));
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount,Panic(EArrayIndexOutOfRange));
	__ASSERT_ALWAYS(anIndex+aCount<=iCount,Panic(EArrayCountTooBig));
	iBase->Delete(anIndex*iLength,aCount*iLength);
	iCount-=aCount;
	}

EXPORT_C TAny *CArrayFixBase::ExpandL(TInt anIndex)
//
// Expand the array to make room for a new record at anIndex.
//
/**
@internalComponent
*/
	{

	if (iBase==NULL)
		iBase=(*iCreateRep)(iLength*iGranularity);
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<=iCount,Panic(EArrayIndexOutOfRange));
	iBase->ExpandL(anIndex*iLength,iLength);
	++iCount;
	return((TAny *)iBase->Ptr(anIndex*iLength).Ptr());
	}

EXPORT_C TInt CArrayFixBase::Find(const TAny *aPtr,TKeyArrayFix &aKey,TInt &anIndex) const
//
// Find in the array using a sequential search.
//
/**
@internalComponent
*/
	{

	if (iCount==0)
	    {
	    anIndex=0;
		return(-1);
		}
	aKey.SetPtr(aPtr);
	SetKey(aKey);
	TInt r=1;
	TInt i=0;
	while (i<Count())
		{
		TInt j=aKey.Compare(i,KIndexPtr);
		if (j==0)
			{
			r=j;
			break;
			}
		i++;
		}
	anIndex=i;
	return(r);
	}

EXPORT_C TInt CArrayFixBase::FindIsq(const TAny *aPtr,TKeyArrayFix &aKey,TInt &anIndex) const
//
// Find in the array using a binary search.
//
/**
@internalComponent
*/
	{

	if (iCount==0)
	    {
	    anIndex=0;
		return(-1);
		}
	aKey.SetPtr(aPtr);
	SetKey(aKey);
    return(User::BinarySearch(Count(),aKey,anIndex));
	}

EXPORT_C void CArrayFixBase::InsertL(TInt anIndex,const TAny *aPtr)
//													  
// Insert a record into the array.
//
/**
@internalComponent
*/
	{

	InsertL(anIndex,aPtr,1);
	}

EXPORT_C void CArrayFixBase::InsertL(TInt anIndex,const TAny *aPtr,TInt aCount)
//													  
// Insert aCount records into the array.
//
/**
@internalComponent
*/
	{

	if (aCount==0)
		return;
	if (iBase==NULL)
		iBase=(*iCreateRep)(iLength*iGranularity);
	__ASSERT_ALWAYS(aCount>0,Panic(EArrayCountNegative2));
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<=iCount,Panic(EArrayIndexOutOfRange));
	iBase->InsertL(anIndex*iLength,aPtr,aCount*iLength);
	iCount+=aCount;
	}

EXPORT_C void CArrayFixBase::InsertRepL(TInt anIndex,const TAny *aPtr,TInt aReplicas)
//													  
// Insert aReplicas copies  of a record into the array.
//
/**
@internalComponent
*/
	{

	if (aReplicas==0)
		return;
	if (iBase==NULL)
		iBase=(*iCreateRep)(iLength*iGranularity);
	__ASSERT_ALWAYS(aReplicas>0,Panic(EArrayReplicasNegative));
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<=iCount,Panic(EArrayIndexOutOfRange));
	TInt pos=anIndex*iLength;
	TInt len=aReplicas*iLength;
	iBase->ExpandL(pos,len);
	for (TInt end=pos+len;pos<end;pos+=iLength)
		iBase->Write(pos,aPtr,iLength);
	iCount+=aReplicas;
	}

EXPORT_C TInt CArrayFixBase::InsertIsqL(const TAny *aPtr,TKeyArrayFix &aKey)
//
// Insert in sequence, no duplicates allowed.
//
/**
@internalComponent
*/
	{

	TInt i=0;
	TInt r=FindIsq(aPtr,aKey,i);
	if (r==0) // a duplicate, leave
		User::Leave(KErrAlreadyExists);
	InsertL(i,aPtr,1);
	return(i);
	}

EXPORT_C TInt CArrayFixBase::InsertIsqAllowDuplicatesL(const TAny *aPtr,TKeyArrayFix &aKey)
//
// Insert in sequence, allow duplicates.
//
/**
@internalComponent
*/
	{

	TInt i=0;
	TInt r=FindIsq(aPtr,aKey,i);
	if (r==0) // a duplicate, insert after
		++i;
	InsertL(i,aPtr,1);
	return(i);
	}

EXPORT_C void CArrayFixBase::ResizeL(TInt aCount,const TAny *aPtr)
//
// Resize the array to contain aCount records, copying a record into any new slots.
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(aCount>=0,Panic(EArrayCountNegative3));
	TInt excess=iCount-aCount;
	if (excess>0)
		Delete(aCount,excess);
	else
		InsertRepL(iCount,aPtr,-excess);
	}

EXPORT_C void CArrayFixBase::SetReserveFlatL(TInt aCount) 
//
// Reserve space to contain aCount items. Only for flat arrays!
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(aCount>=iCount,Panic(EArrayReserveTooSmall));
	if (iBase==NULL)
		iBase=(*iCreateRep)(iLength*iGranularity);
	((CBufFlat*)iBase)->SetReserveL(iLength*aCount);		// dodgy cast. Can we assert the type?
	}

EXPORT_C void CArrayFixBase::SetKey(TKeyArrayFix &aKey) const
//
// Set the key data.
//
/**
@internalComponent
*/
	{

	aKey.Set(iBase,iLength);
	}

EXPORT_C TInt CArrayFixBase::CountR(const CBase *aPtr)
//
// Return the number of items in the array.
//
/**
@internalComponent
*/
	{

	return(((CArrayFixBase *)aPtr)->Count());
	}

EXPORT_C const TAny *CArrayFixBase::AtR(const CBase *aPtr,TInt anIndex)
//
// Return the address of an item in the array.
//
/**
@internalComponent
*/
	{

	return(((CArrayFixBase *)aPtr)->At(anIndex));
	}

EXPORT_C CArrayVarBase::CArrayVarBase(TBufRep aRep,TInt aGranularity)
//
// Constructor
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(aGranularity>0,Panic(EArrayVarInvalidGranularity));
//	iCount=0;
//	iBase=NULL;
	iGranularity=aGranularity;
	iCreateRep=aRep;
	}

EXPORT_C CArrayVarBase::~CArrayVarBase()
/**
Destructor.

Frees all resources owned by the object, prior to its destruction.
*/
	{

	if (iBase)
		{
		Reset();
		delete iBase;
		}
	}

EXPORT_C TInt CArrayVarBase::Length(TInt anIndex) const
/**
Gets the length of a specific element.

@param anIndex The position of the element within the array. The position 
               is relative to zero, (i.e. the first element in the array is
               at position 0). 

@return The length of the element at position anIndex.

@panic E32USER-CBase 21, if anIndex is negative or is greater than the number
       of elements currently in the array.
*/
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount,Panic(EArrayIndexOutOfRange));
	return(((SVarRec *)iBase->Ptr(anIndex*sizeof(SVarRec)).Ptr())->len);
	}

EXPORT_C void CArrayVarBase::Compress()
/**
Removes excess space from the array buffer.

The effect is to reduce the memory allocated to the array buffer so that it is
just sufficient to represent the array. This applies to both flat and segmented
array buffers.

If the array is empty, then the memory allocated to the array buffer is freed.
*/
	{

	if (iBase)
		iBase->Compress();
	}

EXPORT_C void CArrayVarBase::Reset()
/**
Deletes all elements from the array and frees the memory allocated to the array 
buffer.

As each element of a variable array is contained within its own heap cell, 
this function has the effect of freeing all such cells.
*/
	{

	Delete(0,Count());
	}

EXPORT_C TInt CArrayVarBase::Sort(TKeyArrayVar &aKey)
/**
Sorts the elements of the array into key sequence.

@param aKey The key object defining the properties of the key. 

@return KErrNone, if the sort completes successfully.
        KErrGeneral, if the stack overflows.
*/
	{

	if (iCount==0)
		return KErrNone;
	TSwapArray aSwap(iBase,sizeof(SVarRec));
	SetKey(aKey);
	return(User::QuickSort(iCount,aKey,aSwap));
	}

EXPORT_C TAny * CArrayVarBase::At(TInt anIndex) const
//
// Return a pointer to the data in the array.
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount,Panic(EArrayIndexOutOfRange));
	return(((SVarRec *)iBase->Ptr(anIndex*sizeof(SVarRec)).Ptr())->data);
	}

EXPORT_C void CArrayVarBase::Delete(TInt anIndex)
/**
Removes one element from the array.

Deleting elements from the array does not cause the array buffer to be
automatically compressed. Call CArrayVarBase::Compress() to return excess
space to the heap.

@param anIndex The position within the array of the element to delete. The 
               position is relative to zero.
               
@panic E32USER-CBase 21, if anIndex is negative or  greater than the number
       of elements currently in the array.
*/
	{

	Delete(anIndex,1);
	}

EXPORT_C void CArrayVarBase::Delete(TInt anIndex,TInt aCount)
/**
Removes one or more contiguous elements from the array, starting at the
specified position.

Deleting elements from the array does not cause the array buffer to be
automatically compressed. Call CArrayVarBase::Compress() to return excess
space to the heap.

@param anIndex The position within the array from where deletion of elements is
               to start. The position is relative to zero, i.e. zero implies
               that elements, starting with the first, are deleted from the
               array.

@param aCount  The number of elements to be deleted from the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than or equal to the
       number of elements currently in the array.
@panic E32USER-CBase 25, if aCount is negative.
@panic E32USER-CBase 29, if the sum of anIndexPos and aCount is greater than
       the number of elements currently in the array.
*/
	{

	if (aCount==0)
		return;
	__ASSERT_ALWAYS(aCount>0,Panic(EArrayCountNegative4));
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount,Panic(EArrayIndexOutOfRange));
	TInt end=anIndex+aCount;
	__ASSERT_ALWAYS(end<=iCount,Panic(EArrayCountTooBig));
	for (TInt i=anIndex;i<end;i++)
		{
		TPtr8 p=iBase->Ptr(i*sizeof(SVarRec));
		User::Free(((SVarRec *)p.Ptr())->data);
		}
	iBase->Delete(anIndex*sizeof(SVarRec),aCount*sizeof(SVarRec));
	iCount-=aCount;
	}

EXPORT_C TAny *CArrayVarBase::ExpandL(TInt anIndex,TInt aLength)
//
// Expand the array at anIndex.
//
/**
@internalComponent
*/
	{

	if (iBase==NULL)
		iBase=(*iCreateRep)(sizeof(SVarRec)*iGranularity);
	__ASSERT_ALWAYS(aLength>=0,Panic(EArrayLengthNegative));
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<=iCount,Panic(EArrayIndexOutOfRange));
	TAny *pV=User::AllocL(aLength);
	SVarRec s;
	s.data=pV;
	s.len=aLength;
	TRAPD(r,iBase->InsertL(anIndex*sizeof(SVarRec),&s,sizeof(SVarRec)));
	if (r!=KErrNone)
		{
		User::Free(pV);
		User::Leave(r);
		}
	iCount++;
	return(pV);
	}

EXPORT_C TInt CArrayVarBase::Find(const TAny *aPtr,TKeyArrayVar &aKey,TInt &anIndex) const
//
// Find using a sequential search.
//
/**
@internalComponent
*/
	{

	if (iCount==0)
	    {
	    anIndex=0;
		return(-1);
		}
	aKey.SetPtr(aPtr);
	SetKey(aKey);
	TInt ret=(-1);
	TInt i=0;
	while (i<Count())
		{
		TInt j=aKey.Compare(i,KIndexPtr);
		if (j==0)
			{
			ret=j;
			break;
			}
		i++;
		}
	anIndex=i;
	return(ret);
	}

EXPORT_C TInt CArrayVarBase::FindIsq(const TAny *aPtr,TKeyArrayVar &aKey,TInt &anIndex) const
//
// Find using a binary search.
//
/**
@internalComponent
*/
	{

	if (iCount==0)
	    {
	    anIndex=0;
		return(-1);
		}
	aKey.SetPtr(aPtr);
	SetKey(aKey);
	return(User::BinarySearch(Count(),aKey,anIndex));
	}

EXPORT_C void CArrayVarBase::InsertL(TInt anIndex,const TAny *aPtr,TInt aLength)
//
// Insert a new record in the array.
//
/**
@internalComponent
*/
	{

	TAny *pV=ExpandL(anIndex,aLength);
    Mem::Copy(pV,aPtr,aLength);
	}

EXPORT_C TInt CArrayVarBase::InsertIsqL(const TAny *aPtr,TInt aLength,TKeyArrayVar &aKey)
//
// Insert in sequence, no duplicates allowed.
//
/**
@internalComponent
*/
	{

	TInt i=0;
	TInt r=FindIsq(aPtr,aKey,i);
	if (r==0) // a duplicate, leave
		User::Leave(KErrAlreadyExists);
	InsertL(i,aPtr,aLength);
	return(i);
	}

EXPORT_C TInt CArrayVarBase::InsertIsqAllowDuplicatesL(const TAny *aPtr,TInt aLength,TKeyArrayVar &aKey)
//
// Insert in sequence, allow duplicates.
//
/**
@internalComponent
*/
	{

	TInt i=0;
	TInt r=FindIsq(aPtr,aKey,i);
	if (r==0) // a duplicate, insert after
		++i;
	InsertL(i,aPtr,aLength);
	return(i);
	}

EXPORT_C void CArrayVarBase::SetKey(TKeyArrayVar &aKey) const
//
// Set the key data.
//
/**
@internalComponent
*/
	{

	aKey.Set(iBase);
	}

EXPORT_C TInt CArrayVarBase::CountR(const CBase *aPtr)
//
// Return the number of items in the array.
//
/**
@internalComponent
*/
	{

	return(((CArrayVarBase *)aPtr)->Count());
	}

EXPORT_C const TAny *CArrayVarBase::AtR(const CBase *aPtr,TInt anIndex)
//
// Return the address of an item in the array.
//
/**
@internalComponent
*/
	{

	return(((CArrayVarBase *)aPtr)->At(anIndex));
	}

EXPORT_C CArrayPakBase::CArrayPakBase(TBufRep aRep,TInt aGranularity)
//
// Constructor
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(aGranularity>0,Panic(EArrayPakInvalidGranularity));
//	iCount=0;
//	iBase=NULL;
	iGranularity=aGranularity;
	iCreateRep=aRep;
	}

EXPORT_C CArrayPakBase::~CArrayPakBase()
/**
Destructor.

Frees all resources owned by the object, prior to its destruction.
*/
	{

	if (iBase)
		{
		Reset();
		delete iBase;
		}
	}

EXPORT_C TInt CArrayPakBase::Length(TInt anIndex) const
/**
Gets the length of the specified element.

@param anIndex The position of the element within the array. The position 
               is relative to zero, (i.e. the first element in the array is
               at position 0). 

@return The length of the element at position anIndex.

@panic E32USER-CBase 21, if anIndex is negative or is greater than the number
       of elements currently in the array.
*/
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount,Panic(EArrayIndexOutOfRange));
	return(*((TInt *)iBase->Ptr(GetOffset(anIndex)).Ptr()));
	}

EXPORT_C void CArrayPakBase::Compress()
/**
Removes excess space from the array buffer.

The effect is to reduce the memory allocated to the array buffer so that it
is just sufficient to contain the elements of the array.

If the array is empty, then the memory allocated to the array buffer is freed.
*/
	{

	if (iBase)
		iBase->Compress();
	}

EXPORT_C void CArrayPakBase::Reset()
/**
Deletes all elements from the array and frees the memory allocated to the array 
buffer.
*/
	{

	Delete(0,Count());
	}

EXPORT_C void CArrayPakBase::SortL(TKeyArrayVar &aKey)
//
// Builds a transient CArrayVarFlat array, sorts it
// and then copies it back to the original array.
//
/**
Sorts the elements of the array into key sequence.

Note that the function requires a TKeyArrayVar key object because SortL()
creates a temporary CArrayVarFlat array in its implementation and uses that array's 
Sort() member function.

@param aKey The key object defining the properties of the key.

@see CArrayVarFlat
*/
	{

	if (iCount==0)
		return;
//
// First build a variable length flat array.
//
	CArrayVarFlat<TAny> *pVarFlat=NULL;
	TRAPD(r,BuildVarArrayL(pVarFlat))
	if (r==KErrNone)
		{
//
// Now sort it.
//
		r=pVarFlat->Sort(aKey);
		if (r==KErrNone)
			{
			//
			// Delete the records and copy back from pVarFlat.
			//
 			Reset(); // Deletes the records but leaves the memory
			TInt tCount=pVarFlat->Count();
			for (TInt anIndex=0;anIndex<tCount;anIndex++)
	 			{
				TInt lenData=pVarFlat->Length(anIndex);
				TAny *pdata=pVarFlat->At(anIndex);
				TRAP(r,InsertL(anIndex,pdata,lenData));
				if (r!=KErrNone)
					break;
				}
			}
		}
	delete pVarFlat;
	User::LeaveIfError(r);
	}

EXPORT_C TAny *CArrayPakBase::At(TInt anIndex) const
//
// TAny points to the data associated with the record with anIndex.
//
/**
@internalComponent
*/
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount,Panic(EArrayIndexOutOfRange));
	TInt *pR=(TInt *)iBase->Ptr(GetOffset(anIndex)).Ptr();
 	return((TAny *)(pR+1));
	}

EXPORT_C void CArrayPakBase::Delete(TInt anIndex)
/**
Removes a single element from the array.

Deleting elements from the array does not cause the array buffer to be
automatically compressed. Call CArrayPakBase::Compress() to return excess
space to the heap.

@param anIndex The position within the array of the element to delete, relative 
               to zero.
@panic E32USER-CBase 21, if anIndex is negative or is greater than the 
       number of elements currently in the array.
       
@see CArrayPakBase::Compress
*/
	{

	Delete(anIndex,1);
	}

EXPORT_C void CArrayPakBase::Delete(TInt anIndex,TInt aCount)
/**
Removes one or more contiguous elements from the array, starting at a specific 
position.

Deleting elements from the array does not cause the array buffer to be
automatically compressed. Call CArrayPakBase::Compress() to return excess
space to the heap.

@param anIndex The position within the array from where deletion of elements 
               is to start, relative to zero. 
 
@param aCount  The number of elements to be deleted from the array.

@panic E32USER-CBase 21, if anIndex is negative or greater than the number of
       elements currently in the array.
@panic E32USER-CBase 26, if aCount is negative.

@see CArrayPakBase::Compress
*/
	{

	if (aCount==0)
		return;
	__ASSERT_ALWAYS(aCount>0,Panic(EArrayCountNegative5));
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount,Panic(EArrayIndexOutOfRange));
	TInt end=anIndex+aCount;
	__ASSERT_ALWAYS(end<=iCount,Panic(EArrayCountTooBig));
	TInt totalToDelete=0;
	TInt firstRecOffset=0;
	for (TInt i=anIndex;i<end;i++)
		{
		TInt offset=GetOffset(i);
		if (i==anIndex)
			{
			firstRecOffset=offset;
			iCacheIndex=i;
			iCacheOffset=offset;
			}
		TAny *pRecord=(TAny *)iBase->Ptr(offset).Ptr();
		TInt lenData=(*(TInt *)pRecord);
		totalToDelete+=Align4(lenData)+sizeof(TUint);
		}
	iBase->Delete(firstRecOffset,totalToDelete);
	iCount-=aCount;
	}

EXPORT_C TAny *CArrayPakBase::ExpandL(TInt anIndex,TInt aLength)
//
// Expand the array at anIndex.
//
/**
@internalComponent
*/
	{

	if (iBase==NULL)
		iBase=(*iCreateRep)(iGranularity);
	__ASSERT_ALWAYS(aLength>=0,Panic(EArrayLengthNegative));
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<=iCount,Panic(EArrayIndexOutOfRange));
	TInt offset=GetOffset(anIndex);
	iCacheIndex=anIndex;
	iCacheOffset=offset;
	iBase->ExpandL(offset,Align4(aLength+sizeof(TInt)));
	TInt *pR=(TInt *)iBase->Ptr(offset).Ptr();
	*pR=aLength;
	iCount++;
 	return((TAny *)(pR+1));
	}

EXPORT_C TInt CArrayPakBase::Find(const TAny *aPtr,TKeyArrayPak &aKey,TInt &anIndex) const
//
// Find using a sequential search.
//
/**
@internalComponent
*/
	{

	if (iCount==0)
	    {
	    anIndex=0;
		return(-1);
		}
	aKey.SetPtr(aPtr);
	SetKey(aKey);
	TInt ret=(-1);
	TInt i=0;
	while (i<Count())
		{
		TInt j=aKey.Compare(i,KIndexPtr);
		if (j==0)
			{
			ret=j;
			break;
			}
		i++;
		}
	anIndex=i;
	return(ret);
	}

EXPORT_C TInt CArrayPakBase::FindIsq(const TAny *aPtr,TKeyArrayPak &aKey,TInt &anIndex) const
//
// Find using a binary search.
//
/**
@internalComponent
*/
	{

	if (iCount==0)
	    {
	    anIndex=0;
		return(-1);
		}
	aKey.SetPtr(aPtr);
	SetKey(aKey);
	return(User::BinarySearch(Count(),aKey,anIndex));
	}

EXPORT_C void CArrayPakBase::InsertL(TInt anIndex,const TAny *aPtr,TInt aLength)
//
// Inserts a record at index anIndex.
//
/**
@internalComponent
*/
	{

	TAny *pV=ExpandL(anIndex,aLength);
    Mem::Copy(pV,aPtr,aLength);
	}

EXPORT_C TInt CArrayPakBase::InsertIsqL(const TAny *aPtr,TInt aLength,TKeyArrayPak &aKey)
//
// Insert in sequence, no duplicates allowed.
//
/**
@internalComponent
*/
	{

	TInt i=0;
	TInt r=FindIsq(aPtr,aKey,i);
	if (r==0) // a duplicate, leave
		User::Leave(KErrAlreadyExists);
	InsertL(i,aPtr,aLength);
	return(i);
	}

EXPORT_C TInt CArrayPakBase::InsertIsqAllowDuplicatesL(const TAny *aPtr,TInt aLength,TKeyArrayPak &aKey)
//
// Insert in sequence, allow duplicates.
//
/**
@internalComponent
*/
	{

	TInt i=0;
	TInt r=FindIsq(aPtr,aKey,i);
	if (r==0) // a duplicate, insert after
		++i;
	InsertL(i,aPtr,aLength);
	return(i);
	}

EXPORT_C void CArrayPakBase::SetKey(TKeyArrayPak &aKey) const
//
// Set the key data.
//
/**
@internalComponent
*/
	{

	aKey.Set(iBase);
	}

EXPORT_C TInt CArrayPakBase::GetOffset(TInt anIndex) const
//
// Return the offset into the buffer of the record with index anIndex;
//
/**
@internalComponent
*/
	{

	TInt offset=0;
 	TInt curIndex=0;
	if (iCacheIndex<=anIndex)
		{
		curIndex=iCacheIndex;
		offset=iCacheOffset;
		}
	TAny *pRecord=(TAny *)iBase->Ptr(offset).Ptr();
	while (curIndex<anIndex)
		{
		TInt lenData=(*(TInt *)pRecord);
		offset+=Align4(lenData)+sizeof(TUint);
		pRecord=(TAny *)iBase->Ptr(offset).Ptr();
		curIndex++;
		}
	(TInt &)iCacheIndex=anIndex;
	(TInt &)iCacheOffset=offset;
 	return(offset);
	}

EXPORT_C void CArrayPakBase::BuildVarArrayL(CArrayVarFlat<TAny> * &aVarFlat)
//
// Make a copy of the current array as a CArrayVarFlat
//
/**
@internalComponent
*/
	{

	aVarFlat=new(ELeave) CArrayVarFlat<TAny>(iGranularity);
	for (TInt anIndex=0;anIndex<iCount;anIndex++)
		{
		TInt offset=GetOffset(anIndex);
		TAny *pRecord=(TAny *)iBase->Ptr(offset).Ptr();
		TInt lengthData=(*(TInt *)pRecord);
		TAny *pData=(TAny *)((TInt *)pRecord+1);
		aVarFlat->InsertL(anIndex,pData,lengthData);
		}
	}

EXPORT_C TInt CArrayPakBase::CountR(const CBase *aPtr)
//
// Return the number of items in the array.
//
/**
@internalComponent
*/
	{

	return(((CArrayPakBase *)aPtr)->Count());
	}

EXPORT_C const TAny *CArrayPakBase::AtR(const CBase *aPtr,TInt anIndex)
//
// Return the address of an item in the array.
//
/**
@internalComponent
*/
	{

	return(((CArrayPakBase *)aPtr)->At(anIndex));
	}

EXPORT_C CArrayFixFlat<TInt>::CArrayFixFlat(TInt aGranularity)
	: CArrayFix<TInt>((TBufRep)CBufFlat::NewL,aGranularity)
/**
Constructs the array, with the specified granularity, to contain elements of 
TInt type.
	
Note that no memory is allocated to the array buffer by this C++ constructor.
	
@param aGranularity The granularity of the array. 

@panic E32USER-CBase 18 if aGranularity is not positive.
*/
	{}

EXPORT_C CArrayFixFlat<TInt>::~CArrayFixFlat()
/**
Destructor.
*/
	{}

EXPORT_C CArrayFixFlat<TUid>::CArrayFixFlat(TInt aGranularity)
	: CArrayFix<TUid>((TBufRep)CBufFlat::NewL,aGranularity)
/**
Constructs the array, with the specified granularity, to contain elements of 
TUid type.

Note that no memory is allocated to the array buffer by this C++ constructor.
	
@param aGranularity The granularity of the array.

@panic E32USER-CBase 18 if aGranularity is not positive.
*/
	{}

EXPORT_C CArrayFixFlat<TUid>::~CArrayFixFlat()
/**
Destructor.
*/
	{}
