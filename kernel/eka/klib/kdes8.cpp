// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\klib\kdes8.cpp
// 
//

#include <kernel/kernel.h>
#include <kernel/kern_priv.h>

/**
Creates, and returns a pointer to, a new 8-bit heap descriptor.

On construction, the heap descriptor is empty and has zero length.

@param aMaxLength The requested maximum length of the descriptor.
                  This value must be non-negative otherwise the current thread
                  is panicked.
                  Note that the resulting heap cell size and, therefore, the
                  resulting maximum length of the descriptor may be larger than
                  requested.
@return A pointer to the new 8-bit heap descriptor; NULL,
        if the descriptor cannot be created.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
     
@post Calling thread is in a critical section.
       
@panic KernLib 30, if the requested maximum length is negative.
*/
EXPORT_C HBuf8* HBuf8::New(TInt aMaxLength)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"HBuf8::New(TInt aMaxLength)");
	__ASSERT_ALWAYS(aMaxLength>=0,KL::Panic(KL::ETDes8MaxLengthNegative));
	TAny* pM=Kern::Alloc(aMaxLength*sizeof(TUint8)+sizeof(TBufBase8));
	if (pM)
		new (pM) HBuf8(aMaxLength);
	return (HBuf8*)pM;
	}


/**
Creates, and returns a pointer to, a new 8-bit heap descriptor, and copies the
content of the specified descriptor into it.

Both the length and the maximum length of the new heap descriptor are set to
the same value as the length of the specified descriptor.

@param aDes The descriptor whose content is to be copied into the new heap
            descriptor.
            
@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
     
@post Calling thread is in a critical section.
            
@return  A pointer to the new 8-bit heap descriptor; NULL,
         if the descriptor cannot be created.
*/
EXPORT_C HBuf8* HBuf8::New(const TDesC8& aDes)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"HBuf8::New(const TDesC8& aDes)");	
	HBuf8* pB=HBuf8::New(aDes.Length());
	if (pB)
		pB->Copy(aDes);
	return pB;
	}


/**
Expands or contracts the heap descriptor.

This is done by: creating a new heap descriptor, copying the original data
into the new descriptor, and then deleting the original descriptor.

@param aNewMax The requested maximum length of data that the new descriptor
               is to represent.This value must be non-negative and it must not
               be less than the length of any existing data, otherwise the
               current thread is panicked.
               Note that the resulting heap cell size and, therefore, the
               resulting maximum length of the descriptor may be larger than
               requested.

@return A pointer to the new expanded or contracted 8-bit heap descriptor;
        the original descriptor is deleted. NULL, if the new 8-bit heap
        descriptor cannot be created - the original descriptor remains unchanged.

@pre Calling thread must be in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
     
@post Calling thread is in a critical section.   

@panic  KernLib 30, if the requested maximum length is negative.
@panic  KernLib 26, if the requested length is less than the length of any existing data.
*/
EXPORT_C HBuf8* HBuf8::ReAlloc(TInt aNewMax)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"HBuf8::ReAlloc");	
	__ASSERT_ALWAYS(aNewMax>=0,KL::Panic(KL::ETDes8MaxLengthNegative));
	__ASSERT_ALWAYS(Length()<=aNewMax,KL::Panic(KL::ETDes8ReAllocTooSmall));
	HBuf8* pNew=(HBuf8*)Kern::ReAlloc(this, aNewMax*sizeof(TUint8)+sizeof(TDes8) );
	if(pNew)
		pNew->iMaxLength=aNewMax;
	return pNew;
	}


/**
Copies the content of the source descriptor to the destination descriptor.

If the current thread is a user thread, i.e. the mode in spsr_svc is 'User',
then data is read using user mode privileges .

@param aDest The destination descriptor.
@param aSrc  The source descriptor.

@panic KERN-COMMON 19, if aDest is not a valid descriptor type.
@panic KERN-COMMON 23, if aSrc is longer that the maximum length of aDest.
@panic KERN-EXEC   33, if aSrc is not a valid descriptor type.

@pre Do not call from User thread if in a critical section.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.

@post The length of the destination descriptor is equal to the length of the source descriptor.
*/
EXPORT_C void Kern::KUDesGet(TDes8& aDest, const TDesC8& aSrc)
	{
	CHECK_PRECONDITIONS(MASK_NO_CRITICAL_IF_USER|MASK_THREAD_STANDARD,"Kern::KUDesGet");	
//ETDes8BadDescriptorType = 19
//ETDes8Overflow = 23
//EKUDesInfoInvalidType= 33
	TInt ulen, umax;
	TUint8* kptr=(TUint8*)aDest.Ptr();
	const TUint8* uptr=Kern::KUDesInfo(aSrc, ulen, umax);
	aDest.SetLength(ulen);
	kumemget(kptr,uptr,ulen);
	}


/**
Copies the content of the source descriptor to the destination descriptor.

If the current thread is a user thread, i.e. the mode in spsr_svc is 'User',
then data is written using user mode privileges.

@param aDest The destination descriptor.
@param aSrc  The source descriptor.

@panic KERN-COMMON 19, if aSrc is not a valid descriptor type.
@panic KERN-EXEC 33, if aDest is not a valid descriptor type.
@panic KERN-EXEC 34, if aDest is not a modifiable descriptor type.
@panic KERN-EXEC 35, if aSrc is longer that the maximum length of aDest.

@pre  Do not call from User thread if in a critical section.
@pre  Interrupts must be enabled.
@pre  Kernel must be unlocked.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Can be used in a device driver.

@post The length of aDest is equal to the length of aSrc.
@post If aDest is a TPtr type then its maximum length is equal its new length.
*/
//ETDes8BadDescriptorType = 19
//EKUDesInfoInvalidType = 33
//EKUDesSetLengthInvalidType = 34
//EKUDesSetLengthOverflow = 35
EXPORT_C void Kern::KUDesPut(TDes8& aDest, const TDesC8& aSrc)
	{
	CHECK_PRECONDITIONS(MASK_NO_CRITICAL_IF_USER|MASK_THREAD_STANDARD,"Kern::KUDesPut");	
	TInt ulen, umax;
	TInt klen=aSrc.Length();
	const TUint8* kptr=aSrc.Ptr();
	TUint8* uptr=(TUint8*)Kern::KUDesInfo(aDest, ulen, umax);
	Kern::KUDesSetLength(aDest, klen);
	kumemput(uptr,kptr,klen);
	}

#ifndef __DES8_MACHINE_CODED__


/**
Gets information about the specified descriptor.

If the current thread is a user thread, i.e. if the mode in spsr_svc is 'User',
then the descriptor is read using user mode privileges.

@param aSrc The descriptor for which information is to be fetched.
@param aLength On return, set to the length of the descriptor.
@param aMaxLength On return, set to the maximum length of the descriptor,
                  or -1 if the descriptor is not writable.

@return Address of first byte in descriptor.

@panic KERN-EXEC 33, if aSrc is not a valid descriptor type.

@pre  Do not call from User thread if in a critical section.
@pre  Interrupts must be enabled.
@pre  Kernel must be unlocked.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Can be used in a device driver.

*/
EXPORT_C const TUint8* Kern::KUDesInfo(const TDesC8& aSrc, TInt& aLength, TInt& aMaxLength)
	{
	CHECK_PRECONDITIONS(MASK_NO_CRITICAL_IF_USER|MASK_THREAD_STANDARD,"Kern::KUDesInfo");	
//EKUDesInfoInvalidType 33
	TUint32 typelen;
	kumemget32(&typelen,&aSrc,sizeof(typelen));
	TInt type=typelen>>KShiftDesType;
	aLength=typelen&KMaskDesLength;
	aMaxLength=-1;	// if descriptor not writeable
	const TUint8* p=NULL;
	const TAny** pA=(const TAny**)&aSrc;
	switch(type)
		{
		case EBufC: p=(const TUint8*)(pA+1); return p;
		case EPtrC: kumemget32(&p,pA+1,sizeof(TAny*)); return p;
		case EPtr: kumemget32(&p,pA+2,sizeof(TAny*)); break;
		case EBuf: p=(const TUint8*)(pA+2); break;
		case EBufCPtr: kumemget32(&p,pA+2,sizeof(TAny*)); p+=sizeof(TDesC8); break;
		default: K::PanicKernExec(EKUDesInfoInvalidType);
		}
	kumemget32(&aMaxLength,pA+1,sizeof(TInt));
	return p;
	}


/**
Sets the length of the specified descriptor.

If the current thread is a user thread, i.e. if the mode in spsr_svc is 'User',
then the length is written using user mode privileges.

@param aDes The descriptor.
@param aLength The new descriptor length.

@panic KERN-EXEC 34, if aDes is not a modifiable descriptor type.
@panic KERN-EXEC 35, if aLength is longer that the maximum length of aDes.

@pre  Do not call from User thread if in a critical section.
@pre  Interrupts must be enabled.
@pre  Kernel must be unlocked.
@pre  No fast mutex can be held.
@pre  Call in a thread context.
@pre  Can be used in a device driver.

@post The length of aDes is equal to aLength.
@post If aDes is a TPtr type then its maximum length is equal its new length.
*/
EXPORT_C void Kern::KUDesSetLength(TDes8& aDes, TInt aLength)
	{
	CHECK_PRECONDITIONS(MASK_NO_CRITICAL_IF_USER|MASK_THREAD_STANDARD,"Kern::KUDesSetLength");	
//EKUDesSetLengthInvalidType=34
//EKUDesSetLengthOverflow=35
	TUint32 deshdr[2];
	kumemget32(deshdr,&aDes,sizeof(TDes8));
	TInt type=deshdr[0]>>KShiftDesType;
	if (type!=EPtr && type!=EBuf && type!=EBufCPtr)
		K::PanicKernExec(EKUDesSetLengthInvalidType);
	if ((TUint32)aLength>deshdr[1])
		K::PanicKernExec(EKUDesSetLengthOverflow);
	deshdr[0]=(TUint32(type)<<KShiftDesType)|aLength;
	deshdr[1]=aLength;
	kumemput32(&aDes,deshdr,sizeof(TUint32));
	if (type==EBufCPtr)
		{
		TUint32 bufcptr;
		kumemget32(&bufcptr,(&aDes)+1,sizeof(bufcptr));
		kumemput32((TAny*)bufcptr,deshdr+1,sizeof(TUint32));
		}
	}
#endif


#ifndef __DES8_MACHINE_CODED__
/**
Checks whether the specified name is a valid Kernel-side object name.

A name is invalid, if it contains non-ascii characters, or any of
the three characters: "*", "?", ":".

@param  aName The name to be checked.

@return KErrNone, if the name is valid; KErrBadName, if the name is invalid.

@pre Calling thread can be either in a critical section or not.
@pre Interrupts must be enabled.
@pre Kernel must be unlocked.
@pre No fast mutex can be held.
@pre Call in a thread context.
@pre Can be used in a device driver.
*/
#ifndef __KERNEL_MODE__
#error "TDesC is not 8-bit as __KERNEL_MODE__ is not defined (see e32cmn.h)"
#endif
EXPORT_C TInt Kern::ValidateName(const TDesC& aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"Kern::ValidateName");	
	TUint8*	pName = const_cast<TUint8*>(aName.Ptr());
	TInt	pNameLen = aName.Length();
	for(;pNameLen;pName++,pNameLen--)
		if(*pName>0x7e || *pName<0x20 || *pName=='*' || *pName=='?' || *pName==':')
			return KErrBadName;
	return KErrNone;
	}

extern "C" EXPORT_C TInt memicmp(const TAny* aLeft, const TAny* aRight, TUint aLength)
    {
	const TUint8* l = (const TUint8*) aLeft;
	const TUint8* r = (const TUint8*) aRight;
	while (aLength--)
		{
		TInt lc = *l++;
		TInt rc = *r++;
		if (lc>='A' && lc<='Z') lc += ('a'-'A');
		if (rc>='A' && rc<='Z') rc += ('a'-'A');
		lc -= rc;
		if (lc)
			return lc;
		}
	return 0;
	}
#endif

#ifdef __DES8_MACHINE_CODED__
GLDEF_C void KUDesInfoPanicBadDesType()
	{
	K::PanicKernExec(EKUDesInfoInvalidType);
	}

GLDEF_C void KUDesSetLengthPanicBadDesType()
	{
	K::PanicKernExec(EKUDesSetLengthInvalidType);
	}

GLDEF_C void KUDesSetLengthPanicOverflow()
	{
	K::PanicKernExec(EKUDesSetLengthOverflow);
	}
#endif
