// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\multimedia\t_camera_gen.cpp
// 
//

#include <e32test.h>
#include <d32camerasc.h>
#include <e32def.h>
#include <e32def_private.h>
#include "t_camera_display.h"
#include "d_mmcsc.h"

_LIT(KTstLddFileName,"D_MMCSC.LDD");
_LIT(KCamLddFileName,"ECAMERASC.LDD");
_LIT(KCamPddFileName,"CAMERASC.PDD");
_LIT(KCamFreePddExtension,".*");

const TInt KUnit0=0;
const TInt KFrameRate=30;		// Run the test at 30fps

class CCameraHandler;

RTest test(_L("T_CAMERA_GEN"));
CCameraHandler* camera;

/**
Wait for a key press, but timeout so automated tests
are unaffected
*/
void util_getch_withtimeout(TUint aSecs)
	{
	TRequestStatus keyStat;
	test.Console()->Read(keyStat);
	RTimer timer;
	test(timer.CreateLocal()==KErrNone);
	TRequestStatus timerStat;
	timer.After(timerStat,aSecs*1000000);
	User::WaitForRequest(timerStat,keyStat);
	if(keyStat!=KRequestPending)
		(void)test.Console()->KeyCode();
	timer.Cancel();
	timer.Close();
	test.Console()->ReadCancel();
	User::WaitForAnyRequest();
	}

// Define the frame number to capture
// For image capture this should be zero.
const TInt64 KFrameNumberToCapture= 50;

/// Defines camera handler
class CCameraHandler : public CActive
	{
public:
	static CCameraHandler* NewL();
	~CCameraHandler();
	TInt SetConfig(TDevCamCaptureMode aCaptureMode, SDevCamFrameSize aSize,SDevCamPixelFormat aPixelFormat,TBool aCreateChunk,TInt aNumBuffers);
	void GetConfig(TDevCamCaptureMode aCaptureMode);
	TInt GetCaps();
	TInt SetFirstConfig(TUint aOffset);
	TInt Start(TDevCamCaptureMode aCaptureMode);
	TInt Stop();
	void SetCaptureMode(TDevCamCaptureMode aCaptureMode);
private:
	CCameraHandler();
	virtual void RunL();
	virtual void DoCancel();
	TInt Init();
private:
	TCamDisplayHandler iDispHandler[ECamCaptureModeMax];
	RDevCameraSc iCamera;
	RChunk iChunk[ECamCaptureModeMax];
	TInt iFrameCount;
	TDevCamCaptureMode iCaptureMode;
	TInt iCapsSize;
	TAny* iCapsBufPtr;
	TCameraCapsV02* iCameraCaps;
	};

CCameraHandler::CCameraHandler() : CActive(EPriorityStandard)
//
// Constructor for the camera handler
//
	{
	// Active object priority is set to normal
	}

CCameraHandler::~CCameraHandler()
//
// Destructor for the camera handler
//
	{
	for (TInt captureMode=0; captureMode < ECamCaptureModeMax; captureMode++)
		iChunk[captureMode].Close();
	if(iCapsBufPtr)
		User::Free(iCapsBufPtr);
	iCamera.Close();
	// Cancel any active request
	CActive::Cancel();
	}

void CCameraHandler::DoCancel()
	{

	}

CCameraHandler* CCameraHandler::NewL()
//
// Create active camera
//
	{
	test.Printf(_L("NewL\r\n"));
	CCameraHandler *cc = new (ELeave) CCameraHandler;
	TInt r=cc->Init();
	if (r!=KErrNone)
		User::Leave(r);
	CActiveScheduler::Add(cc);
	return(cc);
	}

TInt CCameraHandler::Init()
//
// Initialise hardware
//
	{
	test.Printf(_L("Init\r\n"));
	TInt r;
	for (TInt captureMode=0; captureMode < ECamCaptureModeMax; captureMode++)
		{
		r=iDispHandler[captureMode].Init();
		if (r!=KErrNone)
			return(r);
		}

	// Open camera driver channel
	r=iCamera.Open(KUnit0);
	return(r);
	}

void CCameraHandler::SetCaptureMode(TDevCamCaptureMode aCaptureMode)
	{
	iCaptureMode=aCaptureMode;
	}

TInt CCameraHandler::GetCaps()
	{
	test.Printf(_L("GetCaps\r\n"));
	iCapsSize=iCamera.CapsSize();
	iCapsBufPtr = User::Alloc(iCapsSize);
	TPtr8 capsPtr( (TUint8*)iCapsBufPtr, iCapsSize, iCapsSize );
	TInt r = iCamera.Caps(capsPtr);
	if(r!=KErrNone)
		return r;
	iCameraCaps = (TCameraCapsV02*) capsPtr.Ptr();

	test.Printf(_L("Number of supported pixel formats:%d\r\n"),iCameraCaps->iNumVideoPixelFormats);
	return r;
	}

void CCameraHandler::GetConfig(TDevCamCaptureMode aCaptureMode)
	{
	test.Printf(_L("GetConfig\r\n"));
	// Config camera
	TCameraConfigV02Buf configBuf;
	TCameraConfigV02 &config=configBuf();

	iCamera.GetCamConfig(aCaptureMode, configBuf);

	test.Printf(_L("Pixel Format:%d\r\n"),config.iPixelFormat);
	test.Printf(_L("Frame Size  :%d\r\n"),config.iFrameSize);
	test.Printf(_L("Frame rate  :%dfps\r\n"),config.iFrameRate);
	}

TInt CCameraHandler::SetFirstConfig(TUint aOffset)
	{
	test.Printf(_L("SetFirstConfig\r\n"));
	TInt ret;
	TAny* frameSizeCapsBuf=0;
	SDevCamPixelFormat* pixelFormat;
	SDevCamFrameSize* frameSize;
	TPtr8 frameSizeCapsPtr(0,0,0);
	pixelFormat = (SDevCamPixelFormat*) (iCameraCaps + 1);

	if(camera->iCameraCaps->iNumVideoPixelFormats)
		{
		pixelFormat = pixelFormat + iCameraCaps->iNumImagePixelFormats + iCameraCaps->iNumVideoPixelFormats;
		frameSizeCapsBuf = User::Alloc(pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		new (&frameSizeCapsPtr) TPtr8((TUint8*)frameSizeCapsBuf, pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize), pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		ret=iCamera.FrameSizeCaps(ECamCaptureModeImage, pixelFormat->iPixelFormat, frameSizeCapsPtr);
		test(ret==KErrNone);
		frameSize = (SDevCamFrameSize*) frameSizeCapsPtr.Ptr();
		if((pixelFormat->iNumFrameSizes>aOffset) && aOffset)
			frameSize += aOffset;
		ret=camera->SetConfig(ECamCaptureModeVideo,*frameSize,*pixelFormat,ETrue,3);
		test(ret==KErrNone);
		User::Free(frameSizeCapsBuf);
		return(KErrNone);
		}
	else
		return(KErrNotSupported);
	}

TInt CCameraHandler::SetConfig(TDevCamCaptureMode aCaptureMode, SDevCamFrameSize aSize,SDevCamPixelFormat aPixelFormat,TBool aCreateChunk,TInt aNumBuffers)
	{
	test.Printf(_L("SetConfig\r\n"));
	TInt r=iDispHandler[aCaptureMode].SetConfig(aSize,aPixelFormat);
	if (r!=KErrNone)
		return(r);

	// Config camera
	TCameraConfigV02Buf configBuf;
	TCameraConfigV02 &config=configBuf();
	iCamera.GetCamConfig(aCaptureMode, configBuf);		// Load defaults

	config.iFrameSize=aSize;
	config.iPixelFormat=aPixelFormat;
	config.iFrameRate=KFrameRate;

	TMmSharedChunkBufConfig bufferConfig;
	TPckg<TMmSharedChunkBufConfig> bufferConfigBuf(bufferConfig);

	r=iCamera.SetCamConfig(aCaptureMode,configBuf);
	if (r!=KErrNone)
		return(r);

	if (aCreateChunk)
		{
		r=iCamera.SetBufConfigChunkCreate(aCaptureMode,aNumBuffers,iChunk[aCaptureMode]);
		if (r!=KErrNone)
			return(r);
		}
	else
		{
		// 'aNumBuffers' is ignored here, D_MMCSC.LDD currently uses 2 buffers

		RMmCreateSc tstDrv;
		r=tstDrv.Open();
		if (r!=KErrNone)
			return(r);
		r=tstDrv.GetChunkHandle(iChunk[aCaptureMode]);	// Get a handle on the shared chunk created by the test driver
		if (r!=KErrNone)
			return(r);
		r=tstDrv.GetBufInfo(bufferConfigBuf);
		if (r!=KErrNone)
			return(r);
		r=iCamera.SetBufConfigChunkOpen(aCaptureMode,bufferConfigBuf,iChunk[aCaptureMode]);
		if (r!=KErrNone)
			return(r);

		tstDrv.Close();
		}

	iCamera.GetBufferConfig(aCaptureMode, bufferConfigBuf);
	PrintBufferConf(bufferConfig,test);

	return(r);
	}

TInt CCameraHandler::Start(TDevCamCaptureMode aCaptureMode)
//
// Set object active, start getting images from the camera
//
	{
	test.Printf(_L("Start\r\n"));
	// Set object active
	iFrameCount=0;
	SetActive();
	iCamera.SetCaptureMode(aCaptureMode);
	// Add request for a new image
	TInt r=iCamera.Start();
	if (r==KErrNone)
		iCamera.NotifyNewImage(iStatus);
	return(r);
	}

void CCameraHandler::RunL()
//
// Handles a new request
//
	{
	TInt retId=iStatus.Int();
	TInt retOffset=-1;
	iCamera.BufferIdToOffset(iCaptureMode,retId,retOffset);
	if (retId>=0)
		{
		TUint8* imgBase=iChunk[iCaptureMode].Base()+retOffset;
		TInt r=iDispHandler[iCaptureMode].Process(imgBase);
		test(r==KErrNone);
		// Release the buffer
		test(iCamera.ReleaseBuffer(retId)==KErrNone);
		iFrameCount++;
		}
	else
		test.Printf(_L("Capture error (%d)\r\n"),retId);

	if (iFrameCount<KFrameNumberToCapture)
		{
		// Add request for a new image
		iCamera.NotifyNewImage(iStatus);
		// re-set active
		SetActive();
		}

	else
		{
		CActiveScheduler::Stop();
		}
	}

TInt CCameraHandler::Stop()
//
// Stops camera
//
	{
	test.Printf(_L("Stop\r\n"));
	CActive::Cancel();
	TInt r=iCamera.Stop();
	return(r);
	}

//
// Test for recording a certain number of frames in a particular configuration and displaying
// the results.
//
void TestRecording()
	{
	TInt ret;

	camera->GetConfig(ECamCaptureModeVideo);
	camera->SetCaptureMode(ECamCaptureModeVideo);
	test.Next(_L("Starting camera"));
	ret=camera->Start(ECamCaptureModeVideo);
	test.Printf(_L("Start returned %d\r\n"),ret);
	test(ret==KErrNone);

	// Calculate frame rate.
	// We store cuurent time before receiving data from the camera and
	// after to have received KFrameNumberToCapture pictures, we calculate
	// time elapsed during the reception.
	TTimeIntervalMicroSeconds microseconds ;
	TTime now;
	TTime iTime;
	now.HomeTime();

	test.Next(_L("Starting active scheduler"));
	// Start active scheduler
	CActiveScheduler::Start();

	// We have received all pictures, so we store the new current time
	iTime.HomeTime();

	// We keep this time in microseconds to be more precise
	microseconds = iTime.MicroSecondsFrom(now) ;

	TInt64 timeElapsed = microseconds.Int64();

	// We store in this variable, integer value of the frame rate
	TInt64 intFrameRate = (TInt64)((KFrameNumberToCapture *1000000)/timeElapsed);

	// We store in this variable, decimal value of the frame rate
	TInt64 milliFrameRate = (TInt64)((KFrameNumberToCapture *1000000)%timeElapsed);
	milliFrameRate = (milliFrameRate*1000) / timeElapsed;

	test.Printf(_L("   Frame rate for frames received : %d.%03d frames per second\r\n"), static_cast<TInt>(intFrameRate), static_cast<TInt>(milliFrameRate));
	test.Printf(_L("   Frame rate expected            : %d.000 frames per second\r\n"),KFrameRate);
	test.Next(_L("Stopping camera"));
	// Stop Camera
	ret=camera->Stop();
	test(ret==KErrNone);
	}

//
// Test program main part
//
TInt E32Main()
	{
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Camera module GEN test"));

	test.Next(_L("Loading CAMERA PDD"));
	TInt ret=User::LoadPhysicalDevice(KCamPddFileName);
	test.Printf(_L("Returned %d\r\n"),ret);

	if (ret==KErrNotFound)
		{
		test.Printf(_L("Shared chunk camera driver not supported - test skipped\r\n"));
		test.End();
		test.Close();
		__UHEAP_MARKEND;
		return(KErrNone);
		}

	test(ret==KErrNone || ret==KErrAlreadyExists);

	test.Next(_L("Loading CAMERA LDD"));
	ret=User::LoadLogicalDevice(KCamLddFileName);
	test.Printf(_L("Returned %d\r\n"),ret);
	test(ret==KErrNone || ret==KErrAlreadyExists);

	test.Next(_L("Loading D_MMCSC LDD"));
	ret=User::LoadLogicalDevice(KTstLddFileName);
	test.Printf(_L("Returned %d\r\n"),ret);
	test(ret==KErrNone||ret==KErrAlreadyExists);

	__KHEAP_MARK;

	// Construct and install the active scheduler
	test.Next(_L("Initialising active scheduler"));
	CActiveScheduler *exampleScheduler = new (ELeave) CActiveScheduler();
	// Install as the active scheduler
	CActiveScheduler::Install(exampleScheduler);

	// Create camera handler
	test.Next(_L("Creating camera handler"));
	camera = CCameraHandler::NewL();

	test.Next(_L("+ Getting Camera Capabilities"));
	ret=camera->GetCaps();
	test(ret==KErrNone);

	// SetConfig
	test.Next(_L("Setting Camera Configuration"));
	ret=camera->SetFirstConfig(0);
	test(ret==KErrNone);
	TestRecording();

	// Repeat with a different configuration
	test.Next(_L("Resetting Camera Configuration"));
	ret=camera->SetFirstConfig(1);
	test(ret==KErrNone);
	TestRecording();

	delete camera;
	delete exampleScheduler;

	__KHEAP_MARKEND;

	// Free the PDDs and LDDs
	test.Next(_L("Freeing ECAMERASC LDD"));
	ret=User::FreeLogicalDevice(KDevCameraScName);
	test(ret==KErrNone);

	test.Next(_L("Freeing CAMERASC PDD")) ;
	TFindPhysicalDevice fDr;
	TFullName drivName;
	TName searchName;
	searchName.Append(KDevCameraScName);
	searchName.Append(KCamFreePddExtension);
	fDr.Find(searchName);
	ret=fDr.Next(drivName);
	test(ret==KErrNone);
	ret=User::FreePhysicalDevice(drivName);
	test(ret==KErrNone);

	test.Next(_L("Freeing D_MMCSC LDD"));
	ret=User::FreeLogicalDevice(KDevMmCScName);
	test(ret==KErrNone);

	test.Printf(_L("Hit any key to continue\r\n"));
	util_getch_withtimeout(5);

	test.End();
	test.Close();

	__UHEAP_MARKEND;

	return KErrNone;
	}
