// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\multimedia\t_camera_api.cpp
// 
//

#include <e32test.h>
#include <d32camerasc.h>
#include <e32def.h>
#include <e32def_private.h>
#include "t_camera_display.h"
#include "t_camera_bitmap.h"
#include "d_mmcsc.h"

_LIT(KTstLddFileName,"D_MMCSC.LDD");
_LIT(KCamLddFileName,"ECAMERASC.LDD");

#ifdef __WINSCW__
_LIT(KCamPddFileName,"_TEMPLATE_CAMERASC.PDD");
#else
_LIT(KCamPddFileName,"CAMERASC.PDD");
#endif


_LIT(KCamFreePddExtension,".*");

_LIT(KFrameSize, "%dx%d");
_LIT(KSensor, "d:\\Sensor_");
_LIT(KUnderscore, "_");
_LIT(KBmp, ".bmp");
_LIT(KJpeg, ".jpg");
_LIT(KSpeedTaggedJpeg, ".stj");

LOCAL_D TCameraCapsV02* CameraCaps;
LOCAL_D TInt CapsSize;
LOCAL_D TAny* CapsBufPtr;
RTest Test(_L("T_CAMERA_API"));

const TInt KNumVideoFramesToAllocate=6;
const TInt KNumVideoFramesToCapture=(KNumVideoFramesToAllocate-2);
const TInt KHeapSize=0x4000;
const TInt KUnit0=0;

enum TSecThreadTestId
	{
	ESecThreadTestOpen,
	ESecThreadTestDuplicateHandle,
	ESecThreadReuseHandle
	};

struct SSecondaryThreadInfo
	{
	TSecThreadTestId iTestId;
	TInt iExpectedRetVal;
	TThreadId iThreadId;
	TInt iDrvHandle;
	TInt iCameraModuleIndex;
	};

LOCAL_C TInt secondaryThread(TAny* aTestInfo)
	{
	RTest stest(_L("Secondary test camera thread"));
	stest.Title();

	stest.Start(_L("Check which test to perform"));
	SSecondaryThreadInfo sti =*((SSecondaryThreadInfo*)aTestInfo);
	
	TInt r;
	switch(sti.iTestId)
		{
		case ESecThreadTestOpen:
			{
			stest.Next(_L("Open channel test"));
			RDevCameraSc cam;
			r=cam.Open(sti.iCameraModuleIndex);
			stest(r==sti.iExpectedRetVal);
			cam.Close();
			break;
			}
		case ESecThreadTestDuplicateHandle:
			{
			stest.Next(_L("Duplicate channel handle test"));

			// Get a reference to the main thread - which created the handle
			RThread thread;
			r=thread.Open(sti.iThreadId);
			stest(r==KErrNone);

			// Duplicate the driver handle passed from the other thread - for this thread
			RDevCameraSc cam;
			cam.SetHandle(sti.iDrvHandle);
			r=cam.Duplicate(thread);
			stest(r==sti.iExpectedRetVal);
			cam.Close();
			thread.Close();
			break;
			}
		case ESecThreadReuseHandle:
			{
			stest.Next(_L("Re-use channel test"));
			RDevCameraSc* camPtr=(RDevCameraSc*)sti.iDrvHandle;
			TCameraConfigV02Buf camConfBuf;
			camPtr->GetCamConfig(ECamCaptureModeImage, camConfBuf);	// This should cause a panic.
			break;
			}
		default:
			break;
		}

	stest.End();
	return(KErrNone);
	}

/** Test for defect DEF135950. */
LOCAL_C void DoCamDynamicSettingsTests(RDevCameraSc& aCam, RTest& aTest)
	{
	aTest.Next(_L("DYNAMIC SETTINGS TESTS"));
	
	aTest.Next(_L("Get the Caps size. Should be non-zero"));
	TInt capsSize = aCam.CapsSize();
	aTest(capsSize>0);

	aTest.Next(_L("Get the Capabilities (driver owned copy)."));
	TPtrC8 driverCopy = aCam.Caps();
	aTest(driverCopy.Ptr() != NULL);
	aTest(driverCopy.Length()>0);

	aTest.Next(_L("Test failure (buffer too small)."));
	TAny* capsBufPtr = User::Alloc(capsSize-1);
	aTest(capsBufPtr != NULL);
	TPtr8 smallBuf((TUint8*) capsBufPtr, capsSize-1, capsSize-1);
	aTest(KErrArgument==aCam.Caps(smallBuf));
	User::Free(capsBufPtr);

	aTest.Next(_L("Get the Capabilities (client owned copy)."));
	capsBufPtr = User::Alloc(capsSize);
	aTest(capsBufPtr != NULL);
	TPtr8 clientCopy((TUint8*) capsBufPtr, capsSize, capsSize);
	aTest(KErrNone==aCam.Caps(clientCopy));
	aTest(clientCopy.Ptr() != NULL);
	aTest(clientCopy.Length()>0);

	aTest.Next(_L("Obtain the range for Dynamic Settings from both copies (should be the same)."));
	
	TDynamicRange &driverRangeBrightness = ((TCameraCapsV02*)(driverCopy.Ptr()))->iDynamicRange[ECamAttributeBrightness];
	TDynamicRange &clientRangeBrightness = ((TCameraCapsV02*)(clientCopy.Ptr()))->iDynamicRange[ECamAttributeBrightness];
	
	aTest(driverRangeBrightness.iMin == 0);
	aTest(driverRangeBrightness.iMax == 6);
	aTest(driverRangeBrightness.iMin == clientRangeBrightness.iMin);
	aTest(driverRangeBrightness.iMax == clientRangeBrightness.iMax);

	TDynamicRange &driverRangeContrast = ((TCameraCapsV02*)(driverCopy.Ptr()))->iDynamicRange[ECamAttributeContrast];
	TDynamicRange &clientRangeContrast = ((TCameraCapsV02*)(clientCopy.Ptr()))->iDynamicRange[ECamAttributeContrast];

	aTest(driverRangeContrast.iMin == 0);
	aTest(driverRangeContrast.iMax == 6);
	aTest(driverRangeContrast.iMin == clientRangeContrast.iMin);
	aTest(driverRangeContrast.iMax == clientRangeContrast.iMax);
	
	TDynamicRange &driverRangeColorEffect = ((TCameraCapsV02*)(driverCopy.Ptr()))->iDynamicRange[ECamAttributeColorEffect];
	TDynamicRange &clientRangeColorEffect = ((TCameraCapsV02*)(clientCopy.Ptr()))->iDynamicRange[ECamAttributeColorEffect];

	aTest(driverRangeColorEffect.iMin == 0);
	aTest(driverRangeColorEffect.iMax == 7); // TBC::OV3640 set to 7, template driver set to 0x0040 (enum)
	aTest(driverRangeColorEffect.iMin == clientRangeColorEffect.iMin);
	aTest(driverRangeColorEffect.iMax == clientRangeColorEffect.iMax);

	aTest.Next(_L("Test for invalid Min range."));
	aTest(aCam.SetDynamicAttribute(ECamAttributeBrightness, driverRangeBrightness.iMin-1)==KErrArgument);
	aTest(aCam.SetDynamicAttribute(ECamAttributeContrast, driverRangeContrast.iMin-1)==KErrArgument);
	aTest(aCam.SetDynamicAttribute(ECamAttributeColorEffect, driverRangeColorEffect.iMin-1)==KErrArgument);

	aTest.Next(_L("Test for invalid Max range."));
	aTest(aCam.SetDynamicAttribute(ECamAttributeBrightness, driverRangeBrightness.iMax+1)==KErrArgument);
	aTest(aCam.SetDynamicAttribute(ECamAttributeContrast, driverRangeContrast.iMax+1)==KErrArgument);
	aTest(aCam.SetDynamicAttribute(ECamAttributeColorEffect, driverRangeColorEffect.iMax+1)==KErrArgument);

	aTest.Next(_L("Test all valid settings as reported by range - Brightness"));
	for (TUint i=driverRangeBrightness.iMin; i <= driverRangeBrightness.iMax; ++i)
		{
		aTest(aCam.SetDynamicAttribute(ECamAttributeBrightness, i)==KErrNone);
		}

	aTest.Next(_L("Test all valid settings as reported by range - Contrast"));
	for (TUint j=driverRangeContrast.iMin; j <= driverRangeContrast.iMax; ++j)
		{
		aTest(aCam.SetDynamicAttribute(ECamAttributeContrast, j)==KErrNone);
		}

	aTest.Next(_L("Test all valid settings as reported by range - ColorEffect"));
	for (TUint k=driverRangeColorEffect.iMin; k <= driverRangeColorEffect.iMax; ++k)
		{
		aTest(aCam.SetDynamicAttribute(ECamAttributeColorEffect, k)==KErrNone);
		}

	User::Free(capsBufPtr);
	}

/** Test for defect DEF135949. */
LOCAL_C void DoCamBufferOffsetTests(RDevCameraSc& aCam, RTest& aTest, const SDevCamPixelFormat& aPixelFormat, const SDevCamFrameSize& aFrameSize, TUint aFrameRate)
	{
	TInt r;

	aTest.Next(_L("BUFFER ID OFFSET TESTS"));
	aTest.Printf(_L("PixelFormat = %d, FrameSize = %d x %d\n"),aPixelFormat.iPixelFormat,aFrameSize.iWidth,aFrameSize.iHeight);

	// Configure Image Capture
	aTest.Next(_L("Get the camera config of Image mode"));
	TCameraConfigV02Buf camConfBuf;
	aCam.GetCamConfig(ECamCaptureModeImage, camConfBuf);
	TCameraConfigV02 &camConf=camConfBuf();

	camConf.iFrameSize=aFrameSize;
	camConf.iPixelFormat=aPixelFormat;
	camConf.iFrameRate=aFrameRate;

	// Set the Image configuration.
	aTest.Next(_L("Set the Get the camera config of Image mode"));
	r=aCam.SetCamConfig(ECamCaptureModeImage,camConfBuf);
	aTest(r==KErrNone);
	aCam.GetCamConfig(ECamCaptureModeImage, camConfBuf);
	PrintCamConf(camConf,aTest);

	// Create an Image chunk handle.
	aTest.Next(_L("Create the Image chunk"));
	RChunk imgChunkImage;
	TInt numBuffers=KNumVideoFramesToAllocate;
	r=aCam.SetBufConfigChunkCreate(ECamCaptureModeImage,numBuffers,imgChunkImage);
	aTest(r==KErrNone);

	aTest.Next(_L("Read and display the Image buffer config"));
	TMmSharedChunkBufConfig bufferConfig;
	TPckg<TMmSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	aCam.GetBufferConfig(ECamCaptureModeImage, bufferConfigBuf);
	PrintBufferConf(bufferConfig,aTest);

	// Configure Video Capture
	aTest.Next(_L("Get the camera config of Video mode"));
	aCam.GetCamConfig(ECamCaptureModeVideo, camConfBuf);
	camConf=camConfBuf();

	camConf.iFrameSize=aFrameSize;
	camConf.iPixelFormat=aPixelFormat;
	camConf.iFrameRate=aFrameRate;

	// Set the video configuration.
	aTest.Next(_L("Set the Get the camera config of Video mode"));
	r=aCam.SetCamConfig(ECamCaptureModeVideo,camConfBuf);
	aTest(r==KErrNone);
	aCam.GetCamConfig(ECamCaptureModeVideo, camConfBuf);
	PrintCamConf(camConf,aTest);

	// Create an Video chunk handle.
	aTest.Next(_L("Create the Video chunk"));
	RChunk chunkVideo;
	numBuffers=KNumVideoFramesToAllocate;
	r=aCam.SetBufConfigChunkCreate(ECamCaptureModeVideo,numBuffers,chunkVideo);
	aTest(r==KErrNone);

	aTest.Next(_L("Read and display the Video buffer config"));
	aCam.GetBufferConfig(ECamCaptureModeVideo, bufferConfigBuf);
	PrintBufferConf(bufferConfig,aTest);

	// Test that stop still returns an error.
	r=aCam.Stop();
	aTest(r==KErrGeneral);

	// Set the current capture mode to image
	aTest.Next(_L("Set the camera in Image mode and capture a buffer, then requesting the buffer offset."));
	r=aCam.SetCaptureMode(ECamCaptureModeImage);
	aTest(r==KErrNone);

	// Start the camera
	r=aCam.Start();
	aTest(r==KErrNone);

	aTest.Next(_L("Issue a capture request"));
	TRequestStatus rs1;
	aCam.NotifyNewImage(rs1);
	User::WaitForRequest(rs1);
	TInt retId=rs1.Int();
	TInt retOffset=-1;
	aTest(retId>=0);
	aCam.BufferIdToOffset(ECamCaptureModeImage,retId,retOffset);
	aTest.Printf(_L("Buffer offset: %d(%xH)\r\n"),retId,retOffset);
	aTest(retOffset>=0);

	// Change capture mode
	aTest.Next(_L("Set the capture mode to Video."));
	r=aCam.SetCaptureMode(ECamCaptureModeVideo);
	aTest(r==KErrNone);

	aTest.Next(_L("Request again the offset for the buffer in Image mode."));
	aCam.BufferIdToOffset(ECamCaptureModeImage,retId,retOffset);
	aTest.Printf(_L("Buffer offset: %d(%xH)\r\n"),retId,retOffset);
	aTest(retOffset>=0);

	// Stop the camera.
	r=aCam.Stop();
	aTest(r==KErrNone);

	aTest.Next(_L("Close the chunk"));
	r=aCam.ChunkClose(ECamCaptureModeImage);
	aTest(r==KErrNone);
	r=aCam.ChunkClose(ECamCaptureModeVideo);
	aTest(r==KErrNone);
	}


LOCAL_C void DoCamOpenCapTests(RTest& aTest,TInt aCameraSensorIndex)
	{
	TInt r;

	// Open the camera driver to obtain the basic capabilities for use throughout the tests and
	// then close it again so that it may be confirmed that opening multiple times is ok
	aTest.Next(_L("CHANNEL OPEN AND CAPABILITIES TESTS"));
	RDevCameraSc cam;
	aTest.Next(_L("Open a channel on the camera driver"));
	r=cam.Open(aCameraSensorIndex);
	aTest(r==KErrNone);

	// Make sure that the driver can handle attempts to start it before it has been configured
	aTest.Next(_L("Try to start/stop camera before its configured"));
	r=cam.Start();
	aTest(r==KErrNotReady);
	r=cam.Stop();
	aTest(r==KErrGeneral);

	aTest.Next(_L("Read the capabilities structure size of this device"));
	CapsSize=cam.CapsSize();
	aTest.Next(_L("Read and display the capabilities of this device"));
	CapsBufPtr = User::Alloc(CapsSize);
	TPtr8 capsPtr( (TUint8*)CapsBufPtr, CapsSize, CapsSize );
	r=cam.Caps(capsPtr);
	aTest(r==KErrNone);
	CameraCaps = (TCameraCapsV02*) capsPtr.Ptr();
	PrintCamModes(CameraCaps,aTest);

	TAny* frameSizeCapsBuf;
	SDevCamPixelFormat* pixelFormat;
	TBuf<80> buf;
	SDevCamFrameSize* frameSize;
	TPtr8 frameSizeCapsPtr(0,0,0);
	TUint fsCount, pfCount, theCount = 0;

	/* IMAGE */
	/* Use pixel formats from 0 to CapsBuf->iNumImagePixelFormats */
	buf.Zero();
	buf.Append(KCaptureModeImage);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	pixelFormat = (SDevCamPixelFormat*) (CameraCaps + 1);
	for (pfCount = theCount; pfCount < CameraCaps->iNumImagePixelFormats; pfCount++)
		{
		buf.Zero();
		AppendPixelFormat(buf, pixelFormat->iPixelFormat);
		aTest.Printf(buf);
		frameSizeCapsBuf = User::Alloc(pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		new (&frameSizeCapsPtr) TPtr8((TUint8*)frameSizeCapsBuf, pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize), pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		r=cam.FrameSizeCaps(ECamCaptureModeImage, pixelFormat->iPixelFormat, frameSizeCapsPtr);
		aTest(r==KErrNone);
		frameSize = (SDevCamFrameSize*) frameSizeCapsPtr.Ptr();
		for (fsCount = 0; fsCount < pixelFormat->iNumFrameSizes; fsCount++)
			{
			aTest.Printf(_L("%dx%d "),frameSize->iWidth,frameSize->iHeight);
			frameSize++;
			}
		aTest.Printf(_L("\n"));
		User::Free(frameSizeCapsBuf);
		pixelFormat++;
		}

	/* VIDEO */
	buf.Zero();
	buf.Append(KCaptureModeVideo);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	pixelFormat = (SDevCamPixelFormat*) (CameraCaps + 1);
	pixelFormat += CameraCaps->iNumImagePixelFormats;
	theCount = CameraCaps->iNumImagePixelFormats + CameraCaps->iNumVideoPixelFormats;
	for (pfCount = CameraCaps->iNumImagePixelFormats; pfCount < theCount; pfCount++)
		{
		buf.Zero();
		AppendPixelFormat(buf, pixelFormat->iPixelFormat);
		aTest.Printf(buf);
		frameSizeCapsBuf = User::Alloc(pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		new (&frameSizeCapsPtr) TPtr8((TUint8*)frameSizeCapsBuf, pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize), pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		r=cam.FrameSizeCaps(ECamCaptureModeVideo, pixelFormat->iPixelFormat, frameSizeCapsPtr);
		aTest(r==KErrNone);
		frameSize = (SDevCamFrameSize*) frameSizeCapsPtr.Ptr();
		for (fsCount = 0; fsCount < pixelFormat->iNumFrameSizes; fsCount++)
			{
			aTest.Printf(_L("%dx%d "),frameSize->iWidth,frameSize->iHeight);
			frameSize++;
			}
		aTest.Printf(_L("\n"));
		User::Free(frameSizeCapsBuf);
		pixelFormat++;
		}

	/* VIEW FINDER */
	buf.Zero();
	buf.Append(KCaptureModeViewFinder);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	pixelFormat = (SDevCamPixelFormat*) (CameraCaps + 1);
	pixelFormat += (CameraCaps->iNumImagePixelFormats + CameraCaps->iNumVideoPixelFormats);
	theCount = CameraCaps->iNumImagePixelFormats + CameraCaps->iNumVideoPixelFormats + CameraCaps->iNumViewFinderPixelFormats;
	for (pfCount = CameraCaps->iNumImagePixelFormats + CameraCaps->iNumVideoPixelFormats; pfCount < theCount; pfCount++)
		{
		buf.Zero();
		AppendPixelFormat(buf, pixelFormat->iPixelFormat);
		aTest.Printf(buf);
		frameSizeCapsBuf = User::Alloc(pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		new (&frameSizeCapsPtr) TPtr8((TUint8*)frameSizeCapsBuf, pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize), pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		r=cam.FrameSizeCaps(ECamCaptureModeViewFinder, pixelFormat->iPixelFormat, frameSizeCapsPtr);
		aTest(r==KErrNone);
		frameSize = (SDevCamFrameSize*) frameSizeCapsPtr.Ptr();
		for (fsCount = 0; fsCount < pixelFormat->iNumFrameSizes; fsCount++)
			{
			aTest.Printf(_L("%dx%d "),frameSize->iWidth,frameSize->iHeight);
			frameSize++;
			}
		aTest.Printf(_L("\n"));
		User::Free(frameSizeCapsBuf);
		pixelFormat++;
		}

	aTest.Next(_L("Try opening the same unit a second time."));
	RDevCameraSc cam2;
	r=cam2.Open(aCameraSensorIndex);
	aTest(r==KErrInUse);

	aTest.Next(_L("Open a channel from the 2nd thread without closing the 1st"));
	RThread thread;
	TRequestStatus stat;
	SSecondaryThreadInfo sti;

	// Setup the 2nd thread to open a channel on the same unit
	sti.iTestId=ESecThreadTestOpen;
	sti.iExpectedRetVal=KErrInUse;
	sti.iCameraModuleIndex=aCameraSensorIndex;
	r=thread.Create(_L("Thread"),secondaryThread,KDefaultStackSize,KHeapSize,KHeapSize,&sti);
	Test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	Test(stat.Int()==KErrNone);
	Test(thread.ExitType()==EExitKill);
	thread.Close();
	User::After(10000);	// Wait 10ms

	aTest.Next(_L("Open a channel from the 2nd thread having closed the 1st"));
	cam.Close();		// Close the 1st channel
	sti.iTestId=ESecThreadTestOpen;
	sti.iExpectedRetVal=KErrNone;
	r=thread.Create(_L("Thread02"),secondaryThread,KDefaultStackSize,KHeapSize,KHeapSize,&sti);
	Test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	Test(stat.Int()==KErrNone);
	Test(thread.ExitType()==EExitKill);
	thread.Close();
	User::After(10000);	// Wait 10ms

	aTest.Next(_L("Re-open channel and duplicate it from 2nd thread"));
	r=cam.Open(aCameraSensorIndex);		// Re-open the channel
	aTest(r==KErrNone);
	sti.iTestId=ESecThreadTestDuplicateHandle;
	sti.iExpectedRetVal=KErrAccessDenied;
	sti.iThreadId=RThread().Id();	// Get the ID of this thread
	sti.iDrvHandle=cam.Handle();	// Pass the channel handle

	r=thread.Create(_L("Thread03"),secondaryThread,KDefaultStackSize,KHeapSize,KHeapSize,&sti); // Create secondary thread
	Test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	Test(stat.Int()==KErrNone);
	Test(thread.ExitType()==EExitKill);
	thread.Close();
	User::After(10000);	// Wait 10ms

	aTest.Next(_L("Re-use the same channel from 2nd thread"));
	sti.iTestId=ESecThreadReuseHandle;
	sti.iDrvHandle=(TInt)&cam;	// Pass a pointer to the channel
	r=thread.Create(_L("Thread04"),secondaryThread,KDefaultStackSize,KHeapSize,KHeapSize,&sti); // Create secondary thread
	Test(r==KErrNone);
	thread.Logon(stat);
	thread.Resume();
	User::WaitForRequest(stat);
	TExitCategoryName exitCategoryName = thread.ExitCategory();
	Test.Printf(_L("Thread exit info: Cat:%S, Reason:%x, Type:%d\r\n"),&exitCategoryName,thread.ExitReason(),thread.ExitType());
	Test(thread.ExitType()==EExitPanic);	// Secondary thread is expected to panic
	Test(stat.Int()==KErrNone);
	thread.Close();
	User::After(10000);	// Wait 10ms

	cam.Close();
	}

LOCAL_C void DoCamConfigTests(RDevCameraSc& aCam, RTest& aTest, TDevCamCaptureMode aCaptureMode, const SDevCamPixelFormat& aPixelFormat, const SDevCamFrameSize& aFrameSize)
	{
	TInt r;

	aTest.Next(_L("GETTING & SETTING CONFIG TESTS"));
	aTest.Printf(_L("CaptureMode = %d, PixelFormat = %x, FrameSize = %d x %d\n"),aCaptureMode,aPixelFormat.iPixelFormat,aFrameSize.iWidth,aFrameSize.iHeight);

	aTest.Next(_L("Read and display the default camera config for requested capture mode"));
	TCameraConfigV02Buf camConfBuf;
	aCam.GetCamConfig(aCaptureMode, camConfBuf);
	TCameraConfigV02 &camConf=camConfBuf();
	PrintCamConf(camConf,aTest);

	aTest.Next(_L("Read and display the default buffer config for requested capture mode"));
	TMmSharedChunkBufConfig bufferConfig;
	TPckg<TMmSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	aCam.GetBufferConfig(aCaptureMode, bufferConfigBuf);
	PrintBufferConf(bufferConfig,aTest);

	aTest.Next(_L("Setup the config - creating a chunk"));
	camConf.iFrameSize=aFrameSize;
	camConf.iPixelFormat=aPixelFormat;
	camConf.iFrameRate=aFrameSize.iMaxFrameRate;
	r=aCam.SetCamConfig(aCaptureMode, camConfBuf);
	aTest(r==KErrNone);
	RChunk chunk;
	r=aCam.SetBufConfigChunkCreate(aCaptureMode, KNumVideoFramesToAllocate, chunk);
	aTest(r==KErrNone);
	aCam.GetCamConfig(aCaptureMode, camConfBuf);
	PrintCamConf(camConf,aTest);

	aTest.Next(_L("Read and display the resulting buffer config"));
	aCam.GetBufferConfig(aCaptureMode, bufferConfigBuf);
	PrintBufferConf(bufferConfig,aTest);

	aTest.Next(_L("Close the chunk created"));
	chunk.Close();

	aTest.Next(_L("Open a channel on the test driver"));
	RMmCreateSc tstDrv;
	r=tstDrv.Open();		// Opening the channel results in the creation of a shared chunk.
	aTest(r==KErrNone);

	aTest.Next(_L("Get a handle on its shared chunk"));
	r=tstDrv.GetChunkHandle(chunk);	// Get a handle on the shared chunk created by the test driver
	aTest(r==KErrNone);

	aTest.Next(_L("Read and display the buffer config"));
	TMmSharedChunkBufConfig bufferConfigTest;
	TPckg<TMmSharedChunkBufConfig> bufferConfigBufTest(bufferConfigTest);

	// Get info on the buffers within the shared chunk
	aTest.Next(_L("Get info. on the shared chunk"));
	r=tstDrv.GetBufInfo(bufferConfigBufTest);
	aTest(r==KErrNone);

	PrintBufferConf(bufferConfigTest,aTest);
	aTest.Next(_L("Setup the config - supplying a chunk"));
	r=aCam.SetBufConfigChunkOpen(aCaptureMode, bufferConfigBufTest, chunk);
	aTest(r==KErrNone);
	aCam.GetCamConfig(aCaptureMode, camConfBuf);
	PrintCamConf(camConf,aTest);
	aCam.GetBufferConfig(aCaptureMode, bufferConfigBufTest);
	PrintBufferConf(bufferConfigTest,aTest);

	aTest.Next(_L("Close the chunk driver and the 2nd chunk"));
	tstDrv.Close();
	chunk.Close();
	}

LOCAL_C void DoCamVideoCaptureTests(RDevCameraSc& aCam, RTest& aTest, TDevCamCaptureMode aCaptureMode, const SDevCamPixelFormat& aPixelFormat, const SDevCamFrameSize& aFrameSize, TUint aFrameRate)
	{
	TInt r;

	aTest.Next(_L("VIDEO CAPTURE TESTS"));
	aTest.Printf(_L("CaptureMode = %d, PixelFormat = %x, FrameSize = %d x %d\n"),aCaptureMode,aPixelFormat.iPixelFormat,aFrameSize.iWidth,aFrameSize.iHeight);

	// Configure Video or Viewfinder Capture
	TCameraConfigV02Buf camConfBuf;
	aCam.GetCamConfig(aCaptureMode, camConfBuf);
	TCameraConfigV02 &camConf=camConfBuf();

	camConf.iFrameSize=aFrameSize;
	camConf.iPixelFormat=aPixelFormat;
	camConf.iFrameRate=aFrameRate;

	// Set the camera configuration.
	r=aCam.SetCamConfig(aCaptureMode,camConfBuf);
	aTest(r==KErrNone);
	aCam.GetCamConfig(aCaptureMode, camConfBuf);
	PrintCamConf(camConf,aTest);

	// Create a chunk handle and trigger the buffer creation.
	aTest.Next(_L("Setup the config - creating a chunk"));
	RChunk chunkVideo;
	TInt numBuffers=KNumVideoFramesToAllocate;
	r=aCam.SetBufConfigChunkCreate(aCaptureMode,numBuffers,chunkVideo);
	aTest(r==KErrNone);

	aTest.Next(_L("Read and display the resulting buffer config"));
	TMmSharedChunkBufConfig bufferConfig;
	TPckg<TMmSharedChunkBufConfig> bufferConfigBufVideo(bufferConfig);
	aCam.GetBufferConfig(aCaptureMode, bufferConfigBufVideo);
	PrintBufferConf(bufferConfig,aTest);

	// Request and print the camera and buffer configurations for all three capture modes
	aTest.Next(_L("Read and display the camera configs"));
	aCam.GetCamConfig(ECamCaptureModeVideo, camConfBuf);
	PrintCamConf(camConf,aTest);
	aCam.GetCamConfig(ECamCaptureModeImage, camConfBuf);
	PrintCamConf(camConf,aTest);
	aCam.GetCamConfig(ECamCaptureModeViewFinder, camConfBuf);
	PrintCamConf(camConf,aTest);

	// Create and configure a display handler
	TCamDisplayHandler dispHand;
	r=dispHand.Init();
	aTest(r==KErrNone);

	if (aPixelFormat.iPixelFormat!=EUidPixelFormatJPEG && aPixelFormat.iPixelFormat!=EUidPixelFormatSpeedTaggedJPEG)
		{
		r=dispHand.SetConfig(aFrameSize,aPixelFormat);
		aTest(r==KErrNone);
		}

	// Test that stop still returns an error.
	r=aCam.Stop();
	aTest(r==KErrGeneral);

	// Set the current capture mode
	r=aCam.SetCaptureMode(aCaptureMode);
	aTest(r==KErrNone);

	aTest.Next(_L("Start the camera in video mode"));
	r=aCam.Start();
	aTest(r==KErrNone);

	aTest.Next(_L("Issue capture requests"));
	aTest.Printf(_L("Request %d frames\r\n"),KNumVideoFramesToCapture);
	// Issue new image requests immediately. We'll have to wait while the
	// camera captures the images.
	TRequestStatus rs[KNumVideoFramesToCapture];
	TInt i;
	for (i=0 ; i<KNumVideoFramesToCapture; i++)
		{
		aTest.Printf(_L("Requesting new image... %d\n"), i);
		aCam.NotifyNewImage(rs[i]);
		aTest.Printf(_L("Requested new image... %d\n"), i);
		}

	TInt retId=-1;
	TInt retOffset=-1;
	for (i=0 ; i<KNumVideoFramesToCapture; i++)
		{
		aTest.Printf(_L("Waiting for... %d\n"), i);
		User::WaitForRequest(rs[i]);
		retId=rs[i].Int();
		aTest(retId>=0);
		aTest.Printf(_L("Getting buffer offset for... %d\n"), i);
		aCam.BufferIdToOffset(aCaptureMode,retId,retOffset);
		aTest.Printf(_L("Buffer%d(id:%d) offset: %d(%xH)\r\n"),i,retId,retOffset,retOffset);
		aTest(retOffset>=0);
		}

	TUint8* imgBase;

	// Display each image received for 333ms
	for (i=0 ; i<KNumVideoFramesToCapture; i++)
		{
		if (aPixelFormat.iPixelFormat!=EUidPixelFormatJPEG && aPixelFormat.iPixelFormat!=EUidPixelFormatSpeedTaggedJPEG)
			{
			retId=rs[i].Int();
			aTest(retId>=0);
			aCam.BufferIdToOffset(aCaptureMode,retId,retOffset);
			aTest(retOffset>=0);
			imgBase=chunkVideo.Base()+retOffset;
			r=dispHand.Process(imgBase);
			
#ifdef __WINSCW__
			aTest(r==KErrNotSupported);
#else
			aTest(r==KErrNone);
#endif

			User::After(333000);	// 0.33sec
			}
		}

	aTest.Next(_L("Free the buffers"));
	for (i=0 ; i<KNumVideoFramesToCapture; i++)
		{
		aTest.Printf(_L("Releasing %d(i:%d) "),rs[i].Int(), i);
		r=aCam.ReleaseBuffer(rs[i].Int());
		aTest.Printf(_L("buffer(%d), r=%d\n"),rs[i].Int(),r);
		aTest(r==KErrNone);
		}

	aTest.Next(_L("Issue more capture requests"));
	// Wait a bit more so camera has images ready as soon as we issue the requests
	User::After(500000);	// 0.5sec

	aTest.Printf(_L("Request %d frames\r\n"),KNumVideoFramesToCapture);
	for (i=0 ; i<KNumVideoFramesToCapture; i++)
		aCam.NotifyNewImage(rs[i]);

	for (i=0 ; i<KNumVideoFramesToCapture; i++)
		{
		User::WaitForRequest(rs[i]);
		retId=rs[i].Int();
		aTest(retId>=0);
		aCam.BufferIdToOffset(aCaptureMode,retId,retOffset);
		aTest.Printf(_L("Buffer%d(id:%d) offset: %d(%xH)\r\n"),i,retId,retOffset,retOffset);
		aTest(retOffset>=0);
		}

	aTest.Next(_L("Stop the camera."));
	r=aCam.Stop();
	aTest(r==KErrNone);

	aTest.Next(_L("Start it again."));
	r=aCam.Start();
	aTest(r==KErrNone);

	aTest.Next(_L("Continuously display for 10 secs"));
	RTimer tim;
	tim.CreateLocal();
	TRequestStatus timStatus;
	const TUint KTimeOut=10000000;	// 10 seconds
	tim.After(timStatus,KTimeOut);
	aTest(timStatus==KRequestPending);
	aCam.NotifyNewImage(rs[0]);
	aCam.NotifyNewImage(rs[1]);
	FOREVER
		{
		User::WaitForAnyRequest();
		if (timStatus!=KRequestPending)
			{
			aCam.NotifyNewImageCancel();
			User::WaitForRequest(rs[0]);
			User::WaitForRequest(rs[1]);
			break;
			}
		else if (rs[0]!=KRequestPending)
			{
			retId=rs[0].Int();
			aTest(retId>=0);
			aCam.BufferIdToOffset(aCaptureMode,retId,retOffset);
			aTest(retOffset>=0);
			imgBase=chunkVideo.Base()+retOffset;
			if (aPixelFormat.iPixelFormat!=EUidPixelFormatJPEG && aPixelFormat.iPixelFormat!=EUidPixelFormatSpeedTaggedJPEG)
				{
				r=dispHand.Process(imgBase);
				
#ifdef __WINSCW__
				aTest(r==KErrNotSupported);
#else
				aTest(r==KErrNone);
#endif
				
				}
			r=aCam.ReleaseBuffer(retId);
			aTest(r==KErrNone);
			aCam.NotifyNewImage(rs[0]);
			}
		else if (rs[1]!=KRequestPending)
			{
			retId=rs[1].Int();
			aTest(retId>=0);
			aCam.BufferIdToOffset(aCaptureMode,retId,retOffset);
			aTest(retOffset>=0);
			imgBase=chunkVideo.Base()+retOffset;
			if (aPixelFormat.iPixelFormat!=EUidPixelFormatJPEG && aPixelFormat.iPixelFormat!=EUidPixelFormatSpeedTaggedJPEG)
				{
				r=dispHand.Process(imgBase);
				
#ifdef __WINSCW__
				aTest(r==KErrNotSupported);
#else
				aTest(r==KErrNone);
#endif
				
				}
			r=aCam.ReleaseBuffer(retId);
			aTest(r==KErrNone);
			aCam.NotifyNewImage(rs[1]);
			}
		else
			aTest(0);
		}

	tim.Close();

	aTest.Next(_L("Stop the camera."));
	r=aCam.Stop();
	aTest(r==KErrNone);

	aTest.Next(_L("Close the chunk"));
	r=aCam.ChunkClose(aCaptureMode);
	}

LOCAL_C void DoCamImageCaptureTests(RDevCameraSc& aCam, RTest& aTest, TInt aCameraSensorIndex, const SDevCamPixelFormat& aPixelFormat, const SDevCamFrameSize& aFrameSize, TUint aFrameRate)
	{
	TInt r;

	aTest.Next(_L("IMAGE CAPTURE TESTS"));
	aTest.Printf(_L("PixelFormat = %d, FrameSize = %d x %d\n"),aPixelFormat.iPixelFormat,aFrameSize.iWidth,aFrameSize.iHeight);

	// Configure Image Capture
	TCameraConfigV02Buf camConfBuf;
	aCam.GetCamConfig(ECamCaptureModeImage, camConfBuf);
	TCameraConfigV02 &camConf=camConfBuf();

	camConf.iFrameSize=aFrameSize;
	camConf.iPixelFormat=aPixelFormat;
	camConf.iFrameRate=aFrameRate;

	// Set the camera configuration.
	r=aCam.SetCamConfig(ECamCaptureModeImage,camConfBuf);
	aTest(r==KErrNone);
	aCam.GetCamConfig(ECamCaptureModeImage, camConfBuf);
	PrintCamConf(camConf,aTest);

	// Create a chunk handle and trigger the buffer creation.
	aTest.Next(_L("Setup the config - creating a chunk"));
	RChunk chunkImage;
	TInt numBuffers=KNumVideoFramesToAllocate;
	r=aCam.SetBufConfigChunkCreate(ECamCaptureModeImage,numBuffers,chunkImage);
	aTest(r==KErrNone);

	aTest.Next(_L("Read and display the resulting buffer config"));
	TMmSharedChunkBufConfig bufferConfig;
	TPckg<TMmSharedChunkBufConfig> bufferConfigBufImage(bufferConfig);
	aCam.GetBufferConfig(ECamCaptureModeImage, bufferConfigBufImage);
	PrintBufferConf(bufferConfig,aTest);

	// Create and configure a display handler
	TCamDisplayHandler dispHand;
	r=dispHand.Init();
	aTest(r==KErrNone);

	if (aPixelFormat.iPixelFormat!=EUidPixelFormatJPEG && aPixelFormat.iPixelFormat!=EUidPixelFormatSpeedTaggedJPEG)
		{
		r=dispHand.SetConfig(aFrameSize,aPixelFormat);
		aTest(r==KErrNone);
		}

	// Test that stop still returns an error.
	r=aCam.Stop();
	aTest(r==KErrGeneral);

	// Set the current capture mode
	r=aCam.SetCaptureMode(ECamCaptureModeImage);
	aTest(r==KErrNone);

	aTest.Next(_L("Start the camera in image capture mode"));
	r=aCam.Start();
	aTest(r==KErrNone);

	aTest.Next(_L("Issue a capture request"));
	TRequestStatus rs1;
	aCam.NotifyNewImage(rs1);
	User::WaitForRequest(rs1);
	TInt retId=rs1.Int();
	TInt retOffset=-1;
	aTest(retId>=0);
	aCam.BufferIdToOffset(ECamCaptureModeImage,retId,retOffset);
	aTest.Printf(_L("Buffer offset: %d(%xH)\r\n"),retId,retOffset);
	aTest(retOffset>=0);

	TUint8* imgBase;
	imgBase=chunkImage.Base()+retOffset;

	if (aPixelFormat.iPixelFormat!=EUidPixelFormatJPEG && aPixelFormat.iPixelFormat!=EUidPixelFormatSpeedTaggedJPEG)
		{
		// Display the image received for 1s
		r=dispHand.Process(imgBase);
		
#ifdef __WINSCW__
        aTest(r==KErrNotSupported);
#else
        aTest(r==KErrNone);
#endif
        
		User::After(1000000);	// 1 sec
		}

	// Save the to do MMC card with a filename to indicate its size.  If no MMC card is present
	// then we will just display a warning rather than fail as this is an optional manual step
	TBuf<100> fileName(KSensor);
	fileName.AppendNum(aCameraSensorIndex);
	fileName.Append(KUnderscore);
	fileName.AppendFormat(KFrameSize, aFrameSize.iWidth, aFrameSize.iHeight);
	fileName.Append(KUnderscore);
	AppendPixelFormat(fileName, aPixelFormat.iPixelFormat);

	TBool wrote = ETrue;
	RBitmap bitmap;

	if ((aPixelFormat.iPixelFormat == EUidPixelFormatJPEG) || (aPixelFormat.iPixelFormat == EUidPixelFormatSpeedTaggedJPEG))
		{
		fileName.Append((aPixelFormat.iPixelFormat == EUidPixelFormatJPEG) ? KJpeg : KSpeedTaggedJpeg);
		r=bitmap.WriteBuffer(fileName, imgBase, (aFrameSize.iWidth * aFrameSize.iHeight * aPixelFormat.iPixelWidthInBytes));
		}
	else if ((aPixelFormat.iPixelFormat == EUidPixelFormatYUV_422Interleaved) || (aPixelFormat.iPixelFormat == EUidPixelFormatRGB_565))
		{
		fileName.Append(KBmp);
		r=bitmap.WriteBMP(fileName, imgBase, aPixelFormat, aFrameSize.iWidth, aFrameSize.iHeight);
		}
	else
		{
		wrote = EFalse;
		}

	if (wrote)
		{
		if (r==KErrNone)
			{
			aTest.Printf(_L("Wrote image to %S\n"),&fileName);
			}
		else
			{
			aTest.Printf(_L("Warning: Unable to write %S (error = %d)\r\n"),&fileName,r);
			}
		}

	aTest.Next(_L("Free the buffer"));
	r=aCam.ReleaseBuffer(retId);
	aTest(r==KErrNone);

	aTest.Next(_L("Issue two consecutive capture requests"));
	TRequestStatus rs2;
	aCam.NotifyNewImage(rs1);
	aCam.NotifyNewImage(rs2);

	User::WaitForRequest(rs1);
	retId=rs1.Int();
	aTest(retId>=0);
	aCam.BufferIdToOffset(ECamCaptureModeImage,retId,retOffset);
	aTest.Printf(_L("Buffer0 offset: %d(%xH)\r\n"),retOffset,retOffset);
	aTest(retOffset>=0);

	if (aPixelFormat.iPixelFormat!=EUidPixelFormatJPEG && aPixelFormat.iPixelFormat!=EUidPixelFormatSpeedTaggedJPEG)
		{
		// Display the image received for 1s
		imgBase=chunkImage.Base()+retOffset;
		r=dispHand.Process(imgBase);
		
#ifdef __WINSCW__
        aTest(r==KErrNotSupported);
#else
        aTest(r==KErrNone);
#endif
        
		User::After(1000000);	// 1 sec
		}

	r=aCam.ReleaseBuffer(retId);
	aTest(r==KErrNone);

	User::WaitForRequest(rs2);
	retId=rs2.Int();
	aTest(retId>=0);
	aCam.BufferIdToOffset(ECamCaptureModeImage,retId,retOffset);
	aTest.Printf(_L("Buffer1 offset: %d(%xH)\r\n"),retOffset,retOffset);
	aTest(retOffset>=0);

	if (aPixelFormat.iPixelFormat!=EUidPixelFormatJPEG && aPixelFormat.iPixelFormat!=EUidPixelFormatSpeedTaggedJPEG)
		{
		// Display the image received for 1s
		imgBase=chunkImage.Base()+retOffset;
		r=dispHand.Process(imgBase);
		
#ifdef __WINSCW__
        aTest(r==KErrNotSupported);
#else
        aTest(r==KErrNone);
#endif
        
		User::After(1000000);	// 1 sec
		}

	r=aCam.ReleaseBuffer(retId);
	aTest(r==KErrNone);

	aTest.Next(_L("Issue four more separate capture requests"));
	for (TInt i=0 ; i<4 ; i++)
		{
		aCam.NotifyNewImage(rs1);
		User::WaitForRequest(rs1);
		retId=rs1.Int();
		aTest(retId>=0);
		aCam.BufferIdToOffset(ECamCaptureModeImage,retId,retOffset);
		aTest.Printf(_L("Buffer%d offset: %d(%xH)\r\n"),i,retOffset,retOffset);
		aTest(retOffset>=0);

		if (aPixelFormat.iPixelFormat!=EUidPixelFormatJPEG && aPixelFormat.iPixelFormat!=EUidPixelFormatSpeedTaggedJPEG)
			{
			// Display the image received for 1s
			imgBase=chunkImage.Base()+retOffset;
			r=dispHand.Process(imgBase);
			
#ifdef __WINSCW__
            aTest(r==KErrNotSupported);
#else
            aTest(r==KErrNone);
#endif
            
			User::After(1000000);	// 1 sec
			}
		}

	r=aCam.ReleaseBuffer(retId);
	aTest(r==KErrNone);

	aTest.Next(_L("Stop the camera."));
	r=aCam.Stop();
	aTest(r==KErrNone);

	aTest.Next(_L("Close the chunk"));
	r=aCam.ChunkClose(ECamCaptureModeImage);
	aTest(r==KErrNone);
	}

LOCAL_C void DoCamCancelTests(RTest& aTest, TInt aCameraSensorIndex)
	{
	TInt bufferSize, r;
	SDevCamFrameSize* frameSizes;
	TCameraConfigV02Buf camConfBuf;

	aTest.Next(_L("CAPTURE CANCEL TESTS"));

	RDevCameraSc cam;
	aTest.Next(_L("Open a channel on the camera driver"));
	r=cam.Open(aCameraSensorIndex);
	aTest(r==KErrNone);

	TInt numBuffers=KNumVideoFramesToAllocate;
	SDevCamPixelFormat* pixelFormat = (SDevCamPixelFormat*) (CameraCaps + 1);

	// Test Image Capture, if supported
	if (CameraCaps->iNumImagePixelFormats)
		{
		// If iNumImagePixelFormats is > 0 then the matching iNumFrameSizes should also be > 0
		Test(pixelFormat->iNumFrameSizes > 0);
		frameSizes = new SDevCamFrameSize[pixelFormat->iNumFrameSizes];
		Test(frameSizes != NULL);
		bufferSize = (sizeof(SDevCamFrameSize) * pixelFormat->iNumFrameSizes);
		TPtr8 frameSizesBuf((TUint8*) frameSizes, bufferSize, bufferSize);
		r = cam.FrameSizeCaps(ECamCaptureModeImage, pixelFormat->iPixelFormat, frameSizesBuf);
		Test(r == KErrNone);

		cam.GetCamConfig(ECamCaptureModeImage, camConfBuf);

		camConfBuf().iFrameSize=frameSizes[0];
		camConfBuf().iPixelFormat=*pixelFormat;
		camConfBuf().iFrameRate=frameSizes[0].iMaxFrameRate;

		// Set the camera configuration.
		r=cam.SetCamConfig(ECamCaptureModeImage, camConfBuf);
		aTest(r==KErrNone);
		cam.GetCamConfig(ECamCaptureModeImage, camConfBuf);
		PrintCamConf(camConfBuf(), aTest);

		// Create a chunk handle and trigger the buffer creation.
		RChunk chunkImage;
		aTest.Next(_L("Setup the config - creating a chunk for image capture"));
		r=cam.SetBufConfigChunkCreate(ECamCaptureModeImage,numBuffers,chunkImage);
		aTest(r==KErrNone);

		aTest.Next(_L("Read and display the resulting buffer config"));
		TMmSharedChunkBufConfig bufferConfig;
		TPckg<TMmSharedChunkBufConfig> bufferConfigBufImage(bufferConfig);
		cam.GetBufferConfig(ECamCaptureModeImage, bufferConfigBufImage);
		PrintBufferConf(bufferConfig,aTest);

		// Set the current capture mode
		r=cam.SetCaptureMode(ECamCaptureModeImage);
		aTest(r==KErrNone);

		aTest.Next(_L("Start the camera in image capture mode."));
		r=cam.Start();
		aTest(r==KErrNone);

		TRequestStatus rs[KNumVideoFramesToCapture];
		TInt retId, retOffset;

		aTest.Next(_L("Issue a request and then cancel it"));
		cam.NotifyNewImage(rs[0]);
		cam.NotifyNewImageCancel();
		User::WaitForRequest(rs[0]);
		retId=rs[0].Int();
		aTest(retId==KErrCancel || retId>=0);
		if (retId>=0)
			{
			cam.BufferIdToOffset(ECamCaptureModeImage,retId,retOffset);
			aTest.Printf(_L("Buffer%d : %d\r\n"),retId,retOffset);
			aTest(retOffset>=0);
			cam.ReleaseBuffer(retId);
			}

		aTest.Next(_L("Stop the camera."));
		r=cam.Stop();
		aTest(r==KErrNone);

		aTest.Next(_L("Close the Image chunk"));
		chunkImage.Close();

		delete [] frameSizes;
		}

	// Skip past the image pixel formats
	pixelFormat += CameraCaps->iNumImagePixelFormats;

	// Test Video Capture, if supported
	if (CameraCaps->iNumVideoPixelFormats)
		{
		// If iNumVideoPixelFormats is > 0 then the matching iNumFrameSizes should also be > 0
		Test(pixelFormat->iNumFrameSizes > 0);
		frameSizes = new SDevCamFrameSize[pixelFormat->iNumFrameSizes];
		Test(frameSizes != NULL);
		bufferSize = (sizeof(SDevCamFrameSize) * pixelFormat->iNumFrameSizes);
		TPtr8 frameSizesBuf((TUint8*) frameSizes, bufferSize, bufferSize);
		r = cam.FrameSizeCaps(ECamCaptureModeImage, pixelFormat->iPixelFormat, frameSizesBuf);
		Test(r == KErrNone);

		cam.GetCamConfig(ECamCaptureModeVideo, camConfBuf);

		camConfBuf().iFrameSize=frameSizes[0];
		camConfBuf().iPixelFormat=*pixelFormat;
		camConfBuf().iFrameRate=frameSizes[0].iMaxFrameRate;

		// Set the camera configuration.
		r=cam.SetCamConfig(ECamCaptureModeVideo, camConfBuf);
		aTest(r==KErrNone);
		cam.GetCamConfig(ECamCaptureModeVideo, camConfBuf);
		PrintCamConf(camConfBuf(), aTest);

		// Create a chunk handle and trigger the buffer creation.
		RChunk chunkVideo;
		aTest.Next(_L("Setup the config - creating a chunk for video capture"));
		r=cam.SetBufConfigChunkCreate(ECamCaptureModeVideo,numBuffers,chunkVideo);
		aTest(r==KErrNone);

		aTest.Next(_L("Read and display the resulting buffer config"));
		TMmSharedChunkBufConfig bufferConfig;
		TPckg<TMmSharedChunkBufConfig> bufferConfigBufVideo(bufferConfig);
		cam.GetBufferConfig(ECamCaptureModeVideo, bufferConfigBufVideo);
		PrintBufferConf(bufferConfig,aTest);

		// Set the current capture mode
		r=cam.SetCaptureMode(ECamCaptureModeVideo);
		aTest(r==KErrNone);

		aTest.Next(_L("Start the camera in video mode"));
		r=cam.Start();
		aTest(r==KErrNone);

		aTest.Next(_L("Issue capture requests and globally cancel them all"));
		aTest.Printf(_L("Request %d frames\r\n"),KNumVideoFramesToCapture);
		TRequestStatus rs[KNumVideoFramesToCapture];
		TInt i;
		for (i=0 ; i<KNumVideoFramesToCapture; i++)
			cam.NotifyNewImage(rs[i]);

		cam.NotifyNewImageCancel();

		TInt retOffset = -2;
		TInt retId = -2;
		for (i=0 ; i<KNumVideoFramesToCapture; i++)
			{
			User::WaitForRequest(rs[i]);
			retId=rs[i].Int();
			aTest(retId==KErrCancel || retId>=0);
			if (retId>=0)
				{
				cam.BufferIdToOffset(ECamCaptureModeVideo,retId,retOffset);
				aTest.Printf(_L("Buffer%d : %d\r\n"),retId,retOffset);
				aTest(retOffset>=0);
				cam.ReleaseBuffer(retId);
				}
			}

		aTest.Next(_L("Issue capture requests and individually cancel them all"));
		aTest.Printf(_L("Request %d frames\r\n"),KNumVideoFramesToCapture);

		for (i=0 ; i<KNumVideoFramesToCapture; i++)
			cam.NotifyNewImage(rs[i]);

		for (i=0 ; i<KNumVideoFramesToCapture; i++)
			cam.NotifyNewImageCancel(rs[i]);

		for (i=0 ; i<KNumVideoFramesToCapture; i++)
			{
			User::WaitForRequest(rs[i]);
			retId=rs[i].Int();
			aTest(retId==KErrCancel || retId>=0);
			if (retId>=0)
				{
				cam.BufferIdToOffset(ECamCaptureModeVideo,retId,retOffset);
				aTest.Printf(_L("Buffer%d : %d\r\n"),retId,retOffset);
				aTest(retOffset>=0);
				cam.ReleaseBuffer(retId);
				}
			}

		aTest.Next(_L("Stop the camera."));
		r=cam.Stop();
		aTest(r==KErrNone);

		aTest.Next(_L("Close the Video chunk"));
		chunkVideo.Close();

		delete [] frameSizes;
		}

	// Skip past the video pixel formats
	pixelFormat += CameraCaps->iNumVideoPixelFormats;

	// Test Viewfinder Capture, if supported
	if (CameraCaps->iNumViewFinderPixelFormats)
		{
		// If iNumViewFinderPixelFormats is > 0 then the matching iNumFrameSizes should also be > 0
		Test(pixelFormat->iNumFrameSizes > 0);
		frameSizes = new SDevCamFrameSize[pixelFormat->iNumFrameSizes];
		Test(frameSizes != NULL);
		bufferSize = (sizeof(SDevCamFrameSize) * pixelFormat->iNumFrameSizes);
		TPtr8 frameSizesBuf((TUint8*) frameSizes, bufferSize, bufferSize);
		r = cam.FrameSizeCaps(ECamCaptureModeViewFinder, pixelFormat->iPixelFormat, frameSizesBuf);
		Test(r == KErrNone);

		cam.GetCamConfig(ECamCaptureModeViewFinder, camConfBuf);

		camConfBuf().iFrameSize=frameSizes[0];
		camConfBuf().iPixelFormat=*pixelFormat;
		camConfBuf().iFrameRate=frameSizes[0].iMaxFrameRate;

		// Set the camera configuration.
		r=cam.SetCamConfig(ECamCaptureModeViewFinder, camConfBuf);
		aTest(r==KErrNone);
		cam.GetCamConfig(ECamCaptureModeViewFinder, camConfBuf);
		PrintCamConf(camConfBuf(), aTest);

		// Create a chunk handle and trigger the buffer creation.
		RChunk chunkViewFinder;
		aTest.Next(_L("Setup the config - creating a chunk for viewfinder capture"));
		r=cam.SetBufConfigChunkCreate(ECamCaptureModeViewFinder,numBuffers,chunkViewFinder);
		aTest(r==KErrNone);

		aTest.Next(_L("Read and display the resulting buffer config"));
		TMmSharedChunkBufConfig bufferConfig;
		TPckg<TMmSharedChunkBufConfig> bufferConfigBufViewFinder(bufferConfig);
		cam.GetBufferConfig(ECamCaptureModeViewFinder, bufferConfigBufViewFinder);
		PrintBufferConf(bufferConfig,aTest);

		aTest.Next(_L("Close the Viewfinder chunk"));
		chunkViewFinder.Close();

		delete [] frameSizes;
		}

	aTest.Next(_L("Close the drivers"));
	cam.Close();
	}

void CameraTests(TInt aCameraSensorIndex)
	{
	TUint index, size;
	Test.Printf(_L("Testing unit number: %d\r\n"),aCameraSensorIndex);

	// Perform some basic opening and multithreaded tests.  We don't want just in time debugging during
	// these tests
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);
	DoCamOpenCapTests(Test,aCameraSensorIndex);
	User::SetJustInTime(justInTime);

	// Test request handling
	DoCamCancelTests(Test, aCameraSensorIndex);

	// Re-open the camera driver for use below and to ensure that it can deal with being re-opened (it
	// has already been opened and closed by DoCamOpenCapTests())
	RDevCameraSc cam;
	Test.Next(_L("Open a channel on the camera driver"));
	TInt r=cam.Open(aCameraSensorIndex);
	Test(r==KErrNone);

	// Test Dynamic Settings, e.g. Brightness, Contrast, etc.
	DoCamDynamicSettingsTests(cam, Test);
	
	// Go through all supported image, video and viewfinder pixel formats and perform configuration
	// and capture tests on them all, for all supported frame sizes and frame rates
	TInt bufferSize;
	SDevCamFrameSize* frameSizes;
	TBool imageConfigTestsDone=EFalse, videoConfigTestsDone=EFalse, viewFinderConfigTestsDone=EFalse;
	SDevCamPixelFormat* pixelFormat = (SDevCamPixelFormat*) (CameraCaps + 1);

	for (index = 0; index < CameraCaps->iNumImagePixelFormats; ++index)
		{
		// If iNumImagePixelFormats is > 0 then the matching iNumFrameSizes should also be > 0
		Test(pixelFormat->iNumFrameSizes > 0);
		Test.Printf(_L("Image pixel format %x, number of frame sizes = %d\n"), pixelFormat->iPixelFormat, pixelFormat->iNumFrameSizes);
		frameSizes=new SDevCamFrameSize[pixelFormat->iNumFrameSizes];
		Test(frameSizes!=NULL);
		bufferSize=(sizeof(SDevCamFrameSize) * pixelFormat->iNumFrameSizes);
		TPtr8 frameSizesBuf((TUint8*) frameSizes, bufferSize, bufferSize);
		r = cam.FrameSizeCaps(ECamCaptureModeImage, pixelFormat->iPixelFormat, frameSizesBuf);
		Test(r == KErrNone);

		// Test camera configuration for image capture.  This is only done once for the sake of
		// test expediency
		if (!imageConfigTestsDone)
			{
			imageConfigTestsDone = ETrue;
			DoCamConfigTests(cam, Test, ECamCaptureModeImage, *pixelFormat, frameSizes[0]);
			}

		// Test image capture mode
		for (size = 0; size < pixelFormat->iNumFrameSizes; ++size)
			{
			DoCamImageCaptureTests(cam, Test, aCameraSensorIndex, *pixelFormat, frameSizes[size], frameSizes[size].iMaxFrameRate);
			}

		// Test buffer offset
		for (size = 0; size < pixelFormat->iNumFrameSizes; ++size)
			{
			DoCamBufferOffsetTests(cam, Test, *pixelFormat, frameSizes[size], frameSizes[size].iMaxFrameRate);
			}

		delete [] frameSizes;
		++pixelFormat;
		}

	for (index = 0; index < CameraCaps->iNumVideoPixelFormats; ++index)
		{
		// If iNumVideoPixelFormats is > 0 then the matching iNumFrameSizes should also be > 0
		Test(pixelFormat->iNumFrameSizes > 0);
		Test.Printf(_L("Video pixel format %x, number of frame sizes = %d\n"), pixelFormat->iPixelFormat, pixelFormat->iNumFrameSizes);
		frameSizes=new SDevCamFrameSize[pixelFormat->iNumFrameSizes];
		Test(frameSizes!=NULL);
		bufferSize=(sizeof(SDevCamFrameSize) * pixelFormat->iNumFrameSizes);
		TPtr8 frameSizesBuf((TUint8*) frameSizes, bufferSize, bufferSize);
		r = cam.FrameSizeCaps(ECamCaptureModeVideo, pixelFormat->iPixelFormat, frameSizesBuf);
		Test(r == KErrNone);

		// Test camera configuration for video capture.  This is only done once for the sake of
		// test expediency
		if (!videoConfigTestsDone)
			{
			videoConfigTestsDone=ETrue;
			DoCamConfigTests(cam, Test, ECamCaptureModeVideo, *pixelFormat, frameSizes[0]);
			}

		// Test video capture mode
		for (size = 0; size < pixelFormat->iNumFrameSizes; ++size)
			{
			DoCamVideoCaptureTests(cam, Test, ECamCaptureModeVideo, *pixelFormat,frameSizes[size], frameSizes[size].iMaxFrameRate);
			}
		delete [] frameSizes;
		++pixelFormat;
		}

	for (index = 0; index < CameraCaps->iNumViewFinderPixelFormats; ++index)
		{
		// If iNumViewFinderPixelFormats is > 0 then the matching iNumFrameSizes should also be > 0
		Test(pixelFormat->iNumFrameSizes > 0);
		Test.Printf(_L("Viewfinder pixel format %x, number of frame sizes = %d\n"), pixelFormat->iPixelFormat, pixelFormat->iNumFrameSizes);
		frameSizes=new SDevCamFrameSize[pixelFormat->iNumFrameSizes];
		Test(frameSizes!=NULL);
		bufferSize=(sizeof(SDevCamFrameSize) * pixelFormat->iNumFrameSizes);
		TPtr8 frameSizesBuf((TUint8*) frameSizes, bufferSize, bufferSize);
		r = cam.FrameSizeCaps(ECamCaptureModeViewFinder, pixelFormat->iPixelFormat, frameSizesBuf);
		Test(r == KErrNone);

		// Test camera configuration for view finder capture.  This is only done once for the sake of
		// test expediency
		if (!viewFinderConfigTestsDone)
			{
			viewFinderConfigTestsDone=ETrue;
			DoCamConfigTests(cam, Test, ECamCaptureModeViewFinder, *pixelFormat, frameSizes[0]);
			}

		// Test view finder capture mode
		for (size = 0; size < pixelFormat->iNumFrameSizes; ++size)
			{
			DoCamVideoCaptureTests(cam, Test, ECamCaptureModeViewFinder, *pixelFormat, frameSizes[size], frameSizes[size].iMaxFrameRate);
			}
		delete [] frameSizes;
		++pixelFormat;
		}
	cam.Close();

	// And free the global capabilities buffer that was allocated in DoCamOpenCapTests()
	User::Free(CapsBufPtr);
	}

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;

	Test.Title();
	Test.Start(_L("Camera module API test"));

	Test.Next(_L("Loading CAMERA PDD"));
	TInt r=User::LoadPhysicalDevice(KCamPddFileName);
	Test.Printf(_L("Returned %d\r\n"),r);

	if (r==KErrNotFound)
		{
		Test.Printf(_L("Shared chunk camera driver not supported - test skipped\r\n"));
		Test.End();
		Test.Close();
		__UHEAP_MARKEND;
		return(KErrNone);
		}

	Test(r==KErrNone || r==KErrAlreadyExists);

	Test.Next(_L("Loading CAMERA LDD"));
	r=User::LoadLogicalDevice(KCamLddFileName);
	Test.Printf(_L("Returned %d\r\n"),r);
	Test(r==KErrNone || r==KErrAlreadyExists);

	Test.Next(_L("Loading D_MMCSC LDD"));
	r=User::LoadLogicalDevice(KTstLddFileName);
	Test.Printf(_L("Returned %d\r\n"),r);
	Test(r==KErrNone||r==KErrAlreadyExists);

	__KHEAP_MARK;

	if (User::CommandLineLength()>0)
		{
		TBuf<0x100> cmd;
		TInt moduleIndex = KUnit0;
		User::CommandLine(cmd);
		Test(cmd.Length()>0);
		if (cmd[0]>='0' && cmd[0]<='9')
			moduleIndex=(TInt)(cmd[0]-'0');
		CameraTests(moduleIndex);
		}
	else // If no command line arguments are passed we perform tests on the module 0 and 1
		{
		CameraTests(0);
		}

	__KHEAP_MARKEND;

	// Free the PDDs and LDDs
	Test.Next(_L("Freeing ECAMERASC LDD"));
	r=User::FreeLogicalDevice(KDevCameraScName);
	Test(r==KErrNone);

	Test.Next(_L("Freeing CAMERASC PDD"));
	TFindPhysicalDevice fDr;
	TFullName drivName;
	TName searchName;
	searchName.Append(KDevCameraScName);
	searchName.Append(KCamFreePddExtension);
	fDr.Find(searchName);
	r=fDr.Next(drivName);
	Test(r==KErrNone);
	r=User::FreePhysicalDevice(drivName);
	Test(r==KErrNone);

	Test.Next(_L("Freeing D_MMCSC LDD"));
	r=User::FreeLogicalDevice(KDevMmCScName);
	Test(r==KErrNone);

	Test.End();
	Test.Close();

	__UHEAP_MARKEND;
	return(KErrNone);
	}
