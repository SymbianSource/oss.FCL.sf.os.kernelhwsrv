// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// camera1_test.cpp
// in its implementation.
// 
//

/**
 @file Test code for example data converter device driver which uses Shared Chunks
 @publishedPartner
 @prototype 9.1
*/

#include <e32test.h>
#include <e32svr.h>
#include <e32def.h>
#include <e32def_private.h>
#include "convert1.h"

LOCAL_D RTest test(_L("CONVERT1_TEST"));

RConvert1 Convert;

RConvert1::TConfigBuf ConfigBuf;

_LIT(KConvert1FileName,"convert1_ldd");


_LIT8(KTestData1,"0123456789ABCDEF");
_LIT8(KTestData2,"1032547698@CBEDG");

class RHeapFriend : public RHeap
	{
public: RChunk Chunk() { return *(RChunk*)&iChunkHandle; };
	};

GLDEF_C TInt E32Main()
    {
	test.Title();

	TInt r;
	TRequestStatus s;

	test.Start(_L("Load Device"));
	r=User::LoadLogicalDevice(KConvert1FileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	__KHEAP_MARK;

	test.Next(_L("Open Device"));
	RDevice device;
	r=device.Open(RConvert1::Name());
	test(r==KErrNone);

	test.Next(_L("Get Device Capabilities"));
	RConvert1::TCaps caps;
	TPckg<RConvert1::TCaps>capsPckg(caps);
	capsPckg.FillZ(); // Zero 'caps' so we can tell if GetCaps has really filled it
	device.GetCaps(capsPckg);
	TVersion expectedVer(RConvert1::VersionRequired());
	test(caps.iVersion.iMajor==expectedVer.iMajor);
	test(caps.iVersion.iMinor==expectedVer.iMinor);
	test(caps.iVersion.iBuild==expectedVer.iBuild);
	test(caps.iMaxChannels>0);

	test.Next(_L("Close Device"));
	device.Close();

	test.Next(_L("Open Logical Channel"));
	r=Convert.Open();
	test(r==KErrNone);

	test.Next(_L("GetConfig"));
	RConvert1::TConfig& config=ConfigBuf();
	ConfigBuf.FillZ();   // Zero 'config' so we can tell if GetConfig has really filled it
	r=Convert.GetConfig(ConfigBuf);
	test(r==KErrNone);

	// Check config values
	const TInt KDefaultBufferSize(config.iBufferSize);
	test(KDefaultBufferSize!=0);
	test(config.iSpeed!=0);

	test.Next(_L("SetConfig"));
	config.iBufferSize = KDefaultBufferSize*2;
	config.iCreateInputChunk = ETrue;
	test.Printf(_L("config = %d,%d,%d"),config.iBufferSize,config.iCreateInputChunk,config.iSpeed);
	r=Convert.SetConfig(ConfigBuf);
	test.Printf(_L("result = %d"),r);
	test(r==KErrNone);

	test.Next(_L("Check config set"));
	ConfigBuf.FillZ();
	r=Convert.GetConfig(ConfigBuf);
	test(r==KErrNone);
	test(config.iBufferSize==KDefaultBufferSize*2);

	test.Next(_L("Check access by wrong client"));
	RConvert1 ldd2=Convert;
	r=((RHandleBase&)ldd2).Duplicate(RThread(),EOwnerProcess);
	test(r==KErrAccessDenied);

	test.Next(_L("Check handle duplication"));
	ldd2=Convert;
	r=((RHandleBase&)ldd2).Duplicate(RThread(),EOwnerThread);
	test(r==KErrNone);
	((RHandleBase&)ldd2).Close();

	test.Next(_L("Check input chunk handle"));
	TUint8* chunkBase = Convert.InChunk().Base();

	{
	test.Next(_L("Convert a descriptor"));
	Convert.Convert(KTestData1,s);
	User::WaitForRequest(s);
	r = s.Int();
	test(r>=0);
	TPtrC8 out(Convert.OutChunk().Base()+r,KTestData1().Size());
	test(out==KTestData2);

	test.Next(_L("Convert a descriptor (too big)"));
	RBuf8 big;
	test(big.CreateMax(config.iBufferSize+1)==KErrNone);
	Convert.Convert(big,s);
	User::WaitForRequest(s);
	r = s.Int();
	test(r==KErrTooBig);

	test.Next(_L("Convert a descriptor (max length)"));
	big.SetLength(big.Length()-1);
	Convert.Convert(big,s);
	User::WaitForRequest(s);
	r = s.Int();
	test(r==KErrNone);
	}

	{
	test.Next(_L("Convert a descriptor in a shared chunk"));
	TPtr8 in(chunkBase+KTestData2().Size(),KTestData2().Size());
	in = KTestData2;
	Convert.Convert(in,s);
	User::WaitForRequest(s);
	r = s.Int();
	test(r>=0);
	TPtrC8 out(Convert.OutChunk().Base()+r,KTestData2().Size());
	test(out==KTestData1);

	test.Next(_L("Convert a descriptor in a shared chunk (overflow chunk)"));
	Convert.Convert(TPtr8(chunkBase+1,config.iBufferSize),s);
	User::WaitForRequest(s);
	r = s.Int();
	test(r==KErrNone);
	}

	{
	test.Next(_L("Convert data in a shared chunk"));
	TInt offset = 2*KTestData1().Size();
	TPtr8 in(chunkBase+offset,KTestData1().Size());
	in = KTestData1;
	Convert.Convert(Convert.InChunk(),offset,KTestData1().Size(),s);
	User::WaitForRequest(s);
	r = s.Int();
	test(r>=0);
	TPtrC8 out(Convert.OutChunk().Base()+r,KTestData1().Size());
	test(out==KTestData2);

	test.Next(_L("Convert data in a non-shared chunk (should fail)"));
	Convert.Convert(((RHeapFriend&)User::Heap()).Chunk(),offset,KTestData1().Size(),s);
	User::WaitForRequest(s);
	r = s.Int();
	test(r==KErrArgument);

	test.Next(_L("Convert data in a shared chunk (overflow chunk)"));
	Convert.Convert(Convert.InChunk(),1,config.iBufferSize,s);
	User::WaitForRequest(s);
	r = s.Int();
	test(r==KErrNotFound || r==KErrArgument);
	}

	{
	test.Next(_L("Convert data in input shared chunk"));
	Convert.InBuffer() = KTestData2;
	Convert.Convert(KTestData2().Size(),s);
	User::WaitForRequest(s);
	r = s.Int();
	test(r>=0);
	TPtrC8 out(Convert.OutChunk().Base()+r,KTestData2().Size());
	test(out==KTestData1);
	}

	test.Next(_L("Close Logical Channel"));
	Convert.Close();

	{
	test.Next(_L("Test max number of Logical Channels"));
	RConvert1* channels = new RConvert1[caps.iMaxChannels+1];
	TInt i=0;
	// Open max number of channels
	for(; i<caps.iMaxChannels; i++)
		test(channels[i].Open()==KErrNone);
	// Try one more
	test(channels[i].Open()==KErrInUse);
	// Close all channels
	while(--i>=0)
		channels[i].Close();
	delete[] channels;
	}

	User::After(500000);	// allow any async close operations to complete

	__KHEAP_MARKEND;

	test.Next(_L("Unload Device"));
	User::FreeLogicalDevice(RConvert1::Name());

	test.End();

	return(0);
    }


