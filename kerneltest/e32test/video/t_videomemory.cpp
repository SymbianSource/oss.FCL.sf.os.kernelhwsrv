// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Overview:
// Test the video driver kernel extension that provides chunk handle to access video memory. 
// API Information:
// HAL, UserSvr
// Details:
// - Check that the "old" GetMemoryAddress function still works, for legacy compatibility.
// - Check that we can get a chunk and that we can read/write the memory belonging to that chunk. 
// - Check that asking for a DisplayMemoryHandle twice gives the same piece of memory.  
// - Test that the same memory is available to a second process, by starting second process and
// the second process can write to memory. Validate by confirming that the value in the second process 
// is changed.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <videodriver.h>
#include <hal.h>
#include <e32svr.h>
#include <dispchannel.h>
#include "t_videomemory.h"

LOCAL_D RTest test(_L("T_VIDEOMEMORY"));

#ifndef __WINS__
#define DUMP(x) test.Printf(_L(#x"= %d =0x%08x\n"), x, x)
#endif


LOCAL_C void RunTestsForScreen(TInt aScreenID)
	{

	TInt ret = KErrNone;

#ifdef __WINS__
	RDisplayChannel displayChannel;
	
	test.Next(_L("Open Display Driver"));
	
    _LIT(KDisplayDriver, "display0");
    ret = User::LoadLogicalDevice(KDisplayDriver);
    test(KErrNone == ret || KErrAlreadyExists == ret);
	
	ret = displayChannel.Open(aScreenID);
    test(KErrNone == ret);
		
#endif

	test.Next(_L("Checking Display Memory Address"));
	
	// This is the real basic form of test:
	// Get the display memory address from the HAL.
	// Check that it's not zero - that would be invalid memory.
	// Try to write to the memory - it should not give a page-fault/crash.
	// Try to read the memory - we should get the same value as we wrote. 
	
	TInt memoryAddress=0;
	volatile TUint32 *pMemory = 0;
	ret = HAL::Get(aScreenID, HAL::EDisplayMemoryAddress, memoryAddress);
	test (KErrNone == ret || KErrNotSupported == ret);

	if (KErrNone == ret)
		{
		test.Printf(_L("Display Memory Address = %08x\n"), memoryAddress);
		// Now check that we can write to memoryAddress:
		test (memoryAddress != 0);
		pMemory = reinterpret_cast<TUint32 *>(memoryAddress);
		*pMemory = KTestValue1;
		test(KTestValue1 == *pMemory);
		}
	else
		{
		test.Printf(_L("Memory Address not available from HAL\n"));
		}
	
	// Second basic test. Use the HAL to fetch a handle
	// to the display memory. 
	// Check that the handle is not zero. 
	// Get the base-address of the chunk. 
	// Write this base address with a new value.
	// Read with the chunk base address to see that teh new value is there. 
	// Read the memory address from the above test and check that it changed 
	// to the new value.
	// Note that the memory address from above test MAY NOT BE SET - so 
	// check to see if it's non-zero first.
		
	test.Next(_L("Checking Display Handle"));
	TInt handle = 0;
	volatile TUint32 *pChunkBase = 0;
	RChunk chunk;
	ret = HAL::Get(aScreenID, HALData::EDisplayMemoryHandle, handle);
	test ((KErrNone == ret || KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		// Handle should not be zero. 
		test(0 != handle);
		ret = chunk.SetReturnedHandle(handle);
		test(KErrNone == ret);
		
		pChunkBase = reinterpret_cast<TUint32 *>(chunk.Base());
		test.Printf(_L("Display Memory Address = %08x\n"), reinterpret_cast<TUint>(pChunkBase));
		*pChunkBase = KTestValue2;
		test(KTestValue2 == *pChunkBase);
		// We should see the new value through the pMemory pointer!
		if (pMemory)
			{
			test(KTestValue2 == *pMemory);
			}
	
		}
	else
		{
		test.Printf(_L("Memory Handle not available from HAL - no point in further testing\n"));
		return;
		}
	

	// Check that we can write to more than the first bit of memory. 
	test.Next(_L("Check that we can write to \"all\" of the memory"));
	// First, find the mode with the biggest number of bits per pixel:
	TInt totalModes;
	ret = HAL::Get(aScreenID, HAL::EDisplayNumModes, totalModes);
	test (KErrNone == ret);
	TInt biggestMode = 0;
	TInt maxBitsPerPixel = 0;
	for(TInt mode = 0; mode < totalModes; mode++)
		{
		TInt bitsPerPixel = mode;
		ret = HAL::Get(aScreenID, HAL::EDisplayBitsPerPixel, bitsPerPixel);
		test (KErrNone == ret);
		if (bitsPerPixel > maxBitsPerPixel)
			{
			maxBitsPerPixel = bitsPerPixel;
			biggestMode = mode;
			}
		}
	
	TInt offsetToFirstPixel = biggestMode;
	ret = HAL::Get(aScreenID, HALData::EDisplayOffsetToFirstPixel, offsetToFirstPixel);
	test(KErrNone == ret);
	
	TInt stride = biggestMode;
	ret = HAL::Get(aScreenID, HALData::EDisplayOffsetBetweenLines, stride);
	test(KErrNone == ret);
	
	TInt yPixels = biggestMode;
	ret = HAL::Get(aScreenID, HALData::EDisplayYPixels, yPixels);
	test(KErrNone == ret);
	
	// Note this is no attempt to be precise. xPixels is not 
	TUint maxByte = offsetToFirstPixel + stride * yPixels - sizeof(TUint32);
		
	volatile TUint32 *memPtr = reinterpret_cast<volatile TUint32 *>(reinterpret_cast<volatile TUint8 *>(pChunkBase) + maxByte);
	*memPtr = KTestValue1;
	test(KTestValue1 == *memPtr);
	

	// Ask for a second handle and see that this also points to the same bit of memory.
	test.Next(_L("Checking Display Handle second time"));
	volatile TUint32 *pChunkBase2 = 0;
	ret = HAL::Get(aScreenID, HALData::EDisplayMemoryHandle, handle);
	test ((KErrNone == ret || KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		// Handle should not be zero!
		test(0 != handle);
		RChunk chunk2;
		ret = chunk2.SetReturnedHandle(handle);
		test(KErrNone == ret);
		
		pChunkBase2 = reinterpret_cast<TUint32 *>(chunk2.Base());
		test.Printf(_L("Display Memory Address = %08x\n"), reinterpret_cast<TUint>(pChunkBase));
		test(KTestValue2 == *pChunkBase2);
		*pChunkBase2 = KTestValue3;
		test(KTestValue3 == *pChunkBase2);
		chunk2.Close();
		}
	
	test.Next(_L("Checking Display Handle using second process"));
	
	// Create a process, let it find the handle of the memory, then read it, and write it.
	// Check that the value we have is the new value: KTestValue3.
	_LIT(KProcName, "t_videomemprocess.exe");
	RProcess process;
	
	ret = process.Create(KProcName, KNullDesC);
	test(KErrNone == ret);
	
	TRequestStatus procStatus;
	process.Logon(procStatus);
	process.SetParameter(12, aScreenID);
	process.Resume();
	User::WaitForRequest(procStatus);
	
	test.Next(_L("Checking that second process updated video memory"));
	// Check that we got the new value. 
	test(KTestValue4 == *pChunkBase);
	
	chunk.Close();
	
#ifdef __WINS__
	displayChannel.Close();
#endif
	
	// Now for some negative tests: Attempt to get a handle for a closes display.
	test.Next(_L("Negative test: Check that we CAN NOT use closed screen"));
	ret = HAL::Get(aScreenID, HALData::EDisplayMemoryHandle, handle);
	test (KErrNone != ret);
	}



LOCAL_C void NegativeTests(TInt aMaxScreens)
	{
	TInt handle;
	TInt ret;
	// Another few negative tests: Try invalid screen numbers.
	test.Next(_L("Negative tests: Invalid screen ID's"));
	ret = HAL::Get(aMaxScreens, HALData::EDisplayMemoryHandle, handle);
	test (KErrNone != ret);
	
	ret = HAL::Get(aMaxScreens+1, HALData::EDisplayMemoryHandle, handle);
	test (KErrNone != ret);
	
	ret = HAL::Get(4718, HALData::EDisplayMemoryHandle, handle);
	test (KErrNone != ret);
	
	ret = HAL::Get(-1, HALData::EDisplayMemoryHandle, handle);
	test (KErrNone != ret);
	}



GLDEF_C TInt E32Main()
//
//
    {

	test.Title();
//
#if defined(__EPOC32__) && defined(__CPU_X86)
	test.Printf(_L("Doesn't run on X86\n"));
#else

	test.Start(_L("Testing Video Memory HAL interfaces"));

	TInt screens = 0;	
	TInt ret=HAL::Get(HAL::EDisplayNumberOfScreens, screens);
	test((KErrNone == ret));
	// We expect that there is at least ONE screen. 
	test((screens > 0));

	for(TInt i=0;i<screens;i++)
		{
		RunTestsForScreen(i);
		}
	
	NegativeTests(screens);
#endif
	
	return KErrNone;
}
