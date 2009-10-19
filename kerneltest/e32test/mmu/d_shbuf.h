// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/mmu/d_shbuf.h
//
//

#ifndef D_SHBUF_H
#define D_SHBUF_H

#include <e32cmn.h>
#include <e32ver.h>
#include <e32shbufcmn.h>

class TShPoolInfo;
class TShPoolCreateInfo;

// Device driver names
_LIT(KTestShBufClient, "d_shbuf_client"); // Driver 0
_LIT(KTestShBufOwn, "d_shbuf_own"); // Driver 1

// Some test data
_LIT8(KTestData1, "QTWHEIRSTTYEUSITOAPPAPSIDSFAGFHUJCKKLIZNXGCPVIBLNEMOQFWCERRATPEA");
_LIT8(KTestData2, "8297382917319823712893719824737644563284328746372468732643287463");

const TInt KTestMinimumAlignmentLog2 = 5;
const TInt KTestPoolSizeInBufs = 10;
const TInt KDefaultPoolHandleFlags = EShPoolWriteable | EShPoolAllocate;

class RShBufTestChannel : public RBusLogicalChannel
	{
public:
	enum TTestControl
		{
		ETestOpenUserPool,
		ETestOpenKernelPool,
		ETestCloseUserPool,
		ETestCloseKernelPool,
		ETestManipulateUserBuffer,
		ETestAllocateKernelBuffer,
		ETestCreatePoolPhysAddrCont,
		ETestCreatePoolPhysAddrNonCont,
		ETestAllocateMax,
		ETestBufferAlignmentKernel,
		ETestNegativeTestsKernel,
		ETestCreatePoolContiguousPool,
		ETestPinBuffer,
		// Performance tests
		EFromRShBufProcessAndReturn = 100,
		EFromRShBufProcessAndRelease,
		EFromTPtr8ProcessAndReturn,
		EFromTPtr8ProcessAndRelease
		};

	enum { EClientThread = 0, EOwnThread = 1 };

#ifndef __KERNEL_MODE__
	inline TInt Open(TInt aDriverNo);	// driver 0 executes in client thread, driver 1 has its own thread
	inline TInt OpenUserPool(TInt aHandle, const TShPoolInfo& aPoolInfo);
	inline TInt OpenKernelPool(TShPoolCreateInfo& aInfo, TInt& aHandle);
	inline TInt CloseUserPool();
	inline TInt CloseKernelPool();
	inline TInt ManipulateUserBuffer(TInt aHandle);
	inline TInt AllocateKernelBuffer(TInt aPoolIndex, TInt& aHandle);
	inline TInt CreatePoolPhysAddrCont(TInt aBufSize);
	inline TInt CreatePoolPhysAddrNonCont(TInt aBufSize);
	inline TInt AllocateMax(TInt aPoolIndex, TInt& aAllocated);
	inline TInt BufferAlignmentKernel(TInt aBufSize, TInt aAlignment);
	inline TInt NegativeTestsKernel();
	inline TInt ContiguousPoolKernel(TShPoolCreateInfo& aInfo);
	inline TInt PinBuffer(TInt aPoolHandle, TInt aBufferHandle);
	// Performance tests
	inline TInt FromRShBufProcessAndReturn(TUint aBufSize);
	inline TInt FromRShBufProcessAndRelease(TInt aHandle);
	inline TInt FromTPtr8ProcessAndReturn(TDes8& aBuf, TUint bufferSize);
	inline TInt FromTPtr8ProcessAndRelease(TDes8& aBuf);
#endif // __KERNEL_MODE__
	};

#ifndef __KERNEL_MODE__
inline TInt RShBufTestChannel::Open(TInt aDriverNo)
	{
	if (aDriverNo!=0&&aDriverNo!=1)
		{
		return KErrArgument;
		}
	return (DoCreate((aDriverNo)?(KTestShBufOwn()):(KTestShBufClient()),TVersion(1,0,KE32BuildVersionNumber),KNullUnit,NULL,NULL,EOwnerThread));
	}
inline TInt RShBufTestChannel::OpenUserPool(TInt aHandle, const TShPoolInfo& aPoolInfo)
	{return DoControl(ETestOpenUserPool, (TAny*) aHandle, (TAny*) &aPoolInfo);}
inline TInt RShBufTestChannel::OpenKernelPool(TShPoolCreateInfo& aInfo, TInt& aHandle)
	{return DoControl(ETestOpenKernelPool, (TAny*) &aInfo, (TAny*) &aHandle);}
inline TInt RShBufTestChannel::CloseUserPool()
	{return DoControl(ETestCloseUserPool);}
inline TInt RShBufTestChannel::CloseKernelPool()
	{return DoControl(ETestCloseKernelPool);}
inline TInt RShBufTestChannel::ManipulateUserBuffer(TInt aHandle)
	{return DoControl(ETestManipulateUserBuffer, (TAny*) aHandle);}
inline TInt RShBufTestChannel::AllocateKernelBuffer(TInt aPoolIndex, TInt& aHandle)
	{return DoControl(ETestAllocateKernelBuffer, (TAny*) aPoolIndex, (TAny*) &aHandle);}
inline TInt RShBufTestChannel::CreatePoolPhysAddrCont(TInt aBufSize)
	{return DoControl(ETestCreatePoolPhysAddrCont, (TAny*) aBufSize);}
inline TInt RShBufTestChannel::CreatePoolPhysAddrNonCont(TInt aBufSize)
	{return DoControl(ETestCreatePoolPhysAddrNonCont, (TAny*) aBufSize);}
inline TInt RShBufTestChannel::AllocateMax(TInt aPoolIndex, TInt& aAllocated)
	{return DoControl(ETestAllocateMax, (TAny*) aPoolIndex, (TAny*) &aAllocated);}
inline TInt RShBufTestChannel::BufferAlignmentKernel(TInt aBufSize, TInt aAlignment)
	{return DoControl(ETestBufferAlignmentKernel, (TAny*) aBufSize, (TAny*) aAlignment);}
inline TInt RShBufTestChannel::NegativeTestsKernel()
	{return DoControl(ETestNegativeTestsKernel);}
inline TInt RShBufTestChannel::ContiguousPoolKernel(TShPoolCreateInfo& aInfo)
	{return DoControl(ETestCreatePoolContiguousPool, (TAny*)&aInfo);}
inline TInt RShBufTestChannel::PinBuffer(TInt aPoolHandle, TInt aBufferHandle)
	{return DoControl(ETestPinBuffer, (TAny*) aPoolHandle, (TAny*) aBufferHandle);}
// Performance tests
inline TInt RShBufTestChannel::FromRShBufProcessAndReturn(TUint aBufSize)
	{return DoControl(EFromRShBufProcessAndReturn, (TAny*) aBufSize);}
inline TInt RShBufTestChannel::FromRShBufProcessAndRelease(TInt aHandle)
	{return DoControl(EFromRShBufProcessAndRelease,(TAny*)aHandle);}
inline TInt RShBufTestChannel::FromTPtr8ProcessAndReturn(TDes8& aBuf, TUint aBufSize)
	{return DoControl(EFromTPtr8ProcessAndReturn,(TAny*)&aBuf, (TAny*) aBufSize);}
inline TInt RShBufTestChannel::FromTPtr8ProcessAndRelease(TDes8& aBuf)
	{return DoControl(EFromTPtr8ProcessAndRelease,(TAny*)&aBuf);}
#endif // __KERNEL_MODE__
#endif // D_SHBUF_H
