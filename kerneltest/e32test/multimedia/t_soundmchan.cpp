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
// e32test\multimedia\t_soundmchan.cpp
// 
//

/**
 @file Testing access to the shared chunk sound driver from multiple user side threads.
*/

#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>
#include "t_soundutils.h"

RTest Test(_L("T_SOUNDMCHAN"));

const TInt KHeapSize=0x4000;

enum TSecThreadTestId
	{
	ESecThreadConfigPlayback,
    ESecThreadConfigRecord,
	};	
struct SSecondaryThreadInfo
	{
	TSecThreadTestId iTestId;
//	TInt iExpectedRetVal;
	TThreadId iThreadId;
	TInt iDrvHandle;
	};

_LIT(KSndLddFileName,"ESOUNDSC.LDD");
_LIT(KSndPddFileName,"SOUNDSC.PDD");


LOCAL_C TInt secondaryThread(TAny* aTestInfo)
	{
	RTest stest(_L("Secondary test thread"));
	stest.Title();
	
	stest.Start(_L("Check which test to perform"));
	SSecondaryThreadInfo& sti=*((SSecondaryThreadInfo*)aTestInfo);
	TInt r;
	switch(sti.iTestId)
		{
		case ESecThreadConfigPlayback:
			{
			stest.Next(_L("Duplicate the channel handle passed from main thread"));
			
			// Get a reference to the main thread - which created the handle
			RThread thread;
			r=thread.Open(sti.iThreadId);
			stest(r==KErrNone);
	
			// Duplicate the driver handle passed from the other thread - for this thread
			RSoundSc snddev;
			snddev.SetHandle(sti.iDrvHandle);
			r=snddev.Duplicate(thread);
			stest(r==KErrNone);
			thread.Close();
			
			stest.Next(_L("Configure the driver"));
			// Read the capabilties of this device.
			TSoundFormatsSupportedV02Buf capsBuf;
			snddev.Caps(capsBuf);
			TSoundFormatsSupportedV02& caps=capsBuf();
			
			// Read back the default configuration - which must be valid.
			TCurrentSoundFormatV02Buf formatBuf;
			snddev.AudioFormat(formatBuf);
			TCurrentSoundFormatV02& format=formatBuf();
			
			if (caps.iEncodings&KSoundEncoding16BitPCM)
				format.iEncoding = ESoundEncoding16BitPCM;
			if (caps.iRates&KSoundRate16000Hz)
				format.iRate = ESoundRate16000Hz;
			if (caps.iChannels&KSoundStereoChannel)
				format.iChannels = 2;
			r=snddev.SetAudioFormat(formatBuf);
			stest(r==KErrNone);
			r=snddev.SetVolume(KSoundMaxVolume);
			stest(r==KErrNone);
			
			stest.Next(_L("Close the channel again"));
			snddev.Close();
			
			break;	
			}
			
		case ESecThreadConfigRecord:
			{
			stest.Next(_L("Use the channel passed from main thread to configure driver"));
			
			break;		
			}
			
		default:
			break;
		}
	
//	stest.Getch();	
	stest.End();
	return(KErrNone);	
	}
	
GLDEF_C TInt E32Main()

    {
	__UHEAP_MARK;

	Test.Title();

	TInt r;
	Test.Start(_L("Load sound PDD"));
	r=User::LoadPhysicalDevice(KSndPddFileName);
	if (r==KErrNotFound)
		{
		Test.Printf(_L("Shared chunk sound driver not supported - test skipped\r\n"));
		Test.End();
		Test.Close();
		__UHEAP_MARKEND;
		return(KErrNone);
		}
	Test(r==KErrNone || r==KErrAlreadyExists);
	
	Test.Next(_L("Load sound LDD"));
	r=User::LoadLogicalDevice(KSndLddFileName);
	Test(r==KErrNone || r==KErrAlreadyExists);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUNDMCHAN-224
	@SYMTestCaseDesc 		Opening the channel - more than one channel
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	With the LDD and PDD installed and with all channels closed on the device, 
								open a channel for playback on the device. 
							2)	Without closing the first playback channel, attempt to open a second channel 
								for playback on the same device. 
	@SYMTestExpectedResults	1)	KErrNone - Channel opens successfully. 
							2)	Should fail with KErrInUse.
	@SYMREQ					PREQ1073.4 */	
	
	__KHEAP_MARK;

	Test.Next(_L("Open a channel on the play device"));
	RSoundSc snddev;
	r=snddev.Open(KSoundScTxUnit0);
	Test(r==KErrNone);
	
	Test.Next(_L("Try opening the same unit a second time."));
	RSoundSc snddev2;
	r=snddev2.Open(KSoundScTxUnit0);
	Test(r==KErrInUse);
	
	Test.Next(_L("Query play formats supported"));
	TSoundFormatsSupportedV02Buf capsBuf;
	snddev.Caps(capsBuf);
	TSoundFormatsSupportedV02& caps=capsBuf();
	PrintCaps(caps,Test);
	
	Test.Next(_L("Try playing without setting the buffer config"));
	TRequestStatus pStat;
	snddev.PlayData(pStat,0,0x2000);	// 8K
	User::WaitForRequest(pStat);
	Test(pStat.Int()==KErrNotReady);

	Test.Next(_L("Configure the channel from a 2nd thread"));
	RThread thread;
	TRequestStatus tStat;
	SSecondaryThreadInfo sti;
	
	sti.iTestId=ESecThreadConfigPlayback;
	sti.iThreadId=RThread().Id();	// Get the ID of this thread
	sti.iDrvHandle=snddev.Handle();	// Pass the channel handle
	
	/**	@SYMTestCaseID 		PBASE-T_SOUNDMCHAN-225
	@SYMTestCaseDesc 		Opening the channel - sharing the handle between threads
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	With the LDD and PDD installed and with all channels closed on the device, open a 
								channel for playback on the device. Now create a second thread. Resume this 
								thread - passing the handle to the playback channel to it. Wait for the second 
								thread to terminate.
							2)	In the second thread, duplicate the playback channel handle.
							3)	In the second thread, using the duplicated handle, issue a request to set the audio configuration. 
							4)	In the second thread, using the duplicated handle, issue a request to set the volume. 
							5)	In the second thread, close the handle and exit the thread.
							6)	In the first thread, read back the audio configuration.
							7)	In the first thread, set the buffer configuration, and then issue a request to play 
								audio data. 
							8)	In the first thread, close the channel. 
	@SYMTestExpectedResults	1)	KErrNone - Channel opens successfully. 
							2)	KErrNone - Duplication of the handle succeeds.
							3)	KErrNone - Audio configured successfully.
							4)	KErrNone - Volume set successfully.
							5)	No errors occur closing the channel and exiting the thread.
							6)	The audio configuration should correspond to that set by the second thread.
							7)	KErrNone - Setting the buffer configuration and issuing a play request.
							8)	No errors occur closing the channel.
	@SYMREQ					PREQ1073.4 */	
	
	r=thread.Create(_L("Thread"),secondaryThread,KDefaultStackSize,KHeapSize,KHeapSize,&sti); // Create secondary thread
	Test(r==KErrNone);
	thread.Logon(tStat);
	thread.Resume();
	User::WaitForRequest(tStat);
	Test(tStat.Int()==KErrNone);
//	Test.Printf(_L("Thread exit info: Cat:%S, Reason:%x, Type:%d\r\n"),&thread.ExitCategory(),thread.ExitReason(),thread.ExitType());
	Test(thread.ExitType()==EExitKill);
	thread.Close();
	User::After(10000);	// Wait 10ms
	
	Test.Next(_L("Read back the play configuration"));
	TCurrentSoundFormatV02Buf formatBuf;
	snddev.AudioFormat(formatBuf);
	TCurrentSoundFormatV02& format=formatBuf();
	PrintConfig(format,Test);
	
	Test.Next(_L("Set the buffer configuration"));
	RChunk chunk;
	TInt bufSize=BytesPerSecond(formatBuf()); 	 							// Large enough to hold 1 second of data.
	bufSize=ValidBufferSize(bufSize,caps.iRequestMinSize,formatBuf());		// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=1;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;	
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=snddev.SetBufferChunkCreate(bufferConfigBuf,chunk);
	Test(r==KErrNone);
	snddev.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	Test(bufferConfig.iBufferSizeInBytes==bufSize);
	
	Test.Next(_L("Start playing"));
	r=MakeSineTable(format);
	Test(r==KErrNone);
	r=SetToneFrequency(660,format);
	Test(r==KErrNone); 
	TPtr8 ptr(chunk.Base()+bufferConfig.iBufferOffsetList[0],bufSize);
	WriteTone(ptr,format);
	snddev.PlayData(pStat,bufferConfig.iBufferOffsetList[0],bufSize,KSndFlagLastSample);
	User::WaitForRequest(pStat);
	Test(tStat.Int()==KErrNone);
	
	Test.Next(_L("Close the drivers and the chunk"));
	chunk.Close();
	snddev.Close();
	
	__KHEAP_MARKEND;

	Test.Next(_L("Unload the drivers"));
	
	r=User::FreeLogicalDevice(KDevSoundScName);
	Test.Printf(_L("Unloading %S.LDD - %d\r\n"),&KDevSoundScName,r);
	Test(r==KErrNone);
	
	TName pddName(KDevSoundScName);
	_LIT(KPddWildcardExtension,".*");
	pddName.Append(KPddWildcardExtension);
	TFindPhysicalDevice findPD(pddName);
	TFullName findResult;
	r=findPD.Next(findResult);
	while (r==KErrNone)
		{
		r=User::FreePhysicalDevice(findResult);
		Test.Printf(_L("Unloading %S.PDD - %d\r\n"),&findResult,r);
		Test(r==KErrNone);
		findPD.Find(pddName); // Reset the find handle now that we have deleted something from the container.
		r=findPD.Next(findResult);
		} 
	
	Test.End();
	Test.Close();
	
	Cleanup();
	
	__UHEAP_MARKEND;

	return(KErrNone);
    }
