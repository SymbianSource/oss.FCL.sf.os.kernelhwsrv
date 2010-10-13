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
// This is a test for the PCI driver, so far implemented only on the
// Naviengine platform. It aims to test:
//	-That known values of data in config and memory space, on a given
//	device can be read as expected.
//	-That data can be written and modified in config and memory space
//	-PCI memory buffers mapped or allocated by the PCI driver work as
//	expected. These are
//		-DChunk created by PCI driver and accessible from PCI
//		-DPlatHwChunk created by PCI driver and accessible from PCI
//		-DChunk created externally, then mapped in to PCI memory space
//	There are tests to:
//		- Create and close each buffer. Heap checking ensures proper
//		cleanup
//		- Create and close multiple buffers from multiple threads.
//		This is an SMP focused test to check that the implementation
//		of the chunk manager and allocator in the driver are thread
//		safe. The tests should pass without triggering any assertions in
//		the driver's invariance checks.
//		- Write to buffers from software, and read back via the
//		system to PCI window, and vice-versa -- a loop-back test.
//		This checks that PCI buffers are indeed accessible to PCI devices.
//
// The tests require several pieces of PSL specific information:
//	- A TPciDevice containing the vendor and device IDs of a PCI device
//	to use for testing.
//	- TAddrSpaceTests which identify regions of a device's config and
//	memory space with known values, or which are known to be writable.
//
//	The test driver grants access to the PCI API with the following
//	constructs: 
//	- TUserConfigSpace and TUserMemorySpace, derived from TUserPciSpace,
//	which are user side equivalents of kernel-side objects allowing
//	accesses of different sizes to a PCI device's config space or
//	memory space.
//	- RPciChunk which is derived from and RChunk and corresponds to
//	a kernel-side DChunk, which in turn corresponds to a PCI chunk or
//	buffer. The test driver uses these for all PCI chunk types (a
//	"wrapper" DChunk is used to map the memory of a PCI DPlatHwChunk
//	to user side).
//
//	Known Issues:
//	The test driver d_pci is intended to be platform independent but
//	for now still contains some PSL specific information .eg the test
//	info structure (which should really be passed up from the PSL) and
//	the address and size of the system to pci window. For now the
//	test driver code will remain in the Naviengine baseport directory.
//	If the PCI driver is ever ported to a new platform this can be
//	rectified.
//	
//
//
#include "../misc/test_thread.h"
#include <e32std.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include "t_pci.h"
#include <assp/naviengine/pci.h>

class RPci;
/**
Extends RChunk to hold the PCI address
associated with a chunk.
*/
class RPciChunk: public RChunk
	{
public:
	TUint PciBase()
		{
		return iPciBaseAddr;
		}

	/**
	Return the PCI accessible size
	*/
	TInt Size() const
		{
		return iPciSize;
		}

private:
	friend class RPci;
	TUint iPciBaseAddr;
	TInt iPciSize; //size of the region mapped into PCI
	};

typedef TInt (RPci::*ChunkOpenFn)(RPciChunk&, TInt, TRequestStatus*);

class RPci : public RBusLogicalChannel
	{
public:
	TInt Open();
	TInt GetTestInfo(TPciTestInfo& aTestInfo);

	TInt Open(const TPciDevice&);

	TUint AccessConfigSpace(const TUserConfigSpace& aCs);
	TUint AccessMemorySpace(const TUserMemorySpace& aMs);
	TInt  OpenPciDChunk(RPciChunk& aPciChunk,TInt aPciChunkSize, TRequestStatus* aStatus=0);
	TInt  OpenPciPlatHwChunk(RPciChunk& aPciHwChunk,TInt aPciChunkSize, TRequestStatus* aStatus=0);
	TInt  OpenPciMappedChunk(RPciChunk& aPciMappedChunk,TInt aPciChunkSize, TRequestStatus* aStatus=0);	
	TInt  OpenPciWindowChunk(RChunk& aPciWindowChunk);
	TInt  RunUnitTests();
private:	
	TInt DoOpenPciChunk(RPciChunk& aPciChunk, TInt aPciChunkSize, TPciTestCmd aCmd, TRequestStatus* aStatus);
	};

inline TInt RPci::Open()
	{
	return DoCreate(KPciLddFactory, TVersion(), KNullUnit, NULL, NULL);
	}

inline TInt RPci::Open(const TPciDevice& aDevice) 
	{
	TPckgC<TPciDevice> devicePkg(aDevice);
	return DoCreate(KPciLddFactory, TVersion(), KNullUnit, NULL, &devicePkg);
	}

inline TInt RPci::GetTestInfo(TPciTestInfo& aTestInfo)
	{
	TPckg<TPciTestInfo> info(aTestInfo);
	return DoControl(EGetTestInfo, &info);
	}

inline TInt RPci::RunUnitTests()
	{
	return DoControl(ERunUnitTests);
	}

TUint RPci::AccessConfigSpace(const TUserConfigSpace& aCs)
	{
	TPckgC<TUserConfigSpace> pkg(aCs);
	return DoControl(EAccessConfigSpace, &pkg);
	}

TUint RPci::AccessMemorySpace(const TUserMemorySpace& aMs)
	{
	TPckgC<TUserMemorySpace> pkg(aMs);
	return DoControl(EAccessMemorySpace, &pkg);
	}

TInt RPci::OpenPciDChunk(RPciChunk& aPciChunk,TInt aPciChunkSize, TRequestStatus* aStatus)	
	{	
	return DoOpenPciChunk(aPciChunk, aPciChunkSize, EOpenPciDChunk, aStatus);
	}

TInt RPci::OpenPciPlatHwChunk(RPciChunk& aPciHwChunk,TInt aPciChunkSize, TRequestStatus* aStatus)	
	{
	return DoOpenPciChunk(aPciHwChunk, aPciChunkSize, EOpenPciPlatHwChunk, aStatus);
	}

TInt RPci::OpenPciMappedChunk(RPciChunk& aPciMappedChunk,TInt aPciChunkSize, TRequestStatus* aStatus)	
	{
	return DoOpenPciChunk(aPciMappedChunk, aPciChunkSize, EOpenPciMappedChunk, aStatus);
	}

TInt RPci::OpenPciWindowChunk(RChunk& aPciWindowChunk)
	{	
	TUint chunkHandle = DoControl(EOpenPciWindowChunk);			
	return aPciWindowChunk.SetReturnedHandle(chunkHandle);
	}

TInt RPci::DoOpenPciChunk(RPciChunk& aPciChunk, TInt aPciChunkSize, TPciTestCmd aCmd, TRequestStatus* aStatus)
	{
	const TInt constPciChunkSize = aPciChunkSize;
	TPciChunkCreateInfo info(constPciChunkSize, aPciChunk.iPciBaseAddr, aStatus);
	TPckgC<TPciChunkCreateInfo> pkg(info);

	TUint chunkHandle = DoControl(aCmd, &pkg);	
	
	const TInt r = aPciChunk.SetReturnedHandle(chunkHandle);
	if(r == KErrNone)
		{		
		aPciChunk.iPciSize = constPciChunkSize;					
		}
	return r;
	}

TUserPciSpace::TUserPciSpace(RPci& aPci)
	:iPci(&aPci)
	{}

TUserConfigSpace::TUserConfigSpace(RPci& aPci)
	:TUserPciSpace(aPci)
	{}

TUint TUserConfigSpace::Call()
	{
	return iPci->AccessConfigSpace(*this);
	}

TUserPciSpace* TUserConfigSpace::Clone() const
	{
	return new TUserConfigSpace(*this);
	}

TUserMemorySpace::TUserMemorySpace(RPci& aPci, TInt aBarIndex)
	:TUserPciSpace(aPci), iBarIndex(aBarIndex)
	{}

TUint TUserMemorySpace::Call()
	{
	return iPci->AccessMemorySpace(*this);
	}

TUserPciSpace* TUserMemorySpace::Clone() const
	{
	return new TUserMemorySpace(*this);
	}

/**
Test address allocator
*/
TInt TestRunPciUnitTest(RPci& pci)
	{		
	return pci.RunUnitTests();
	}


/**
Read from a defined address in memory or config space, compare against expected values.
8,16, and 32 bit accesses performed.

@param aSpace Object gving access to either the config or memory space of a PCI device
@param aInfo Contains the address and expected value of a dword
*/
void TestReadAddressSpace(TUserPciSpace& aSpace, const TPciTestInfo::TAddrSpaceTest& aInfo, RTest& test, TBool aVerbose=EFalse)
	{
	const TUint os = aInfo.iOffset;
	//Iterate over different widths, and possible
	//subfields of 32 bit word
	for(TInt bitWidth=32; bitWidth>=8; bitWidth>>=1)
		{
		const TInt numberOfFields = (32/bitWidth);
		for(TInt i=0; i< numberOfFields; i++)
			{
			const TInt extraByteOffset = i * (bitWidth >> 3);
			const TInt byteOffset = os + extraByteOffset;
			if(aVerbose)
				test.Printf(_L("Access bitWidth=%d byte offset=%d\n"), bitWidth, byteOffset);

			const TUint expected = aInfo.Expected(bitWidth, byteOffset);
			const TUint read = aSpace.Read(bitWidth, byteOffset);
			if(aVerbose)
				test.Printf(_L("expect 0x%08x, read 0x%08x\n"), expected, read);
			test_Equal(expected, read);
			}
		}
	}

/**
Verify writes and modifications to a defined address in memory or config space. 8,16, and 32 bit
accesses performed.

@param aSpace Object gving access to either the config or memory space of a PCI device
@param aInfo Contains the address of a (at least partially) writable dword
*/
void TestWriteAddressSpace(TUserPciSpace& aSpace, TPciTestInfo::TAddrSpaceTest& aInfo, RTest& test, TBool aVerbose=EFalse)
	{
	const TUint original = aSpace.Read(32, aInfo.iOffset);
	const TUint os = aInfo.iOffset;
	TUint mask = ~aInfo.iReadOnlyMask;

	//The pattern will be truncated when used with bit widths
	//less than 32.
	const TUint initPattern = 0xFFFFFFFF;

	for(TInt bitWidth=32; bitWidth>=8; bitWidth>>=1)
		{
		const TUint pattern = initPattern >> (32-bitWidth);
		const TInt numberOfFields = (32/bitWidth);
		for(TInt i=0; i< numberOfFields; i++)
			{
			const TInt extraByteOffset = i * (bitWidth >> 3);
			const TInt byteOffset = os + extraByteOffset;
			if(aVerbose)
				test.Printf(_L("Access bitWidth=%d byte offset=%d\n"), bitWidth, byteOffset);
			//the full dword we expect
			//currently assume that the unwritable bits will be 0
			const TUint writeExpect = (pattern << (bitWidth * i) ) & mask; 
			const TUint clearExpect = 0;
						
			//do write followed by clear
			const TUint expect[] = {writeExpect, clearExpect};
			const TUint write[] = {pattern, 0};
			for(TInt n = 0; n < 2; n++)
				{
				aSpace.Write(bitWidth, byteOffset, write[n]);
				TUint result = aSpace.Read(32, os);
							
				if(aVerbose)
					test.Printf(_L("wrote 0x%08x, expect 0x%08x, read 0x%08x\n"),
						write[n], expect[n], result);
				test_Equal(expect[n], result);
				}

			//test Modify calls. Set then clear pattern
			TUint set[] = {pattern, 0};
			TUint clear[] = {0, pattern};

			for(TInt m = 0; m < 2; m++)
				{	
				aSpace.Modify(bitWidth, byteOffset, clear[m], set[m]);
				TUint result = aSpace.Read(32, os);
						
				if(aVerbose)
					test.Printf(_L("clear 0x%08x, set 0x%08x,  expect 0x%08x, read 0x%08x\n"), clear[m], set[m], expect[m], result);
				test_Equal(expect[m], result);
				}
			}
		}

	//restore orginal value or we will not be able to access device
	aSpace.Write(32, os, original);
	}


/**
Verify that a PCI DChunk can be opened and closed from user side

@param pci  The RPci object to use
@param test The RTest object to use
@param aPciChunkSize The size of the DChunk which would be created
*/
void TestOpenAndCloseDChunk(RPci& pci,RTest& test,TInt aPciChunkSize)
	{
	RPciChunk testPciDChunk;

	// Create and open Chunk
	TRequestStatus status;
	TInt r = pci.OpenPciDChunk(testPciDChunk,aPciChunkSize, &status);	
	test_KErrNone(r);
	
	test(testPciDChunk.IsWritable());
	test(testPciDChunk.IsReadable());

	test.Printf(_L("PCI Chunk base = 0x%08x\n"), testPciDChunk.Base());
	test.Printf(_L("PCI Chunk size = %d\n"), testPciDChunk.Size());
	test.Printf(_L("PCI Address = 0x%08x\n"), testPciDChunk.PciBase());	

	//Close Chunk
	test.Next(_L("Close PCI Chunk handle"));	

	RTest::CloseHandleAndWaitForDestruction(testPciDChunk);
	User::WaitForRequest(status);
	}

/**
Verify that a PCI PlatHwChunk can be opened and closed from user side


@param pci  The RPci object to use
@param test The RTest object to use
@param aPciChunkSize The size of the PlatHwChunk which would be created
*/
void TestOpenAndClosePciPlatHwChunk(RPci& pci,RTest& test,TInt aPciChunkSize)
	{
	RPciChunk testPciPlatHwChunk;

	// Create and open Chunk
	TRequestStatus status;
	TInt r = pci.OpenPciPlatHwChunk(testPciPlatHwChunk,aPciChunkSize, &status);	
	test_KErrNone(r);
	
	test(testPciPlatHwChunk.IsWritable());
	test(testPciPlatHwChunk.IsReadable());

	test.Printf(_L("PCI Chunk base = 0x%08x\n"), testPciPlatHwChunk.Base());
	test.Printf(_L("PCI Chunk size = %d\n"), testPciPlatHwChunk.Size());
	test.Printf(_L("PCI Address = 0x%08x\n"), testPciPlatHwChunk.PciBase());	

	//Close Chunk	
	testPciPlatHwChunk.Close();
	User::WaitForRequest(status);
	test.Next(_L("Closed PCI PlatHwChunk handle"));	
	}

/**
Verify that pci-mapped DChunk can be opended and closed form user side 

@param pci  The RPci object to use
@param test The RTest object to use
@param aPciChunkSize The size of the pci-mapped DChunk which would be created
*/
void TestPciMapppedChunk(RPci& pci,RTest& test,TInt aPciChunkSize)
	{
	RPciChunk testPciMappedChunk;

	// Create and open Chunk
	TRequestStatus status;
	TInt r = pci.OpenPciMappedChunk(testPciMappedChunk,aPciChunkSize, &status);	
	test_KErrNone(r);
	
	test(testPciMappedChunk.IsWritable());
	test(testPciMappedChunk.IsReadable());

	test.Printf(_L("PCI Chunk base = 0x%08x\n"), testPciMappedChunk.Base());
	test.Printf(_L("PCI Chunk size = %d\n"), testPciMappedChunk.Size());
	test.Printf(_L("PCI Address = 0x%08x\n"), testPciMappedChunk.PciBase());	

	//Close Chunk
	testPciMappedChunk.Close();
	User::WaitForRequest(status);
	test.Next(_L("Closed PCI Mapped Chunk handle"));	
	}

/**
Verify that an RChunk can be open to grant access to the internal PCI window from the user side

@param pci  The RPci object to use
@param test The RTest object to use
*/
void TestPciWindowChunk(RPci& pci,RTest& test)
	{
	RChunk testPciWindowChunk;

	// Create and open DChunk
	TInt r = pci.OpenPciWindowChunk(testPciWindowChunk);	
	test_KErrNone(r);
	
	test(testPciWindowChunk.IsWritable());
	test(testPciWindowChunk.IsReadable());

	test.Printf(_L("PCI Window Chunk base = 0x%08x\n"), testPciWindowChunk.Base());
	test.Printf(_L("PCI Window Chunk size = %d\n"), testPciWindowChunk.Size());
	
	//Close Chunk
	testPciWindowChunk.Close();
	test.Next(_L("Closed PCI Window Chunk handle"));	
	}


class CPciTest : public CTest
	{
protected:
	CPciTest(const TDesC& aName, TInt aIterations, RPci& aDevice)
		: CTest(aName, aIterations), iDevice(aDevice)
		{}

	RPci iDevice;
	};

/**
Each instance of test will open a chunk, using the function specified in
the template argument, FUNC.

The total number of chunks that can be opened by all instances is limited
by iMaxCount.

All intances of the test will hold their chunk open until iMaxCount has
been reached.
*/
template<ChunkOpenFn FUNC>
class CPciOpenChunkTest : public CPciTest
	{
public:
	CPciOpenChunkTest(const TDesC& aName, TInt aIterations, RPci& aDevice,
			RSemaphore aSemOpen, RSemaphore aSemClose, RFastLock aLock, TInt aMaxCount)
		:CPciTest(aName, aIterations, aDevice),
			iSemOpen(aSemOpen), iSemClose(aSemClose), iLock(aLock), iMaxCount(aMaxCount)
		{
		}

	virtual void RunTest()
		{
		RTest test(iName);
		RPciChunk chunk;

		iSemOpen.Wait();
		TRequestStatus status;
		const TInt chunkSize = 0x400;
		//open chunk by calling FUNC
		TInt r = ((iDevice).*(FUNC))(chunk, chunkSize, &status);
		test_KErrNone(r);

		iLock.Wait();
		iOpenCount++;
		test.Printf(_L("Opened chunk %d\n"), iOpenCount);
		if(iOpenCount == iMaxCount)
			{
			test.Printf(_L("Opened=%d, max=%d: Allow chunks to close\n"), iOpenCount, iMaxCount);
			//release all waiting threads
			//plus 1 preincrement so this
			//thread also passes
			iSemClose.Signal(iOpenCount);			
			iOpenCount = 0;
			}	
		iLock.Signal();


		iSemClose.Wait();
		chunk.Close();
		User::WaitForRequest(status);

		// permit another chunk to be opened  
		iSemOpen.Signal();
		test.Close();
		}

	virtual CTest* Clone() const
		{
		//make shallow copy
		return new CPciOpenChunkTest(*this);
		}


private:
	RSemaphore& iSemOpen; ///!< Represents the number of available PCI mappings
	RSemaphore& iSemClose; ///!< Represents the number of threads waiting to close their chunk
	RFastLock& iLock;
	static TInt iOpenCount;
	const TInt iMaxCount;
	};

template<ChunkOpenFn FUNC>
TInt CPciOpenChunkTest<FUNC>::iOpenCount = 0;


/**
Test which will perform various reads from a PCI address
space (config or memory) and confirm that values are read
as expected
*/
class CPciAddressSpaceRead : public CPciTest
	{
public:
	CPciAddressSpaceRead(const TDesC& aName, TInt aIterations, RPci& aDevice,
		const TUserPciSpace& aSpace, const TPciTestInfo::TAddrSpaceTest& aInfo)
		:CPciTest(aName, aIterations, aDevice),
			iAddressSpace(aSpace.Clone()), iSpaceTestInfo(aInfo)
	{
	}

	CPciAddressSpaceRead(const CPciAddressSpaceRead& aOther)
		:CPciTest(aOther)/* TODO-REVIEW have object-sliced aOther - is this ok?*/,
			iAddressSpace(aOther.iAddressSpace->Clone()), iSpaceTestInfo(aOther.iSpaceTestInfo)
	{
	}

	virtual ~CPciAddressSpaceRead()
		{
		delete iAddressSpace;
		}

	virtual void RunTest()
		{
		__UHEAP_MARK;
		RTest test(iName);
		TestReadAddressSpace(*iAddressSpace, iSpaceTestInfo, test);
		test.Close();
		__UHEAP_MARKEND;
		}

	virtual CTest* Clone() const
		{
		//make shallow copy
		return new CPciAddressSpaceRead(*this);
		}

private:
	TUserPciSpace* iAddressSpace;
	const TPciTestInfo::TAddrSpaceTest& iSpaceTestInfo;
	};

/**
For aBuffer, test writing to it then reading back from aWindow
then write via window and read back from chunk

@param test The RTest object to use
@param aBuffer RChunk corresponding to a PCI accessible buffer
@param aWindow RChunk coressponding an appropriate System-to-PCI memory window
It is presumed to start at PCI address 0
*/
void DoLoopBackTest(RTest& test, RPciChunk aBuffer, RChunk aWindow)
	{
	test.Start(_L("Test accessing memory via PCI"));

	TUint8* const bufferBase = aBuffer.Base();
	const TUint bufferSize = aBuffer.Size();
	const TUint bufferPciBase = aBuffer.PciBase();

	TUint8* const windowBase = aWindow.Base();
	const TUint windowSize = aWindow.Size();

#define PRINT(N) RDebug::Printf("%s = 0x%08x (%d)", #N, (N), (N)) 
	PRINT(bufferBase);
	PRINT(bufferSize);
	PRINT(bufferPciBase);

	PRINT(windowBase);
	PRINT(windowSize);

#undef PRINT

	//need to check that the end of the buffer
	//is within the windowed region
	test(bufferPciBase + bufferSize <= windowSize);
	TUint8* const bufferBaseWithinWindow = windowBase + bufferPciBase;

	test.Next(_L("write chunk"));
	for(TUint i = 0; i < bufferSize; ++i)
		{
		//each byte will hold its own offset modulo 256
		bufferBase[i] = (TUint8)i;
		}

	test.Next(_L("read back via window"));
	for(TUint j=0; j < bufferSize; ++j)
		{
		const TUint8 result = bufferBaseWithinWindow[j];
		test_Equal(j%256, result);
		}

	//clear chunk
	memclr(bufferBase, bufferSize);
	test.Next(_L("write via window"));
	for(TUint k=0; k < bufferSize; ++k)
		{
		//each byte will hold its own offset modulo 256
		bufferBaseWithinWindow[k] = (TUint8)k;
		}

	test.Next(_L("read back from chunk"));
	for(TUint l=0; l < bufferSize; ++l)
		{
		const TUint8 result = bufferBase[l];
		test_Equal(l%256, result);
		}

	test.End();
	}

/**
Take care of opening a chunk, running the test and closing
*/
template<ChunkOpenFn OPEN_FUNC>
inline void LoopBackTest(RPci& aPci, RTest& test, RChunk& aWindow)
	{
	RPciChunk pciChunk;
	const TInt chunkSize = 0x400; //1k

	//call the specified chunk opening function
	TRequestStatus status;
	TInt r = ((aPci).*(OPEN_FUNC))(pciChunk, chunkSize, &status);	
	test_KErrNone(r);
	DoLoopBackTest(test, pciChunk, aWindow);
	pciChunk.Close();
	User::WaitForRequest(status);
	}

/**
Run the loopback test for the 3 types of buffer supported by the PCI driver.
DChunk
DPlatChunk
Mapped In external memory
*/
void TestLoopBack(RPci& aPci, RTest& test)
	{
	test.Next(_L("Open PCI window"));
	RChunk window;
	
	TInt r = aPci.OpenPciWindowChunk(window);	
	test_KErrNone(r);

	test.Next(_L("DChunk"));
	LoopBackTest<&RPci::OpenPciDChunk>(aPci, test, window);

	test.Next(_L("DPlatHwChunk"));
	LoopBackTest<&RPci::OpenPciPlatHwChunk>(aPci, test, window);

	test.Next(_L("DChunk (mapped in)"));
	LoopBackTest<&RPci::OpenPciMappedChunk>(aPci, test, window);

	window.Close();
	}
#ifndef __VC32__ //visual studio 6 doesn't approve of pointer to member function template parameters
/**
Run the CPciOpenChunkTest for each type of chunk. This function also creates (and destroys) the
necessary semaphores and locks.
CPciOpenChunkTest objects are run in multiple threads using MultipleTestRun().

@param aDevice Handle to the test driver
@param test RTest to use.
@param aBufferLimit The maximum number of buffers which can be opened simultaneously
*/
void TestBufferOpenConcurrency(RPci& aDevice, RTest& test, TInt aBufferLimit)
	{
	RSemaphore semaphoreOpen;
	RSemaphore semaphoreClose;
	RFastLock lock;

	TInt r = semaphoreOpen.CreateLocal(aBufferLimit);
	test_KErrNone(r);

	r = semaphoreClose.CreateLocal(0);
	test_KErrNone(r);

	r = lock.CreateLocal();
	test_KErrNone(r);

	const TInt iterations = 3;
	{
	test.Printf(_L("Opening %d PCI DChunks in %d threads\n"), aBufferLimit, aBufferLimit);
	CPciOpenChunkTest<&RPci::OpenPciDChunk>
		dChunkTest(_L("Concurrent-DChunk"), iterations, aDevice, semaphoreOpen, semaphoreClose, lock, aBufferLimit);

	MultipleTestRun(test, dChunkTest, aBufferLimit);
	}

	{
	test.Printf(_L("Opening %d PCI DPlatHwChunks in %d threads\n"), aBufferLimit, aBufferLimit);
	CPciOpenChunkTest<&RPci::OpenPciPlatHwChunk>
		platChunkTest(_L("Concurrent-DPlatHwChunk"), iterations, aDevice, semaphoreOpen, semaphoreClose, lock, aBufferLimit);

	MultipleTestRun(test, platChunkTest, aBufferLimit);
	}

	{
	test.Printf(_L("Opening %d PCI Mapped chunks in %d threads\n"), aBufferLimit, aBufferLimit);
	CPciOpenChunkTest<&RPci::OpenPciMappedChunk>
		mappedChunkTest(_L("Concurrent-DChunk(mapped)"), iterations, aDevice, semaphoreOpen, semaphoreClose, lock, aBufferLimit);

	MultipleTestRun(test, mappedChunkTest, aBufferLimit);
	}

	semaphoreOpen.Close();
	semaphoreClose.Close();
	lock.Close();
	}
#endif

TInt E32Main()
	{
	__UHEAP_MARK;

	_LIT(KPci, "PCI");
	RTest test(KPci);
	test.Start(_L("Running PCI tests\n"));

	TInt r = User::LoadLogicalDevice(KPciLdd);

	__KHEAP_MARK;
	
	if(r==KErrNotFound)
		{
		test.Printf(_L("No PCI system present - skipping test\n"));
		return KErrNone;
		}
	if(r!=KErrNone && r!=KErrAlreadyExists)
		{
		test_KErrNone(r);
		}
	
	test.Next(_L("Open non-existant device\n"));
	RPci device;
	TPciDevice unavailable;
	r = device.Open(unavailable);
	test_Equal(KErrNotFound, r);

	RPci pciInfo;
	r = pciInfo.Open();
	test_KErrNone(r);

	test.Next(_L("Get test info from driver\n"));
	TPciTestInfo info;
	r = pciInfo.GetTestInfo(info);
	test_KErrNone(r);
	pciInfo.Close();

	test.Next(_L("Open test device\n"));
	r = device.Open(info.iDevice);
	test_KErrNone(r);

	test.Next(_L("Run Device Unit Test\n"));
	r=TestRunPciUnitTest(device);	
	test_KErrNone(r);

	test.Next(_L("Read config space\n"));
	TUserConfigSpace cs(device);
	TestReadAddressSpace(cs, info.iCfgSpaceRead, test);

	test.Next(_L("Write config space\n"));
	TestWriteAddressSpace(cs, info.iCfgSpaceWrite, test);
	
	test.Next(_L("Read memory space\n"));
	TUserMemorySpace ms(device, info.iMemSpaceIndex);
	TestReadAddressSpace(ms, info.iMemSpaceRead, test);

	test.Next(_L("Modify memory space\n"));
	TestWriteAddressSpace(ms, info.iMemSpaceWrite, test);

	{
	const TInt addrSpaceThreadCount = 4;
	const TInt iterations = 100;
	test.Next(_L("Concurrent config space reads")); 
	CPciAddressSpaceRead cfgSpaceRead(_L("Cfg Space Read"), iterations, device, cs, info.iCfgSpaceRead);
	MultipleTestRun(test, cfgSpaceRead, addrSpaceThreadCount);

	test.Next(_L("Concurrent memory space reads")); 
	CPciAddressSpaceRead memSpaceRead(_L("Memory Space Read"), iterations, device, ms, info.iMemSpaceRead);
	MultipleTestRun(test, memSpaceRead, addrSpaceThreadCount);
	}

	TInt testDChunkSize = 0x4000;
	test.Next(_L("Open and Close DChunks\n"));	
	TestOpenAndCloseDChunk(device,test,testDChunkSize);
	
	TInt testDPlatChunkSize = 0x2000;
	test.Next(_L("Open and Close PlatHwChunks\n"));	
	TestOpenAndClosePciPlatHwChunk(device,test,testDPlatChunkSize);

	//TestPciMapppedChunk() fails for sizes greater than 4K.
	//The issue is that a block of externally mapped memory must be
	//naturally alligned in order to be accessible to the PCI bus (ie
	//an 8k buffer would have to start at an address which is a
	//multiple of 8k.
	//
	//Now we could fix this for sure on the kernel side, by making
	//sure we only commit correctly aligned memory into the chunk (as
	//the pci driver itself does),
	//However, by using a 4k chunk, we know this will be on a page
	//boundary so the alignment is correct (assuming the page size
	//isn't changed). 	
	TInt testMapppedChunkSize = 0x1000; 
	test.Next(_L("Open and Close Pci Mappped Chunk\n"));	
	TestPciMapppedChunk(device,test,testMapppedChunkSize);

	test.Next(_L("Open and Close Pci Window Chunk\n"));	
	TestPciWindowChunk(device,test);

	const TInt numberOfThreads = info.iNumberOfBars;
	test.Printf(_L("Open buffers concurrently, max supported = %d\n"), numberOfThreads);
#ifndef __VC32__
	TestBufferOpenConcurrency(device, test, numberOfThreads);
#else
	test.Printf(_L("TestBufferOpenConcurrency not implemented for WINS"), numberOfThreads);
#endif

	test.Next(_L("Test loop back"));	
	TestLoopBack(device, test);

	device.Close();
	__KHEAP_MARKEND;

	r = User::FreeLogicalDevice(KPciLdd);
	test_KErrNone(r);

	test.End();
	test.Close();

	__UHEAP_MARKEND;
	return KErrNone;
	}	
