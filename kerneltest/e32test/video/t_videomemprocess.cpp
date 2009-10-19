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
// T_VIDEOMEMORY.CPP
// Overview:
// Test the video driver kernel extension that provides chunk handle to access video memory. 
// This executable is a second process that is started by the main t_videomemory test process.
// API Information:
// HAL, UserSvr
// Details:
// - Check that we can get a chunk and that we can read/write the memory belonging to that chunk.
// - When this process exits, the main process will check that the value has changed in it's copy 
// - of the chunk.  
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

LOCAL_D RTest test(_L("T_VIDEOMEMPROCESS"));

#ifndef __WINS__
#define DUMP(x) test.Printf(_L(#x"= %d =0x%08x\n"), x, x)
#endif


LOCAL_C TInt RunTestsForScreen(TInt aScreenID)
	{
	TInt ret = KErrNone;
	
	test.Next(_L("Checking Display Handle"));
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
		
	TInt handle = 0;
	volatile TUint32 *pChunkBase = 0;
	ret = HAL::Get(aScreenID, HALData::EDisplayMemoryHandle,handle);
	test ((KErrNone == ret || KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		test(0 != handle);
		RChunk chunk;
		ret = chunk.SetReturnedHandle(handle);
	    test(KErrNone == ret);
		if (KErrNone != ret)
			{
			return ret;
			}
		
		pChunkBase = reinterpret_cast<TUint32 *>(chunk.Base());
		test.Printf(_L("Display Memory Address = %08x\n"), reinterpret_cast<TInt>(pChunkBase));
		// Now check that we can read and write the memory that the chunk holds:
		// First check that it contains what we expect. 
		test(KTestValue3 == *pChunkBase);

		// Now check that we can CHANGE it. 
		*pChunkBase = KTestValue4;
		test(KTestValue4 == *pChunkBase);
		}

	return KErrNone;
	}



GLDEF_C TInt E32Main()
//
//
    {
	test.Title();

#if defined(__EPOC32__) && defined(__CPU_X86)
	test.Printf(_L("Doesn't run on X86\n"));
#else
	TInt ret = KErrNone;
	
	TInt screen;
	User::GetTIntParameter(12, screen);
	
	test.Start(_L("Testing Video Memory HAL interfaces (second process)"));

	//Hack: Only use screen 0 for now - use passed argument(s) later on. 
	ret = RunTestsForScreen(screen);
	test((ret == KErrNone));
#endif
	
	return KErrNone;
}
