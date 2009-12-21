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
// e32test\multimedia\t_sound_api.cpp
// 
//

/**
 @file Shared chunk sound driver API test code.
*/

#include <e32test.h>
#include <f32file.h>
#include "t_soundutils.h"

#define CHECK(aValue) {Test(aValue);}
#define CHECK_NOERROR(aValue) { TInt v=(aValue); if(v) { Test.Printf(_L("Error value = %d\n"),v); Test(EFalse,__LINE__); }}
#define CHECK_EQUAL(aValue1,aValue2) { TInt v1=(aValue1); TInt v2=(aValue2); if(v1!=v2) { Test.Printf(_L("Error value = %d\n"),v1); Test(EFalse,__LINE__); }}

_LIT(KSndLddFileName,"ESOUNDSC.LDD");
_LIT(KSndPddFileName,"SOUNDSC.PDD");

RTest Test(_L("T_SOUND_API"));
RSoundSc TxSoundDevice;
RSoundSc RxSoundDevice;

TSoundFormatsSupportedV02Buf RecordCapsBuf;
TSoundFormatsSupportedV02Buf PlayCapsBuf;
TCurrentSoundFormatV02Buf PlayFormatBuf;
TCurrentSoundFormatV02Buf RecordFormatBuf;

LOCAL_C TInt Load()
	{
	TInt r;
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-219
	@SYMTestCaseDesc 		Installing the PDD
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	Attempt to install the PDD (using User::LoadPhysicalDevice()). 
							2)	Without un-installing it, attempt to install it a second time.
	@SYMTestExpectedResults	1)	KErrNone - PDD loads successfully. 
							2)	Should fail with KErrAlreadyExists.
	@SYMREQ					PREQ1073.4 */

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
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-220
	@SYMTestCaseDesc 		Installing the LDD
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	Attempt to install the LDD (using User::LoadLogicalDevice()). 
							2)	Without un-installing it, attempt to install it a second time.
	@SYMTestExpectedResults	1)	KErrNone - LDD loads successfully. 
							2)	Should fail with KErrAlreadyExists.
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Load sound LDD"));
	r=User::LoadLogicalDevice(KSndLddFileName);
	CHECK(r==KErrNone || r==KErrAlreadyExists);
	r=User::LoadPhysicalDevice(KSndPddFileName);
	CHECK(r==KErrAlreadyExists);

	Test.End();
	return(KErrNone);
	}

LOCAL_C void TestSetAudioFormat()
	{
	TInt r;
	Test.Printf(_L("Testing setting the audio format\r\n"));
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-227
	@SYMTestCaseDesc 		Setting the audio configuration - channel configuration.
	@SYMTestPriority 		Critical
	@SYMTestActions			Read the audio capabilities of the playback channel. For each of the possible 6 channel 
							configurations, issue a request on the playback channel to set this audio configuration.
	@SYMTestExpectedResults	Either KErrNone if the playback channel supports the channel configuration, or
							KErrNotSupported if the playback channel does not support the channel configuration. 
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Try to set every possible audio channel configuration"));
	TxSoundDevice.AudioFormat(PlayFormatBuf);	// Read back the current setting which must be valid.
	TInt i;
	for (i=0 ; i < 6 ; i++)
		{
		PlayFormatBuf().iChannels = (i+1);
		r=TxSoundDevice.SetAudioFormat(PlayFormatBuf);
		if (PlayCapsBuf().iChannels & (1<<i))
			{
			CHECK_NOERROR(r);				// Caps reports it is supported
			}					
		else
			{
			CHECK(r==KErrNotSupported);		// Caps reports it is not supported
			}		
		}
		
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-228
	@SYMTestCaseDesc 		Setting the audio configuration - sample rate.
	@SYMTestPriority 		Critical
	@SYMTestActions			Read the audio capabilities of the playback channel. For each of the possible 14 sample 
							rates, issue a request on the playback channel to set this audio configuration.
	@SYMTestExpectedResults	Either KErrNone if the playback channel supports the sample rate, or
							KErrNotSupported if the playback channel does not support the sample rate. 
	@SYMREQ					PREQ1073.4 */
		
	Test.Next(_L("Try to set every possible sample rate"));
	TxSoundDevice.AudioFormat(PlayFormatBuf);	// Read back the current setting which must be valid.
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		PlayFormatBuf().iRate = (TSoundRate)i;
		r=TxSoundDevice.SetAudioFormat(PlayFormatBuf);
		if (PlayCapsBuf().iRates & (1<<i))
			{
			CHECK_NOERROR(r);				// Caps reports it is supported
			}					
		else
			{
			CHECK(r==KErrNotSupported);		// Caps reports it is not supported
			}		
		}	
		
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-229
	@SYMTestCaseDesc 		Setting the audio configuration - audio encoding.
	@SYMTestPriority 		Critical
	@SYMTestActions			Read the audio capabilities of the playback channel. For each of the possible 3 audio 
							encodings, issue a request on the playback channel to set this audio configuration.
	@SYMTestExpectedResults	Either KErrNone if the playback channel supports the audio encoding, or
							KErrNotSupported if the playback channel does not support the audio encoding. 
	@SYMREQ					PREQ1073.4 */	
		
	Test.Next(_L("Try to set every possible encoding"));
	TxSoundDevice.AudioFormat(PlayFormatBuf);	// Read back the current setting which must be valid.
	for (i=0 ; i <= (TInt)ESoundEncoding24BitPCM ; i++)
		{
		PlayFormatBuf().iEncoding = (TSoundEncoding)i;
		r=TxSoundDevice.SetAudioFormat(PlayFormatBuf);
		if (PlayCapsBuf().iEncodings & (1<<i))
			{
			CHECK_NOERROR(r);				// Caps reports it is supported
			}					
		else
			{
			CHECK(r==KErrNotSupported);		// Caps reports it is not supported
			}		
		}		
		
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-230
	@SYMTestCaseDesc 		Setting the audio configuration - audio data format.
	@SYMTestPriority 		Critical
	@SYMTestActions			Read the audio capabilities of the playback channel. For each of the possible 2 audio data 
							formats, issue a request on the playback channel to set this audio configuration.
	@SYMTestExpectedResults	Either KErrNone if the playback channel supports the audio data format, or
							KErrNotSupported if the playback channel does not support the audio data format. 
	@SYMREQ					PREQ1073.4 */	
	
	Test.Next(_L("Try to set every possible data format"));
	TxSoundDevice.AudioFormat(PlayFormatBuf);	// Read back the current setting which must be valid.
	for (i=0 ; i <= (TInt)ESoundDataFormatNonInterleaved ; i++)
		{
		PlayFormatBuf().iDataFormat = (TSoundDataFormat)i;
		r=TxSoundDevice.SetAudioFormat(PlayFormatBuf);
		if (PlayCapsBuf().iDataFormats & (1<<i))
			{
			CHECK_NOERROR(r);				// Caps reports it is supported
			}					
		else
			{
			CHECK(r==KErrNotSupported);		// Caps reports it is not supported
			}		
		}
		
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-231
	@SYMTestCaseDesc 		Setting the audio configuration - altering the configuration while transferring data.
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	Read the audio capabilities of the playback channel and apply a playback audio 
								configuration that is supported by channel. Set the play buffer configuration and 
								then start playing data from one of the buffers. While still playing, attempt to alter 
								the audio configuration of the playback channel.
							2)	Read the audio capabilities of the record channel and apply an audio record 
								configuration that is supported by channel. Set the record buffer configuration and 
								then start recording data into one of the buffers. While still recording, attempt 
								to alter the audio configuration of the record channel.
	@SYMTestExpectedResults	1)	Altering the audio configuration should fail with KErrInUse. 
							2)	Altering the audio configuration should fail with KErrInUse.
	@SYMREQ					PREQ1073.4 */
		
	Test.Next(_L("Try to alter the audio format while playing"));

	// Set the audio configuration of the playback channel ready to start playing
	TxSoundDevice.AudioFormat(PlayFormatBuf);	// Read back the current setting which must be valid.
	if (PlayCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;

	PlayFormatBuf().iChannels = 2;
	
	// find first supported rate and set the the audio configuration to use it
	TxSoundDevice.AudioFormat(PlayFormatBuf);	// Read back the current setting which must be valid.
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		PlayFormatBuf().iRate = (TSoundRate)i;
		r=TxSoundDevice.SetAudioFormat(PlayFormatBuf);
		if (PlayCapsBuf().iRates & (1<<i))
			{
			CHECK_NOERROR(r);				// Caps reports it is supported
			break;
			}					
		}	
	
	PrintConfig(PlayFormatBuf(),Test);
	r=TxSoundDevice.SetAudioFormat(PlayFormatBuf);
	CHECK_NOERROR(r);
	
	// Set the play buffer configuration.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(PlayFormatBuf()); 										// Large enough to hold 1 second of data.
	bufSize=ValidBufferSize(bufSize,PlayCapsBuf().iRequestMinSize,PlayFormatBuf());		// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=3;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;	
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	TxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);
	
	// Start playing
	r=MakeSineTable(PlayFormatBuf());
	CHECK_NOERROR(r);
	r=SetToneFrequency(660,PlayFormatBuf());
	CHECK_NOERROR(r);
	TPtr8 ptr(chunk.Base()+bufferConfig.iBufferOffsetList[0],bufSize);
	WriteTone(ptr,PlayFormatBuf());
	TRequestStatus stat;
	TxSoundDevice.PlayData(stat,bufferConfig.iBufferOffsetList[0],bufSize,KSndFlagLastSample);

	// Check that altering the audio format while playing - fails
	r=TxSoundDevice.SetAudioFormat(PlayFormatBuf);
	CHECK(r==KErrInUse);

	User::WaitForRequest(stat);
	CHECK_EQUAL(stat.Int(),KErrNone);
	
	Test.Next(_L("Try to alter the audio format while recording"));
	
	// Set the audio configuration of the record channel ready to start recording
	RxSoundDevice.AudioFormat(RecordFormatBuf);	// Read back the current setting which must be valid.
	if (RecordCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	RecordFormatBuf().iChannels = 2;

	// find first supported rate and set the the audio configuration to use it
	for (i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
		{
		RecordFormatBuf().iRate = (TSoundRate)i;
		r=TxSoundDevice.SetAudioFormat(RecordFormatBuf);
		if (RecordCapsBuf().iRates & (1<<i))
			{
			CHECK_NOERROR(r);				// Caps reports it is supported
			break;
			}					
		}
	
	// Use the same shared chunk for recording
	r=RxSoundDevice.SetBufferChunkOpen(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	
	// Start recording
	TInt length;
	RxSoundDevice.RecordData(stat,length);
	
	// Check that altering the audio format while recording - fails
	r=RxSoundDevice.SetAudioFormat(RecordFormatBuf);
	CHECK(r==KErrInUse);
	
	User::WaitForRequest(stat);
	TInt retOffset=stat.Int();
	CHECK(retOffset>=0);
	CHECK(length>0);
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);
	
	RxSoundDevice.CancelRecordData();	// Stop the driver from recording.
	chunk.Close();
	}

LOCAL_C void TestSetBufferConfig(TInt aBufferConfigFlags)
	{
	
	Test.Printf(_L("Testing setting the buffer configuration\r\n"));
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-232
	@SYMTestCaseDesc 		Setting the buffer configuration - varying the number of buffers.
	@SYMTestPriority 		Critical
	@SYMTestActions			Issue a request on the playback channel to create a shared chunk (i.e. using 
							SetBufferChunkCreate()) with a buffer configuration containing a single buffer. Attempt 
							to open the same shared chunk on the record channel (i.e. using SetBufferChunkOpen()). 
							Then close the chunk.
							Repeat this same sequence using buffer configurations containing 2 - 8 buffers.
	@SYMTestExpectedResults	In every case the attempt to create and open the shared chunk should return KErrNone. 
	@SYMREQ					PREQ1073.4 */	
	
	Test.Next(_L("Test creating/opening a chunk, varying the number of buffers "));
	TInt r;
	RChunk chunk;
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iBufferSizeInBytes=0x7FE0;			// 32K - 32 bytes
	if (PlayCapsBuf().iRequestMinSize)
		bufferConfig.iBufferSizeInBytes&=~(PlayCapsBuf().iRequestMinSize-1); 	// Assumes iRequestMinSize is same for play and record
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	
	TInt i;
	for (i=1 ; i<=8 ; i++)
		{
		bufferConfig.iFlags=aBufferConfigFlags;
		bufferConfig.iNumBuffers=i;
		
		// Create the shared chunk on the play channel
		r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
		CHECK_NOERROR(r);
		TxSoundDevice.GetBufferConfig(bufferConfigBuf);
		PrintBufferConf(bufferConfig,Test);
		
		// Open the same shared chunk on the record channel
		r=RxSoundDevice.SetBufferChunkOpen(bufferConfigBuf,chunk);
		CHECK_NOERROR(r);
	
		chunk.Close();
		}
		
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-233
	@SYMTestCaseDesc 		Setting the buffer configuration - varying the size of the buffers.
	@SYMTestPriority 		Critical
	@SYMTestActions			Issue a request on the playback channel to create a shared chunk (i.e. using 
							SetBufferChunkCreate()) with a buffer configuration containing multiple buffers 
							each of size 32K bytes. Attempt to open the same shared chunk on the record channel 
							(i.e. using SetBufferChunkOpen()). Then close the chunk. Repeat this same sequence 
							using buffer configurations containing buffers of size 16K, 8K and 4K bytes. 
	@SYMTestExpectedResults	In every case the attempt to create and open the shared chunk should return KErrNone. 
	@SYMREQ					PREQ1073.4 */		
		
	Test.Next(_L("Test creating/opening a chunk, varying the size of the buffers "));
	bufferConfig.iNumBuffers=4;
	
	for (i=1 ; i<=8 ; i*=2)
		{
		bufferConfig.iFlags=aBufferConfigFlags;
		bufferConfig.iBufferSizeInBytes=(0x8000/i);
		
		// Create the shared chunk on the play channel
		r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
		CHECK_NOERROR(r);
		TxSoundDevice.GetBufferConfig(bufferConfigBuf);
		PrintBufferConf(bufferConfig,Test);
		
		// Open the same shared chunk on the record channel
		r=RxSoundDevice.SetBufferChunkOpen(bufferConfigBuf,chunk);
		CHECK_NOERROR(r);
		
		chunk.Close();
		}
		
	// Try creating a shared chunk for the record channel specifying an illegal buffer size.
	if (RecordCapsBuf().iRequestMinSize)
		{
		Test.Next(_L("Test creating a chunk, with an illegal buffer size"));
		bufferConfig.iNumBuffers=1;
		bufferConfig.iBufferSizeInBytes=0x1001;		// 4K + 1byte
		bufferConfig.iFlags=aBufferConfigFlags;
		r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
		CHECK(r==KErrArgument);
		chunk.Close();		
		}	
			
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-234
	@SYMTestCaseDesc 		Setting the buffer configuration - specifying the buffer offsets.
	@SYMTestPriority 		Critical
	@SYMTestActions			Issue a request on the record channel to create a shared chunk (i.e. 
							using SetBufferChunkCreate()) with an buffer configuration containing multiple buffers 
							where the offset of each buffer is specified by the client. Perform this test repeatedly 
							with buffer offsets specified as follows:-
							1)	Valid buffer offsets
							2)	Offsets resulting on overlapping  buffers
							3)	Offsets which aren't aligned with the request alignment restrictions for the device (if it has any).
	@SYMTestExpectedResults	1)	Setting the buffer configuration should return KErrNone. 
							2)	Setting the buffer configuration should fail with KErrArgument.
							3)	Setting the buffer configuration should fail with KErrArgument.
	@SYMREQ					PREQ1073.4 */			
/*		
	// ***** Setting the buffer configuration - specifying the buffer offsets is not supported. *****
	
	Test.Next(_L("Test creating a chunk, specifying the buffer offsets"));
	TInt bufSize=0x2000;
	bufferConfig.iNumBuffers=8;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iBufferOffsetList[0]=0;
	bufferConfig.iBufferOffsetList[1]=bufSize;
	bufferConfig.iBufferOffsetList[2]=bufSize*4;
	bufferConfig.iBufferOffsetList[3]=bufSize*6;
	bufferConfig.iBufferOffsetList[4]=bufSize*7;
	bufferConfig.iBufferOffsetList[5]=bufSize*9;
	bufferConfig.iBufferOffsetList[6]=bufSize*10;
	bufferConfig.iBufferOffsetList[7]=bufSize*12;
	bufferConfig.iFlags=(aBufferConfigFlags|KScFlagBufOffsetListInUse);
	bufferConfigBuf.SetLength(sizeof(bufferConfig));	// Otherwise set shorter by previous GetBufferConfig().
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	chunk.Close();	
	
	Test.Next(_L("Test creating a chunk, with invalid buffer offsets specified"));
	
	// Firstly with 2nd buffer overlapping the 1st
	bufSize=0x2000;
	bufferConfig.iNumBuffers=3;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iBufferOffsetList[0]=0;
	bufferConfig.iBufferOffsetList[1]=bufSize/2;
	bufferConfig.iBufferOffsetList[2]=bufSize*2;
	bufferConfig.iFlags=(aBufferConfigFlags|KScFlagBufOffsetListInUse);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK(r==KErrArgument);
	chunk.Close();	
	
	// Now with offset of 3rd buffer not on a page boundary
	if (RecordCapsBuf().iRequestAlignment)
		{
		bufSize=0x2000;
		bufferConfig.iNumBuffers=3;
		bufferConfig.iBufferSizeInBytes=bufSize;
		bufferConfig.iBufferOffsetList[0]=0;
		bufferConfig.iBufferOffsetList[1]=bufSize;
		bufferConfig.iBufferOffsetList[2]=(bufSize*2)+1;
		bufferConfig.iFlags=(aBufferConfigFlags|KScFlagBufOffsetListInUse);
		r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
		CHECK(r==KErrArgument);
		chunk.Close();		
		}
*/		
	}

LOCAL_C void TestPlay()
	{
	
	Test.Printf(_L("Testing play operation\r\n"));
	
	// Close and re-open the playback channel to reset the driver.
	TxSoundDevice.Close();
	TInt r = TxSoundDevice.Open(KSoundScTxUnit0);
	CHECK_NOERROR(r);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-235
	@SYMTestCaseDesc 		Play operation - playing without having set the buffer configuration.
	@SYMTestPriority 		Critical
	@SYMTestActions			Issue a request on the playback channel to play some audio data before having set-up 
							the buffer configuration.
	@SYMTestExpectedResults	The play request should fail with KErrNotReady.
	@SYMREQ					PREQ1073.4 */			
	
	Test.Next(_L("Test playing without setting buffer config"));
	TRequestStatus stat[2];
	TxSoundDevice.PlayData(stat[0],0,0x2000);	// 8K
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrNotReady);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-236
	@SYMTestCaseDesc 		Play operation - playing without having set the audio configuration.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the buffer configuration on the playback channel and then issue a request to 
							play audio data before having set-up the audio configuration
	@SYMTestExpectedResults	The play request should complete with KErrNone - with the driver using its default 
							audio configuration for the transfer.
	@SYMREQ					PREQ1073.4 */	
	
	Test.Next(_L("Test playing without setting audio config"));
	
	// Read back the default play configuration
	TxSoundDevice.AudioFormat(PlayFormatBuf);
	TCurrentSoundFormatV02 playFormat=PlayFormatBuf();
	PrintConfig(playFormat,Test);
	
	// Set the play buffer configuration.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(PlayFormatBuf()); 										// Large enough to hold 1 second of data.
	bufSize=ValidBufferSize(bufSize,PlayCapsBuf().iRequestMinSize,PlayFormatBuf());		// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=1;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;	
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	TxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);
	
	// Start playing
	r=MakeSineTable(PlayFormatBuf());
	CHECK_NOERROR(r);
	r=SetToneFrequency(660,PlayFormatBuf());
	CHECK_NOERROR(r); 
	TPtr8 ptr(chunk.Base()+bufferConfig.iBufferOffsetList[0],bufSize);
	WriteTone(ptr,PlayFormatBuf());
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize,KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_NOERROR(stat[0].Int());
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-239
	@SYMTestCaseDesc 		Play operation - with invalid arguments.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the playback channel to commence playing audio data using a buffer configuration 
							containing multiple buffers. Now issue a series of transfer requests to play data where the 
							requests contain invalid request arguments as follows:-
							1)	A request with a buffer offset before the start of the first buffer.
							2)	A request with a buffer offset between buffers.
							3)	A request with a length too large for a buffer.
							4)	A request with a buffer offset that doesn't comply with the offset restrictions of 
								the driver (i.e. TSoundFormatsSupportedV02:: iRequestAlignment).
							5)	A request with a length that doesn't comply with the length restrictions of 
								the driver (i.e. TSoundFormatsSupportedV02::iRequestMinSize).
	@SYMTestExpectedResults	1)	The play request should fail with KErrArgument.
							2)	The play request should fail with KErrArgument.
							3)	The play request should fail with KErrArgument.
							4)	The play request should fail with KErrArgument.
							5)	The play request should fail with KErrArgument
	@SYMREQ					PREQ1073.4 */	
	
	Test.Next(_L("Test playing with invalid arguments"));
	
	// With a buffer offset before the start of the buffer.
	TxSoundDevice.PlayData(stat[0],(bufferConfig.iBufferOffsetList[0]-0x100),bufSize,KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrArgument);
	
	// With a buffer offset beyond the end of the buffer
	TxSoundDevice.PlayData(stat[0],(bufferConfig.iBufferOffsetList[0]+bufSize+0x100),bufSize,KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrArgument);
	
	// With a length too large for the buffer
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],(bufSize+0x100),KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrArgument);
	
	// With a length that doesn't comply with the length restrictions of the driver
	if (PlayCapsBuf().iRequestMinSize)
		{
		TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],(bufSize-1),KSndFlagLastSample);
		User::WaitForRequest(stat[0]);
		CHECK_EQUAL(stat[0].Int(),KErrArgument);
		}
		
	// With an offset that doesn't comply with the offset restrictions of the driver
	if (PlayCapsBuf().iRequestAlignment)
		{
		TInt ln=bufSize/2;
		if (PlayCapsBuf().iRequestMinSize)
			ln&=~(PlayCapsBuf().iRequestMinSize-1);	// Keep the buffer length valid for the driver.
		TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0]+1,ln,KSndFlagLastSample);
		User::WaitForRequest(stat[0]);
		CHECK_EQUAL(stat[0].Int(),KErrArgument);
		}	
		
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-242
	@SYMTestCaseDesc 		Play operation - play underflow detection.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the playback channel and then setup the buffer 
							configuration. Issue a single request to play audio data from one of the buffers 
							(without supplying the KSndFlagLastSample flag) and wait for the request to complete.
	@SYMTestExpectedResults	The driver should successfully play the audio data supplied but the request should 
							return the error value: KErrUnderflow.
	@SYMREQ					PREQ1073.4 */		
	
	Test.Next(_L("Test play underflow detection"));
	
	// Now set the audio configuration to a known state.
	if (PlayCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	if (PlayCapsBuf().iRates&KSoundRate16000Hz)
		PlayFormatBuf().iRate = ESoundRate16000Hz;
	if (PlayCapsBuf().iChannels&KSoundStereoChannel)
		PlayFormatBuf().iChannels = 2;
	PrintConfig(PlayFormatBuf(),Test);
	r=TxSoundDevice.SetAudioFormat(PlayFormatBuf);
	CHECK_NOERROR(r);
	
	// Reset the play buffer configuration to correspond to the audio configuration.
	bufSize=BytesPerSecond(PlayFormatBuf()); 											// Large enough to hold 1 second of data (64K)
	bufSize=ValidBufferSize(bufSize,PlayCapsBuf().iRequestMinSize,PlayFormatBuf());		// Keep the buffer length valid for driver.
	bufferConfig.iNumBuffers=1;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;	
	r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	TxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);
	
	// Start playing
	r=MakeSineTable(PlayFormatBuf());
	CHECK_NOERROR(r);
	r=SetToneFrequency(660,PlayFormatBuf());
	CHECK_NOERROR(r); 
	ptr.Set(chunk.Base()+bufferConfig.iBufferOffsetList[0],bufSize,bufSize);
	WriteTone(ptr,PlayFormatBuf());
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize);
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrUnderflow);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-243
	@SYMTestCaseDesc 		Play operation - playing multiple transfers from a single buffer.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the playback channel and setup the buffer configuration 
							so the shared chunk contains just a single large buffer. Issue a pair of requests to play 
							audio data from the buffer (marking the last with the KSndFlagLastSample flag) and wait 
							for the requests to complete.
	@SYMTestExpectedResults	The driver should successfully play the audio data supplied and both requests should 
							complete with KErrNone.
	@SYMREQ					PREQ1073.4 */	
	
	Test.Next(_L("Test playing multiple transfers from a single buffer"));
	
	// Start playing
	TInt len=bufSize/2;
	if (PlayCapsBuf().iRequestMinSize)
		len&=~(PlayCapsBuf().iRequestMinSize-1);	// Keep the buffer length valid for the driver.
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],len);
	TxSoundDevice.PlayData(stat[1],(bufferConfig.iBufferOffsetList[0]+len),len,KSndFlagLastSample);
	bool brokenByPaging = false;
	if((stat[0] != KRequestPending) || (stat[1] != KRequestPending))
		{
		brokenByPaging = true;
		Test.Printf(_L("Paging gap between PlayData calls - skipping test\n"));
		}
	User::WaitForRequest(stat[0]);
	if(!brokenByPaging) CHECK_NOERROR(stat[0].Int());
	User::WaitForRequest(stat[1]);
	if(!brokenByPaging) CHECK_NOERROR(stat[1].Int());
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-244
	@SYMTestCaseDesc 		Play operation - tracking the count of bytes transferred.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the playback channel for playing audio data. 
							1)	Reset the channel's count of bytes transferred and read back the count.
							2)	Issue a play request of known length and wait for it to complete. Now read the 
								count of bytes transferred.
							3)	Reset the channel's count of bytes transferred and read back the count.
	@SYMTestExpectedResults	1)	The count of bytes transferred should equal zero.
							2)	The count of bytes transferred should equal the length of the play request.
							3)	The count of bytes transferred should equal zero.
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Test tracking the number of bytes played"));
	TxSoundDevice.ResetBytesTransferred();
	TInt bytesPlayed=TxSoundDevice.BytesTransferred();
	CHECK(bytesPlayed==0);
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],len,KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_NOERROR(stat[0].Int());
	bytesPlayed=TxSoundDevice.BytesTransferred();
	CHECK(bytesPlayed==len);
	TxSoundDevice.ResetBytesTransferred();
	bytesPlayed=TxSoundDevice.BytesTransferred();
	CHECK(bytesPlayed==0);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-246
	@SYMTestCaseDesc 		Play operation - testing the driver's handling of PDD play errors.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the playback channel for playing audio data. Using the CustomConfig() test mode 
							supported by the driver, induce the following errors returned from the driver PDD back 
							to the LDD:-
								1)	Induce a KErrTimedOut error at the start of the transfer and then issue a play request.
								2)	Induce a KErrTimedOut error once transfer has commenced and then issue a play request. 
								Once this completes, issue a further play request.
	@SYMTestExpectedResults	1)	The play request should fail immediately with KErrTimedOut (with no audible output being 
								heard).
							2)	The driver should output a short section of audible data before the first play request 
								fails with KErrTimedOut. However, the driver should recover from this and the second 
								play request should complete with KErrNone.
	@SYMREQ					PREQ1073.4 */
	
#ifdef _DEBUG	
	Test.Next(_L("Test the driver's handling of PDD play errors"));
	
	// First induce an error at the start of the transfer
	// Use an LDD test mode which induces KErrTimedOut at the start of the transfer.
	r=TxSoundDevice.CustomConfig(KSndCustom_ForceStartTransferError);
	CHECK_NOERROR(r);
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize,KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrTimedOut);
	
	// Now induce an error once transfer has commenced
	// Use an LDD test mode which induces KErrTimedOut once transfer has commenced.
	r=TxSoundDevice.CustomConfig(KSndCustom_ForceTransferDataError);
	CHECK_NOERROR(r);
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize,KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_EQUAL(stat[0].Int(),KErrTimedOut);
	
	// Check that the driver recovers
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],bufSize,KSndFlagLastSample);
	User::WaitForRequest(stat[0]);
	CHECK_NOERROR(stat[0].Int());
#endif	
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-247
	@SYMTestCaseDesc 		Play operation - closing the channel cancels any play requests.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the playback channel for playing audio data. Issue a play request and 
							while this is outstanding, close the playback channel.
	@SYMTestExpectedResults	The outstanding request should complete with KErrCancel.
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Test that closing the channel cancels a play request"));
	TxSoundDevice.PlayData(stat[0],bufferConfig.iBufferOffsetList[0],len,KSndFlagLastSample);
	CHECK(stat[0]==KRequestPending);
	TxSoundDevice.Close();
	User::WaitForRequest(stat[0]);
	CHECK(stat[0]==KErrCancel);
	
	Test.Next(_L("Re-open the channel"));
	r = TxSoundDevice.Open(KSoundScTxUnit0);
	CHECK_NOERROR(r);
	
	chunk.Close();
	}
	
LOCAL_C void TestRecord()
	{
	
	Test.Printf(_L("Testing record operation\r\n"));
	
	// Close and re-open the record channel to reset the driver.
	RxSoundDevice.Close();
	TInt r = RxSoundDevice.Open(KSoundScRxUnit0);
	CHECK_NOERROR(r);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-251
	@SYMTestCaseDesc 		Record operation - recording without having set the buffer configuration.
	@SYMTestPriority 		Critical
	@SYMTestActions			Issue a request on the record channel to record some audio data before 
							having set-up the buffer configuration.
	@SYMTestExpectedResults	The record request should fail with KErrNotReady.
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Test recording without setting buffer config"));
	
	TRequestStatus stat;
	TInt length;
	RxSoundDevice.RecordData(stat,length);
	User::WaitForRequest(stat);
	TInt retOffset=stat.Int();
	CHECK_EQUAL(retOffset,KErrNotReady);
	
	// Test releasing a buffer without setting buffer config
	r=RxSoundDevice.ReleaseBuffer(0);
	CHECK(r==KErrNotReady);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-252
	@SYMTestCaseDesc 		Record operation - recording without having set the audio configuration.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the buffer configuration on the record channel. 
							1)	Issue a request to record a buffer of audio data before having set-up the audio 
								configuration.
							2)	Release the buffer.
	@SYMTestExpectedResults	1)	The record request should complete with KErrNone - with the driver using its 
								default audio configuration for the transfer.
							2)	The request to release the buffer should return KErrNone
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Test recording without setting audio config"));
	
	// Read back the default record configuration
	RxSoundDevice.AudioFormat(RecordFormatBuf);
	TCurrentSoundFormatV02 recordFormat=RecordFormatBuf();
	PrintConfig(recordFormat,Test);
	
	// Set the record buffer configuration.
	RChunk chunk;
	TInt bufSize=BytesPerSecond(RecordFormatBuf()); 										// Large enough to hold 1 second of data.
	bufSize=ValidBufferSize(bufSize,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());		// Keep the buffer length valid for driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=3;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;	
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);
	
	// Start recording
	RxSoundDevice.SetVolume(KSoundMaxVolume);
	RxSoundDevice.RecordData(stat,length);		// Start recording here
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	CHECK(retOffset>=0);
	CHECK(length==bufSize);
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-255
	@SYMTestCaseDesc 		Record operation - invalid attempts to release a record buffer.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the record channel to record audio data. Issue a request to record a buffer of audio data
							and then release the buffer.
							1)	Request to release the same buffer just released (without having re-claimed it with a 
								further record request).
							2)	Release a further buffer - specifying an illegal buffer offset for the buffer.
							3)	Stop the driver from recording (using CancelRecordData()). Now release a buffer while 
								recording is disabled.
	@SYMTestExpectedResults	1)	The request to release the buffer should fail with KErrNotFound.
							2)	The request to release the buffer should fail with KErrNotFound.
							3)	The request to release the buffer should fail with KErrNotFound.
	@SYMREQ					PREQ1073.4 */
	
	// Test releasing a buffer twice
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK(r==KErrNotFound);
	
	// Test releasing an invalid buffer offset
	r=RxSoundDevice.ReleaseBuffer(0);
	CHECK(r==KErrNotFound);
	
	// Third test case for PBASE-T_SOUND_API-255 is performed later in this function.
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-256
	@SYMTestCaseDesc 		Record operation - record overflow detection.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the record channel and then setup the buffer configuration 
							so the shared chunk contains three record buffers.
							1)	Issue a request to record a buffer of audio data (to start the driver recording).
							2)	When the request completes, release the buffer immediately.
							3)	Wait just over 2 buffer periods (i.e. the time it takes for the driver to fill 2 record 
								buffers) and then issue a further record request.
							4)	To test that the driver recovers from the overflow situation, immediately issue a further 
								record request. 
	@SYMTestExpectedResults	1)	The record request should complete with KErrNone.
							2)	The request to release the buffer should return KErrNone.
							3)	The record request should fail with KErrOverflow.
							4)	The record request should complete with KErrNone.
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Test for record overflow"));

	// Now set the audio configuration to a known state.
	RxSoundDevice.CancelRecordData();	// Stop the driver from recording.

	RxSoundDevice.AudioFormat(RecordFormatBuf);	// Read back the current setting which must be valid.
	if (RecordCapsBuf().iEncodings&KSoundEncoding16BitPCM)
		RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	if (RecordCapsBuf().iChannels&KSoundStereoChannel)
		RecordFormatBuf().iChannels = 2;

	// find first supported rate and set the the audio configuration to use it
	for (TInt i=0 ; i <= (TInt)ESoundRate48000Hz ; i++)
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
	
	// Reset the record buffer configuration to correspond to the audio configuration.
	bufSize=BytesPerSecond(RecordFormatBuf())/2; 	 										// Large enough to hold 0.5 seconds of data (64K)
	bufSize=ValidBufferSize(bufSize,RecordCapsBuf().iRequestMinSize,RecordFormatBuf());		// Keep the buffer length valid for driver.
	bufferConfig.iNumBuffers=3;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;	
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	CHECK_NOERROR(r);
	RxSoundDevice.GetBufferConfig(bufferConfigBuf);
	PrintBufferConf(bufferConfig,Test);
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);
	
	// Test releasing a buffer before we have started recording
	r=RxSoundDevice.ReleaseBuffer(bufferConfig.iBufferOffsetList[0]);
	CHECK(r==KErrNotFound);
	
	RxSoundDevice.RecordData(stat,length);	// Start recording here
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	CHECK(retOffset>=0);
	CHECK(length==bufSize);
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);
	
	User::After(1100000); // Wait 2 buffer periods (1.1 seconds) for data overflow. 
	
	RxSoundDevice.RecordData(stat,length);
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	CHECK(retOffset==KErrOverflow);
	
	Test.Next(_L("Test that the driver recovers from overflow"));
	
	RxSoundDevice.RecordData(stat,length);
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	CHECK(retOffset>=0);
	CHECK(length==bufSize);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-257
	@SYMTestCaseDesc 		Record operation - when client is too slow releasing buffers.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the audio configuration on the record channel and then setup the buffer configuration 
							so the shared chunk contains three record buffers. Issue a request to record a buffer of 
							audio data (to start the driver recording). When this request completes, issue a further 
							record request without releasing the first buffer.
	@SYMTestExpectedResults	The second record request should fail with KErrInUse.
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Test when client is too slow releasing buffers"));
	
	RxSoundDevice.RecordData(stat,length);
	User::WaitForRequest(stat);
	TInt retOffset1=stat.Int();
	CHECK(retOffset1==KErrInUse);	
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-258
	@SYMTestCaseDesc 		Record operation - tracking the count of bytes transferred.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the record channel for recording audio data. 
							1)	Reset the channel's count of bytes transferred and read back the count.
							2)	Issue a record request (to start the driver recording) and wait for it to complete. Immediately read the count of bytes transferred.
							3)	Reset the channel's count of bytes transferred and read back the count
	@SYMTestExpectedResults	1)	The count of bytes transferred should equal zero.
							2)	The count of bytes transferred should equal the size of a record buffer.
							3)	The count of bytes transferred should equal zero.
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Test tracking the number of bytes recorded"));
	RxSoundDevice.CancelRecordData();				// Stop the driver from recording.
	RxSoundDevice.ResetBytesTransferred();
	TInt bytesRecorded=RxSoundDevice.BytesTransferred();
	CHECK(bytesRecorded==0);
	RxSoundDevice.RecordData(stat,length);			// Re-start recording here
	User::WaitForRequest(stat);
	
	// Quickly check the number of bytes recorded
	bytesRecorded=RxSoundDevice.BytesTransferred();
	CHECK(bytesRecorded==bufSize);
	
	// Now check the request was successful and release the buffer
	retOffset=stat.Int();
	CHECK(retOffset>=0);
	CHECK(length==bufSize);
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);
	
	RxSoundDevice.ResetBytesTransferred();
	bytesRecorded=RxSoundDevice.BytesTransferred();
	CHECK(bytesRecorded==0);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-260
	@SYMTestCaseDesc 		Record operation - testing the driver's handling of PDD record errors.
	@SYMTestPriority 		Critical
	@SYMTestActions			Setup the record channel for recording. Using the CustomConfig() test mode supported by the driver, induce 
							the following errors returned from the driver PDD back to the LDD:-
							1)	Induce a KErrTimedOut error at the start of the transfer. Then, issue a record request to 
								start the driver recording. Once this completes, stop the channel from recording. 
							2)	Induce a KErrTimedOut error once transfer has commenced. Then, issue a record request to 
								re-start the driver recording. Once this completes, issue a further record request.
	@SYMTestExpectedResults	1)	The record request should fail immediately with KErrTimedOut. 
							2)	The first record request should soon fail with KErrTimedOut. However, the driver should 
								recover from this and the subsequent record request should complete with KErrNone.
	@SYMREQ					PREQ1073.4 */
	
#ifdef _DEBUG	
	Test.Next(_L("Test the driver's handling of PDD record errors"));
	
	Test.Printf(_L("Induce an error at the start of the transfer.\r\n"));
	// Use an LDD test mode which induces KErrTimedOut at the start of the transfer.
	RxSoundDevice.CancelRecordData();				// Stop the driver from recording.
	r=RxSoundDevice.CustomConfig(KSndCustom_ForceStartTransferError);
	CHECK_NOERROR(r);
	RxSoundDevice.RecordData(stat,length);			// Re-start recording here
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	CHECK(retOffset==KErrTimedOut);
	RxSoundDevice.CancelRecordData();				// Stop the driver from recording.
	
	Test.Printf(_L("Induce an error once transfer has commenced.\r\n"));
	// Use an LDD test mode which induces KErrTimedOut once transfer has commenced.
	r=RxSoundDevice.CustomConfig(KSndCustom_ForceTransferDataError);
	CHECK_NOERROR(r);
	RxSoundDevice.RecordData(stat,length);			// Re-start recording here
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	CHECK(retOffset==KErrTimedOut);
	
	Test.Printf(_L("Check that the driver recovers.\r\n"));
	RxSoundDevice.RecordData(stat,length);
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	CHECK(retOffset>=0);
	CHECK(length==bufSize);
	r=RxSoundDevice.ReleaseBuffer(retOffset);
	CHECK_NOERROR(r);
	
/*	Test disabled 	
	Test.Printf(_L("Induce LDD to be slow servicing transfer completes from PDD.\r\n"));
	r=RxSoundDevice.CustomConfig(KSndCustom_ForceTransferTimeout);
	CHECK_NOERROR(r);
	RxSoundDevice.RecordData(stat,length);
	User::WaitForRequest(stat);
	retOffset=stat.Int();
	Test.Printf(_L("Driver returned %d.\r\n"),retOffset); */
#endif	
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-261
	@SYMTestCaseDesc 		Record operation - closing the channel cancels any record requests.
	@SYMTestPriority 		Critical
	@SYMTestActions			Start the record channel for recording audio data. Issue a record request and while this is 
							outstanding, close the record channel
	@SYMTestExpectedResults	The outstanding request should complete with KErrCancel.
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Test that closing the channel cancels a play request"));
	RxSoundDevice.RecordData(stat,length);
	CHECK(stat==KRequestPending);
	RxSoundDevice.Close();
	User::WaitForRequest(stat);
	CHECK(stat==KErrCancel);
	
	Test.Next(_L("Re-open the channel"));
	r = RxSoundDevice.Open(KSoundScRxUnit0);
	CHECK_NOERROR(r);
	
	chunk.Close();
	}

#ifdef _DEBUG
LOCAL_C void TestChangeOfHwConfigNotifier()
	{
	
	Test.Printf(_L("Testing the change of h/w config notifier\r\n"));
	
	// If support for the notifier is not supported on this platform, use a debug custom config command to force
	// support for the notifier.
	TInt r;
	if (!PlayCapsBuf().iHwConfigNotificationSupport)
		{
		r=TxSoundDevice.CustomConfig(KSndCustom_ForceHwConfigNotifSupported);
		CHECK_NOERROR(r);
		}
		
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-264
	@SYMTestCaseDesc 		Change of h/w config. notifier - receiving notification.
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	Register for notification of a change in the hardware configuration of the device on 
								either the record or playback channel. Using the CustomConfig() test mode supported by the 
								driver, trigger the notifier - signalling that the headset is not present.
							2) 	Re-register for notification of a change in the hardware configuration on the same channel.
								Again using the CustomConfig() test mode, trigger the notifier - this time signalling that the 
								headset is present.
	@SYMTestExpectedResults	1)	The outstanding request for notification should complete with KErrNone and the status of 
								the bolean aHeadsetPresent should be false.
							2)	The outstanding request for notification should complete with KErrNone and the status of
								the bolean aHeadsetPresent should be true.
	@SYMREQ					PREQ1073.4 */	
	
	// Register for notification of a change in the hardware configuration of the device.
	TRequestStatus stat;
	TBool headsetPresent=ETrue;
	TxSoundDevice.NotifyChangeOfHwConfig(stat,headsetPresent);
	CHECK(stat==KRequestPending);
	
	Test.Next(_L("Trigger the notifier"));
	// Use another debug custom config command to trigger the notifier.
	r=TxSoundDevice.CustomConfig(KSndCustom_CompleteChangeOfHwConfig,(TAny*)0x00);	// Signal headset not present.
	CHECK_NOERROR(r);
	User::WaitForRequest(stat);
	CHECK_NOERROR(stat.Int());
	CHECK(!headsetPresent);
	
	Test.Next(_L("Trigger the notifier again"));
	TxSoundDevice.NotifyChangeOfHwConfig(stat,headsetPresent);
	CHECK(stat==KRequestPending);
	r=TxSoundDevice.CustomConfig(KSndCustom_CompleteChangeOfHwConfig,(TAny*)0x01);	// Signal headset is present.
	CHECK_NOERROR(r);
	User::WaitForRequest(stat);
	CHECK_NOERROR(stat.Int());
	CHECK(headsetPresent);
	
	Test.Next(_L("Test cancelling the notifier"));
	TxSoundDevice.NotifyChangeOfHwConfig(stat,headsetPresent);
	CHECK(stat==KRequestPending);
	TxSoundDevice.CancelNotifyChangeOfHwConfig();
	User::WaitForRequest(stat);
	CHECK(stat==KErrCancel);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-265
	@SYMTestCaseDesc 		Change of h/w config. notifier - receiving notification.
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	Register for notification of a change in the hardware configuration of the device on 
								either the record or playback channel. Using the CustomConfig() test mode supported by the 
								driver, trigger the notifier - signalling that the headset is not present.
							2) 	Re-register for notification of a change in the hardware configuration on the same channel.
								Again using the CustomConfig() test mode, trigger the notifier - this time signalling that the 
								headset is present.
	@SYMTestExpectedResults	1)	The outstanding request for notification should complete with KErrNone and the status of 
								the bolean aHeadsetPresent should be false.
							2)	The outstanding request for notification should complete with KErrNone and the status of
								the bolean aHeadsetPresent should be true.
	@SYMREQ					PREQ1073.4 */	
	
	Test.Next(_L("Test that closing the channel cancels the notifier"));
	TxSoundDevice.NotifyChangeOfHwConfig(stat,headsetPresent);
	CHECK(stat==KRequestPending);
	TxSoundDevice.Close();
	User::WaitForRequest(stat);
	CHECK(stat==KErrCancel);
	
	Test.Next(_L("Re-open the channel"));
	r = TxSoundDevice.Open(KSoundScTxUnit0);
	CHECK_NOERROR(r);
	}
#endif	
	
LOCAL_C void TestUnloadDrivers()
	{
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-221
	@SYMTestCaseDesc 		Un-installing the LDD
	@SYMTestPriority 		Critical
	@SYMTestActions 		Attempt to un-install the LDD (using User::FreeLogicalDevice()). 
	@SYMTestExpectedResults KErrNone - LDD unloads successfully 
	@SYMREQ					PREQ1073.4 */
	
	TInt r=User::FreeLogicalDevice(KDevSoundScName);
	Test.Printf(_L("Unloading %S.LDD - %d\r\n"),&KDevSoundScName,r);
	CHECK_NOERROR(r);
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-222
	@SYMTestCaseDesc 		Un-installing the PDD
	@SYMTestPriority 		Critical
	@SYMTestActions 		Attempt to un-install the PDD (using User::FreePhysicalDevice()). 
	@SYMTestExpectedResults KErrNone - PDD unloads successfully 
	@SYMREQ					PREQ1073.4 */
	
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

/**
Invoke the helper executable, tell it what unit to open RSoundSc for.

@param aUnit Parameter to supply to RSoundSc ie. KSoundScTxUnit0, KSoundScRxUnit0
@param aExpectedError The expected return code from RSoundSc::Open
 */
static void RunHelper(TInt aUnit, TInt aExpectedError)
	{
	TInt r;
	
	RProcess ph;
	r = ph.Create(KHelperExe, KNullDesC);
	CHECK_NOERROR(r);
	r = ph.SetParameter(KSlot, aUnit); 
	CHECK_NOERROR(r);

	TRequestStatus rsh;
	ph.Logon(rsh);
	Test(rsh == KRequestPending);
	ph.Resume();
	User::WaitForRequest(rsh);

	// process has died so check the panic category and reason match the expected values
	CHECK_EQUAL(ph.ExitType(), EExitPanic);
	Test(ph.ExitCategory()==KSoundAPICaps,__LINE__,(const TText*)__FILE__);
	CHECK_EQUAL(ph.ExitReason(), aExpectedError);

	ph.Close();
	}

/**
Delete the copy of the helper exe which was created with altered capabilities
*/
static TInt DeleteHelper()
	{
	RFs fs;
	TInt r = fs.Connect();
	CHECK_NOERROR(r);
	TFileName fileName(KSysBin);
	fileName.Append(KPathDelimiter);
	fileName+=KHelperExe;
	//delete modified version of helper exe
	r = fs.Delete(fileName);
	fs.Close();
	return r;
	}

/**
Use setcap.exe to create a copy of the helper executable, t_sound_api_helper.exe
called t_sound_api_helper_caps.exe with the supplied capabilities.

@param aCaps A bitmask of capabilities to assign to helper exe.
*/
static void SetHelperCaps(TUint32 aCaps)
	{
	TInt r;
	_LIT(KCommandLineArgsFormat, "%S %08x %S\\%S");
	TBuf<128> cmdLine;
	cmdLine.Format(KCommandLineArgsFormat, &KHelperExeBase, aCaps, &KSysBin, &KHelperExe);

	RProcess p;
	r = p.Create(_L("setcap.exe"), cmdLine);
	CHECK_NOERROR(r);

	TRequestStatus rs;
	p.Logon(rs);
	CHECK_EQUAL(rs.Int(),KRequestPending);
	p.Resume();
	User::WaitForRequest(rs);

	p.Close();
	}

/**
Test that ECapabilityMultimediaDD is required
in order to open RSoundSc for playback and that
ECapabilityUserEnvironment is needed, in addition,
to open for record.
*/
static void TestCapabilityChecks()
	{
	//convert capabilities into bitmask understood by setcap.exe
	const TUint32 KUserEnvironment = 1<<ECapabilityUserEnvironment;
	const TUint32 KMultimediaDD= 1<<ECapabilityMultimediaDD;
	
	//try to delete helper just in case it has been left hanging about from a previous run which failed
	DeleteHelper();
	SetHelperCaps(NULL);
	RunHelper(KSoundScTxUnit0,KErrPermissionDenied);
	RunHelper(KSoundScRxUnit0,KErrPermissionDenied);
	
	CHECK_NOERROR(DeleteHelper());
	SetHelperCaps(KMultimediaDD);
	RunHelper(KSoundScTxUnit0,KErrNone);
	RunHelper(KSoundScRxUnit0,KErrPermissionDenied);

	CHECK_NOERROR(DeleteHelper());
	SetHelperCaps(KUserEnvironment);
	RunHelper(KSoundScTxUnit0,KErrPermissionDenied);
	RunHelper(KSoundScRxUnit0,KErrPermissionDenied);

	CHECK_NOERROR(DeleteHelper());
	SetHelperCaps(KUserEnvironment|KMultimediaDD);
	RunHelper(KSoundScTxUnit0,KErrNone);
	RunHelper(KSoundScRxUnit0,KErrNone);

	CHECK_NOERROR(DeleteHelper());
	}

TInt E32Main()
	{
	TInt r;

	__UHEAP_MARK;

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

	// The __KHEAP_MARK and __KHEAP_MARKEND macros unfortunately have to be commented out or they
	// generate false positives on the x86pc target build.  This is to do with the launching of the
	// helper executable, which results in another component allocating memory and not freeing it,
	// thus causing the macros here to trigger a kern-exec 17 that is not related to the sound driver
	// itself.  The same macros are fine on other platforms.  See DEF129273 for more information
	//__KHEAP_MARK;

	Test.Next(_L("Test Capability Checking"));
	TestCapabilityChecks();
		
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

	Test.Next(_L("Query record formats supported"));
	RxSoundDevice.Caps(RecordCapsBuf);
	TSoundFormatsSupportedV02 recordCaps=RecordCapsBuf();
	PrintCaps(recordCaps,Test);
	
	TestSetAudioFormat();
	TestSetBufferConfig(0);							// Without guard pages.
	TestSetBufferConfig(KScFlagUseGuardPages);		// With guard pages.
	TestPlay();
	TestRecord();
#ifdef _DEBUG		
	TestChangeOfHwConfigNotifier();
#endif			
	// Note that API testing of pause/resume and volume setting are covered in T_SOUND2.
	
	/**	@SYMTestCaseID 		PBASE-T_SOUND_API-226
	@SYMTestCaseDesc 		Closing the channel
	@SYMTestPriority 		Critical
	@SYMTestActions			1)	With a playback channel open on the device, close the channel. 
							2)	With a record channel open on the device, close the channel. 
	@SYMTestExpectedResults	1)	No errors occur closing the channel.
							2)	No errors occur closing the channel.
	@SYMREQ					PREQ1073.4 */
	
	Test.Next(_L("Close channels"));
	RxSoundDevice.Close();
	TxSoundDevice.Close();
	
	//__KHEAP_MARKEND;

	// Now that both the channels are closed, unload the LDD and the PDDs.
	TestUnloadDrivers();

	Test.End();
	Test.Close();

	Cleanup();
	
	__UHEAP_MARKEND;

	return(KErrNone);
	}
