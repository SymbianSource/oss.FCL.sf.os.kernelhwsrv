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
// in its implementation.
// 
//

/**
 @file Test code for example camera device driver which uses Shared Chunks
 @publishedPartner
 @prototype 9.1
*/

#include <e32test.h>
#include <e32svr.h>
#include <e32def.h>
#include <e32def_private.h>
#include "camera1.h"

LOCAL_D RTest test(_L("CAMERA1_TEST"));

RCamera1 Camera;

RCamera1::TConfigBuf ConfigBuf;

const TInt KMaxBuffers = 8;

_LIT(KCamera1FileName,"camera1_ldd");

enum TBufferMode
	{
	EReleaseOnCapture,
	EReleaseInBlocks,
	};

void Capture(TInt aNumBuffers, TBufferMode aMode)
	{
	TInt r;

	test(aNumBuffers<=KMaxBuffers);

	test.Start(_L("SetConfig"));
	ConfigBuf().iNumImageBuffers = aNumBuffers;
	ConfigBuf().iFrameRate=10;
	r=Camera.SetConfig(ConfigBuf);
	test(r==KErrNone);

	// Base address of chunk which contains images
	TUint8* chunkBase=Camera.ImageChunk().Base();

	test.Next(_L("StartCapture"));
	r=Camera.StartCapture();
	test(r==KErrNone);

	test.Next(_L("Capture images..."));
	TInt imageBuffer[KMaxBuffers]; // Array of image buffers we've received
	memset(imageBuffer,~0,sizeof(imageBuffer)); // Initialise to 'empty' (-1)
	TInt lastFrameCounter = -1;
	TInt bufNum = 0;

	// Stream load of images...
	for(TInt i=0; i<20*aNumBuffers; i++)
		{
		// Stall half way through streaming test...
		if(i==10+aNumBuffers-1)
			{
			test.Next(_L("Stall during image capture"));
			User::After(500000);
			}

		// Get an image...
		TRequestStatus s;
		if(aMode==EReleaseInBlocks)
			Camera.CaptureImage(s);
		else
			Camera.CaptureImage(s,imageBuffer[bufNum]);
		User::WaitForRequest(s);

		// imageOffset = capture result
		TInt imageOffset=s.Int();
		imageBuffer[bufNum] = imageOffset;

		// Error?
		if(imageOffset<0)
			{
			test.Printf(_L("Error = %d\n"),imageOffset);
			test(0);
			}

		// Check image memory is accessable and get counter
		TInt frameCounter = *(TInt*)(chunkBase+imageOffset);  // Test driver puts frame counter at image start
		RDebug::Print(_L("Capture %08x(%04d)\n"),imageOffset,frameCounter);
		test(frameCounter>lastFrameCounter);

		// Move on to next buffer...
		if(++bufNum>=aNumBuffers)
			{
			if(aMode==EReleaseInBlocks)
				{
				// Release all the image buffers we have...
				for(bufNum=0; bufNum<aNumBuffers; bufNum++)
					{
					RDebug::Print(_L("Release %08x\n"),imageBuffer[bufNum]);
					r=Camera.ReleaseImage(imageBuffer[bufNum]);
					imageBuffer[bufNum] = -1;
					test(r==KErrNone);
					}
				}
			bufNum = 0;
			}
		}

	test.Next(_L("EndCapture"));
	r=Camera.EndCapture();
	test(r==KErrNone);

	test.End();
	}


GLDEF_C TInt E32Main()
    {
	test.Title();

	TInt r;

	test.Start(_L("Loading CAMERA1 Device"));
	r=User::LoadLogicalDevice(KCamera1FileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	__KHEAP_MARK;

	test.Next(_L("Open Device"));
	RDevice device;
	r=device.Open(RCamera1::Name());
	test(r==KErrNone);

	test.Next(_L("Get Device Capabilities"));
	RCamera1::TCaps caps;
	TPckg<RCamera1::TCaps>capsPckg(caps);
	capsPckg.FillZ(); // Zero 'caps' so we can tell if GetCaps has really filled it
	device.GetCaps(capsPckg);
	TVersion expectedVer(RCamera1::VersionRequired());
	test(caps.iVersion.iMajor==expectedVer.iMajor);
	test(caps.iVersion.iMinor==expectedVer.iMinor);
	test(caps.iVersion.iBuild==expectedVer.iBuild);

	test.Next(_L("Close Device"));
	device.Close();

	test.Next(_L("Open Logical Channel"));
	r=Camera.Open();
	test(r==KErrNone);

	test.Next(_L("GetConfig"));
	RCamera1::TConfig& config=ConfigBuf();
	ConfigBuf.FillZ();   // Zero 'config' so we can tell if GetConfig has really filled it
	r=Camera.GetConfig(ConfigBuf);
	test(r==KErrNone);

	const TSize KDefaultImageSize(config.iImageSize);
	test(KDefaultImageSize.iWidth!=0);
	test(KDefaultImageSize.iHeight!=0);
	test(config.iImageBytesPerPixel!=0);

	test.Next(_L("StartCapture (before SetConfig has been called)"));
	r=Camera.StartCapture();
	test(r==KErrNotReady);

	test.Next(_L("SetConfig"));
	config.iImageSize.iWidth = KDefaultImageSize.iWidth/2;
	config.iImageSize.iHeight = KDefaultImageSize.iHeight/2;
	config.iFrameRate = 2; // Slow rate to give timing dependant tests a chance
	config.iNumImageBuffers = 1;
	r=Camera.SetConfig(ConfigBuf);
	test(r==KErrNone);

	test.Next(_L("Check handle duplication"));
	RCamera1 Camera2=Camera;
	r=Camera2.Duplicate(RThread(),EOwnerProcess);
	test(r==KErrNone);
	Camera2.Close();

	test.Next(_L("Check config set"));
	ConfigBuf.FillZ();
	r=Camera.GetConfig(ConfigBuf);
	test(r==KErrNone);
	test(config.iImageSize.iWidth==KDefaultImageSize.iWidth/2);
	test(config.iImageSize.iHeight==KDefaultImageSize.iHeight/2);
	test(ConfigBuf().iFrameRate==2);
	test(ConfigBuf().iNumImageBuffers==1);

	test.Next(_L("Check image chunk handle"));
	test(Camera.ImageChunk().Handle()!=KNullHandle);
	Camera.ImageChunk().Base();

	test.Next(_L("CaptureImage (before StartCapture has been called)"));
	TRequestStatus s;
	Camera.CaptureImage(s);
	User::WaitForRequest(s);
	test(s.Int()==KErrNotReady);

	test.Next(_L("StartCapture"));
	r=Camera.StartCapture();
	test(r==KErrNone);

	test.Next(_L("StartCapture again"));
	r=Camera.StartCapture();
	test(r==KErrInUse);

	test.Next(_L("SetConfig whilst capturing"));
	r=Camera.SetConfig(ConfigBuf);
	test(r==KErrInUse);

	test.Next(_L("CaptureImage"));
	Camera.CaptureImage(s);

	test.Next(_L("CaptureImage again (before last has completed)"));
	TRequestStatus s2;
	Camera.CaptureImage(s2);
	User::WaitForRequest(s2);
	test(s2.Int()==KErrInUse);

	test.Next(_L("CaptureCancel"));
	Camera.CaptureImageCancel();
	User::WaitForRequest(s);
	test(s.Int()==KErrCancel);

	test.Next(_L("CaptureCancel again"));
	Camera.CaptureImageCancel();

	test.Next(_L("CaptureImage"));
	Camera.CaptureImage(s);
	User::WaitForRequest(s);
	test(s.Int()>=0);

	test.Next(_L("CaptureImage again (before releasing previous image)"));
	Camera.CaptureImage(s2);
	User::WaitForRequest(s2);
	test(s2.Int()==KErrOverflow);

	test.Next(_L("ReleaseImage"));
	r=Camera.ReleaseImage(s.Int());
	test(r==KErrNone);

	test.Next(_L("ReleaseImage again"));
	r=Camera.ReleaseImage(s.Int());
	test(r==KErrNotFound);

	test.Next(_L("CaptureImage"));
	Camera.CaptureImage(s);

	test.Next(_L("EndCapture"));
	r=Camera.EndCapture();
	test(r==KErrNone);
	User::WaitForRequest(s);
	test(s.Int()==KErrCancel);

	test.Next(_L("CaptureImage streaming with 1 buffer and ReleaseOnCapture"));
	Capture(1,EReleaseOnCapture);
	test.Next(_L("CaptureImage streaming with 1 buffer and EReleaseInBlocks"));
	Capture(1,EReleaseInBlocks);

	test.Next(_L("CaptureImage streaming with 2 buffers and ReleaseOnCapture"));
	Capture(2,EReleaseOnCapture);
	test.Next(_L("CaptureImage streaming with 2 buffers and EReleaseInBlocks"));
	Capture(2,EReleaseInBlocks);

	test.Next(_L("CaptureImage streaming with 3 buffers and ReleaseOnCapture"));
	Capture(3,EReleaseOnCapture);
	test.Next(_L("CaptureImage streaming with 3 buffers and EReleaseInBlocks"));
	Capture(3,EReleaseInBlocks);

	test.Next(_L("Close Logical Channel"));
	Camera.Close();

	test.Next(_L("Test cleanup 1"));

		test.Start(_L("Open Logical Channel"));
		r=Camera.Open();
		test(r==KErrNone);

		test.Next(_L("Close Logical Channel"));
		Camera.Close();

		test.End();

	test.Next(_L("Test cleanup 2"));

		test.Start(_L("Open Logical Channel"));
		r=Camera.Open();
		test(r==KErrNone);

		test.Next(_L("SetConfig"));
		r=Camera.SetConfig(ConfigBuf);
		test(r==KErrNone);

		test.Next(_L("Close Logical Channel"));
		Camera.Close();

		test.End();

	test.Next(_L("Test cleanup 2"));

		test.Start(_L("Open Logical Channel"));
		r=Camera.Open();
		test(r==KErrNone);

		test.Next(_L("SetConfig"));
		r=Camera.SetConfig(ConfigBuf);
		test(r==KErrNone);

		test.Next(_L("StartCapture"));
		r=Camera.StartCapture();
		test(r==KErrNone);

		test.Next(_L("Close Logical Channel"));
		Camera.Close();

		test.End();

	test.Next(_L("Test cleanup 3"));

		test.Start(_L("Open Logical Channel"));
		r=Camera.Open();
		test(r==KErrNone);

		test.Next(_L("SetConfig"));
		r=Camera.SetConfig(ConfigBuf);
		test(r==KErrNone);

		test.Next(_L("StartCapture"));
		r=Camera.StartCapture();
		test(r==KErrNone);

		test.Next(_L("CaptureImage"));
		Camera.CaptureImage(s);
		User::WaitForRequest(s);

		test.Next(_L("Close Logical Channel"));
		Camera.Close();

		test.End();

	test.End();

	User::After(500000);	// allow any async close operations to complete

	__KHEAP_MARKEND;

	return(0);
    }


