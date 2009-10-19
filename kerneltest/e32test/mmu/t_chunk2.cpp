// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_chunk2.cpp
// Overview:
// Test RChunk class
// API Information:
// RChunk
// Details:
// - Test creating local chunks and moving chunks home address, verify
// results are as expected.
// - Create numerous local chunks, test allocating more space by 
// adjusting chunk size, check for allocation failure, verify results 
// are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32hal.h>
#include <e32panic.h>
#include "mmudetect.h"
#include "d_gobble.h"
#include "freeram.h"

TInt GlobalValue=299792458;
TInt *GlobalPtr=&GlobalValue;

LOCAL_D RTest test(_L("T_CHUNK2"));
LOCAL_D RTest t(_L("ShareThread"));
LOCAL_D RChunk gChunk;

LOCAL_D TPtr nullPtr(NULL,0);

LOCAL_C void TestAllocFailure(RChunk& c1, RChunk& c2)
	{
	TInt free=FreeRam();
	test.Printf(_L("Free RAM %dK\n"),free/1024);
	TInt size1=free-0x80000;	// 512K less than available
	TInt size2=0xFF000;			// 255 pages
	test.Printf(_L("Adjusting chunk 1 to size %dK\n"),size1/1024);
	TInt r=c1.Adjust(size1);
	test(r==KErrNone);
	test.Printf(_L("Attempting to adjust chunk 2 to size %dK\n"),size2/1024);
	r=c2.Adjust(size2);
	test(r==KErrNoMemory);
	TInt size3=FreeRam();
	test.Printf(_L("Attempting to adjust chunk 2 to size %dK\n"),size3/1024);
	r=c2.Adjust(size3);
	TInt free2=FreeRam();
	if (r==KErrNone)
		{
		test.Printf(_L("Succeeded - free RAM now %dK\n"),free2/1024);
		test.Printf(_L("Freeing chunk 2\n"));
		r=c2.Adjust(0);
		test(r==KErrNone);
		}
	else
		test.Printf(_L("Failed - free RAM now %dK\n"),free2/1024);
	test.Printf(_L("Freeing chunk 1\n"));
	r=c1.Adjust(0);
	test(r==KErrNone);
	test.Printf(_L("Checking free RAM\n"));
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	free2=FreeRam();
	test.Printf(_L("Free RAM %dK\n"),free2/1024);
	test(free==free2);
	}

void TestInterleavedAlloc()
	{
	TInt pageSize;
	RChunk c1;
	RChunk c2;
	TInt r;
	r=UserHal::PageSizeInBytes(pageSize);
	test(r==KErrNone);
	r=c1.CreateLocal(0,0x100000);
	test(r==KErrNone);
	r=c2.CreateLocal(0,0x100000);
	test(r==KErrNone);
	TInt step;
	for (step=1; step<=32; ++step)
		{
		test.Printf(_L("Step size %x\n"),step*pageSize);
		while (c1.Size()<64*pageSize)
			{
			r=c1.Adjust(c1.Size()+step*pageSize);
			test(r==KErrNone);
			r=c2.Adjust(c2.Size()+step*pageSize);
			test(r==KErrNone);
			}
		c1.Adjust(0);
		c2.Adjust(0);
		}
	c1.Close();
	c2.Close();
	}

TInt E32Main()
//
//	Test RChunk class
//
	{

	test.Title();
	if (!HaveVirtMem())
		{
		test.Printf(_L("This test requires an MMU\n"));
		return KErrNone;
		}
	TestInterleavedAlloc();
	test.Start(_L("Test moving chunks home addresses"));
	test.Printf(_L("GlobalValue=%d\n"),*GlobalPtr);
	++*GlobalPtr;
	test.Printf(_L("GlobalValue=%d\n"),GlobalValue);

	test.Next(_L("Load gobbler LDD"));
	TInt r = User::LoadLogicalDevice(KGobblerLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);
	RGobbler gobbler;
	r = gobbler.Open();
	test(r==KErrNone);
	TUint32 taken = gobbler.GobbleRAM(128*1024*1024);
	test.Printf(_L("Gobbled: %dK\n"), taken/1024);

	TInt free=FreeRam();
	test.Printf(_L("Free RAM 0x%08X bytes\n"),free);

	RChunk chunk1;
	r=chunk1.CreateLocal(0x400,0x800000);
	test (r==KErrNone);
	TInt i;
	TInt *p=(TInt*)chunk1.Base();
	test.Printf(_L("Chunk 1 Base %08X\n"),p);
	for (i=0; i<0x100; i++)
		*p++=i*i+41;
	RChunk chunk2;
	r=chunk2.CreateLocal(0x400,0x800000);
	test (r==KErrNone);
	TInt *p2=(TInt*)chunk2.Base();
	test.Printf(_L("Chunk 2 Base %08X\n"),p2);
	for (i=0; i<0x100; i++)
		*p2++=i*i*i+487;
	r=chunk1.Adjust(0x120000);
	test (r==KErrNone);
	for (i=0x100; i<0x48000; i++)
		*p++=i*i+41;
	r=chunk2.Adjust(0x120000);
	test (r==KErrNone);
	for (i=0x100; i<0x48000; i++)
		*p2++=i*i*i+487;
	p=(TInt*)chunk1.Base();
	p2=(TInt*)chunk2.Base();
	for(i=0; i<0x48000; i++)
		{
		TInt read1=*p++;
		TInt read2=*p2++;
		if (read1 != (i*i+41))
			{
			test.Printf(_L("Chunk 1 i=%X, read %08X expected %08X\n"),i,read1,i*i+41);
			//test.Getch();
			}
		if (read2 != (i*i*i+487))
			{
			test.Printf(_L("Chunk 2 i=%X, read %08X expected %08X\n"),i,read2,i*i*i+487);
			//test.Getch();
			}
		}
	chunk1.Close();
	chunk2.Close();
	
	TInt free2=FreeRam();
	test.Printf(_L("Free RAM 0x%08X bytes\n"),free2);
	test(free2==free);

	// Chunks must not be paged otherwise they will not effect the amount 
	// of free ram reported plus on h4 swap size is less than the total ram.
	TChunkCreateInfo createInfo;
	createInfo.SetNormal(0, free+2097152);
	createInfo.SetPaging(TChunkCreateInfo::EUnpaged);
	RChunk c1;
	test_KErrNone(c1.Create(createInfo));
	createInfo.SetNormal(0, 0x1000000);
	RChunk c2;
	RChunk c3;
	RChunk c4;
	RChunk c5;
	RChunk c6;
	test_KErrNone(c2.Create(createInfo));
	test_KErrNone(c3.Create(createInfo));
	test_KErrNone(c4.Create(createInfo));
	test_KErrNone(c5.Create(createInfo));
	test_KErrNone(c6.Create(createInfo));

	TestAllocFailure(c1,c2);
	r=c3.Adjust(1024);
	test(r==KErrNone);
	TestAllocFailure(c1,c2);
	r=c4.Adjust(1024);
	test(r==KErrNone);
	TestAllocFailure(c1,c2);
	r=c5.Adjust(1024);
	test(r==KErrNone);
	TestAllocFailure(c1,c2);
	r=c6.Adjust(1024);
	test(r==KErrNone);
	TestAllocFailure(c1,c2);

	c1.Close();
	c2.Close();
	c3.Close();
	c4.Close();
	c5.Close();
	c6.Close();
	gobbler.Close();

	test.End();
	test.Close();
	return(KErrNone);
	}


