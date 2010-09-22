// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_chunk.cpp
// Overview:
// Test RChunk class
// API Information:
// RChunk, RChangeNotifier, TFindChunk
// Details:
// - Test adjusting chunks: create a global chunk, adjust it, do some size 
// checks, verify Create() rounds up to chunk multiple, verify that adjust
// request for size greater than MaxSize returns an error.
// - Test creating chunks with small sizes (0 and 1), verify results are as expected.
// - Create multiple local and global chunks, verify that Size,MaxSize,Name & FullName methods 
// are as expected. 
// Check Create method of global chunk if the name is already in use. Check the name can be 
// reused after the chunk is closed.
// - Perform adjust tests on a chunk, verify results are as expected.
// - Open and test global chunks with multiple references, verify results are as expected.
// - Open and test local chunks, check TFindChunk::Next method, verify results are as expected.
// - Check user thread is panicked if it creates or adjusts chunk with negative size or of an
// invalid type or with an invalid name.
// - Check the chunk is filled in with the appropriate clear value after creation, verify results
// are as expected.
// - Test sharing a chunk between threads, verify results are as expected.
// - Create a thread to watch for notification of changes in free memory and
// changes in out of memory status. Verify adjusting an RChunk generates
// the expected notifications.
// - Test finding a global chunk by name and verify results are as expected.
// - Check read-only global chunks cannot be written to by other processes.
// - Test Top and Bottom API with DoubleEnded chunk
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32panic.h>
#include <e32svr.h>
#include <hal.h>
#include "mmudetect.h"
#include "d_gobble.h"
#include "freeram.h"

const TInt KHeapSize=0x200;

//const TInt KNumberOfChunks=10; can't have that many
const TInt KNumberOfChunks=3;

const TInt KChunkNum=5;
const TInt KNormalReturn=194;

#ifdef __WINS__
const TInt KMinChunkSizeInBytesMinus1=0x0000ffff;
const TUint KMinChunkSizeInBytesMask=0xffff0000;
#elif defined (__X86__)
const TInt KMinChunkSizeInBytesMinus1=0x003fffff;
const TUint KMinChunkSizeInBytesMask=0xffc00000;
#else
const TInt KMinChunkSizeInBytesMinus1=0x000fffff;
const TUint KMinChunkSizeInBytesMask=0xfff00000;
#endif

const TInt KMinPageSizeInBytesMinus1=0x00000fff;
const TUint KMinPageSizeInBytesMask=0xfffff000;
TInt gPageSize;

LOCAL_D RTest test(_L("T_CHUNK"));
LOCAL_D RTest t(_L("ShareThread"));
LOCAL_D RChunk gChunk;

LOCAL_D TPtr nullPtr(NULL,0);

TUint32 MemModel;

enum TDirective
	{
	ENormal,
	ECreateNegative,
	EAdjustNegative,
	ECreateInvalidType,
	ECreateInvalidName,
	ECreateNoName,
	};

const TUint8 KDfltClearByte = 0x3;
TBool CheckChunkCleared(RChunk& aRC, TInt aOffset=0, TUint8 aClearByte = KDfltClearByte)
	{
	TUint8* base = aRC.Base()+aOffset;
	TInt size = aRC.Size();
	test.Printf(_L("Testing chunk for 0x%x - size: %d!\n"), aClearByte, size);
	TBool ret=ETrue;
	for(TInt i = 0; i<size; i++)
		if(base[i] != aClearByte)
			ret=EFalse;
	memset((TAny*)base, 0x05, size);
	return ret;
	}

TInt roundToPageSize(TInt aSize)
	{

	return(((aSize+KMinPageSizeInBytesMinus1)&KMinPageSizeInBytesMask));
	}

TInt roundToChunkSize(TInt aSize)
	{
	if(MemModel==EMemModelTypeFlexible)
		return roundToPageSize(aSize);
	return(((aSize+KMinChunkSizeInBytesMinus1)&KMinChunkSizeInBytesMask));
	}

TInt ThreadEntry2(TAny* /*aParam*/)
//
//	Thread to read from shared chunk
//
	{
	RChunk c;
	TInt r;
	r=c.OpenGlobal(_L("Marmalade"),ETrue);
	t(r==KErrNone);
	TUint8* base=c.Base();
	for (TInt8 i=0;i<10;i++)
		t(*base++==i); // check the chunk has 0-9
	c.Close();
	return(KErrNone);
	}

TInt ThreadEntry(TAny* aDirective)
//
//	Thread to create a Panic in a variety of ways
//
	{

	switch((TUint)aDirective)
		{
	case ENormal:
		{
		return KNormalReturn;
		}
	case ECreateNegative:
		{
		gChunk.CreateLocal(0x10,-0x10);
		test(EFalse); 
		}
	case EAdjustNegative:
		{
		gChunk.Adjust(-0x10);
		test(EFalse);
		}
	case ECreateInvalidType :
		{
		TChunkCreateInfo createInfo;
		createInfo.SetCode(gPageSize, gPageSize);
		_LIT(KChunkName, "Chunky");
		createInfo.SetGlobal(KChunkName);
		RChunk chunk;
		chunk.Create(createInfo);
		test(EFalse);
		}
	default:
		test(EFalse);
		}
	return(KErrNone);
	}

//
//	Test the clear flags for the specified chunk.
//	Assumes chunk has one commited region from base to aBytes.
//
void TestClearChunk(RChunk& aChunk, TUint aBytes, TUint8 aClearByte)
	{
	test_Equal(ETrue, CheckChunkCleared(aChunk, 0, aClearByte));
	test_KErrNone(aChunk.Adjust(0));
	test_KErrNone(aChunk.Adjust(aBytes));
	test_Equal(ETrue, CheckChunkCleared(aChunk, 0, aClearByte));
	aChunk.Close();
	}


//
// Create the specified thread and verify the exit reason.
//
void TestThreadExit(TExitType aExitType, TInt aExitReason, TThreadFunction aFunc, TAny* aThreadParam)
	{
	RThread thread;
	test_KErrNone(thread.Create(_L("RChunkPanicThread"), aFunc, KDefaultStackSize, 
								KHeapSize, KHeapSize, aThreadParam));
	// Disable JIT debugging.
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);

	TRequestStatus status;
	thread.Logon(status); 
	thread.Resume();
	User::WaitForRequest(status);
	test_Equal(aExitType, thread.ExitType());
	test_Equal(aExitReason, status.Int());
	test_Equal(aExitReason, thread.ExitReason());
	if (aExitType == EExitPanic)
		test(thread.ExitCategory()==_L("USER"));
	CLOSE_AND_WAIT(thread);

	// Put JIT debugging back to previous status.
	User::SetJustInTime(justInTime);
	}


//
// Thread function to create a chunk with invalid attributes
//
TInt ChunkPanicThread(TAny* aCreateInfo)
	{
	// This should panic.
	RChunk chunk;
	chunk.Create((*(TChunkCreateInfo*) aCreateInfo));
	test(EFalse);

	return KErrGeneral;	// Shouldn't reach here.
	}

void testInitialise()
	{
	test.Next(_L("Load gobbler LDD"));
	TInt r = User::LoadLogicalDevice(KGobblerLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	// get system info...
	MemModel = UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL) & EMemModelTypeMask;

	test_KErrNone(UserHal::PageSizeInBytes(gPageSize));
	}


void test1()
//
//	Test creating chunks with silly sizes
//
	{

	__KHEAP_MARK;
	RChunk chunk;
	TInt r;
	test.Start(_L("Create global chunks"));
	r=chunk.CreateGlobal(_L("Fopp"),1,1);
	test(r==KErrNone);
	chunk.Close();
	r=chunk.CreateGlobal(_L("Fopp"),0,0);
	test(r==KErrArgument);

	__KHEAP_CHECK(0);

	test.Next(_L("Create local chunks"));	  
	r=chunk.CreateLocal(1,1);
	test(r==KErrNone);
	chunk.Close();
	r=chunk.CreateLocal(0,0);
	test(r==KErrArgument);
	test.End();
	__KHEAP_MARKEND;
	}


void test2(TInt aSize)
//
//	Test local/global chunk creation
//
	{

	RChunk lchunk[KNumberOfChunks];
	RChunk gchunk[KNumberOfChunks];
	RChunk chunk;

	__KHEAP_MARK;
	test.Start(_L("Create multiple local and global chunks"));

	TInt i;
	TBuf<0x40> name;
	TFullName fullname;
	for (i=0; i<KNumberOfChunks; i++)
		{
		TInt r;
		// create a local chunk
		r=lchunk[i].CreateLocal(aSize,aSize);
		test(r==KErrNone);
		r=lchunk[i].MaxSize();
		test(r==roundToChunkSize(aSize));
		test(lchunk[i].Size()==roundToPageSize(aSize)); // was 0
		test(lchunk[i].Name().Left(6)==_L("Local-"));

		fullname=RProcess().Name();
		fullname+=_L("::");
		fullname+=lchunk[i].Name();
		test(lchunk[i].FullName().CompareF(fullname)==0);

		// create a global chunk
		name.Format(_L("Chunk%d"),i);
		r=gchunk[i].CreateGlobal(name,aSize,aSize);
		test(r==KErrNone);
  		test(gchunk[i].MaxSize()==roundToChunkSize(aSize));
		test(gchunk[i].Size()==roundToPageSize(aSize)); // was 0
		test(gchunk[i].Name()==name);
		}

	// make room for another chunk
	lchunk[KNumberOfChunks-1].Close();
	gchunk[KNumberOfChunks-1].Close();

//	Try to create a global chunk with a duplicate name
	test.Next(_L("Create a chunk with a duplicate name"));
	TInt r=chunk.CreateGlobal(_L("Chunk0"),aSize,aSize);
	test(r==KErrAlreadyExists);

 	test.Next(_L("Close all chunks"));
	for (i=0; i<KNumberOfChunks-1; i++)
		{
		lchunk[i].Close();
		gchunk[i].Close();
		}

	test.Next(_L("Reuse a name"));
	r=chunk.CreateGlobal(_L("Chunk1"),aSize,aSize);
	test(r==KErrNone);
	test(chunk.MaxSize()==roundToChunkSize(aSize));
   	test(chunk.Size()==roundToPageSize(aSize));	
	test(chunk.Name()==_L("Chunk1"));
  	chunk.Close();

	test.End();
	__KHEAP_MARKEND;
	}

void test3_2(RChunk& aChunk, TInt aSize)
//
//	Perform Adjust tests on aChunk
//
	{

	TInt r;
	test.Start(_L("Adjust to full size"));
	r=aChunk.Adjust(aSize);
 	test(r==KErrNone);
	test(aChunk.Size()==roundToPageSize(aSize));
	test(aChunk.MaxSize()==roundToChunkSize(aSize));

	test.Next(_L("Adjust a chunk to half size"));
	r=aChunk.Adjust(aSize/2);
	test(r==KErrNone);
	test(aChunk.Size()==roundToPageSize(aSize/2));
	test(aChunk.MaxSize()==roundToChunkSize(aSize));

	test.Next(_L("Adjust to same size"));
	r=aChunk.Adjust(aSize/2);
	test(r==KErrNone);
	test(aChunk.Size()==roundToPageSize(aSize/2));
	test(aChunk.MaxSize()==roundToChunkSize(aSize));

	test.Next(_L("Adjusting to size=0"));
 	r=aChunk.Adjust(0);
	test(r==KErrNone);
	test(aChunk.Size()==0);
	test(aChunk.MaxSize()==roundToChunkSize(aSize));

	test.Next(_L("Adjust back to half size"));
	r=aChunk.Adjust(aSize/2);
	test(r==KErrNone);
	test(aChunk.Size()==roundToPageSize(aSize/2));
	test(aChunk.MaxSize()==roundToChunkSize(aSize));

	test.End();
	}


void test3(TInt aSize)
//
//	Test Adjust size of chunk	
//
	{

	RChunk chunk;
	__KHEAP_MARK;	
	TInt r;
//	Run Adjust tests with a local chunk
	test.Start(_L("Local Chunk.Adjust()"));
	r=chunk.CreateLocal(aSize,aSize);
	test3_2(chunk, aSize);
	chunk.Close();
//	Run Adjust tests with a global chunk
	test.Next(_L("Global Chunk.Adjust()"));
	r=chunk.CreateGlobal(_L("Fopp"),aSize,aSize);
	test(KErrNone==r);
	test3_2(chunk,aSize);
	chunk.Close();
	test.End();
	__KHEAP_MARKEND;
	}

void test4(TInt aSize)
//
//	Test OpenGlobal
//
	{

	RChunk chunk[KChunkNum];
	RChunk c1, c2, c3;
	TName name;

	__KHEAP_MARK;
	TInt i,r;
	for (i=0; i<KChunkNum; i++)
		{
		name.Format(_L("Chunk%d"), i);
		r=chunk[i].CreateGlobal(name,aSize,aSize);
		}

	test.Start(_L("Open Global chunks"));
	r=c1.OpenGlobal(_L("Chunk1"),EFalse); // 2nd ref to this chunk
	test(r==KErrNone);
	r=c2.OpenGlobal(_L("Chunk3"),EFalse); // 2nd ref to this chunk
	test(r==KErrNone);
	r=c3.OpenGlobal(_L("Chunk4"),EFalse); // 2nd ref to this chunk
	test(r==KErrNone);
	c3.Close();

	test.Next(_L("Attempt Open non-existant chunk"));
	r=c3.OpenGlobal(_L("Non Existant Chunk"),EFalse);
	test(r==KErrNotFound);

	for (i=0; i<KChunkNum; i++)
		{
		test.Printf(_L("Closing chunk %d\n"),i);
		chunk[i].Close();
		}

	test.Next(_L("Test chunks with multiple references are still valid"));
	r=c1.Adjust(aSize/2);
	test(r==KErrNone);
	test(c1.MaxSize()==roundToChunkSize(aSize));
	test(c1.Size()==roundToPageSize(aSize/2));
	test(c1.Name()==_L("Chunk1"));

	r=c2.Adjust(aSize/2);
	test(r==KErrNone);
	test(c2.MaxSize()==roundToChunkSize(aSize));
	test(c2.Size()==roundToPageSize(aSize/2));
	test(c2.Name()==_L("Chunk3"));

//	Open another reference to a chunk
	r=c3.OpenGlobal(_L("Chunk3"),EFalse);
	test(r==KErrNone);
	test(c3.Base()==c2.Base());
	test(c3.Size()==c2.Size());
	test(c2.Size()!=aSize);
//	Adjust with one reference
	r=c3.Adjust(aSize);
	test(r==KErrNone);
//	Test sizes from the other
	test(c2.Size()==roundToPageSize(aSize));
	test(c2.MaxSize()==roundToChunkSize(aSize));

	c1.Close();
	c2.Close();
	c3.Close();

	test.Next(_L("And check the heap..."));
	test.End();
	__KHEAP_MARKEND;
	}

void test5(TInt aSize)
//
//	Test Open
//
	{
	
	RChunk chunk[2*KNumberOfChunks];
	RChunk c1;

	test.Start(_L("Creating Local and Global Chunks"));
	__KHEAP_MARK;
	TInt i,r;
	TBuf<0x40> b;

//	Create KNumberOfChunks Global chunks
	for (i=0; i<KNumberOfChunks; i++)
		{
		b.Format(_L("Chunk%d"), i);
		r=chunk[i].CreateGlobal(b,aSize,aSize);
		test(chunk[i].Name()==b);
		test(r==KErrNone);
		
		b.Format(_L("This is chunk %d"), i);
		b.Append(TChar(0));
		chunk[i].Adjust(aSize);
		Mem::Copy(chunk[i].Base(), b.Ptr(), b.Size());
		test(chunk[i].MaxSize()==roundToChunkSize(aSize));
		test(chunk[i].Size()==roundToPageSize(aSize));
		}

	test.Next(_L("Find and Open the Chunks"));
	TFindChunk find;
	TFullName name;
	for (i=0; i<KNumberOfChunks; i++)
		{
		test.Printf(_L("Opening chunk %d\n"),i);
		find.Find(chunk[i].FullName());
		r = find.Next(name);
		test(r==KErrNone);
		c1.Open(find);
		b=TPtrC((TText*)c1.Base());
		name.Format(_L("This is chunk %d"), i);
		test(b==name);
		c1.Close();
		}
	
	test.Next(_L("Close chunks"));
	for (i=0; i<KNumberOfChunks; i++)
		chunk[i].Close();

	test.End();
	__KHEAP_MARKEND;
	}


void test7(TInt aSize)
//
//	Deliberately cause RChunk panics
//
	{
	__KHEAP_MARK;

//	ENormal
	test.Start(_L("Test panic thread"));
	TestThreadExit(EExitKill, KNormalReturn, ThreadEntry, (TAny*)ENormal);

//	ECreateNegative
	test.Next(_L("Create Chunk with a negative size"));
	TestThreadExit(EExitPanic, EChkCreateMaxSizeNegative, ThreadEntry, (TAny*) ECreateNegative);

//	EAdjustNegative
	test.Next(_L("Adjust a Chunk to Size = -0x10"));
	gChunk.CreateLocal(aSize,aSize);
	TestThreadExit(EExitPanic, EChkAdjustNewSizeNegative, ThreadEntry, (TAny*) EAdjustNegative);
	gChunk.Close();

// ECreateInvalidType
	test.Next(_L("Create chunk of invalid type"));
	TestThreadExit(EExitPanic, EChkCreateInvalidType, ThreadEntry, (TAny*) ECreateInvalidType);

	test.End();

	__KHEAP_MARKEND;
	}


void testClear(TInt aSize)
	{
	__KHEAP_MARK;
	test.Start(_L("Test clearing memory (Platform Security)"));
	
	RChunk c1,c2,c3,c4,c5,c6,c7,c8,c9,c10;
	TInt r;
	
	TBuf<0x40> b;
	
	b.Copy(_L("Chunk"));
	
	r=c1.CreateGlobal(b,aSize,aSize);
	test(r==KErrNone);
	
	test((TBool)ETrue==CheckChunkCleared(c1));
	c1.Close();
	
	r=c2.CreateLocal(aSize,aSize,EOwnerProcess);
	test(r==KErrNone);
	
	test((TBool)ETrue==CheckChunkCleared(c2));
	c2.Close();

	r=c3.CreateLocalCode(aSize,aSize,EOwnerProcess);
	test(r==KErrNone);
	
	test((TBool)ETrue==CheckChunkCleared(c3));
	c3.Close();

	r=c4.CreateDoubleEndedLocal(0x1000,0x1000+aSize,0x100000);
	test(r==KErrNone);
	
	test((TBool)ETrue==CheckChunkCleared(c4,c4.Bottom()));
	c4.Close();

	r=c5.CreateDoubleEndedGlobal(b,0x1000,0x1000+aSize,0x100000,EOwnerProcess);
	test(r==KErrNone);
	
	test((TBool)ETrue==CheckChunkCleared(c5,c5.Bottom()));
	c5.Close();

	r=c6.CreateDisconnectedLocal(0x1000,0x1000+aSize,0x100000);
	test(r==KErrNone);
	
	test((TBool)ETrue==CheckChunkCleared(c6,0x1000));
	c6.Close();

	r=c7.CreateDisconnectedGlobal(b,0x1000,0x1000+aSize,0x100000,EOwnerProcess);
	test(r==KErrNone);
	
	test((TBool)ETrue==CheckChunkCleared(c7,0x1000));
	c7.Close();

	test.Next(_L("Test setting the clear byte of RChunk::Create()"));

	TChunkCreateInfo createInfo;
	createInfo.SetNormal(aSize, aSize);
	test_KErrNone(c10.Create(createInfo));
	TestClearChunk(c10, aSize, KDfltClearByte);

	createInfo.SetClearByte(0x0);
	test_KErrNone(c8.Create(createInfo));
	TestClearChunk(c8, aSize, 0x0);

	createInfo.SetClearByte(0xff);
	test_KErrNone(c9.Create(createInfo));
	TestClearChunk(c9, aSize, 0xff);

	test.End();
	__KHEAP_MARKEND;
	}

void testShare()
//
// Test sharing a chunk between threads
//
	{
	test.Start(_L("Test chunk sharing between threads"));

	test.Next(_L("Create chunk Marmalade"));
	TInt r=0;
	RChunk chunk;
	TInt size=0x1000;
	TInt maxSize=0x5000;
	r=0;
	r=chunk.CreateGlobal(_L("Marmalade"),size,maxSize);
	test(r==KErrNone);
	test.Next(_L("Write 0-9 to it"));
	TUint8* base=chunk.Base();
	for (TInt8 j=0;j<10;j++)
		*base++=j; // write 0 - 9 to the chunk

	RThread t;
	TRequestStatus stat;
	test.Next(_L("Create reader thread"));
	r=t.Create(_L("RChunkShareThread"), ThreadEntry2, KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
	t.Logon(stat);
	test.Next(_L("Resume reader thread"));
	t.Resume();
	User::WaitForRequest(stat);	
	CLOSE_AND_WAIT(t);
	chunk.Close();

	test.End();
	}

void FindChunks()
    { // taken from some code written by SteveG
    test.Start(_L("Finding chunks...\n"));

    TFullName name=_L("*");
    TFindChunk find(name);
	TInt i=0;


    while (find.Next(name)==KErrNone)
        {
        RChunk chunk;
        test.Printf(_L("Chunk name %S\n"),&name);
		TInt err=chunk.Open(find);
        if (err)
            test.Printf(_L("Error %d opening chunk"),err);
        else
        	{
			TBuf<16> access;
			if (chunk.IsWritable())
				access=_L("ReadWrite");
			else if (chunk.IsReadable())
				access=_L("ReadOnly");
			else
				access=_L("No Access");
            test.Printf(_L("Chunk size %08x bytes, %S\n"),chunk.Size(),&access);
			chunk.Close();
			i++;
			}
        User::After(1000000);
  	    }
    test.End();
    }

void testAdjustChunk()
	{
	test.Start(_L("Test adjusting chunks"));

	RChunk hermione;
	
	test.Next(_L("Create global chunk"));
	TInt r=hermione.CreateGlobal(_L("Hermione"),0x1000,0x100000);
	test(r==KErrNone);
	TUint32* base=(TUint32*)hermione.Base();
	TUint32* top=(TUint32*)(hermione.Base()+hermione.Size());
	TUint32* i;

	test.Printf(_L("Base = %08x, Top = %08x\n"),base,top);
	test.Next(_L("Check I can write to all of it"));
	for (i=base;i<top;i++)
		*i=0xdeaddead;

	test.Next(_L("Adjust the chunk"));
	r=hermione.Adjust(0x1400);
	test(r==KErrNone);

	base=(TUint32*)hermione.Base();
	top=(TUint32*)(hermione.Base()+hermione.Size());
	test.Printf(_L("Base = %08x, Top = %08x\n"),base,top);
	test.Next(_L("Check I can write to all of the bigger chunk"));
	for (i=base;i<top;i++)
		*i=0xdeaddead;

	hermione.Close();

	test.Next(_L("Do some size checks"));
	RChunk wibble;
	r=wibble.CreateGlobal(_L("Wibble"),0x1,gPageSize*8);
	test(r==KErrNone);
	test.Next(_L("Check create rounds up to page multiple"));
	test(wibble.Size()==(TInt)gPageSize);
	test.Next(_L("Check create rounds up to chunk multiple"));
	test(wibble.MaxSize()==roundToChunkSize(gPageSize*8));

	test.Next(_L("Check adjust rounds up to page multiple"));
	r=wibble.Adjust((gPageSize*6)-12);
	test(r==KErrNone);
	test(wibble.Size()==gPageSize*6);

	test.Next(_L("Different number, same size"));
	r=wibble.Adjust((gPageSize*6)-18);
	test(r==KErrNone);
	test(wibble.Size()==gPageSize*6);

	test.Next(_L("Check adjust > MaxSize returns error"));
	r=wibble.Adjust(wibble.MaxSize()+gPageSize);
	test(r==KErrArgument);

	wibble.Close();
	test.End();
	}

TInt NotifierCount=0;
TInt OOMCount=0;
RChangeNotifier Notifier;
RThread NtfThrd;
_LIT(KNotifierThreadName,"NotifierThread");

TInt NotifierThread(TAny*)
	{
	TInt r=Notifier.Create();
	while (r==KErrNone)
		{
		TRequestStatus s;
		r=Notifier.Logon(s);
		if (r!=KErrNone)
			break;
		User::WaitForRequest(s);
		if (s.Int()&EChangesFreeMemory)
			++NotifierCount;
		if (s.Int()&EChangesOutOfMemory)
			++OOMCount;
		}
	Notifier.Close();
	return r;
	}


void WaitForNotifier()
	{
	User::After(500000);		// wait for notifier
	}


void CheckNotifierCount(TInt aLevel, TInt aOom)
	{
	WaitForNotifier();
	if (NtfThrd.ExitType()!=EExitPending)
		{
		TExitCategoryName exitCat=NtfThrd.ExitCategory();
		test.Printf(_L("Thread exited: %d,%d,%S"),NtfThrd.ExitType(),NtfThrd.ExitReason(),&exitCat);
		test(0);
		}
	TInt c1=NotifierCount;
	TInt c2=OOMCount;
	if (c1!=aLevel || c2!=aOom)
		{
		test.Printf(_L("Count %d,%d Expected %d,%d"),c1,c2,aLevel,aOom);
		test(0);
		}
	}

void testNotifiers()
	{
	RGobbler gobbler;
	TInt r = gobbler.Open();
	test_KErrNone(r);
	TUint32 taken = gobbler.GobbleRAM(128*1024*1024);
	test.Printf(_L("Gobbled: %dK\n"), taken/1024);
	test.Printf(_L("Free RAM 0x%08X bytes\n"),FreeRam());

	test.Next(_L("Create thread"));
	r=NtfThrd.Create(KNotifierThreadName,NotifierThread,KDefaultStackSize,NULL,NULL);
	test_KErrNone(r);
	NtfThrd.SetPriority(EPriorityMore);
	NtfThrd.Resume();
	test.Next(_L("Check for initial notifier"));
	CheckNotifierCount(1,1);
	TInt free=FreeRam();
	test.Printf(_L("Free RAM: %dK\n"),free/1024);
	test_Value(free, free >= 1048576);
	test.Next(_L("Set thresholds"));
	r=UserSvr::SetMemoryThresholds(65536,524288);	// low=64K good=512K
	test_KErrNone(r);
	test.Next(_L("Create chunk"));
	// Chunk must not be paged otherwise it will not effect the amount 
	// of free ram reported plus on h4 swap size is less than the total ram.
	TInt totalRam;
	test_KErrNone(HAL::Get(HAL::EMemoryRAM, totalRam));
	TChunkCreateInfo createInfo;
	createInfo.SetNormal(0, totalRam);
	createInfo.SetPaging(TChunkCreateInfo::EUnpaged);
	RChunk c;
	test_KErrNone(c.Create(createInfo));
	const TInt KBufferSpace = 768*1024;	// 768K buffer
	CheckNotifierCount(1,1);
	test.Next(_L("Leave 768K"));
	r=c.Adjust(free-KBufferSpace);	// leave 768K
	test(r==KErrNone);
	CheckNotifierCount(1,1);		// shouldn't get notifier
	TInt free2=FreeRam();
	test.Printf(_L("Free RAM: %dK\n"),free2/1024);
	test(free2<=KBufferSpace);
	TInt free3=free-(KBufferSpace-free2);	// this accounts for space used by page tables
	test.Next(_L("Leave 32K"));
	r=c.Adjust(free3-32768);		// leave 32K
	test_KErrNone(r);
	CheckNotifierCount(2,1);		// should get notifier
	test.Next(_L("Leave 28K"));
	r=c.Adjust(free3-28672);		// leave 28K
	test_KErrNone(r);
	CheckNotifierCount(2,1);		// shouldn't get another notifier
	test.Next(_L("Ask for too much"));
	r=c.Adjust(totalRam);			// try to get more than available
	test_Equal(KErrNoMemory, r);
	CheckNotifierCount(2,2);		// should get another notifier
	test.Next(_L("Leave 128K"));
	r=c.Adjust(free3-131072);		// leave 128K
	test_KErrNone(r);;
	CheckNotifierCount(2,2);		// shouldn't get another notifier
	test.Next(_L("Leave 640K"));
	r=c.Adjust(free3-655360);		// leave 640K
	test_KErrNone(r);
	CheckNotifierCount(3,2);		// should get another notifier
	test.Next(_L("Leave 1M"));
	r=c.Adjust(free3-1048576);		// leave 1M
	test_KErrNone(r);
	CheckNotifierCount(3,2);		// shouldn't get another notifier
	test.Next(_L("Ask for too much"));
	r=c.Adjust(totalRam);			// try to get more than available
	test_Equal(KErrNoMemory, r);

	TInt notifierCount = 3;
	if(MemModel==EMemModelTypeFlexible)
		{
		// on flexible memory model, we get memory changed notifiers
		// on failed memory allocation; this hack lets the test code
		// pass this as acceptable behaviour...
		WaitForNotifier();
		notifierCount = NotifierCount; // expect whatever we actually got
		}

	CheckNotifierCount(notifierCount,3);		// should get another notifier
	test.Next(_L("Leave 1M"));
	r=c.Adjust(free3-1048576);					// leave 1M
	test_KErrNone(r);
	CheckNotifierCount(notifierCount,3);		// shouldn't get another notifier

	c.Close();
	TRequestStatus s;
	NtfThrd.Logon(s);
	NtfThrd.Kill(0);
	User::WaitForRequest(s);
	CLOSE_AND_WAIT(NtfThrd);
	Notifier.Close();
	gobbler.Close();
	}


// TestFullAddressSpace is used to stress the memory allocation mechanism(beyond the 1GB limit).
// However, the memory model can introduce limitations in the total amount of memory a single 
// process is allowed to allocate. To make the test more generic before closing the reserved 
// chunks for this test we trigger the creation of a new process. This process executes 
// t_chunk again, passing argument "extended". The result is that more chunks will be created
// through another call to TestFullAddressSpace, with the parameter extendedFlag set to true. 
// Eventually the total amount of allocated space will overcome the 1Gb limit in any case.

void TestFullAddressSpace(TBool extendedFlag )
	{
	test.Start(_L("Fill process address space with chunks\n"));
	RChunk chunk[2][11];
	TInt total = 0;
	TInt i;
	TInt j;
	TInt r;
		
	for(j=0; j<=1; j++)
		{
		if(!j)
			test.Next(_L("Creating local chunks"));
		else
			test.Next(_L("Creating global chunks"));
		for(i=10; i>=0; --i)
			{
			TInt size = 1<<(20+i);
			
			if(!j)
				r = chunk[j][i].CreateDisconnectedLocal(0,0,size);
			else
				r = chunk[j][i].CreateDisconnectedGlobal(KNullDesC,0,0,size);
			TBuf<128> text;
			text.AppendFormat(_L("Create %dMB chunk returns %d"),1<<i,r);
			test.Next(text);
			if(r!=KErrNoMemory)
				{
				test(r==KErrNone);
				// commit memory to each 1MB region,
				// this excercises page table allocation
				volatile TUint8* base = (TUint8*)chunk[j][i].Base();
				for(TInt o=0; o<size; o+=1<<20)
					{
					r = chunk[j][i].Commit(o,1);
					test(r==KErrNone);
					// access the commited memory...
					base[o] = (TUint8)(o&0xff);
					test(base[o]==(TUint8)(o&0xff));
					base[o] = (TUint8)~(o&0xff);
					test(base[o]==(TUint8)~(o&0xff));
					}
				total += 1<<i;
				}
			}
		}
		
	if (extendedFlag == EFalse)
		{
		
		test.Printf(_L("Total chunk size created was %d MB\n\n"),total);	  

		if(total<1024)
			{
			_LIT(KOtherProcessName,"t_chunk");
			_LIT(KProcessArgs,"extended");
			
			RProcess process;      
			r=process.Create(KOtherProcessName,KProcessArgs );
			test.Printf(_L("Creating new process( t_chunk extended) returns %d\n"),r );
			test( r == KErrNone);
		   
			TRequestStatus status;
			process.Logon(status);
			process.Resume(); 
			
			User::WaitForRequest(status);
			  
			test(process.ExitType() == EExitKill);
			test(process.ExitReason() == 0);
			process.Close();
			}
	
		}           
  	else
  		test.Printf(_L("Total chunk size created by the new process was %d MB\n"),total); 
 
	for(j=0; j<=1; j++)
		for(i=10; i>=0; --i)
			chunk[j][i].Close();
	test.End();
	}	


#ifdef __WINS__
void TestExecLocalCode()
	{
	RChunk c;
	TInt size = 10 * 1024;
	TInt rc = c.CreateLocalCode(size, size, EOwnerProcess);
	test_KErrNone(rc);
	TUint8 *p = c.Base();
	TUint32 (*func)() = (TUint32 (*)())p;
	test.Printf(_L("Create small function in the new code chunk\n"));
	*p++ = 0xB8;		// mov eax, 0x12345678
	*p++ = 0x78;
	*p++ = 0x56;
	*p++ = 0x34;
	*p++ = 0x12;
	*p   = 0xC3;		// ret
	test.Printf(_L("Going to call the new function\n"));
	TUint32 res = (*func)();
	test_Equal(0x12345678, res);
	c.Close();
	}
#endif	//  __WINS__


_LIT(KChunkName, "CloseChunk");

struct TRequestData
	{
	RSemaphore requestSem;
	RSemaphore completionSem;
	RSemaphore nextItSem;
	};

TInt CloseThread(TAny* data)
	{
	TRequestData* reqData = (TRequestData*)data;
	ASSERT(reqData);
	
	for(;;)
		{
		// Wait for a request to open and close the chunk.
		reqData->requestSem.Wait();
		
		// Try to open the chunk (may have already been closed by another thread).
		RChunk chunk;
		TInt r = chunk.OpenGlobal(KChunkName, EFalse, EOwnerThread);
		if (r != KErrNone)
			{
			// The chunk was already closed...
			r = (r == KErrNotFound) ? KErrNone : r;	// Ensure no debug output for expected failures.
			
			if(r != KErrNone)
				{
				test.Printf(_L("CloseThread RChunk::OpenGlobal Err: %d\n"), r);
				test_KErrNone(r);
				}
			}
		else
			{ 
			// Close the chunk.
			chunk.Close();
			}
		
		// Tell our parent we have completed this iteration and wait for the next.
		reqData->completionSem.Signal();
		reqData->nextItSem.Wait();
		}
	}

void TestClosure()
	{
	const TUint KCloseThreads = 50;
	RThread thread[KCloseThreads];
	TRequestStatus dead[KCloseThreads];

	// We need three semaphores or we risk signal stealing if one thread gets ahead of the
	// others and starts a second iteration before the other threads have been signalled
	// and have begun their first iteration.  Such a situation results in deadlock so we
	// force all threads to finish the iteration first using the nextItSem semaphore to
	// ensure we can only move to the next iteration once every thread has completed the
	// current iteration.
	TRequestData reqData;
	test_KErrNone(reqData.requestSem.CreateLocal(0));
	test_KErrNone(reqData.completionSem.CreateLocal(0));
	test_KErrNone(reqData.nextItSem.CreateLocal(0));
	
	TUint i = 0;

	// Create thread pool.  We do this rather than create 50 threads
	// over and over again for 800 times - the kernel's garbage collection
	// does not keep up and we run out of backing store.
	for (; i < KCloseThreads; i++)
		{
		test_KErrNone(thread[i].Create(KNullDesC, CloseThread, KDefaultStackSize, NULL, (TAny*)&reqData));
		thread[i].Logon(dead[i]);
		thread[i].SetPriority(EPriorityMuchLess);
		thread[i].Resume();
		}

	for (TUint delay = 200; delay < 1000; delay++)
		{
		test.Printf(_L("Closure delay %dus\r"), delay);

		// Create a global chunk.
		RChunk chunk;
		test_KErrNone(chunk.CreateGlobal(KChunkName, gPageSize, gPageSize));

		// Release the threads so they can try to close the handle.
		reqData.requestSem.Signal(KCloseThreads);

		// Wait for the delay then close the handle ourselves.
		User::AfterHighRes(delay);
		chunk.Close();

		// Wait for the threads to complete then release them for the next iteration.
		for (i = 0; i < KCloseThreads; i++)
			{
			reqData.completionSem.Wait();
			}
		reqData.nextItSem.Signal(KCloseThreads);

		// Ensure garbage collection is complete to prevent the kernel's
		// garbage collection from being overwhelmed and causing the
		// backing store to be exhausted.
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
		}

	// Kill thread pool.
	for (i = 0; i < KCloseThreads; i++)
		{
		thread[i].Kill(KErrNone);
		User::WaitForRequest(dead[i]);
		test(KErrNone == thread[i].ExitReason());
		test_Equal(EExitKill, thread[i].ExitType());
		thread[i].Close();
		}
		
	reqData.requestSem.Close();
	reqData.completionSem.Close();
	reqData.nextItSem.Close();

	// Ensure garbage collection is complete to prevent false positive
	// kernel memory leaks.
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	}
	

/**Returns true if argument is found in the command line*/
TBool IsInCommandLine(const TDesC& aArg)
	{
	TBuf<64> c;
	User::CommandLine(c);
	if (c.FindF(aArg) >= 0)
		return ETrue;
	return EFalse;
	}

_LIT(KTestChunkReadOnly, "TestReadOnlyChunk");
_LIT(KTestSemaphoreReadOnly, "TestReadOnlySemaphore");
_LIT(KTestParamRo, "restro");
_LIT(KTestParamRw, "restrw");
_LIT(KTestParamWait, "restwait");
_LIT(KTestParamWritableChunk, "restwritable");

enum TTestProcessParameters
	{
	ETestRw = 0x1,
	ETestWait = 0x2,
	ETestWritableChunk = 0x4,
	};

void TestReadOnlyProcess(TUint aParams)
	{
	TInt r;
	RChunk chunk;
	RSemaphore sem;

	test.Start(_L("Open global chunk"));
	r = chunk.OpenGlobal(KTestChunkReadOnly, EFalse);
	test_KErrNone(r);

	test(chunk.IsReadable());
	r = chunk.Adjust(1);
	if (aParams & ETestWritableChunk)
		{
		test(chunk.IsWritable());
		test_KErrNone(r);
		}
	else
		{
		test(!chunk.IsWritable());
		test_Equal(KErrAccessDenied, r);
		}

	if (aParams & ETestWait)
		{
		RProcess::Rendezvous(KErrNone);
		test.Next(_L("Wait on semaphore"));
		r = sem.OpenGlobal(KTestSemaphoreReadOnly);
		test_KErrNone(r);
		sem.Wait();
		}

	test.Next(_L("Read"));
	TUint8 read = *(volatile TUint8*) chunk.Base();
	(void) read;

	if (aParams & ETestRw)
		{
		test.Next(_L("Write"));
		TUint8* write = chunk.Base();
		*write = 0x3d;
		}

	chunk.Close();
	if (aParams & ETestWait)
		{
		sem.Close();
		}
	test.End();
	}

void TestReadOnly()
	{
	TInt r;
	RChunk chunk;
	RProcess process1;
	RProcess process2;
	RSemaphore sem;
	TRequestStatus rs;
	TRequestStatus rv;

	// Assumption is made that any memory model from Flexible onwards that supports
	// read-only memory also supports read-only chunks
	if (MemModelType() < EMemModelTypeFlexible || !HaveWriteProt())
		{
		test.Printf(_L("Memory model is not expected to support Read-Only Chunks\n"));
		return;
		}

	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);

	test.Start(_L("Create writable global chunk"));
	TChunkCreateInfo info;
	info.SetNormal(0, 1234567);
	info.SetGlobal(KTestChunkReadOnly);
	r = chunk.Create(info);
	test_KErrNone(r);
	test(chunk.IsReadable());
	test(chunk.IsWritable());

	test.Next(_L("Adjust size"));
	r = chunk.Adjust(1); // add one page
	test_KErrNone(r);

	test.Next(_L("Attempt read/write 1"));
	r = process1.Create(RProcess().FileName(), KTestParamWritableChunk);
	test_KErrNone(r);
	process1.Logon(rs);
	process1.Resume();
	User::WaitForRequest(rs);
	test_Equal(EExitKill, process1.ExitType());
	test_KErrNone(process1.ExitReason());
	CLOSE_AND_WAIT(process1);
	CLOSE_AND_WAIT(chunk);

	test.Next(_L("Create read-only global chunk"));
	info.SetReadOnly();
	r = chunk.Create(info);
	test_KErrNone(r);
	test(chunk.IsReadable());
	test(chunk.IsWritable());
	// To keep in sync with the 'process2' process
	r = sem.CreateGlobal(KTestSemaphoreReadOnly, 0);
	test_KErrNone(r);

	test.Next(_L("Attempt read 1"));
	r = process1.Create(RProcess().FileName(), KTestParamRo);
	test_KErrNone(r);
	process1.Logon(rs);
	process1.Resume();
	User::WaitForRequest(rs);
	test_Equal(EExitPanic, process1.ExitType());
	test_Equal(3, process1.ExitReason()); // KERN-EXEC 3 assumed
	CLOSE_AND_WAIT(process1);
	// Create second process before commiting memory and make it wait
	r = process2.Create(RProcess().FileName(), KTestParamWait);
	test_KErrNone(r)
	process2.Rendezvous(rv);
	process2.Resume();
	User::WaitForRequest(rv);

	test.Next(_L("Adjust size"));
	r = chunk.Adjust(1); // add one page
	test_KErrNone(r);

	test.Next(_L("Attempt read 2"));
	r = process1.Create(RProcess().FileName(), KTestParamRo);
	test_KErrNone(r);
	process1.Logon(rs);
	process1.Resume();
	User::WaitForRequest(rs);
	test_Equal(EExitKill, process1.ExitType());
	test_KErrNone(process1.ExitReason());
	CLOSE_AND_WAIT(process1);

	test.Next(_L("Attempt read/write 1"));
	r = process1.Create(RProcess().FileName(), KTestParamRw);
	test_KErrNone(r);
	process1.Logon(rs);
	process1.Resume();
	User::WaitForRequest(rs);
	test_Equal(EExitPanic, process1.ExitType());
	test_Equal(3, process1.ExitReason()); // KERN-EXEC 3 assumed
	CLOSE_AND_WAIT(process1);
	// Controlling process is not affected
	TUint8* write = chunk.Base();
	*write = 0x77;

	test.Next(_L("Attempt read/write 2"));
	test_Equal(EExitPending, process2.ExitType());
	process2.Logon(rs);
	sem.Signal();
	User::WaitForRequest(rs);
	test_Equal(EExitPanic, process2.ExitType());
	test_Equal(3, process2.ExitReason()); // KERN-EXEC 3 assumed
	CLOSE_AND_WAIT(process2);

	chunk.Close();
	sem.Close();
	test.End();
	User::SetJustInTime(jit);
	}


void TestTopBottom()
//
// test chunk top and bottom exec calls and check size
//
	{
	RChunk chunk;
	TInt r;
	TInt init_bottom = 0x1000;
	TInt init_top = 0x5000;
	TInt max=0x10000;

	TInt top;
	TInt bottom;

	r=chunk.CreateDoubleEndedLocal(init_bottom,init_top,max);
	test_KErrNone(r);

	top=chunk.Top();
	bottom=chunk.Bottom();
	test_Equal(top, init_top);
	test_Equal(bottom, init_bottom);
	
	r=chunk.AdjustDoubleEnded(init_bottom+0x1000, init_top-0x1000);
	test_KErrNone(r);

	top=chunk.Top();
	bottom=chunk.Bottom();
	test_Equal(top, init_top-0x1000);
	test_Equal(bottom, init_bottom+0x1000);
	
	chunk.Close();
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
	testInitialise();

	// Turn off lazy dll unloading so the kernel heap checking isn't affected.
	RLoader l;
	test(l.Connect()==KErrNone);
	test(l.CancelLazyDllUnload()==KErrNone);
	l.Close();

	_LIT(KExtended,"extended");

	if (IsInCommandLine(KExtended))
		{
		__KHEAP_MARK;
		test.Printf(_L("t_chunk extended was called. Ready to call TestFullAddressSpace(Etrue) \n"));
		TestFullAddressSpace(ETrue);
		__KHEAP_MARKEND;
		}
	else if (IsInCommandLine(KTestParamRo))
		{
		test_KErrNone(User::RenameProcess(KTestParamRo));
		TestReadOnlyProcess(0);
		}
	else if (IsInCommandLine(KTestParamRw))
		{
		test_KErrNone(User::RenameProcess(KTestParamRw));
		TestReadOnlyProcess(ETestRw);
		}
	else if (IsInCommandLine(KTestParamWait))
		{
		test_KErrNone(User::RenameProcess(KTestParamWait));
		TestReadOnlyProcess(ETestRw | ETestWait);
		}
	else if (IsInCommandLine(KTestParamWritableChunk))
		{
		test_KErrNone(User::RenameProcess(KTestParamWritableChunk));
		TestReadOnlyProcess(ETestWritableChunk | ETestRw);
		}
	else 
		{
		__KHEAP_MARK;
		test.Start(_L("Testing.."));
		testAdjustChunk();
		test.Next(_L("Test1"));
		test1();
		test.Next(_L("Test2"));
		test2(0x80);
		test.Next(_L("Test3"));
		test3(0x7000);
		test.Next(_L("Test4"));
		test4(0x7000);
		test.Next(_L("Test5"));
		test5(0x80);
		test.Next(_L("Test7"));
		test7(0x80);
		test.Next(_L("Test chunk data clearing attributes"));
		testClear(0x500);
		testClear(07000);
		test.Next(_L("Test9"));
		testShare();
		test.Next(_L("Test memory notifiers"));
		testNotifiers();
		test.Next(_L("FindChunks"));
		FindChunks();
		
		test.Next(_L("Test full address space"));
		TestFullAddressSpace(EFalse);

#ifdef __WINS__
		test.Next(_L("Test execution of code in a local code chunk on emulator"));
		TestExecLocalCode();
#endif	//  __WINS__

		test.Next(_L("Test for race conditions in chunk closure"));
		TestClosure();
		test.Next(_L("Read-only chunks"));
		TestReadOnly();

		test.Next(_L("Test chunk top and bottom"));
		TestTopBottom();

		test.End();
		__KHEAP_MARKEND;
		}

	test.Close();
	return(KErrNone);
	}
