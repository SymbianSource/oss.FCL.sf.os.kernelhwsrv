// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\multimedia\t_sound2.cpp
//
//

/**
 @file General test code for the shared chunk sound driver - based on T_SOUND.
*/

#include <e32test.h>
#include "t_soundutils.h"
#include <hal.h>
#include <e32def.h>
#include <e32def_private.h>

// #define SOAKTEST

const TSoundRate speedTable[] = {ESoundRate48000Hz,ESoundRate44100Hz,ESoundRate32000Hz,ESoundRate29400Hz,
								 ESoundRate24000Hz,ESoundRate22050Hz,ESoundRate16000Hz,ESoundRate14700Hz,
								 ESoundRate12000Hz,ESoundRate11025Hz,ESoundRate9600Hz,ESoundRate8820Hz,
								 ESoundRate8000Hz,ESoundRate7350Hz,(TSoundRate)-1};	// ALL RATES DECENDING

#define CHECK(aValue) {Test(aValue,__LINE__);}
#define CHECK_NOERROR(aValue) { TInt v=(aValue); if(v) { Test.Printf(_L("Error value = %d\n"),v); Test(EFalse,__LINE__); }}
#define CHECK_EQUAL(aValue1,aValue2) { TInt v1=(aValue1); TInt v2=(aValue2); if(v1!=v2) { Test.Printf(_L("Error value = %d\n"),v1); Test(EFalse,__LINE__); }}
#define CHECK_POSITIVE(aOffset) { if(aOffset<0) { Test.Printf(_L("CHECK_POSITIVE(%d) failed\n"), aOffset); Test(EFalse,__LINE__); } }

_LIT(KSndLddFileName,"ESOUNDSC.LDD");
_LIT(KSndPddFileName,"SOUNDSC.PDD");

RTest Test(_L("T_SOUND2"));
RSoundSc TxSoundDevice;
RSoundSc RxSoundDevice;

TSoundFormatsSupportedV02Buf RecordCapsBuf;
TSoundFormatsSupportedV02Buf PlayCapsBuf;
TCurrentSoundFormatV02Buf PlayFormatBuf;
TCurrentSoundFormatV02Buf RecordFormatBuf;

LOCAL_C TInt Load()
	{
	TInt r;

	Test.Start(_L("Load sound PDD"));
	r=User::LoadPhysicalDevice(KSndPddFileName);
	if (r==KErrNotFound)
		{
		Test.End();
		return(r);
		}
	CHECK(r==KErrNone || r==KErrAlreadyExists);
	r=User::LoadPhysicalDevice(KSndPddFileName);
	CHECK(r==KErrAlreadyExists);

	Test.Next(_L("Load sound LDD"));
	r=User::LoadLogicalDevice(KSndLddFileName);
	CHECK(r==KErrNone || r==KErrAlreadyExists);
	r=User::LoadPhysicalDevice(KSndPddFileName);
	CHECK(r==KErrAlreadyExists);

	Test.End();
	return(KErrNone);
	}

LOCAL_C void CheckConfig(const TCurrentSoundFormatV02& aConfig,const TSoundFormatsSupportedV02& aCaps)
	{
	if (!((1<<(aConfig.iChannels-1)) & aCaps.iChannels))
		CHECK_NOERROR(ETrue);
	if (!((1<<aConfig.iRate) & aCaps.iRates))
		CHECK_NOERROR(ETrue);
	if (!((1<<aConfig.iEncoding) & aCaps.iEncodings))
		CHECK_NOERROR(ETrue);
	if (!((1<<aConfig.iDataFormat) & aCaps.iDataFormats))
		CHECK_NOERROR(ETrue);
	}


////////////////////////////////////////////////////////////////////////////////
// Tests

const TInt KMaxLinearVolume=256;
const TInt KLinearTodB[KMaxLinearVolume+1] =
	{
	0  ,158,170,177,182,186,189,192,194,196,198,200,201,203,204,205,
	206,207,208,209,210,211,212,213,213,214,215,215,216,217,217,218,
	218,219,219,220,220,221,221,222,222,223,223,224,224,224,225,225,
	225,226,226,226,227,227,227,228,228,228,229,229,229,230,230,230,
	230,231,231,231,231,232,232,232,232,233,233,233,233,234,234,234,
	234,235,235,235,235,235,236,236,236,236,236,237,237,237,237,237,
	237,238,238,238,238,238,239,239,239,239,239,239,240,240,240,240,
	240,240,240,241,241,241,241,241,241,241,242,242,242,242,242,242,
	242,243,243,243,243,243,243,243,244,244,244,244,244,244,244,244,
	245,245,245,245,245,245,245,245,245,246,246,246,246,246,246,246,
	246,246,247,247,247,247,247,247,247,247,247,247,248,248,248,248,
	248,248,248,248,248,248,249,249,249,249,249,249,249,249,249,249,
	250,250,250,250,250,250,250,250,250,250,250,250,251,251,251,251,
	251,251,251,251,251,251,251,251,252,252,252,252,252,252,252,252,
	252,252,252,252,252,253,253,253,253,253,253,253,253,253,253,253,
	253,253,254,254,254,254,254,254,254,254,254,254,254,254,254,254,255
	};

LOCAL_C void TestBasicPlayFunctions()
	{
	TRequestStatus stat[2];

	Test.Next(_L("Preparing to play..."));
	if (PlayCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	PlayFormatBuf().iChannels = 1;
	PrintConfig(PlayFormatBuf(),Test);
	TInt r = TxSoundDevice.SetAudioFormat(PlayFormatBuf);
	CHECK_NOERROR(r);

	// Set the play buffer configuration, then read it back.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(PlayFormatBuf())/8; 									// Large enough to hold 1/8th second of data.
	bufSize=ValidBufferSize(bufSize,PlayCapsBuf().iRequestMinSize,PlayFormatBuf());		// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=2;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;		// All buffers will be contiguous
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	TxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);
	TPtr8* tPtr[2];
	TInt i;
	for (i=0;i<2;i++)
		tPtr[i]=new TPtr8(chunk.Base()+bufferConfig.iBufferOffsetList[i],bufSize);

	/**	@SYMTestCaseID 		PBASE-T_SOUND2-249
	@SYMTestCaseDesc 		Play pause / resume - pausing and resuming before playback has commenced.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the playback channel and then setup the buffer configuration.
							1)	Attempt to resume playback before playback has been started.
							2)	Attempt to pause playback before playback has been started.
	@SYMTestExpectedResults	1)	The resume request should complete with KErrNotReady.
							2)	The pause request should complete with KErrNotReady
	@SYMREQ					PREQ1073.4 */

	Test.Printf(_L("Resume when not playing\r\n"));
	r=TxSoundDevice.Resume();
	CHECK(r==KErrNotReady)

	Test.Printf(_L("Pause when not playing\r\n"));
	r=TxSoundDevice.Pause();
	CHECK(r==KErrNotReady)

	/**	@SYMTestCaseID 		PBASE-T_SOUND2-237
	@SYMTestCaseDesc 		Play operation - with zero length.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the playback channel and then setup the buffer
							configuration. Issue a transfer request from one of the buffers to play data specifying a
							length of zero.
	@SYMTestExpectedResults	The play request should complete with KErrNone.
	@SYMREQ					PREQ1073.4 */

	Test.Next(_L("Play empty buffer"));
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],0,KSndFlagLastSample);	// Play length of zero
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrNone);

	/**	@SYMTestCaseID 		PBASE-T_SOUND2-238
	@SYMTestCaseDesc 		Play operation - with a short transfer.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the playback channel and then setup the buffer configuration.
							1)	Issue a transfer requests from one of the buffers to play data, specifying a length equal to the
								minimum request size that the device supports - i.e.
								TSoundFormatsSupportedV02::iRequestMinSize, or 2 bytes, whichever is greater.
							2)	Issue a transfer requests from one of the buffers to play data, specifying a length equal
								to twice the minimum request size that the device supports, or 34 bytes, whichever
								is greater.
	@SYMTestExpectedResults	1)	The play request should complete with KErrNone.
							2)	The play request should complete with KErrNone.
	@SYMREQ					PREQ1073.4 */

	Test.Next(_L("Play short buffer"));
	TInt len=Max(2,PlayCapsBuf().iRequestMinSize);
	Test.Printf(_L("Play length is %d bytes\r\n"),len);
	tPtr[0]->FillZ(bufSize);
	tPtr[1]->FillZ(len);
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize,0);
	TxSoundDevice.PlayData(stat[1],bufferConfig.iBufferOffsetList[1],len,KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrNone);
	User::WaitForRequest(stat[1]);
	CHECK_EQUAL(stat[1].Int(),KErrNone);

	Test.Next(_L("Play a slightly longer buffer"));
	len=Max(34,(PlayCapsBuf().iRequestMinSize<<1));
	if (PlayCapsBuf().iRequestMinSize)
			len&=~(PlayCapsBuf().iRequestMinSize-1);
	Test.Printf(_L("Play length is %d bytes\r\n"),len);
	tPtr[1]->FillZ(bufSize);
	tPtr[0]->FillZ(len);
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[1],bufSize,0); // Play 2nd buffer 1st
	TxSoundDevice.PlayData(stat[1],bufferConfig.iBufferOffsetList[0],len,KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrNone);
	User::WaitForRequest(stat[1]);
	CHECK_EQUAL(stat[1].Int(),KErrNone);

	/**	@SYMTestCaseID 		PBASE-T_SOUND2-240
	@SYMTestCaseDesc 		Play operation - altering the volume.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the playback channel and then setup the buffer configuration
							so it contains multiple buffers. Using multiple simultaneous play requests, play 4 seconds
							of continuous tone - with each individual play request consisting of 1/8th second of tone.
							Each time a request completes, increase the volume slightly - starting at the minimum
							and ending at maximum volume. (Ensure the last request is marked with the
							KSndFlagLastSample flag).
	@SYMTestExpectedResults	The driver should successfully play 4 seconds of tone with all requests completing with
							KErrNone.
	@SYMREQ					PREQ1073.4 */

	/**	@SYMTestCaseID 		PBASE-T_SOUND2-250
	@SYMTestCaseDesc 		Play pause / resume - pausing and resuming while playback is in progress.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the playback channel and then setup the buffer configuration
							so it contains multiple buffers. Reset the channel's count of bytes transferred.
							Using multiple simultaneous play requests, play 4 seconds of continuous tone - with
							each individual play request consisting of 1/8th second of tone.
							1)	After 10 requests have completed, pause transfer for 2 seconds, then resume it.
							2)	After 20 requests have completed, attempt to resume playback while playback is not paused.
							3)	With only 0.25 second of tone still to play, pause transfer for 1 second, then resume it.
							4)	10ms after resuming, pause transfer again for 1 second, then resume it.
							5)	Once transfer has completed, read back the count of bytes transferred.
	@SYMTestExpectedResults	1)	Playback of the tone should be interrupted for 2 seconds with the pause and resume requests
								both completing with KErrNone.
							2)	The resume request should complete with KErrNotReady.
							3)	Playback of the tone should be interrupted for 1 second with the pause and resume requests
								both completing with KErrNone.
							4)	Playback of the tone should be interrupted for 1 second with the pause and resume requests
								both completing with KErrNone.
							5)	The count of bytes transferred should not be affected by pausing and resuming playback
								(i.e. it should equal the value calculated for 4 seconds at the selected sampe rate and
								number of channels).
	@SYMREQ					PREQ1073.4 */

	Test.Next(_L("Playing..."));
	r=MakeSineTable(PlayFormatBuf());
	CHECK_NOERROR(r);
	r=SetToneFrequency(440,PlayFormatBuf());
	CHECK_NOERROR(r);
	TxSoundDevice.ResetBytesTransferred();
	TInt remainingPlayCount = BytesPerSecond(PlayFormatBuf())*4/bufSize;

	// Set the initial value for the volume.
	TInt bytesToPlay = remainingPlayCount*bufSize;
	TInt bytesPlayed = 0;
	TInt vol = I64LOW(TInt64(KMaxLinearVolume)*TInt64(bytesPlayed)/TInt64(bytesToPlay));
	vol = KLinearTodB[vol];			// Rather than varying the volume logarithmically (in dB), vary it linearly (as done by MM).
	TxSoundDevice.SetVolume(vol);

	// Issue a pair of play requests.
	WriteTone(*tPtr[0],PlayFormatBuf());
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize);
	WriteTone(*tPtr[1],PlayFormatBuf());
	TxSoundDevice.PlayData(stat[1],bufferConfig.iBufferOffsetList[1],bufSize);

	TInt lcount = 0;
	TUint flags;
	while (remainingPlayCount>2)
		{
		// Wait for either of the outstanding play requests to complete.
		User::WaitForAnyRequest();
		remainingPlayCount--;

		// Work out which request this applies to.
		for (i=0;i<2;i++)
			{
			if (stat[i]!=KRequestPending)
				break;
			}
		CHECK(i<2);
		CHECK_NOERROR(stat[i].Int());

		// Issue a further play request using the buffer just made free.
		WriteTone(*tPtr[i],PlayFormatBuf());
		flags=(remainingPlayCount<=2)?KSndFlagLastSample:0;
		TxSoundDevice.PlayData(stat[i],bufferConfig.iBufferOffsetList[i],bufSize,flags);

		// Adjust the volume
		bytesPlayed = TxSoundDevice.BytesTransferred();
		vol = I64LOW(TInt64(KMaxLinearVolume)*TInt64(bytesPlayed)/TInt64(bytesToPlay));
		vol = KLinearTodB[vol];			// Rather than varying the volume logarithmically (in dB), vary it linearly (as done by MM).
		Test.Printf(_L("Bytes played = %d (vol = %d)\r\n"),bytesPlayed,vol);
		TxSoundDevice.SetVolume(vol);

		if (lcount == 10)
			{
			// Do a pause/resume
			r=TxSoundDevice.Pause();
			CHECK_NOERROR(r);
			Test.Printf(_L("Pause 2 seconds\r\n"));
			User::After(2000000);
			Test.Printf(_L("Restart\r\n"));
			r=TxSoundDevice.Resume();
			CHECK_NOERROR(r);
			}
		if (lcount == 20)
			{
			Test.Printf(_L("Resume when playing\r\n"));
			r=TxSoundDevice.Resume();
			CHECK(r==KErrNotReady)
			}

		CHECK_EQUAL(TxSoundDevice.Volume(),vol);
		CHECK_EQUAL(TxSoundDevice.SetAudioFormat(PlayFormatBuf),KErrInUse);
		lcount++;
		}

	// Last 2 play requests still outstanding - do a pause/resume
	r=TxSoundDevice.Pause();
	CHECK_NOERROR(r);
	Test.Printf(_L("Pause 1 second\r\n"));
	User::After(1000000);
	Test.Printf(_L("Restart\r\n"));
	r=TxSoundDevice.Resume();
	CHECK_NOERROR(r);
	bytesPlayed = TxSoundDevice.BytesTransferred();

	User::After(10000); // 10ms

	r=TxSoundDevice.Pause();
	Test.Printf(_L("Bytes played = %d\r\n"),bytesPlayed);

	CHECK_NOERROR(r);
	Test.Printf(_L("Pause 1 second\r\n"));
	User::After(1000000);
	Test.Printf(_L("Restart\r\n"));
	r=TxSoundDevice.Resume();
	CHECK_NOERROR(r);
	bytesPlayed = TxSoundDevice.BytesTransferred();
	Test.Printf(_L("Bytes played = %d\r\n"),bytesPlayed);

	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrNone);
	User::WaitForRequest(stat[1]);
	CHECK_EQUAL(stat[1].Int(),KErrNone);

	bytesPlayed = TxSoundDevice.BytesTransferred();
	Test.Printf(_L("Bytes played = %d vs %d\n"),bytesPlayed, bytesToPlay);
	CHECK_EQUAL(bytesToPlay,bytesPlayed);

	TxSoundDevice.ResetBytesTransferred();
	CHECK_EQUAL(TxSoundDevice.BytesTransferred(),0);

	Test.Next(_L("Pause and resume when not playing"));
	TxSoundDevice.Pause();
	TxSoundDevice.Resume();

	chunk.Close();
	for (i=0;i<2;i++)
		delete tPtr[i];
	}

LOCAL_C void TestBasicRecordFunctions()
	{
	TRequestStatus stat;
	TInt length, r, i;

	Test.Next(_L("Preparing to record..."));

	if (RecordCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	RecordFormatBuf().iChannels = 2;

	// find first supported rate and set the the audio configuration to use it
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		RecordFormatBuf().iRate = (TSoundRate)i;
		r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
		if (RecordCapsBuf().iRates & (1<<i))
			{
			CHECK_NOERROR(r);				// Caps reports it is supported
			break;
			}
		}

	PrintConfig(RecordFormatBuf(),Test);

	// Set the record buffer configuration, then read it back.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(RecordFormatBuf())/8; 									// Large enough to hold 1/8th second of data.
	bufSize=ValidBufferSize(bufSize,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());	// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=3;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;																// All buffers will be contiguous
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);

	Test.Next(_L("Test for record overflow"));
	RxSoundDevice.SetVolume(KSoundMaxVolume);
	RxSoundDevice.RecordData(stat,length);
	User::WaitForRequest(stat);
	TInt retOffset=stat.Int();
	CHECK_POSITIVE(retOffset);
	CHECK(length>0);
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);

	User::After(500000);			// Wait 1/2 second for data overflow.

	RxSoundDevice.RecordData(stat,length);
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	CHECK(retOffset==KErrOverflow);

	// Make sure we can issue a successful RecordData after recovering from overflow.
	RxSoundDevice.RecordData(stat,length);
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	CHECK_POSITIVE(retOffset);
    r=RxSoundDevice.ReleaseBuffer(retOffset);
    CHECK_NOERROR(r);

	RxSoundDevice.CancelRecordData();	// Stop the driver from recording.
	chunk.Close();
	}

/**	@SYMTestCaseID 			PBASE-T_SOUND2-241
	@SYMTestCaseDesc 		Play operation - playing all rates.
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	For each of the sample rates supported by the device, setup the audio configuration
								on the playback channel for mono operation (i.e. 1 audio channel) and then setup
								the buffer configuration so it contains multiple buffers. Using multiple simultaneous play requests,
								play 4 seconds of continuous tone - with each individual play request consisting of 1/8th second of
								tone. (Ensure the last request is marked with the KSndFlagLastSample flag).
							2)	Repeat the above with the driver configured for stereo operation (i.e. 2 audio channels).
	@SYMTestExpectedResults	1)	For each of the sample rates supported by the device, the driver should successfully play
								4 seconds of tone, with no interruptions in the sound produced and with all requests
								completing with KErrNone.
							2)	For each of the sample rates supported by the device, the driver should successfully play
								4 seconds of tone, with no interruptions in the sound produced and with all requests
								completing with KErrNone.
	@SYMREQ					PREQ1073.4
*/
LOCAL_C void TestPlayAllRates(TInt aNumChannels,TInt aNumSeconds)
	{
	TRequestStatus stat[2];
	TPtr8* tPtr[2];
	TInt i;
	for (i=0;i<2;i++)
		tPtr[i]=new TPtr8(NULL,0);

	Test.Next(_L("Play all rates test"));
	Test.Printf(_L("Number of channels %d, duration %d seconds\n"), aNumChannels, aNumSeconds);

	if (PlayCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	PlayFormatBuf().iChannels = aNumChannels;
	TInt r=MakeSineTable(PlayFormatBuf());
	CHECK_NOERROR(r);

	TxSoundDevice.SetVolume(KSoundMaxVolume);

	RChunk chunk;
	TInt speed = 0;
	while (speedTable[speed]>=0)
		{
		PlayFormatBuf().iRate = speedTable[speed++];
		PlayFormatBuf().iChannels = aNumChannels;

		// Set the play format.
		Test.Printf(_L("Testing playback rate %d...\r\n"),RateInSamplesPerSecond(PlayFormatBuf().iRate));
		r = TxSoundDevice.SetAudioFormat(PlayFormatBuf);
		if (r==KErrNotSupported)
			{
			Test.Printf(_L("Sample rate not supported\r\n"));
			continue;
			}
		CHECK_NOERROR(r);

		// Set the play buffer configuration, then read it back.
		TInt bufSize=BytesPerSecond(PlayFormatBuf())/4; 	 								// Large enough to hold 1/4th second of data.
		bufSize=ValidBufferSize(bufSize,PlayCapsBuf().iRequestMinSize,PlayFormatBuf());		// Keep the buffer length valid for driver.
		TTestSharedChunkBufConfig bufferConfig;
		bufferConfig.iNumBuffers=2;
		bufferConfig.iBufferSizeInBytes=bufSize;
		bufferConfig.iFlags=0;																// All buffers will be contiguous
		TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
		r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
		CHECK_NOERROR(r);
		TxSoundDevice.GetBufferConfig(bufferConfigBuf);
		PrintBufferConf(bufferConfig,Test);
		CHECK(bufferConfig.iBufferSizeInBytes==bufSize);
		tPtr[0]->Set(chunk.Base()+bufferConfig.iBufferOffsetList[0],0,bufSize);
		tPtr[1]->Set(chunk.Base()+bufferConfig.iBufferOffsetList[1],0,bufSize);

		r=SetToneFrequency(440,PlayFormatBuf());
		CHECK_NOERROR(r);
		TxSoundDevice.ResetBytesTransferred();
		CHECK_EQUAL(TxSoundDevice.BytesTransferred(),0);

		// Issue a pair of play requests.
		WriteTone(*tPtr[0],PlayFormatBuf());
		TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize);
		WriteTone(*tPtr[1],PlayFormatBuf());
		TxSoundDevice.PlayData(stat[1],bufferConfig.iBufferOffsetList[1],bufSize);

		TInt remainingPlayCount = BytesPerSecond(PlayFormatBuf())*aNumSeconds/bufSize;
		TInt bytesToPlay = remainingPlayCount*bufSize;
		TInt bytesPlayed = 0;
		TInt i;
		TUint flags;
		while(remainingPlayCount>2)
			{
			// Wait for either of the outstanding play requests to complete.
			User::WaitForAnyRequest();
			remainingPlayCount--;

			// Work out which request this applies to.
			for (i=0;i<2;i++)
				{
				if (stat[i]!=KRequestPending)
					break;
				}
			CHECK(i<2);
			CHECK_NOERROR(stat[i].Int());

			WriteTone(*tPtr[i],PlayFormatBuf());
			flags=(remainingPlayCount<=2)?KSndFlagLastSample:0;
			TxSoundDevice.PlayData(stat[i],bufferConfig.iBufferOffsetList[i],bufSize,flags);
			}

		// Last 2 play requests still outstanding.
		User::WaitForRequest(stat[0]);
		CHECK_NOERROR(stat[0].Int());
		User::WaitForRequest(stat[1]);
		CHECK_NOERROR(stat[1].Int());

		Test.Printf(_L("Sample rate successful\r\n"));
		bytesPlayed = TxSoundDevice.BytesTransferred();
		CHECK_EQUAL(bytesToPlay,bytesPlayed);
		chunk.Close();
		}

	for (i=0;i<2;i++)
		delete tPtr[i];
	}

/**	@SYMTestCaseID 			PBASE-T_SOUND2-254
	@SYMTestCaseDesc 		Record operation - recording all rates.
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	For each of the sample rates supported by the device, setup the audio configuration on
								the record channel for mono operation (i.e. 1 audio channel) and then setup the buffer
								configuration so it contains multiple buffers. Using multiple simultaneous record
								requests, record 4 seconds of audio data - with each individual record request being
								for 1/8th second of data.
							2)	Repeat the above with the driver configured for stereo operation (i.e. 2 audio channels).
	@SYMTestExpectedResults	1)	For each of the sample rates supported by the device, the driver should successfully
							record 4 seconds of data, with all requests completing with KErrNone.
							2)	For each of the sample rates supported by the device, the driver should successfully
							record 4 seconds of data, with all requests completing with KErrNone
	@SYMREQ					PREQ1073.4
*/
LOCAL_C void TestRecordAllRates(TInt aNumChannels,TInt aNumSeconds)
	{

	TRequestStatus stat[2];
	TInt length[2];

	Test.Next(_L("Record all rate test"));
	Test.Printf(_L("Number of channels %d, duration %d seconds\n"), aNumChannels, aNumSeconds);

	if (RecordCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;

	RChunk chunk;
	TInt speed = 0;
	while (speedTable[speed]>=0)
		{
		RecordFormatBuf().iRate = speedTable[speed++];
		RecordFormatBuf().iChannels = aNumChannels;

		// Set the record format.
		Test.Printf(_L("Testing record rate %d...\r\n"),RateInSamplesPerSecond(RecordFormatBuf().iRate));
		TInt r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
		if (r==KErrNotSupported)
			{
			Test.Printf(_L("Sample rate not supported\r\n"));
			continue;
			}
		CHECK_NOERROR(r);

		// Set the record buffer configuration, then read it back.
		TInt bufSize=BytesPerSecond(RecordFormatBuf())/8; 									// Large enough to hold 1/8th second of data.
		bufSize=ValidBufferSize(bufSize,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());	// Keep the buffer length valid for driver.
		TTestSharedChunkBufConfig bufferConfig;
		bufferConfig.iNumBuffers=4;
		bufferConfig.iBufferSizeInBytes=bufSize;
		bufferConfig.iFlags=0;																// All buffers will be contiguous
		TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
		r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
		CHECK_NOERROR(r);
		RxSoundDevice.GetBufferConfig(bufferConfigBuf);
		PrintBufferConf(bufferConfig,Test);
		CHECK(bufferConfig.iBufferSizeInBytes==bufSize);

		TInt remainingRecordCount = BytesPerSecond(RecordFormatBuf())*aNumSeconds/bufSize;
		TInt bytesToRecord = remainingRecordCount*bufSize;
		TInt bytesRecorded = 0;

		// The driver rounds up the buffer size to the nearest page meaning the total duration
		// to complete this test won't be exactly equal to 'aNumSeconds' anymore. Hence, the
		// predicted time is no longer simply: aNumSeconds * 1000000.
		TInt64 predictedTime = (bufSize * remainingRecordCount);
		predictedTime*=1000000;
		predictedTime+=BytesPerSecond(RecordFormatBuf())>>1;
		predictedTime/=BytesPerSecond(RecordFormatBuf());
		TTime starttime;
		starttime.HomeTime();

		// Issue a pair of record requests.
		TInt vol = I64LOW(TInt64(KSoundMaxVolume)*TInt64(bytesRecorded)/TInt64(bytesToRecord));
		RxSoundDevice.SetVolume(vol);
		RxSoundDevice.RecordData(stat[0],length[0]);
		RxSoundDevice.RecordData(stat[1],length[1]);
		TInt currentReq=0;

		TInt retOffset;
		do
			{
			// Wait for the next expected request to complete.
			User::WaitForRequest(stat[currentReq]);
			remainingRecordCount--;
			retOffset=stat[currentReq].Int();
			CHECK_POSITIVE(retOffset);

			CHECK(length[currentReq]>0);
			bytesRecorded += length[currentReq];

			r=RxSoundDevice.ReleaseBuffer(retOffset);
			CHECK_NOERROR(r);
			CHECK_EQUAL(RxSoundDevice.Volume(),vol);
			CHECK_EQUAL(RxSoundDevice.SetAudioFormat(RecordFormatBuf),KErrInUse);

			vol = I64LOW(TInt64(KSoundMaxVolume)*TInt64(bytesRecorded)/TInt64(bytesToRecord));
			RxSoundDevice.SetVolume(vol);

			// Don't issue any further record requests on the last two loop passes - to allow for the
			// two record requests made before the loop started.
			if (remainingRecordCount>=2)
				RxSoundDevice.RecordData(stat[currentReq],length[currentReq]);

			currentReq^=0x01;	// Toggle the current req. indicator
			}
		while(remainingRecordCount>0);

		TTime endtime;
		endtime.HomeTime();
		TInt64 elapsedTime = endtime.Int64()-starttime.Int64();	// us
		Test.Printf(_L("Recorded %d bytes in %d us\n"),bytesRecorded, I64LOW(elapsedTime));
		if (elapsedTime < predictedTime)
			{
			Test.Printf(_L("**** FAIL: time travelling; record took less time than it could have done\n"));
			// CHECK_NOERROR(1);
			}
		CHECK_EQUAL(bytesToRecord,bytesRecorded);
		Test.Printf(_L("Sample rate successful\r\n"));

		RxSoundDevice.CancelRecordData();	// Stop the driver from recording.
		chunk.Close();
		}
	}

/**	@SYMTestCaseID 			PBASE-T_SOUND2-253
	@SYMTestCaseDesc 		Record operation - altering the record level.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the record channel and then setup the buffer configuration
							so it contains multiple buffers. Using multiple simultaneous record requests, record 10
							seconds of audio data - with each individual record request being for 1/8th second of data.
							Each time a request completes, increase the record level slightly - starting at the minimum
							and ending at maximum record level.
	@SYMTestExpectedResults	The driver should successfully record 10 seconds of data - i.e. all requests should complete
							with KErrNone.
	@SYMREQ					PREQ1073.4
*/
LOCAL_C void TestRecordVolume(TInt aNumChannels,TInt aNumSeconds)
	{
	TRequestStatus stat[2];
	TInt length[2];
	TInt r, i;

	Test.Next(_L("Preparing to test variable record levels..."));

	if (RecordCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;

	RecordFormatBuf().iChannels = aNumChannels;

	// find first supported rate and set the the audio configuration to use it
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		RecordFormatBuf().iRate = (TSoundRate)i;
		r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
		if (RecordCapsBuf().iRates & (1<<i))
			{
			CHECK_NOERROR(r);				// Caps reports it is supported
			break;
			}
		}
	PrintConfig(RecordFormatBuf(),Test);

	// Set the record buffer configuration, then read it back.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(RecordFormatBuf())/8; 									// Large enough to hold 1/8th second of data.
	bufSize=ValidBufferSize(bufSize,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());	// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=8;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;																// All buffers will be contiguous
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);

	Test.Next(_L("Recording..."));
	TInt remainingRecordCount = BytesPerSecond(RecordFormatBuf())*aNumSeconds/bufSize;
	TInt bytesToRecord = remainingRecordCount*bufSize;
	TInt bytesRecorded = 0;

	// Issue a pair of record requests.
	RxSoundDevice.SetVolume(KSoundMaxVolume);
	RxSoundDevice.RecordData(stat[0],length[0]);
	RxSoundDevice.RecordData(stat[1],length[1]);
	TInt currentReq=0;

	TInt retOffset;
	do
		{
		// Adjust the record level.
		TInt vol = I64LOW(TInt64(KSoundMaxVolume)*TInt64(bytesRecorded)/TInt64(bytesToRecord));
		r=RxSoundDevice.SetVolume(vol);
		CHECK_NOERROR(r);

		// Wait for the next expected request to complete.
		User::WaitForRequest(stat[currentReq]);
		remainingRecordCount--;
		retOffset=stat[currentReq].Int();
		CHECK_POSITIVE(retOffset);

		// Check the length recorded and update bytes recorded.
		CHECK(length[currentReq]>0);
		bytesRecorded += length[currentReq];
		Test.Printf(_L("."));

		// Read back the record level / check we can't reconfig while recording.
		CHECK_EQUAL(RxSoundDevice.Volume(),vol);
		CHECK_EQUAL(RxSoundDevice.SetAudioFormat(RecordFormatBuf),KErrInUse);

		// Now release the buffer and issue another record request.
		r=RxSoundDevice.ReleaseBuffer(retOffset);
		CHECK_NOERROR(r);
		// Don't issue any further record requests on the last two loop passes - to allow for the
		// two record requests made before the loop started.
		if (remainingRecordCount>=2)
			RxSoundDevice.RecordData(stat[currentReq],length[currentReq]);

		currentReq^=0x01;	// Toggle the current req. indicator
		}
	while(remainingRecordCount>0);

	CHECK_EQUAL(bytesToRecord,bytesRecorded);

	RxSoundDevice.CancelRecordData();	// Stop the driver from recording.
	Test.Printf(_L("\nBytes recorded = %d\r\n"),bytesRecorded);
	chunk.Close();
	}

/**	@SYMTestCaseID 			PBASE-T_SOUND2-245
	@SYMTestCaseDesc 		Play operation - play cancellation.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the playback channel and then setup the buffer configuration
							so it contains two buffers.
							1)	Issue two simultaneous play requests, one from each buffer, each of 1/2 second of tone.
								Wait for the first one to complete and issue a further play request from the same buffer.
								Then immediately cancel all outstanding play requests (using CancelPlayData()).
							2)	Issue two simultaneous play requests, one from each buffer, each of 1/2 second of tone.
								Wait for the first one to complete and issue a further play request from the same buffer.
								Then immediately cancel the 2nd (i.e. now active) play request (using Cancel()).
	@SYMTestExpectedResults	1)	Both outstanding requests should complete, either with KErrNone or with KErrCancel.
							2)	The second request should complete, either with KErrNone or with KErrCancel whereas the
								third should complete only with KErrNone.
	@SYMREQ					PREQ1073.4
*/
LOCAL_C void TestPlayCancel()
	{
	TRequestStatus stat[2];
	TPtr8* tPtr[2];
	TInt i, r;
	for (i=0;i<2;i++)
		tPtr[i]=new TPtr8(NULL,0);

	Test.Next(_L("Test play cancellation"));

	if (PlayCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	PlayFormatBuf().iChannels = 2;

	// find first supported rate and set the the audio configuration to use it
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		// check record channel
		PlayFormatBuf().iRate = (TSoundRate)i;
		r = TxSoundDevice.SetAudioFormat(PlayFormatBuf);
		if (PlayCapsBuf().iRates & (1 << i))
			{
			CHECK_NOERROR(r);		// Caps reports it is supported
			break;
			}
		}
	PrintConfig(PlayFormatBuf(),Test);
	r=MakeSineTable(PlayFormatBuf());
	CHECK_NOERROR(r);

	// Set the play buffer configuration, then read it back.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(PlayFormatBuf())/2; 									// Large enough to hold 1/2 second of data.
	bufSize=ValidBufferSize(bufSize,PlayCapsBuf().iRequestMinSize,PlayFormatBuf());		// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=2;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;																// All buffers will be contiguous
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	TxSoundDevice.GetBufferConfig(bufferConfigBuf);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);
	tPtr[0]->Set(chunk.Base()+bufferConfig.iBufferOffsetList[0],0,bufSize);
	tPtr[1]->Set(chunk.Base()+bufferConfig.iBufferOffsetList[1],0,bufSize);

	Test.Next(_L("Test cancelling all outstanding requests"));
	// Issue a pair of play requests.
	r=SetToneFrequency(440,PlayFormatBuf());
	CHECK_NOERROR(r);
	WriteTone(*tPtr[0],PlayFormatBuf());
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize);
	WriteTone(*tPtr[1],PlayFormatBuf());
	TxSoundDevice.PlayData(stat[1],bufferConfig.iBufferOffsetList[1],bufSize);

	// Wait for the 1st request to complete. Then, re-queue a further request but then
	// immediately cancel both requests.
	User::WaitForRequest(stat[0]);
	CHECK_NOERROR(stat[0].Int());
	WriteTone(*tPtr[0],PlayFormatBuf());
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize);
	TxSoundDevice.CancelPlayData();

	User::WaitForRequest(stat[1]);
	if (stat[1]==KErrNone)
		Test.Printf(_L("Note: 2nd request finished without cancel error\r\n"));
	else
		CHECK_EQUAL(stat[1].Int(),KErrCancel);
	User::WaitForRequest(stat[0]);
	if (stat[0]==KErrNone)
		Test.Printf(_L("Note: 3rd request finished without cancel error\r\n"));
	else
		CHECK_EQUAL(stat[0].Int(),KErrCancel);

	Test.Next(_L("Test cancelling an individual requests"));
	// Issue a further pair of play requests.
	r=SetToneFrequency(440,PlayFormatBuf());
	CHECK_NOERROR(r);
	WriteTone(*tPtr[0],PlayFormatBuf());
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize,0);
	WriteTone(*tPtr[1],PlayFormatBuf());
	TxSoundDevice.PlayData(stat[1],bufferConfig.iBufferOffsetList[1],bufSize,0);

	// Again, wait for the 1st request to complete. Then, re-queue a further request but then
	// immediately cancel the 2nd request.
	User::WaitForRequest(stat[0]);
	CHECK_NOERROR(stat[0].Int());
	WriteTone(*tPtr[0],PlayFormatBuf());
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize,KSndFlagLastSample);
	TxSoundDevice.Cancel(stat[1]);

	User::WaitForRequest(stat[1]);
	if (stat[1]==KErrNone)
		Test.Printf(_L("Note: 2nd request finished without cancel error\r\n"));
	else
		CHECK_EQUAL(stat[1].Int(),KErrCancel);
	User::WaitForRequest(stat[0]);
	CHECK_NOERROR(stat[0].Int());

	chunk.Close();
	Test.Printf(_L("Cancel play test completed successful\r\n"));

	for (i=0;i<2;i++)
		delete tPtr[i];
	}

/**	@SYMTestCaseID 			PBASE-T_SOUND2-259
	@SYMTestCaseDesc 		Record operation - record cancellation.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the record channel and then setup the buffer configuration
							so it contains multiple buffers.
							1)	Issue two simultaneous record requests requests, each for 1/2 second of data. Wait for
								the first one to complete and issue a further record request. Then immediately cancel all
								outstanding record requests (using CancelRecordData()).
							2)	Issue two simultaneous record requests, each for 1/2 second of data. Wait for the first
								one to complete and issue a further record request. Then immediately cancel the 2nd (i.e.
								now active) record request (using Cancel()).
	@SYMTestExpectedResults	1)	Both outstanding requests should complete, either with KErrNone or with KErrCancel.
							2)	The second requests should complete, either with KErrNone or with KErrCancel whereas the
								third should complete only with KErrNone.
	@SYMREQ					PREQ1073.4
*/
LOCAL_C void TestRecordCancel()
	{
	TRequestStatus stat[2];
	TInt length[2];
	TPtr8* tPtr[2];
	TInt i, r;
	for (i=0;i<2;i++)
		tPtr[i]=new TPtr8(NULL,0);

	Test.Next(_L("Test record cancellation"));

	if (RecordCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	RecordFormatBuf().iChannels = 2;

	// find first supported rate and set the the audio configuration to use it
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		RecordFormatBuf().iRate = (TSoundRate)i;
		r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
		if (RecordCapsBuf().iRates & (1<<i))
			{
			CHECK_NOERROR(r);				// Caps reports it is supported
			break;
			}
		}

	PrintConfig(RecordFormatBuf(),Test);

	// Set the record buffer configuration, then read it back.
	RChunk chunk;
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=3;
	bufferConfig.iBufferSizeInBytes=BytesPerSecond(RecordFormatBuf())/2; // Large enough to hold 1/2 second of data.
	bufferConfig.iBufferSizeInBytes=ValidBufferSize(bufferConfig.iBufferSizeInBytes,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());		// Keep the buffer length valid for driver.
	bufferConfig.iFlags=0;		// All buffers will be contiguous
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);

	Test.Next(_L("Test cancelling all outstanding requests"));
	// Issue a pair of record requests.
	RxSoundDevice.RecordData(stat[0],length[0]);
	RxSoundDevice.RecordData(stat[1],length[1]);

	// Wait for the 1st request to complete. Then, re-queue a further request but then
	// immediately cancel both requests.
	TInt retOffset;
	User::WaitForRequest(stat[0]);
	retOffset=stat[0].Int();
	CHECK_POSITIVE(retOffset);
	CHECK(length[0]>0);
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);
	RxSoundDevice.RecordData(stat[0],length[0]);
	RxSoundDevice.CancelRecordData();

	User::WaitForRequest(stat[1]);
	retOffset=stat[1].Int();
	if (retOffset>=0)
		Test.Printf(_L("Note: 2nd request finished without cancel error\r\n"));
	else
		CHECK_EQUAL(retOffset,KErrCancel);
	User::WaitForRequest(stat[0]);
	retOffset=stat[0].Int();
	if (retOffset>=0)
		Test.Printf(_L("Note: 3rd request finished without cancel error\r\n"));
	else
		CHECK_EQUAL(retOffset,KErrCancel);

	Test.Next(_L("Test cancelling an individual requests"));
	// Issue a further pair of record requests.
	RxSoundDevice.RecordData(stat[0],length[0]);
	RxSoundDevice.RecordData(stat[1],length[1]);

	// Again, wait for the 1st request to complete. Then, re-queue a further request but then
	// immediately cancel the 2nd request.
	User::WaitForRequest(stat[0]);
	retOffset=stat[0].Int();
	CHECK_POSITIVE(retOffset);
	CHECK(length[0]>0);
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);
	RxSoundDevice.RecordData(stat[0],length[0]);
	RxSoundDevice.Cancel(stat[1]);

	User::WaitForRequest(stat[1]);
	retOffset=stat[1].Int();
	if (retOffset>=0)
		Test.Printf(_L("Note: 2nd request finished without cancel error\r\n"));
	else
		CHECK_EQUAL(retOffset,KErrCancel);
	User::WaitForRequest(stat[0]);
	retOffset=stat[0].Int();
	CHECK_POSITIVE(retOffset);
	CHECK(length[0]>0);

	RxSoundDevice.CancelRecordData();	// Stop the driver from recording.
	chunk.Close();
	Test.Printf(_L("Cancel record test completed successful\r\n"));

	for (i=0;i<2;i++)
		delete tPtr[i];
	}

/**	@SYMTestCaseID 			PBASE-T_SOUND2-262
	@SYMTestCaseDesc 		Play pause / resume - pausing and resuming before playback has commenced.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the record channel and then setup the buffer configuration.
							1)	Attempt to resume recording before recording has been started.
							2)	Attempt to pause recording before recording has been started.
	@SYMTestExpectedResults	1)	The resume request should complete with KErrNotReady.
							2)	The pause request should complete with KErrNotReady.
	@SYMREQ					PREQ1073.4
*/

/**	@SYMTestCaseID 			PBASE-T_SOUND2-263
	@SYMTestCaseDesc 		Record pause / resume - pausing and resuming while recording is in progress.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the record channel and then setup the buffer configuration
							so it contains multiple buffers. For the audio configuration selected, calculate the total
							number of bytes expected to be transferred in order record 4 seconds of data. Using multiple
							simultaneous record requests, record 4 seconds of audio data - with each individual record
							request being for 1/8th second of data. Increment a count of actual total bytes transferred
							by examining the count of bytes stored in the buffer for each request as it completes.
							1) After 10 requests have completed, pause transfer for 1 second, then resume it. If pausing
							causes a record request to complete with a shorter than requested length then reduce the
							count of expected total bytes transferred.
							2) Repeat step 1 when 20 requests have completed.
							3) Once transfer has completed, compare the counts of expected and actual total bytes
							transferred.
	@SYMTestExpectedResults	1)	The pause and resume requests should both complete with KErrNone.
							2)	The pause and resume requests should both complete with KErrNone.
							3)	The counts should be equal.
	@SYMREQ					PREQ1073.4
*/
LOCAL_C void TestRecordPauseResume(TUint aChannels)
	{
	TRequestStatus stat[2];
	TInt length[2];

	Test.Next(_L("Test record pause and resume"));
	RecordFormatBuf().iRate = ESoundRate44100Hz;
	if (RecordCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	RecordFormatBuf().iChannels = aChannels;
	PrintConfig(RecordFormatBuf(),Test);
	TInt r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
	CHECK_NOERROR(r);

	// Set the record buffer configuration, then read it back.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(RecordFormatBuf())/8; 									// Large enough to hold 1/8th second of data.
	bufSize=ValidBufferSize(bufSize,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());	// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=8;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;																// All buffers will be contiguous
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);

	Test.Printf(_L("Resume when not recording\r\n"));
	r=RxSoundDevice.Resume();
	CHECK(r==KErrNotReady)

	Test.Printf(_L("Pause when not recording\r\n"));
	r=RxSoundDevice.Pause();
	CHECK(r==KErrNotReady)

	// Record for 4 seconds
	Test.Printf(_L("Record...\r\n"));
	TInt remainingRecordCount = BytesPerSecond(RecordFormatBuf())*4/bufSize;
	TInt bytesToRecord = remainingRecordCount*bufSize;
	TInt bytesRecorded = 0;

	// Issue a pair of record requests.
	RxSoundDevice.SetVolume(KSoundMaxVolume);
	length[0] = -42;
	RxSoundDevice.RecordData(stat[0],length[0]);
	length[1] = -42;
	RxSoundDevice.RecordData(stat[1],length[1]);
	TInt currentReq=0;

	TInt lcount = 0;
	TInt retOffset;
	do
		{
		// Do a pause / resume on 10th and 20th loop passes.
		if (lcount && !(lcount%10))
			{
			// Do a pause/resume
			Test.Printf(_L("Pause 1 second\r\n"));
			r=RxSoundDevice.Pause();
			CHECK_NOERROR(r);

			// Pausing record may result in the driver completing with a buffer not completely full. This isn't an error. Otherwise, all outstanding
			// requests should complete with KErrCancel. Wait for the 1st outstanding request to complete.
			User::WaitForRequest(stat[currentReq]);
			retOffset=stat[currentReq].Int();
			if (retOffset>=0)
				{
				// Partially filled buffer. We need to adjust the bytes expected when an incomplete buffer arrives.
				remainingRecordCount--;

				CHECK_POSITIVE(length[currentReq]);
				CHECK(length[currentReq]<=bufSize);
				bytesRecorded += length[currentReq];
				if (length[currentReq]<bufSize)
					bytesToRecord-=(bufSize-length[currentReq]);
				Test.Printf(_L("1st outstanding req partially completed(len=%d)\r\n"),length[currentReq]);

				r=RxSoundDevice.ReleaseBuffer(retOffset); // Release the buffer ready for resuming
				CHECK_NOERROR(r);
				}
			else
				{
				CHECK(retOffset==KErrCancel);
				Test.Printf(_L("1st outstanding req cancelled\r\n"));
				}
			currentReq^=0x01;	// Toggle the current req. indicator

			// Wait for the 2nd outstanding request to complete
			User::WaitForRequest(stat[currentReq]);
			retOffset=stat[currentReq].Int();
			CHECK(retOffset==KErrCancel);
			Test.Printf(_L("2nd outstanding req cancelled\r\n"));

			// Idle for 1 second, resume and then re-issue a pair of record requests.
			User::After(1000000);
			Test.Printf(_L("Resume\r\n"));
			r=RxSoundDevice.Resume();
			CHECK_NOERROR(r);
			RxSoundDevice.RecordData(stat[0],length[0]);
			RxSoundDevice.RecordData(stat[1],length[1]);
			currentReq=0;
			}

		// Wait for the next expected request to complete.
		User::WaitForRequest(stat[currentReq]);
		remainingRecordCount--;
		retOffset=stat[currentReq].Int();
		CHECK_POSITIVE(retOffset);
		CHECK(length[currentReq]>0);
		bytesRecorded += length[currentReq];

		// Now release the buffer and issue another record request
		r=RxSoundDevice.ReleaseBuffer(retOffset);
		CHECK_NOERROR(r);
		// Don't issue any further record requests on the last two loop passes - to allow for the
		// two record requests made before the loop started.
		if (remainingRecordCount>=2)
			RxSoundDevice.RecordData(stat[currentReq],length[currentReq]);
		lcount++;
		currentReq^=0x01;	// Toggle the current req. indicator
		}
	while(remainingRecordCount>0);

	CHECK_EQUAL(bytesToRecord,bytesRecorded);
	RxSoundDevice.CancelRecordData();	// Stop the driver from recording.
	Test.Printf(_L("Record pause/resume successful\r\n"));

	Test.Next(_L("Test record pause alone"));

	// Issue a single record request, wait for it to complete and then release it.
	RxSoundDevice.RecordData(stat[0],length[0]);
	User::WaitForRequest(stat[0]);
	retOffset=stat[0].Int();
	CHECK_POSITIVE(retOffset);
	CHECK(length[0]==bufSize);
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);

	// Without issuing another record request, wait for a duration equal to one record buffer, then pause.
	User::After(150000);	// Wait a bit longer than 1 buffer's worth (125000 is 1/8 second).
	Test.Printf(_L("Pause\r\n"));
	r=RxSoundDevice.Pause();
	CHECK_NOERROR(r);

	// Check that there is at least 1 buffer's worth of record data available
	RxSoundDevice.RecordData(stat[0],length[0]);
	User::WaitForRequest(stat[0]);
	retOffset=stat[0].Int();
	CHECK_POSITIVE(retOffset);
	CHECK(length[0]==bufSize);
	Test.Printf(_L("1st req completed successfully\r\n"));
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);

	// There's probably also a partially filled buffer
	RxSoundDevice.RecordData(stat[0],length[0]);
	User::WaitForRequest(stat[0]);
	retOffset=stat[0].Int();
	if (retOffset>=0)
		{
		// Partially filled buffer.
		CHECK(length[0]>0);
		CHECK(length[0] <= bufSize);
		Test.Printf(_L("2nd req partially completed(len=%d)\r\n"),length[0]);
		r=RxSoundDevice.ReleaseBuffer(retOffset);
		CHECK_NOERROR(r);
		}
	else
		{
		CHECK(retOffset==KErrCancel);
		Test.Printf(_L("2nd req cancelled\r\n"));
		}

	for(;;)
		{
		// Read all buffers until driver is empty. The RecordData call after that should immediately return with KErrCancel
		Test.Printf(_L("Draining driver\r\n"));
		RxSoundDevice.RecordData(stat[0],length[0]);
		User::WaitForRequest(stat[0]);
		retOffset=stat[0].Int();
		if(retOffset==KErrCancel)
			{
			break;
			}
		CHECK_NOERROR(retOffset);
		}
	Test.Printf(_L("Driver empty\r\n"));

	r=RxSoundDevice.Resume();			// Don't leave it in paused state.
	CHECK_NOERROR(r);
	RxSoundDevice.CancelRecordData();	// Stop the driver from recording.
	Test.Printf(_L("Record pause successful\r\n"));

	chunk.Close();
	}

template <class T>
class RQueue
    {
public:
    RQueue() 
        : iArray() 
        { }
    void Close() 
        { 
        iArray.Close(); 
        }
    TInt Count() const 
        { 
        return iArray.Count(); 
        }
    void PushL(const T &aItem) 
        {
        iArray.AppendL(aItem); 
        }
    T PopL()
        {
        if(iArray.Count() == 0)
            {
            User::Leave(KErrUnderflow);
            }
        const T ret = iArray[0];
        iArray.Remove(0);
        return ret;
        }
private:
    RArray<T> iArray;
    };
/**	@SYMTestCaseID 			PBASE-T_SOUND2-248
	@SYMTestCaseDesc 		Play operation - simultaneous play and record on the same device, using a common shared chunk.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the record channel and setup an identical audio
							configuration on the playback channel. On the record channel, create a shared chunk
							(i.e. using SetBufferChunkCreate()) with a buffer configuration containing multiple buffers.
							Open the same shared chunk on the playback channel (i.e. using SetBufferChunkOpen()).
							Set the volume to maximum level in both channels. Record 10 seconds of audio data - with
							each individual record request being for 1/8th second of data. As soon as each record request
							completes, issue a corresponding request on the playback channel to playback the
							1/8th second of data recorded. Only release each buffer for further recording once its
							contents have been played back. Continue until 10 seconds of audio data has been both
							recorded and played back. (Ensure the last play request is marked with the
							KSndFlagLastSample flag).
	@SYMTestExpectedResults	The driver should successfully record and play 10 seconds of data - with all requests
							completing with KErrNone.
	@SYMREQ					PREQ1073.4
*/
void TestSimultaneousPlayRecord()
	{
	Test.Next(_L("Preparing to record/play simultaneously..."));
	TInt r = KErrNone;
	TInt i;
	// Setup the same sound configuration for both - record and play channels
	if (RecordCapsBuf().iEncodings & KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;

	if (PlayCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;

	RecordFormatBuf().iChannels = 2;
	PlayFormatBuf().iChannels = 2;

	// find first supported rate and set the the audio configuration to use it
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		// check record channel
		RecordFormatBuf().iRate = (TSoundRate)i;
		r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
		if (RecordCapsBuf().iRates & (1 << i))
			{
			CHECK_NOERROR(r);		// Caps reports it is supported

			// ..and try the same bitrate for playback
			PlayFormatBuf().iRate = (TSoundRate)i;
			r = TxSoundDevice.SetAudioFormat(PlayFormatBuf);
			if (PlayCapsBuf().iRates & (1 << i))
				{
				CHECK_NOERROR(r);		// Caps reports it is supported
				break;
				}
			}
		}

	// both channels are set at this point, continue
	PrintConfig(RecordFormatBuf(),Test);
	PrintConfig(PlayFormatBuf(),Test);

	// Set the volume level in both channels
	RxSoundDevice.SetVolume(KSoundMaxVolume);
	TxSoundDevice.SetVolume(KSoundMaxVolume - 10);

	// Set the record buffer configuration, then read it back.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(RecordFormatBuf())/8; 									// Large enough to hold 1/8th second of data.
	bufSize=ValidBufferSize(bufSize,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());	// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=7+7;														// Must be able to use less than this
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;																// All buffers will be contiguous
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);

	// Assign the same chunk to the play channel.
	r=TxSoundDevice.SetBufferChunkOpen(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);

	Test.Next(_L("Starting transfer..."));
	TInt remainingRecordCount = BytesPerSecond(RecordFormatBuf())*10/bufSize; // 10 seconds
	TInt remainingPlayCount = remainingRecordCount;
	TInt bytesToTransfer = remainingRecordCount*bufSize;
	TInt bytesRecorded = 0;
	TInt bytesPlayed = 0;

	TRequestStatus stat[3]; 	// 1 record + 2 play request statuses
	TInt length;
	TInt activePlayOffset[2];	// To keep track of the buffer offset for each active play request.
	RQueue<TInt> playQueue; // A list containing the offsets for any buffer which is waiting to be played.

	// Issue three record requests and wait for them to complete.
	TInt retOffset;
	for (i=0 ; i<3 ; i++)
		{
		RxSoundDevice.RecordData(stat[2],length);
		User::WaitForRequest(stat[2]);
		retOffset=stat[2].Int();
//		Test.Printf(_L("RECORD(%d)-Buf %d\r\n"),i,retOffset);
		CHECK_POSITIVE(retOffset);
		CHECK(length==bufSize);
		bytesRecorded += length;
		playQueue.PushL(retOffset);
		remainingRecordCount--;
		Test.Printf(_L("."));
		}

	// Start playing first two buffers
	TUint flags=0;
	activePlayOffset[0]=playQueue.PopL();
	TxSoundDevice.PlayData(stat[0],activePlayOffset[0],bufSize,flags);
	activePlayOffset[1]=playQueue.PopL();
	TxSoundDevice.PlayData(stat[1],activePlayOffset[1],bufSize,flags);

	// Now queue the next record request.
	RxSoundDevice.RecordData(stat[2],length);

	do
		{
		// Wait for the next request to complete.
		User::WaitForAnyRequest();

		// Work out which request this applies to.
		for (i=0;i<3;i++)
			{
			if (stat[i]!=KRequestPending)
				break;
			}
		CHECK(i<3);

		if (i==2)
			{
			// It is the record request that has completed
			remainingRecordCount--;
			retOffset=stat[2].Int();
//			Test.Printf(_L("RECORD(%d)-Buf %d\r\n"),remainingRecordCount,retOffset);
			CHECK_POSITIVE(retOffset);
			CHECK(length==bufSize);
			bytesRecorded += length;
			Test.Printf(_L("."));

			// Add the buffer to playQueue
			playQueue.PushL(retOffset);

			// If we haven't recorded enough data yet then record some more.
			if (remainingRecordCount>0)
				RxSoundDevice.RecordData(stat[2],length);
			else
				{
				Test.Printf(_L("***Disabling stat[2]\r\n"));
				stat[2]=KRequestPending;
				}
			}
		else
			{
			// Its one of the play requests that have completed
			if(stat[i].Int() >= 0)
				{
				// release the buffer.
//				Test.Printf(_L("PLAY(%d) i%d CompBuf %d\r\n"),remainingPlayCount-1,i,activePlayOffset[i]);
				r=RxSoundDevice.ReleaseBuffer(activePlayOffset[i]);
				CHECK_NOERROR(r);
				Test.Printf(_L("*"));
				}
			else
				{
				// Play failed - but we ignore underflow because it often happens on WDP roms.
				CHECK(stat[i].Int() == KErrUnderflow);
				Test.Printf(_L("U"));
				}

			remainingPlayCount--;
			bytesPlayed += bufSize;

			// If there are buffers available then issue a further play request and update the 'next to play' list.
			if (playQueue.Count() != 0)
				{
				activePlayOffset[i]=playQueue.PopL();

//				Test.Printf(_L("PLAY(%d) i%d NextBuf%d\r\n"),remainingPlayCount,i,activePlayOffset[i]);
				flags=(remainingPlayCount<=2)?KSndFlagLastSample:0;
				TxSoundDevice.PlayData(stat[i],activePlayOffset[i],bufSize,flags);
				}
			else
				{
				Test.Printf(_L("***Disabling stat[%d]\r\n"), i, stat[i].Int());
				stat[i]=KRequestPending;
				}
			}
		}
	while (remainingRecordCount>0 || remainingPlayCount>0);
	playQueue.Close();
	CHECK_EQUAL(bytesToTransfer,bytesRecorded);
	CHECK_EQUAL(bytesToTransfer,bytesPlayed);

	RxSoundDevice.CancelRecordData();	// Stop the driver from recording.
	chunk.Close();

	Test.Printf(_L("\nSimultaneous test ends\r\n"));
	return;
	}

void TestSpeed()
	{
	Test.Next(_L("Preparing to measure record/playback speed..."));
	TInt r = KErrNone;
	TInt i;
	// Setup the same sound configuration for both - record and play channels
	if (RecordCapsBuf().iEncodings & KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;

	if (PlayCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;

	RecordFormatBuf().iChannels = 2;
	PlayFormatBuf().iChannels = 2;

	// find first supported rate and set the the audio configuration to use it
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		// check record channel
		RecordFormatBuf().iRate = (TSoundRate)i;
		r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
		if (RecordCapsBuf().iRates & (1 << i))
			{
			CHECK_NOERROR(r);		// Caps reports it is supported

			// ..and try the same bitrate for playback
			PlayFormatBuf().iRate = (TSoundRate)i;
			r = TxSoundDevice.SetAudioFormat(PlayFormatBuf);
			if (PlayCapsBuf().iRates & (1 << i))
				{
				CHECK_NOERROR(r);		// Caps reports it is supported
				break;
				}
			}
		}

	// both channels are set at this point, continue
	PrintConfig(RecordFormatBuf(),Test);
	PrintConfig(PlayFormatBuf(),Test);

	// Set the volume level in both channels
	RxSoundDevice.SetVolume(KSoundMaxVolume);
	TxSoundDevice.SetVolume(KSoundMaxVolume - 10);

	// Set the record buffer configuration, then read it back.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(RecordFormatBuf())/8; 									// Large enough to hold 1/8th second of data.
	bufSize=ValidBufferSize(bufSize,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());	// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=7+7;															// Must be able to use less than this
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;																// All buffers will be contiguous
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);

	// Assign the same chunk to the play channel.
	r=TxSoundDevice.SetBufferChunkOpen(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);

	Test.Next(_L("Starting recording speed test..."));
	TInt length;

	// Recording speed test
	TTime preTime, postTime;
	preTime.UniversalTime();
	for(i=0; i<100; ++i)
		{
		TRequestStatus s;
		length = -1;
		
		TTime preBufTime, postBufTime;
		preBufTime.UniversalTime();
		RxSoundDevice.RecordData(s, length);
		User::WaitForRequest(s);
		
		postBufTime.UniversalTime();
		TTimeIntervalMicroSeconds elapsedBufTime = postBufTime.MicroSecondsFrom(preBufTime);
		Test.Printf(_L("\tElapsed buf (%d/100) recording time %d\n"), i, elapsedBufTime.Int64());
		CHECK(s.Int() >= 0);
		CHECK(RxSoundDevice.ReleaseBuffer(s.Int()) == KErrNone);
		CHECK(length == bufSize);
		}
	postTime.UniversalTime();
	TTimeIntervalMicroSeconds elapsedRecordingTime = postTime.MicroSecondsFrom(preTime);
	Test.Printf(_L("Elapsed recording time %d\n"), elapsedRecordingTime.Int64());
	Test.Printf(_L("Record timing done\n"));


	//
	// Playback test
	//
	TxSoundDevice.CancelPlayData();
	struct RequestInfo {
		TRequestStatus s_m;
		TUint bufOffset_m;
		};
	RequestInfo requestA;
	RequestInfo requestB;
	
	// Get two buffers for playback speed test
	RxSoundDevice.RecordData(requestA.s_m, length);
	User::WaitForRequest(requestA.s_m);
	CHECK(requestA.s_m.Int() >= 0);
	requestA.bufOffset_m = requestA.s_m.Int();

    RxSoundDevice.RecordData(requestB.s_m, length);
    User::WaitForRequest(requestB.s_m);
    CHECK(requestB.s_m.Int() >= 0);
    requestB.bufOffset_m = requestB.s_m.Int();

    Test.Printf(_L("buf offsets %d %d\n"), requestA.bufOffset_m, requestB.bufOffset_m);
	
	RequestInfo *prevRequest = &requestA;
	RequestInfo *currRequest = &requestB;

	// Issue initial play request
	TxSoundDevice.PlayData(prevRequest->s_m, prevRequest->bufOffset_m, bufSize);

	preTime.UniversalTime();
	for(i=0; i<100; ++i)
		{
		// Issue new request so we do not underflow....
		TxSoundDevice.PlayData(currRequest->s_m, currRequest->bufOffset_m, bufSize, (i==99)?(KSndFlagLastSample) : (0));

		// Wait for previous request to complete
		TTime preBufTime, postBufTime;
		preBufTime.UniversalTime();
		User::WaitForRequest(prevRequest->s_m);
		CHECK_NOERROR(prevRequest->s_m.Int());

		postBufTime.UniversalTime();
		TTimeIntervalMicroSeconds elapsedBufTime = postBufTime.MicroSecondsFrom(preBufTime);
		Test.Printf(_L("\tElapsed buf (%d/100) playback time %d\n"), i, elapsedBufTime.Int64());

		// Swap previous and current requests
		RequestInfo *p = prevRequest;
		prevRequest = currRequest;
		currRequest = p;
		}
	
	postTime.UniversalTime();
	TTimeIntervalMicroSeconds elapsedPlaybackTime = postTime.MicroSecondsFrom(preTime);
    Test.Printf(_L("Elapsed playback time  = %d us\n"), elapsedPlaybackTime.Int64());
    Test.Printf(_L("Elapsed recording time = %d us\n"), elapsedRecordingTime.Int64());
	
	double play = (double) elapsedPlaybackTime.Int64();
	double record = (double) elapsedRecordingTime.Int64();
	Test.Printf(_L("difference %f%%\n"), (play*100)/record);
	
	User::WaitForRequest(prevRequest->s_m);
	CHECK_NOERROR(prevRequest->s_m.Int());

    // Free the two buffers
    CHECK(RxSoundDevice.ReleaseBuffer(requestA.bufOffset_m) == KErrNone);
    CHECK(RxSoundDevice.ReleaseBuffer(requestB.bufOffset_m) == KErrNone);
 
	Test.Printf(_L("Playback done\n"));
    TxSoundDevice.CancelPlayData();
    RxSoundDevice.CancelPlayData();

	chunk.Close();
	return;		
}

#ifdef __WINS__
void TestDefectDTWMM00678()
{
	// DTW-MM00678  RSoundSc::RecordData() returns recorded length > allocated buffer size 
    TRequestStatus status[3];
    TInt length[3];
	Test.Next(_L("DTW-MM00678  RSoundSc::RecordData() returns recorded length > allocated buffer size"));

	// Make sure recording is not in progress
	RxSoundDevice.CancelRecordData();

	TInt r = KErrNone;
	TInt i;
	// Setup the same sound configuration for both - record and play channels
	if (RecordCapsBuf().iEncodings & KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;

	RecordFormatBuf().iChannels = 2;

	// Find first supported rate and set the the audio configuration to use it
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		// check record channel
		RecordFormatBuf().iRate = (TSoundRate)i;
		if (RecordCapsBuf().iRates & (1 << i))
			{
			// Caps reports it is supported
			r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
			CHECK_NOERROR(r);
			break;
			}
		}
	// Check we found/set a valid format
	CHECK(i <= ESoundRate48000Hz);

	// Set recording format
	PrintConfig(RecordFormatBuf(),Test);

	// Set the volume level
	RxSoundDevice.SetVolume(KSoundMaxVolume);

	// Set the record buffer configuration, then read it back.
	RChunk chunk;
	TInt bufSize = 64 * 1024; // The defect is seen, on windows, when the buffer size is 64k and the LDD does 2x 32k transfers per buffer
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=7;		
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);

	// Calculate time required to fill a single 64k byte buffer
 	TUint32 durationOneBufferMsec = (1000 * bufSize) / BytesPerSecond(RecordFormatBuf());
    Test.Printf(_L("durationOneBufferMsec %d\n"), durationOneBufferMsec);

	// Start recording....
    Test.Printf(_L("Issue 3 RecordData requests then wait to pause during second internal 32k internal transfers of the second buffer...\n"));
	for(i=0; i<3; ++i)
		{
	    RxSoundDevice.RecordData(status[i], length[i]);
		}
    
	// Wait for 1 3/4 64k buffers. In  other words, wait for 3.75x32k byte internal transfers so we pause during the second transfer of the second buffer
 	User::After(durationOneBufferMsec *1000 * (1 + 3/4) );

    CHECK_NOERROR(RxSoundDevice.Pause());
    Test.Printf(_L("Paused\n"));

	for(i=0; i<3; ++i)
		{
		User::WaitForRequest(status[i]);
		Test.Printf(_L("status[%d].Int() = %d\n"), i, status[i].Int());
		Test.Printf(_L("length[%d] = %d\n"), i, length[i]);
		}

	bool testValid = true;

	if((status[0].Int() < 0) || (length[0] != bufSize))
		{
		testValid = false;
		Test.Printf(_L("Test invalid because pause hit first request\n"));
		}

	if(testValid && (status[1].Int() == KErrCancel))
		{
		testValid = false;
		Test.Printf(_L("Test invalid because pause hit before second request started\n"));
		}

	if(testValid && (status[2].Int() != KErrCancel))
		{
		testValid = false;
		Test.Printf(_L("Test invalid because pause missed all requests\n"));
		}

	if(testValid)
		{
		Test.Printf(_L("Appear to have issued pause at the correct time, check results\n"));
		// First request should have completed with a full buffer of data
		CHECK(status[0].Int() >= 0);
    	CHECK(length[0] == bufSize);

		// second request should have been truncated
		CHECK(status[1].Int() >= 0);
   		CHECK(length[1] < bufSize);

		// Last request should have been cancelled.
		CHECK(status[2].Int() == KErrCancel);
		}
	Test.Printf(_L("DTW-MM00678 test done\r\n"));

    //CHECK_NOERROR(RxSoundDevice.Resume());
	
    // Make sure recording is not in progress
	RxSoundDevice.CancelRecordData();
	TxSoundDevice.CancelPlayData();
	
	chunk.Close();
	return;
	}
#endif

LOCAL_C void TestUnloadDrivers()
	{
	TInt r=User::FreeLogicalDevice(KDevSoundScName);
	Test.Printf(_L("Unloading %S.LDD - %d\r\n"),&KDevSoundScName,r);
	CHECK_NOERROR(r);

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
		CHECK_NOERROR(r);
		findPD.Find(pddName); // Reset the find handle now that we have deleted something from the container.
		r=findPD.Next(findResult);
		}
	}

void TestTimePlayed()
	{
	TTimeIntervalMicroSecondsBuf timeIntervalBuf;

	// Don't try to do the tests if TimePlayed() is not supported
	TInt r = TxSoundDevice.TimePlayed(timeIntervalBuf);
	if (r == KErrNotSupported)
		{
		Test.Printf(_L("TimePlayed() is not supported, skipping tests\n"));
		return;
		}
	CHECK_NOERROR(r);

	TInt rate;

	// Find first supported rate and set the the audio configuration to use it
	for (rate = 0; rate <= ESoundRate48000Hz; ++rate)
		{
		if (PlayCapsBuf().iRates & (1 << rate))
			{
			break;
			}
		}

	// Test mono and Stereo
	for (TInt channels=1; channels<=2; ++channels)
	{
		TRequestStatus stat[2];

		Test.Next(_L("Preparing to play..."));
		if (PlayCapsBuf().iEncodings&KSoundEncoding16BitPCM)
			PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;
		PlayFormatBuf().iRate = (TSoundRate) rate;
		PlayFormatBuf().iChannels = channels;
		PrintConfig(PlayFormatBuf(),Test);
		r = TxSoundDevice.SetAudioFormat(PlayFormatBuf);
		CHECK_NOERROR(r);

		// Set the play buffer configuration, then read it back.
		RChunk chunk;
		TInt bufSize=BytesPerSecond(PlayFormatBuf()); 									// Large enough to hold 1 second of data.
		bufSize=ValidBufferSize(bufSize,PlayCapsBuf().iRequestMinSize,PlayFormatBuf());		// Keep the buffer length valid for driver.
		TTestSharedChunkBufConfig bufferConfig;
		bufferConfig.iNumBuffers=2;
		bufferConfig.iBufferSizeInBytes=bufSize;
		bufferConfig.iFlags=0;		// All buffers will be contiguous
		TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
		r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
		CHECK_NOERROR(r);
		TxSoundDevice.GetBufferConfig(bufferConfigBuf);
		PrintBufferConf(bufferConfig,Test);
		CHECK(bufferConfig.iBufferSizeInBytes==bufSize);
		TPtr8* tPtr[2];
		TInt i;
		for (i=0;i<2;i++)
			tPtr[i]=new TPtr8(chunk.Base()+bufferConfig.iBufferOffsetList[i],bufSize);


		r=MakeSineTable(PlayFormatBuf());
		CHECK_NOERROR(r);
		r=SetToneFrequency(440,PlayFormatBuf());
		CHECK_NOERROR(r);

		WriteTone(*tPtr[0],PlayFormatBuf());
		WriteTone(*tPtr[1],PlayFormatBuf());

		// set up a timer to interrogate time played
		TRequestStatus timerStat;
		RTimer timer;
		timer.CreateLocal();
		TTimeIntervalMicroSeconds32 timerInterval(50000);

		TInt64 currentTime, previousTime;

		Test.Next(_L("Time Played..."));

		currentTime = previousTime = MAKE_TINT64(0,0);

		TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize,0);
		TxSoundDevice.PlayData(stat[1],bufferConfig.iBufferOffsetList[1],bufSize,KSndFlagLastSample);

		// check requests are pending
		CHECK_EQUAL(stat[0].Int(),KRequestPending);
		CHECK_EQUAL(stat[1].Int(),KRequestPending);

		// check time recorded is not supported for play channel
		CHECK(TxSoundDevice.TimeRecorded(timeIntervalBuf)==KErrNotSupported);

		timer.After(timerStat,timerInterval);
		// first buffer
		while (stat[0] == KRequestPending)
			{
			User::WaitForRequest(stat[0],timerStat);
			Test.Printf(_L("stat[0] %x, timerStat %x\n"),stat[0].Int(),timerStat.Int());
			previousTime = currentTime;
			r = TxSoundDevice.TimePlayed(timeIntervalBuf);
			CHECK_NOERROR(r);
			currentTime = timeIntervalBuf().Int64();
			Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

			// ensure time is increasing or function is not supported
			CHECK((I64LOW(currentTime) >= I64LOW(previousTime)) || r == KErrNotSupported);

			if (timerStat != KRequestPending)
				{
				timer.After(timerStat,timerInterval);
				}
			}
		timer.Cancel();

		Test.Printf(_L("stat[0] %x, timerStat %x\n"),stat[0].Int(),timerStat.Int());
		r = TxSoundDevice.TimePlayed(timeIntervalBuf);
		CHECK_NOERROR(r);
		currentTime = timeIntervalBuf().Int64();
		Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

		CHECK_EQUAL(stat[0].Int(),KErrNone);

		timer.After(timerStat,timerInterval);
		// second buffer
		while (stat[1] == KRequestPending)
			{
			User::WaitForRequest(stat[1],timerStat);
			Test.Printf(_L("stat[1] %x, timerStat %x\n"),stat[1].Int(),timerStat.Int());
			previousTime = currentTime;
			r = TxSoundDevice.TimePlayed(timeIntervalBuf);
			CHECK_NOERROR(r);

			currentTime = timeIntervalBuf().Int64();
			Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

			// ensure time is increasing or function is not supported
			if (stat[1] == KRequestPending) // still playing
				{
				CHECK((I64LOW(currentTime) >= I64LOW(previousTime)) || r == KErrNotSupported);
				}

			if (timerStat != KRequestPending)
				{
				timer.After(timerStat,timerInterval);
				}

			}
		timer.Cancel();

		Test.Printf(_L("stat[1] %x, timerStat %x\n"),stat[1].Int(),timerStat.Int());
		r = TxSoundDevice.TimePlayed(timeIntervalBuf);
		CHECK_NOERROR(r);

		currentTime = timeIntervalBuf().Int64();
		Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(timeIntervalBuf().Int64()),I64LOW(timeIntervalBuf().Int64()));

		CHECK_EQUAL(stat[1].Int(),KErrNone);

		//
		// Time Played with pause
		//

		Test.Next(_L("Time Played with pause..."));

		TTimeIntervalMicroSeconds32 pauseInterval(2000000);
		TBool paused = EFalse;

		currentTime = previousTime = MAKE_TINT64(0,0);
		timer.Cancel();

		TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize,0);
		TxSoundDevice.PlayData(stat[1],bufferConfig.iBufferOffsetList[1],bufSize,KSndFlagLastSample);

		// check requests are pending
		CHECK_EQUAL(stat[0].Int(),KRequestPending);
		CHECK_EQUAL(stat[1].Int(),KRequestPending);

		// check time recorded is not supported for play channel
		CHECK(TxSoundDevice.TimeRecorded(timeIntervalBuf)==KErrNotSupported);

		timer.After(timerStat,timerInterval);
		// first buffer
		while (stat[0] == KRequestPending)
			{
			User::WaitForRequest(stat[0],timerStat);
			Test.Printf(_L("stat[0] %x, timerStat %x\n"),stat[0].Int(),timerStat.Int());
			previousTime = currentTime;
			r = TxSoundDevice.TimePlayed(timeIntervalBuf);
			CHECK_NOERROR(r);
			currentTime = timeIntervalBuf().Int64();
			Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

			// ensure time is increasing or function is not supported
			CHECK((I64LOW(currentTime) >= I64LOW(previousTime)) || r == KErrNotSupported);

			// Pause and resume ...
			if (paused == EFalse && I64LOW(currentTime) > 500000)
				{
				paused = ETrue;
				TxSoundDevice.Pause();
				r = TxSoundDevice.TimePlayed(timeIntervalBuf);
				CHECK_NOERROR(r);
				TInt64 pausedTime1 = timeIntervalBuf().Int64();
				Test.Printf(_L("Paused time_high %d, time_low %d\n"),I64HIGH(pausedTime1),I64LOW(pausedTime1));

				User::After(pauseInterval);

				r = TxSoundDevice.TimePlayed(timeIntervalBuf);
				CHECK_NOERROR(r);
				TInt64 pausedTime2 = timeIntervalBuf().Int64();
				Test.Printf(_L("Resumed time_high %d, time_low %d\n"),I64HIGH(pausedTime2),I64LOW(pausedTime2));
				//CHECK(pausedTime1 == pausedTime2);
				TxSoundDevice.Resume();
				}

			if (timerStat != KRequestPending)
				{
				timer.After(timerStat,timerInterval);
				}
			}
		timer.Cancel();

		Test.Printf(_L("stat[0] %x, timerStat %x\n"),stat[0].Int(),timerStat.Int());
		r = TxSoundDevice.TimePlayed(timeIntervalBuf);
		CHECK_NOERROR(r);
		currentTime = timeIntervalBuf().Int64();
		Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

		CHECK_EQUAL(stat[0].Int(),KErrNone);

		timer.After(timerStat,timerInterval);
		// second buffer
		while (stat[1] == KRequestPending)
			{
			User::WaitForRequest(stat[1],timerStat);
			Test.Printf(_L("stat[1] %x, timerStat %x\n"),stat[1].Int(),timerStat.Int());
			previousTime = currentTime;
			r = TxSoundDevice.TimePlayed(timeIntervalBuf);
			CHECK_NOERROR(r);

			currentTime = timeIntervalBuf().Int64();
			Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

			// ensure time is increasing or function is not supported
			if (stat[1] == KRequestPending) // still playing
				{
				CHECK((I64LOW(currentTime) >= I64LOW(previousTime)) || r == KErrNotSupported);
				}

			if (timerStat != KRequestPending)
				{
				timer.After(timerStat,timerInterval);
				}

			}
		timer.Cancel();

		Test.Printf(_L("stat[1] %x, timerStat %x\n"),stat[1].Int(),timerStat.Int());
		r = TxSoundDevice.TimePlayed(timeIntervalBuf);
		CHECK_NOERROR(r);

		currentTime = timeIntervalBuf().Int64();
		Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(timeIntervalBuf().Int64()),I64LOW(timeIntervalBuf().Int64()));

		CHECK_EQUAL(stat[1].Int(),KErrNone);

		// clean up
		timer.Close();
		chunk.Close();
		for (i=0;i<2;i++)
			delete tPtr[i];

		} // channel loop
	}

void TestTimeRecorded()
	{
	TTimeIntervalMicroSecondsBuf timeIntervalBuf;

	TInt r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
	if (r == KErrNotSupported)
		{
		Test.Printf(_L("TimeRecorded() is not supported, skipping tests\n"));
		return;
		}
	CHECK_NOERROR(r);

	TInt rate;

	// Find first supported rate and set the the audio configuration to use it
	for (rate = 0; rate <= ESoundRate48000Hz; ++rate)
		{
		if (PlayCapsBuf().iRates & (1 << rate))
			{
			break;
			}
		}

	// Test mono and Stereo
	for (TInt channels=1; channels<=2; ++channels)
	{
		TRequestStatus stat[2];

		Test.Next(_L("Preparing to record..."));
		if (RecordCapsBuf().iEncodings&KSoundEncoding16BitPCM)
			RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;
		RecordFormatBuf().iRate = (TSoundRate) rate;
		RecordFormatBuf().iChannels = channels;
		PrintConfig(RecordFormatBuf(),Test);
		r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
		CHECK_NOERROR(r);

		// Set the play buffer configuration, then read it back.
		RChunk chunk;
		TInt bufSize=BytesPerSecond(RecordFormatBuf()); 									// Large enough to hold 1 second of data.
		bufSize=ValidBufferSize(bufSize,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());		// Keep the buffer length valid for driver.
		TTestSharedChunkBufConfig bufferConfig;
		bufferConfig.iNumBuffers=4;
		bufferConfig.iBufferSizeInBytes=bufSize;
		bufferConfig.iFlags=0;		// All buffers will be contiguous
		TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
		r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
		CHECK_NOERROR(r);
		RxSoundDevice.GetBufferConfig(bufferConfigBuf);
		PrintBufferConf(bufferConfig,Test);
		CHECK(bufferConfig.iBufferSizeInBytes==bufSize);

		// set up a timer to interrogate time played
		TRequestStatus timerStat;
		RTimer timer;
		timer.CreateLocal();
		TTimeIntervalMicroSeconds32 timerInterval(50000);

		TInt64 currentTime, previousTime;

		Test.Next(_L("Time Recorded..."));

		currentTime = previousTime = MAKE_TINT64(0,0);

		TInt length1=0, length2=0;
		RxSoundDevice.RecordData(stat[0],length1);
		RxSoundDevice.RecordData(stat[1],length2);

		// check requests are pending
		CHECK_EQUAL(stat[0].Int(),KRequestPending);
		CHECK_EQUAL(stat[1].Int(),KRequestPending);

		// check time played is not supported for record channel
		CHECK(RxSoundDevice.TimePlayed(timeIntervalBuf)==KErrNotSupported);

		timer.After(timerStat,timerInterval);
		// first buffer
		while (stat[0] == KRequestPending)
			{
			User::WaitForRequest(stat[0],timerStat);
			Test.Printf(_L("stat[0] %x, timerStat %x\n"),stat[0].Int(),timerStat.Int());
			previousTime = currentTime;
			r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
			CHECK_NOERROR(r);
			currentTime = timeIntervalBuf().Int64();
			Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

			// ensure time is increasing or function is not supported
			CHECK((I64LOW(currentTime) >= I64LOW(previousTime)) || r == KErrNotSupported);

			if (timerStat != KRequestPending)
				{
				timer.After(timerStat,timerInterval);
				}
			}
		timer.Cancel();

		Test.Printf(_L("stat[0] %x, timerStat %x\n"),stat[0].Int(),timerStat.Int());
		r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
		CHECK_NOERROR(r);
		currentTime = timeIntervalBuf().Int64();
		Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

		CHECK(stat[0].Int() == 0);

		timer.After(timerStat,timerInterval);
		// second buffer
		while (stat[1] == KRequestPending)
			{
			User::WaitForRequest(stat[1],timerStat);
			Test.Printf(_L("stat[1] %x, timerStat %x\n"),stat[1].Int(),timerStat.Int());
			previousTime = currentTime;
			r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
			CHECK_NOERROR(r);
			currentTime = timeIntervalBuf().Int64();
			Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

			// ensure time is increasing or function is not supported
			if (stat[1] == KRequestPending) // still playing
				{
				CHECK((I64LOW(currentTime) >= I64LOW(previousTime)) || r == KErrNotSupported);
				}

			if (timerStat != KRequestPending)
				{
				timer.After(timerStat,timerInterval);
				}

			}
		timer.Cancel();

		Test.Printf(_L("stat[1] %x, timerStat %x\n"),stat[1].Int(),timerStat.Int());
		r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
		CHECK_NOERROR(r);
		currentTime = timeIntervalBuf().Int64();
		Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(timeIntervalBuf().Int64()),I64LOW(timeIntervalBuf().Int64()));

		CHECK(stat[1].Int() > 0);

		// stop recording into the next buffer
		RxSoundDevice.CancelRecordData();

		// Release the buffers
		r = RxSoundDevice.ReleaseBuffer(stat[0].Int());
		CHECK_EQUAL(r, KErrNone);
		r = RxSoundDevice.ReleaseBuffer(stat[1].Int());
		CHECK_EQUAL(r, KErrNone);

		//
		// Time Recorded with pause
		//

		Test.Next(_L("Time Recorded with pause..."));

		TTimeIntervalMicroSeconds32 pauseInterval(2000000);
		TBool paused = EFalse;

		currentTime = previousTime = MAKE_TINT64(0,0);

	    // Record and discard some data to make sure all testing is not within the first buffer...
		RxSoundDevice.RecordData(stat[0],length1);
        User::WaitForRequest(stat[0]);
        CHECK(stat[0].Int() >= 0);
        RxSoundDevice.ReleaseBuffer(stat[0].Int());
        
		RxSoundDevice.RecordData(stat[0],length1);
        RxSoundDevice.RecordData(stat[1],length2);
		
		// check requests are pending
		CHECK_EQUAL(stat[0].Int(),KRequestPending);

		// check time recorded is not supported for play channel
		CHECK(RxSoundDevice.TimePlayed(timeIntervalBuf)==KErrNotSupported);

		timer.After(timerStat,timerInterval);
		// first buffer
		while (stat[0] == KRequestPending)
			{
			User::WaitForRequest(stat[0],timerStat);
			Test.Printf(_L("stat[0] %x, timerStat %x\n"),stat[0].Int(),timerStat.Int());
			previousTime = currentTime;
			r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
			CHECK_NOERROR(r);
			currentTime = timeIntervalBuf().Int64();
			Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

			// ensure time is increasing or function is not supported
			CHECK((I64LOW(currentTime) >= I64LOW(previousTime)) || r == KErrNotSupported);

			// Pause and resume ...
			if (paused == EFalse && I64LOW(currentTime) > 500000)
				{
				paused = ETrue;
				RxSoundDevice.Pause();
				r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
				CHECK_NOERROR(r);
				TInt64 pausedTime1 = timeIntervalBuf().Int64();
				Test.Printf(_L("Paused time_high %d, time_low %d\n"),I64HIGH(pausedTime1),I64LOW(pausedTime1));
	            // Check time is increasing
	            CHECK((I64LOW(pausedTime1) >= I64LOW(currentTime)));

				User::After(pauseInterval);

				r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
				CHECK_NOERROR(r);
				TInt64 pausedTime2 = timeIntervalBuf().Int64();
				Test.Printf(_L("Resumed time_high %d, time_low %d\n"),I64HIGH(pausedTime2),I64LOW(pausedTime2));
                // Check time is unchanged
                CHECK((I64LOW(pausedTime2) == I64LOW(pausedTime1)));
				}

			if (timerStat != KRequestPending)
				{
				timer.After(timerStat,timerInterval);
				}
			}

		timer.Cancel();

		// Buffer should complete normally or be empty (indicated by KErrCancel)
		CHECK((stat[0].Int() >= 0) || (stat[0].Int() == KErrCancel));
		// Release the first buffer, if it contained any data
		if (stat[0].Int() >= 0)
			{
			r = RxSoundDevice.ReleaseBuffer(stat[0].Int());
			CHECK_EQUAL(r, KErrNone);
			}
		// Check second buffer completed or cancelled
		User::WaitForRequest(stat[1]);
		CHECK_EQUAL(stat[1].Int(), KErrCancel);

		// Now resume the recording
		r = RxSoundDevice.Resume();
		CHECK_EQUAL(r, KErrNone);

		// Need to re-setup buffers
		RxSoundDevice.RecordData(stat[0],length1);
		RxSoundDevice.RecordData(stat[1],length2);

		timer.After(timerStat,timerInterval);
		// another buffer
		while (stat[0] == KRequestPending)
			{
			User::WaitForRequest(stat[0],timerStat);
			Test.Printf(_L("stat[0] %x, timerStat %x\n"),stat[0].Int(),timerStat.Int());
			previousTime = currentTime;
			r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
			CHECK_NOERROR(r);
			currentTime = timeIntervalBuf().Int64();
			Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(currentTime),I64LOW(currentTime));

			// ensure time is increasing or function is not supported
			if (stat[0] == KRequestPending) // still recording
				{
				CHECK((I64LOW(currentTime) >= I64LOW(previousTime)) || r == KErrNotSupported);
				}

			if (timerStat != KRequestPending)
				{
				timer.After(timerStat,timerInterval);
				}

			}
		timer.Cancel();

		Test.Printf(_L("stat[0] %x, timerStat %x\n"),stat[0].Int(),timerStat.Int());
		r = RxSoundDevice.TimeRecorded(timeIntervalBuf);
		CHECK_NOERROR(r);
		currentTime = timeIntervalBuf().Int64();
		Test.Printf(_L("time_high %d, time_low %d\n"),I64HIGH(timeIntervalBuf().Int64()),I64LOW(timeIntervalBuf().Int64()));

		CHECK(stat[0].Int() > 0);

		// stop recording into the next buffer
		RxSoundDevice.CancelRecordData();

		// Release the buffers
		r = RxSoundDevice.ReleaseBuffer(stat[0].Int());
		CHECK_EQUAL(r, KErrNone);

		// clean up
		timer.Close();
		chunk.Close();
		} // channel loop
	}

TInt E32Main()
	{
//	User::SetDebugMask(0x10,1); // Enable KSOUND1

	__UHEAP_MARK;
	TInt r;

	Test.Title();

	Test.Start(_L("Load"));
	if (Load()==KErrNotFound)
		{
		Test.Printf(_L("Shared chunk sound driver not supported - test skipped\r\n"));
		Test.End();
		Test.Close();
		__UHEAP_MARKEND;
		return(KErrNone);
		}

	__KHEAP_MARK;

	/**	@SYMTestCaseID 		PBASE-T_SOUND2-223
	@SYMTestCaseDesc 		Opening the channel - normal
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	With the LDD and PDD installed and with all channels closed on the device,
								open a channel for playback on the device.
							2)	Without closing the playback channel, open a channel for record on the device.
							3)	Close the playback channel and then open it again.
							4)	Close the record channel and then open it again.
	@SYMTestExpectedResults	1)	KErrNone - Channel opens successfully.
							2)	KErrNone - Channel opens successfully.
							3)	KErrNone - Channel re-opens successfully.
							4)	KErrNone - Channel re-opens successfully.
	@SYMREQ					PREQ1073.4 */

	Test.Next(_L("Open playback channel"));
	r = TxSoundDevice.Open(KSoundScTxUnit0);
	CHECK_NOERROR(r);

	Test.Next(_L("Open record channel"));
	r = RxSoundDevice.Open(KSoundScRxUnit0);
	CHECK_NOERROR(r);

	Test.Next(_L("Query play formats supported"));
	TxSoundDevice.Caps(PlayCapsBuf);
	TSoundFormatsSupportedV02 playCaps=PlayCapsBuf();
	PrintCaps(playCaps,Test);

	Test.Next(_L("Close playback channel"));
	TxSoundDevice.Close();
	Test.Next(_L("Re-open playback channel"));
	r = TxSoundDevice.Open(KSoundScTxUnit0);
	CHECK_NOERROR(r);
	Test.Next(_L("Close record channel"));
	RxSoundDevice.Close();
	Test.Next(_L("Re-open record channel"));
	r = RxSoundDevice.Open(KSoundScRxUnit0);
	CHECK_NOERROR(r);

	Test.Next(_L("Query play formats supported"));
	TxSoundDevice.Caps(PlayCapsBuf);
	PrintCaps(playCaps,Test);

	Test.Next(_L("Query record formats supported"));
	RxSoundDevice.Caps(RecordCapsBuf);
	TSoundFormatsSupportedV02 recordCaps=RecordCapsBuf();
	PrintCaps(recordCaps,Test);

	Test.Next(_L("Query current play settings"));
	TxSoundDevice.AudioFormat(PlayFormatBuf);
	TCurrentSoundFormatV02 playFormat=PlayFormatBuf();
	PrintConfig(playFormat,Test);
	CheckConfig(playFormat,playCaps);

	Test.Next(_L("Query current record settings"));
	RxSoundDevice.AudioFormat(RecordFormatBuf);
	TCurrentSoundFormatV02 recordFormat=RecordFormatBuf();
	PrintConfig(recordFormat,Test);
	CheckConfig(recordFormat,recordCaps);

	Test.Next(_L("Set play format"));
	if (playCaps.iEncodings&KSoundEncoding16BitPCM)
		playFormat.iEncoding = ESoundEncoding16BitPCM;
	PrintConfig(playFormat,Test);
	r = TxSoundDevice.SetAudioFormat(PlayFormatBuf);
	CHECK_NOERROR(r);

	Test.Next(_L("Set Record Format"));
	if (recordCaps.iEncodings&KSoundEncoding16BitPCM)
		recordFormat.iEncoding = ESoundEncoding16BitPCM;
	PrintConfig(recordFormat,Test);
	r = RxSoundDevice.SetAudioFormat(RecordFormatBuf);
	CHECK_NOERROR(r);

#ifdef SOAKTEST
	TInt freeRamInBytes=0;
	TTime stim;
	stim.HomeTime();

	FOREVER
		{
#endif

		TestBasicPlayFunctions();
		TestBasicRecordFunctions();
		TestPlayAllRates(1,4);
		TestPlayAllRates(2,4);
		TestRecordAllRates(1,4);
		TestRecordAllRates(2,4);
		TestRecordVolume(2,10);
		TestPlayCancel();
		TestRecordCancel();
		TestRecordPauseResume(1);
		TestRecordPauseResume(2);
		TestSimultaneousPlayRecord();
		TestTimePlayed();
		TestTimeRecorded();
#ifdef __WINS__
		TestDefectDTWMM00678();
#endif
        //TestSpeed(); // Gives information which may help debug h4 FMM issues

#ifdef SOAKTEST
		TInt free;
		HAL::Get(HAL::EMemoryRAMFree,free);
	Test.Printf(_L("Free ram is %d bytes\r\n"),free);
//	if (freeRamInBytes)
//		CHECK(freeRamInBytes == free)
	freeRamInBytes=free;

	TTime ntim;
		ntim.HomeTime();
		TTimeIntervalMinutes elapsed;
		r=ntim.MinutesFrom(stim,elapsed);
		CHECK_NOERROR(r);
		Test.Printf(_L("Test has been running for %d minutes\r\n"),elapsed.Int());
		}
#endif

	Test.Next(_L("Close channels"));
	RxSoundDevice.Close();
	TxSoundDevice.Close();

	__KHEAP_MARKEND;

	// Now that both the channels are closed, unload the LDD and the PDDs.
	TestUnloadDrivers();

	Test.End();
	Test.Close();

	Cleanup();
	__UHEAP_MARKEND;
	User::Allocator().Check();
	return(KErrNone);
	}
