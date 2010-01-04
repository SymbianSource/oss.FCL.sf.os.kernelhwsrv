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
// e32test\multimedia\t_soundwav.cpp
// Record wav to a file "t_soundwav filename [rate] [channels] [seconds]" where filename does not exist.
// Play a wav file "t_soundwav filename" where filename exists.
// 
//

/**
 @file Shared chunk sound driver WAV file player and recorder utility.
*/

#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>
#include "t_soundutils.h"
#include <f32file.h>

#define CHECK(aValue) {Test(aValue);}
#define CHECK_NOERROR(aValue) { TInt v=(aValue); if(v) { Test.Printf(_L("Error value = %d\n"),v); Test(EFalse,__LINE__); }}
#define CHECK_EQUAL(aValue1,aValue2) { TInt v1=(aValue1); TInt v2=(aValue2); if(v1!=v2) { Test.Printf(_L("Error value = %d\n"),v1); Test(EFalse,__LINE__); }}

_LIT(KSndLddFileName,"ESOUNDSC.LDD");
_LIT(KSndPddFileName,"SOUNDSC.PDD");

RTest Test(_L("T_SOUNDWAV"));
RSoundSc TxSoundDevice;
RSoundSc RxSoundDevice;

TSoundFormatsSupportedV02Buf RecordCapsBuf;
TSoundFormatsSupportedV02Buf PlayCapsBuf;
TCurrentSoundFormatV02Buf PlayFormatBuf;
TCurrentSoundFormatV02Buf RecordFormatBuf;

TBuf<256> CommandLine;
RFs Fs;

//
// This is the WAV header structure used for playing and recording files
//
struct WAVEheader
	{
	char ckID[4];				// chunk id 'RIFF'
	TUint32 ckSize;				// chunk size
	char wave_ckID[4];			// wave chunk id 'WAVE'
	char fmt_ckID[4];			// format chunk id 'fmt '
	TUint32 fmt_ckSize;			// format chunk size
	TUint16 formatTag;			// format tag currently pcm
	TUint16 nChannels;			// number of channels
	TUint32 nSamplesPerSec;		// sample rate in hz
	TUint32 nAvgBytesPerSec;	// average bytes per second
	TUint16 nBlockAlign;		// number of bytes per sample
	TUint16 nBitsPerSample;		// number of bits in a sample
	char data_ckID[4];			// data chunk id 'data'
	TUint32 data_ckSize;		// length of data chunk
	};
	
LOCAL_C TInt SamplesPerSecondToRate(TInt aRateInSamplesPerSecond,TSoundRate& aRate)
	{
	switch (aRateInSamplesPerSecond)
		{
		case 7350: 	aRate=ESoundRate7350Hz; break;
		case 8000: 	aRate=ESoundRate8000Hz; break;
		case 8820: 	aRate=ESoundRate8820Hz; break;
		case 9600: 	aRate=ESoundRate9600Hz; break;
		case 11025: aRate=ESoundRate11025Hz; break;
		case 12000: aRate=ESoundRate12000Hz; break;
		case 14700:	aRate=ESoundRate14700Hz; break;
		case 16000: aRate=ESoundRate16000Hz; break;
		case 22050: aRate=ESoundRate22050Hz; break;
		case 24000: aRate=ESoundRate24000Hz; break;
		case 29400: aRate=ESoundRate29400Hz; break;
		case 32000: aRate=ESoundRate32000Hz; break;
		case 44100: aRate=ESoundRate44100Hz; break;
		case 48000: aRate=ESoundRate48000Hz; break;
		default: return(KErrArgument);
		};
	return(KErrNone);	
	}
	
LOCAL_C TInt RecordBufferSizeInBytes(TCurrentSoundFormatV02& aFormat)
	{
	switch (aFormat.iRate)
		{
		case ESoundRate7350Hz: return(8192);
		case ESoundRate8000Hz: return(8192);
		case ESoundRate8820Hz: return(12288);
		case ESoundRate9600Hz: return(12288);
		case ESoundRate11025Hz: return(12288);
		case ESoundRate12000Hz: return(12288);
		case ESoundRate14700Hz: return(12288);
		case ESoundRate16000Hz: return(12288);
		case ESoundRate22050Hz: return(16384);
		case ESoundRate24000Hz: return(16384);
		case ESoundRate29400Hz: return(24576);
		case ESoundRate32000Hz: return(24576);
		case ESoundRate44100Hz: return(32768);
		case ESoundRate48000Hz: return(32768);
		default: return(32768);
		};	
	}	
	
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
	
	Test.Next(_L("Load sound LDD"));
	r=User::LoadLogicalDevice(KSndLddFileName);
	CHECK(r==KErrNone || r==KErrAlreadyExists);

	Test.End();
	return(KErrNone);
	}

LOCAL_C TInt WavPlay()
	{
	RChunk chunk;
	
	// Parse the commandline and get a filename to use
	TLex l(CommandLine);
	TFileName thisfile=RProcess().FileName();
	TPtrC token=l.NextToken();
	if (token.MatchF(thisfile)==0)
		token.Set(l.NextToken());

	if (token.Length()==0)
		{
		// No args, skip to end
		Test.Printf(_L("Invalid configuration\r\n"));
		return(KErrArgument);
		}
		
	Test.Next(_L("Play Wav file"));

	// Assume that the argument is a WAV filename
	TFileName wavFilename=token;
	TInt r;
	RFile source;
	r = source.Open(Fs,wavFilename,EFileRead);
	if (r!=KErrNone)
		{
		Test.Printf(_L("Open failed(%d)\r\n"), r);
		return(r);
		}

	// Read the pcm header
	WAVEheader header;
	TPtr8 headerDes((TUint8 *)&header,sizeof(struct WAVEheader),sizeof(struct WAVEheader));
	r = source.Read(headerDes);
	if (r!=KErrNone)
		{
		source.Close();
		return(r);
		}
	Test.Printf(_L("Header Read %d bytes\r\n"),headerDes.Size());

	if (headerDes.Size() != sizeof(struct WAVEheader)) // EOF
		{
		Test.Printf(_L("Couldn't read a header(%d bytes)\r\n"),headerDes.Size());
		source.Close();
		return(KErrCorrupt);
		}

	Test.Printf(_L("Header rate:%d channels:%d tag:%d bits:%d (%d bytes/s) align %d datalen:%d fmt_ckSize:%d ckSize:%d\n"),
			header.nSamplesPerSec, header.nChannels, header.formatTag, header.nBitsPerSample,
			header.nAvgBytesPerSec, header.nBlockAlign, header.data_ckSize, header.fmt_ckSize, header.ckSize);

	if (header.formatTag != 1) // not pcm
		{
		Test.Printf(_L("Format not PCM(%d)\r\n"),header.formatTag);
		source.Close();
		return(KErrNotSupported);
		}

	if (header.nBitsPerSample != 16) // not 16 bit
		{
		Test.Printf(_L("Format not 16 bit PCM(%d bits)\r\n"),header.nBitsPerSample);
		source.Close();
		return(KErrNotSupported);
		}
		
	TSoundRate rate;	
	if (SamplesPerSecondToRate(header.nSamplesPerSec,rate)!=KErrNone)	
		{
		Test.Printf(_L("Format specifies a rate not supported(%d)\r\n"),header.nSamplesPerSec);
		source.Close();
		return(KErrNotSupported);
		}

	TxSoundDevice.AudioFormat(PlayFormatBuf);	// Read back the current setting which must be valid.
	PlayFormatBuf().iChannels = header.nChannels;
	PlayFormatBuf().iRate = rate;
	PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	
	// Set the play buffer configuration.
	TInt bufSize=BytesPerSecond(PlayFormatBuf())/8; 	// Large enough to hold 1/8th second of data.
	bufSize&=~(header.nBlockAlign-1);					// Keep the buffer length a multiple of the bytes per sample (assumes 16bitPCM, 1 or 2 chans).
	if (PlayCapsBuf().iRequestMinSize)
		bufSize&=~(PlayCapsBuf().iRequestMinSize-1); 	// Keep the buffer length valid for the driver.
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=3;
	bufferConfig.iBufferSizeInBytes=bufSize;
	bufferConfig.iFlags=0;	
	PrintBufferConf(bufferConfig,Test);
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=TxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	if (r!=KErrNone)
		{
		Test.Printf(_L("Buffer configuration not supported(%d)\r\n"),r);
		source.Close();
		return(r);
		}
	TxSoundDevice.GetBufferConfig(bufferConfigBuf);			// Read back the configuration - to get the buffer offsets
	CHECK(bufferConfig.iBufferSizeInBytes==bufSize);

	// Set the audio play configuration.
	TxSoundDevice.SetVolume(KSoundMaxVolume - (KSoundMaxVolume / 4)); // set volume to 75%
	PrintConfig(PlayFormatBuf(),Test);
	r=TxSoundDevice.SetAudioFormat(PlayFormatBuf);
	if (r!=KErrNone)
		{
		Test.Printf(_L("Format not supported\r\n"));
		source.Close();
		chunk.Close();
		return(r);
		}
	TxSoundDevice.ResetBytesTransferred();

	TInt32 bytesToPlay = header.data_ckSize;
	TTime starttime;
	starttime.HomeTime();

	TRequestStatus stat[3];
	TPtr8* tPtr[3];
	TInt i;
	for (i=0;i<3;i++)
		tPtr[i]=new TPtr8(NULL,0); 

	TTime startTime;
	startTime.HomeTime();
	
	// Start off by issuing a play request for each buffer (assuming that the file is long enough). Use the full size
	// of each buffer.
	TInt stillToRead=bytesToPlay;
	TInt stillNotPlayed=bytesToPlay;
	TUint flags;
	for (i=0 ; i<3 ; i++)
		{
		// Setup the descriptor for reading in the data from the file.
		tPtr[i]->Set(chunk.Base()+bufferConfig.iBufferOffsetList[i],0,bufSize); 
		
		// If there is still data to read to play then read this into the descriptor
		// and then write it to the driver.
		if (stillToRead)
			{
			r=source.Read(*tPtr[i],Min(stillToRead,bufSize));
			if (r!=KErrNone)
				{
				Test.Printf(_L("Initial file read error(%d)\r\n"),r);
				source.Close();
				chunk.Close();
				return(r);
				}
			stillToRead-=tPtr[i]->Length();
			flags=(stillToRead>0)?0:KSndFlagLastSample;
			TxSoundDevice.PlayData(stat[i],bufferConfig.iBufferOffsetList[i],tPtr[i]->Length(),flags);
			}
		else
			stat[i]=KRequestPending;	
		}	
		
	FOREVER
		{
		// Wait for any one of the outstanding play requests to complete.
		User::WaitForAnyRequest();

		TTime currentTime;
		currentTime.HomeTime();
		TInt64 elapsedTime = currentTime.Int64()-startTime.Int64();	// us
		TTimeIntervalMicroSecondsBuf timePlayedBuf;
		if(TxSoundDevice.TimePlayed(timePlayedBuf) == KErrNone)
			{
			// Compare TimePlayed with the actual elapsed time. They should be different, but not drift apart too badly...
			TInt32 offset = TInt32(elapsedTime - timePlayedBuf().Int64());
			Test.Printf(_L("\telapsedTime - TimePlayed = %d ms\n"), offset/1000);
			}		
	
		// Work out which buffer this applies to
		for (i=0 ; i<3 ; i++)
			{
			if (stat[i]!=KRequestPending)
				break;
			}
		if (i>=3)
			{
			Test.Printf(_L("I/O error\r\n"));
			source.Close();
			chunk.Close();
			return(KErrGeneral);
			}
	
		// Check that the transfer was succesful and whether we have now played all the file.
		if (stat[i]!=KErrNone)
			{
			Test.Printf(_L("Play error(%d)\r\n"),stat[i].Int());
			source.Close();
			chunk.Close();
			return(stat[i].Int());
			}
		Test.Printf(_L("Played %d bytes(%d) - %d\r\n"),tPtr[i]->Length(),i,stat[i].Int());
		stillNotPlayed-=tPtr[i]->Length();
		CHECK(stillNotPlayed>=0);
		if (!stillNotPlayed)
			break;
	
		// Still more to be played so read the next part of the file into the descriptor for this
		// buffer and then write it to the driver.
		if (stillToRead)
			{
			TInt len=Min(stillToRead,bufSize);
		
			// If we've got to the end of the file and the driver is particular about the request length then 
			// zero fill the entire buffer so we can play extra zeros after the last sample from the file.
			if (len<bufSize && PlayCapsBuf().iRequestMinSize)
				tPtr[i]->FillZ(bufSize);
		
			// Read the next part of the file 
			r=source.Read(*tPtr[i],len);			// This will alter the length of the descriptor
			if (r!=KErrNone)
				{
				Test.Printf(_L("File read error(%d)\r\n"),r);
				source.Close();
				chunk.Close();
				return(r);
				}
			stillToRead-=tPtr[i]->Length();
		
			// If we've got to the end of the file and the driver is particular about the request length then
			// round up the length to the next valid boundary. This is OK since we zero filled.
			if (tPtr[i]->Length() < bufSize && PlayCapsBuf().iRequestMinSize)
				{
				TUint m=PlayCapsBuf().iRequestMinSize-1;
				len=(tPtr[i]->Length() + m) & ~m;
				}
		
			// Write it to the driver.
			flags=(stillToRead>0)?0:KSndFlagLastSample;
			TxSoundDevice.PlayData(stat[i],bufferConfig.iBufferOffsetList[i],len,flags);
			}
		else
			stat[i]=KRequestPending;		
		}
	
	// Delete all the variables again.	
	for (i=0 ; i<3 ; i++)
		delete tPtr[i];
	
	TTime endtime;
	endtime.HomeTime();

	Test.Printf(_L("Done playing\r\n"));
	Test.Printf(_L("Bytes played = %d\r\n"),TxSoundDevice.BytesTransferred());
	Test.Printf(_L("Delta time = %d\r\n"),endtime.Int64()-starttime.Int64());

	chunk.Close();
	source.Close();
	return(KErrNone);
	}

LOCAL_C TInt WavRecord()
	{
	// Parse the commandline and get a filename to use
	TLex l(CommandLine);
	TParse destinationName;
	if (destinationName.SetNoWild(l.NextToken(),0,0)!=KErrNone)
		{
		Test.Printf(_L("No arg, skipping\r\n"));
		return(KErrArgument);
		}
	Test.Next(_L("Record Wav file"));

	// Open the file for writing
	TInt r;
	RFile destination;
	r = destination.Replace(Fs,destinationName.FullName(),EFileWrite);
	if (r!=KErrNone)
		{
		Test.Printf(_L("Open file for write failed(%d)\n"), r);
		return(r);
		}		
	Test.Printf(_L("File opened for write\r\n"));
	
	Test.Next(_L("Preparing to record"));
	
	// Get the rate
	TLex cl(l.NextToken());
	TUint32 tmpRate;
	TSoundRate rate;
	r = cl.Val(tmpRate,EDecimal);
	if (r == KErrNone && (r=SamplesPerSecondToRate(tmpRate,rate))==KErrNone)
		{
		Test.Printf(_L("Parsed rate: %d\r\n"), tmpRate);
		RecordFormatBuf().iRate = rate;
		}
	else
		{
		Test.Printf(_L("Parse rate failed(%d)\r\n"),r);
		RecordFormatBuf().iRate = ESoundRate32000Hz;
		}

	// Get number of channels
	TLex cl_chan(l.NextToken());
	TUint32 tmpChannels;
	r = cl_chan.Val(tmpChannels,EDecimal);
	if (r == KErrNone)
		{
		Test.Printf(_L("Parsed %d channels\r\n"),tmpChannels);
		RecordFormatBuf().iChannels = tmpChannels;
		}
	else
		{
		Test.Printf(_L("Parse channels failed(%d)\r\n"), r);
		RecordFormatBuf().iChannels = 2;
		}

	RecordFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	
	// Set the record buffer configuration.
	RChunk chunk;
	TTestSharedChunkBufConfig bufferConfig;
	bufferConfig.iNumBuffers=4;
	bufferConfig.iBufferSizeInBytes=RecordBufferSizeInBytes(RecordFormatBuf());
	if (RecordCapsBuf().iRequestMinSize)
		bufferConfig.iBufferSizeInBytes&=~(RecordCapsBuf().iRequestMinSize-1); 	// Keep the buffer length valid for the driver.
	bufferConfig.iFlags=0;	
	PrintBufferConf(bufferConfig,Test);
	TPckg<TTestSharedChunkBufConfig> bufferConfigBuf(bufferConfig);
	r=RxSoundDevice.SetBufferChunkCreate(bufferConfigBuf,chunk);
	if (r!=KErrNone)
		{
		Test.Printf(_L("Buffer configuration not supported(%d)\r\n"),r);
		return(r);
		}
	
	// Set the audio record configuration.
	RxSoundDevice.SetVolume(KSoundMaxVolume);
	PrintConfig(RecordFormatBuf(),Test);
	r=RxSoundDevice.SetAudioFormat(RecordFormatBuf);
	if (r!=KErrNone)
		{
		Test.Printf(_L("Format not supported\r\n"));
		return(r);
		}

	// Get length in seconds
	TLex cl_seconds(l.NextToken());
	TUint32 tmpSeconds;
	r = cl_seconds.Val(tmpSeconds,EDecimal);
	if (r == KErrNone)
		{
		Test.Printf(_L("Parsed %d seconds\r\n"),tmpSeconds);
		}
	else
		{
		Test.Printf(_L("Parse seconds failed(%d)\r\n"),r);
		tmpSeconds=10;
		}
	TInt bytesToRecord = BytesPerSecond(RecordFormatBuf())*tmpSeconds;	
		
	Test.Next(_L("Recording..."));
	
	// Lay down a file header
	WAVEheader header;
	TPtr8 headerDes((TUint8 *)&header, sizeof(struct WAVEheader), sizeof(struct WAVEheader));

	// "RIFF"
	header.ckID[0] = 'R'; header.ckID[1] = 'I';
	header.ckID[2] = 'F'; header.ckID[3] = 'F';
	// "WAVE"
	header.wave_ckID[0] = 'W'; header.wave_ckID[1] = 'A';
	header.wave_ckID[2] = 'V'; header.wave_ckID[3] = 'E';
	// "fmt "
	header.fmt_ckID[0] = 'f'; header.fmt_ckID[1] = 'm';
	header.fmt_ckID[2] = 't'; header.fmt_ckID[3] = ' ';
	// "data"
	header.data_ckID[0] = 'd'; header.data_ckID[1] = 'a';
	header.data_ckID[2] = 't'; header.data_ckID[3] = 'a';

	header.nChannels		= (TUint16)RecordFormatBuf().iChannels;
	header.nSamplesPerSec	= RateInSamplesPerSecond(RecordFormatBuf().iRate);
	header.nBitsPerSample	= 16;
	header.nBlockAlign		= TUint16((RecordFormatBuf().iChannels == 2) ? 4 : 2);
	header.formatTag		= 1;	// type 1 is PCM
	header.fmt_ckSize		= 16;
	header.nAvgBytesPerSec	= BytesPerSecond(RecordFormatBuf());
	header.data_ckSize		= bytesToRecord;
	header.ckSize			= bytesToRecord + sizeof(struct WAVEheader) - 8;

	Test.Printf(_L("Header rate:%d channels:%d tag:%d bits:%d (%d bytes/s) align %d datalen:%d fmt_ckSize:%d ckSize:%d\r\n"),
			header.nSamplesPerSec, header.nChannels, header.formatTag, header.nBitsPerSample,
			header.nAvgBytesPerSec, header.nBlockAlign, header.data_ckSize, header.fmt_ckSize, header.ckSize, sizeof(struct WAVEheader));

	r = destination.Write(headerDes);

	TRequestStatus stat;
	TInt length;
	TPtrC8 buf;

	TTime startTime;
	startTime.HomeTime();
	
	// Start off by issuing a record request.
	TTime starttime;
	starttime.HomeTime();
	TInt bytesRecorded = 0;
	RxSoundDevice.RecordData(stat,length);

	TInt pausesToDo = 10;
	pausesToDo = 0;
	FOREVER
		{
		// Wait for the outstanding record request to complete.
        User::After(6000);

		User::WaitForAnyRequest();
		if (stat==KRequestPending)
			return(KErrGeneral);

		TTime currentTime;
		currentTime.HomeTime();
		TInt64 elapsedTime = currentTime.Int64()-startTime.Int64();	// us
		TTimeIntervalMicroSecondsBuf timeRecordedBuf;
		if(RxSoundDevice.TimeRecorded(timeRecordedBuf) == KErrNone)
			{
			// Compare TimeRecorded with the actual elapsed time. They should be different, but not drift apart too badly...
			TInt32 offset = TInt32(elapsedTime - timeRecordedBuf().Int64());
			Test.Printf(_L("\telapsedTime - TimeRecorded = %d ms\n"), offset/1000);
			}		
			
		// Check whether the record request was succesful.
		TInt retOffset=stat.Int();
		if (retOffset<0)
			{
			Test.Printf(_L("Record failed(%d)\r\n"),retOffset);
			return(retOffset);
			}
		
		// Successfully recorded another buffer so write the recorded data to the record file and release the buffer.
		buf.Set((const TUint8*)(chunk.Base()+retOffset),length);
		r=destination.Write(buf);
		if (r!=KErrNone)
			{
			Test.Printf(_L("File write failed(%d)\r\n"),r);
			return(r);
			}
		r=RxSoundDevice.ReleaseBuffer(retOffset);
		if (r!=KErrNone)
			{
			Test.Printf(_L("Release buffer failed(%d)\r\n"),r);
			return(r);
			}
		
		Test.Printf(_L("Recorded %d more bytes - %d\r\n"),length,retOffset);

		if((pausesToDo > 0) && (bytesRecorded > bytesToRecord/2))
			{
			--pausesToDo;
			Test.Printf(_L("Pause\r\n"));
			RxSoundDevice.Pause();
			Test.Printf(_L("Paused, sleeping for 0.5 seconds\r\n"));
			User::After(500*1000);
            Test.Printf(_L("Resume\r\n"));
			RxSoundDevice.Resume();
			}
		
		// Check whether we have now recorded all the data. If more to record then queue a further request
		bytesRecorded+=length;
		if (bytesRecorded<bytesToRecord)
		    {
            Test.Printf(_L("RecordData\r\n"));
			RxSoundDevice.RecordData(stat,length);
		    }
		else
			break;
		}
	
	RxSoundDevice.CancelRecordData();	// Stop the driver from recording.
	
	TTime endtime;
	endtime.HomeTime();

	TInt64 elapsedTime = endtime.Int64()-starttime.Int64();	// us
	Test.Printf(_L("Delta time = %d\r\n"),I64LOW(elapsedTime));
	Test.Printf(_L("Seconds in buffer: %d (%d)\r\n"), bytesRecorded / header.nAvgBytesPerSec, (bytesRecorded / header.nAvgBytesPerSec)*1000000);

	if (I64LOW(elapsedTime) <= (bytesRecorded / header.nAvgBytesPerSec)*1000000)
		{
		Test.Printf(_L("Time travelling; record took less time than it should have done\r\n"));
		return(KErrGeneral);
		}
	
	chunk.Close();
	destination.Close();

	Test.Printf(_L("Record finished\r\n"));
	return(KErrNone);
	}
	
// Quick test block to write the output of the signal generator to a file.
// You'll need to comment out the reference to playdata in writetone and link
// the function into the command line processing (search on this function name).	
/*
LOCAL_C void TestWaveformGenerator()
	{
	
	Test.Next(_L("Testing waveform generator"));
	// Parse the commandline and get a filename to use
	TLex l(CommandLine);
	TParse destinationName;
	if (destinationName.SetNoWild(l.NextToken(),0,0)!=KErrNone)
		{
		Test.Printf(_L("No arg, skipping\r\n"));
		return;
		}

	// Open the file for writing
	TInt r;
	RFile destination;
	r = destination.Replace(Fs,destinationName.FullName(),EFileWrite);
	if (r!=KErrNone)
		{
		Test.Printf(_L("Open file for write failed(%d)\r\n"), r);
		}
	
	Test.Printf(_L("File opened for write\r\n"));
	Test.Next(_L("Preparing to record data"));

	// Get the rate
	TLex cl(l.NextToken());
	TUint32 tmpRate;
	TSoundRate rate;
	r = cl.Val(tmpRate,EDecimal);
	if (r == KErrNone && (r=SamplesPerSecondToRate(tmpRate,rate))==KErrNone)
		{
		Test.Printf(_L("Parsed rate: %d\r\n"), tmpRate);
		PlayFormatBuf().iRate = rate;
		}
	else
		{
		Test.Printf(_L("Parse rate failed(%d)\r\n"),r);
		PlayFormatBuf().iRate = ESoundRate32000Hz;
		}

	// Get number of channels
	TLex cl_chan(l.NextToken());
	TUint32 tmpChannels;
	r = cl_chan.Val(tmpChannels,EDecimal);
	if (r == KErrNone)
		{
		Test.Printf(_L("Parsed %d channels\r\n"),tmpChannels);
		PlayFormatBuf().iChannels = tmpChannels;
		}
	else
		{
		Test.Printf(_L("Parse channels failed(%d)\r\n"), r);
		PlayFormatBuf().iChannels = 2;
		}

	PlayFormatBuf().iEncoding = ESoundEncoding16BitPCM;
	PrintConfig(PlayFormatBuf(),Test);
	
	TInt bufferSize=BytesPerSecond(PlayFormatBuf())/8;
	TUint8* buffer = (TUint8*)User::Alloc(bufferSize*sizeof(TUint8));
	if (buffer==NULL)
		{
		Test.Printf(_L("Out of memory\r\n"));
		return;
		}
	TPtr8 bufferDes(buffer,bufferSize,bufferSize);	

	Test.Next(_L("Recording..."));
	TInt i = BytesPerSecond(PlayFormatBuf())*10/bufferSize;
	TInt bytesToRecord = i * bufferSize;

	// Lay down a file header
	WAVEheader header;
	TPtr8 headerDes((TUint8 *)&header, sizeof(struct WAVEheader), sizeof(struct WAVEheader));

	// "RIFF"
	header.ckID[0] = 'R'; header.ckID[1] = 'I';
	header.ckID[2] = 'F'; header.ckID[3] = 'F';
	// "WAVE"
	header.wave_ckID[0] = 'W'; header.wave_ckID[1] = 'A';
	header.wave_ckID[2] = 'V'; header.wave_ckID[3] = 'E';
	// "fmt "
	header.fmt_ckID[0] = 'f'; header.fmt_ckID[1] = 'm';
	header.fmt_ckID[2] = 't'; header.fmt_ckID[3] = ' ';
	// "data"
	header.data_ckID[0] = 'd'; header.data_ckID[1] = 'a';
	header.data_ckID[2] = 't'; header.data_ckID[3] = 'a';

	header.nChannels		= PlayFormatBuf().iChannels;
	header.nSamplesPerSec	= RateInSamplesPerSecond(PlayFormatBuf().iRate);
	header.nBitsPerSample	= 16;
	header.nBlockAlign		= 4;
	header.formatTag		= 1;
	header.fmt_ckSize		= 16;
	header.nAvgBytesPerSec	= BytesPerSecond(PlayFormatBuf());
	header.data_ckSize		= bytesToRecord;
	header.ckSize			= bytesToRecord + sizeof(struct WAVEheader) - 8;

	Test.Printf(_L("Header rate:%d channels:%d tag:%d bits:%d (%d bytes/s) align %d datalen:%d fmt_ckSize:%d ckSize:%d\r\n"),
			header.nSamplesPerSec, header.nChannels, header.formatTag, header.nBitsPerSample,
			header.nAvgBytesPerSec, header.nBlockAlign, header.data_ckSize, header.fmt_ckSize, header.ckSize);

	r = destination.Write(headerDes);

	MakeSineTable(PlayFormatBuf());
	SetToneFrequency(440,PlayFormatBuf()); // 'A'

	while(--i>0)
		{
		WriteTone(bufferDes,PlayFormatBuf());
		r = destination.Write(bufferDes);
		if (r!=KErrNone)
			{
			Test.Printf(_L("Write failed(%d)\r\n"),r);
			break;
			}
		}
	Test.Printf(_L("Finished\r\n"));

	delete buffer;
	destination.Close();
	}
*/
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

	__KHEAP_MARK;

	Test.Next(_L("Open playback channel"));
	r = TxSoundDevice.Open(KSoundScTxUnit0);
	if (r!=KErrNone)
		{
		Test.Printf(_L("Open playback channel error(%d)\r\n"),r);
		Test(0);
		}
	
	Test.Next(_L("Open record channel"));
	r = RxSoundDevice.Open(KSoundScRxUnit0);
	if (r!=KErrNone)
		{
		Test.Printf(_L("Open record channel error(%d)\r\n"),r);
		Test(0);
		}
	
	Test.Next(_L("Query play formats supported"));
	TxSoundDevice.Caps(PlayCapsBuf);
	TSoundFormatsSupportedV02 playCaps=PlayCapsBuf();
	PrintCaps(playCaps,Test);

	Test.Next(_L("Query record formats supported"));
	RxSoundDevice.Caps(RecordCapsBuf);
	TSoundFormatsSupportedV02 recordCaps=RecordCapsBuf();
	PrintCaps(recordCaps,Test);
	
	Test.Next(_L("Connect to the file server"));
	r = Fs.Connect();
	if (r!=KErrNone)
		{
		Test.Printf(_L("Connect to the file server error(%d)\r\n"),r);
		Test(0);
		}
		
	if (User::CommandLineLength())
		{
		User::CommandLine(CommandLine);
		TLex l(CommandLine);
		TPtrC token=l.NextToken();

		TInt count=0;
		while (token.Length()!=0)
			{
			++count;
			token.Set(l.NextToken());
			}
		Test.Printf(_L("Command line %d parameters\r\n"),count);

		if (count==1)		// If 1 parameter try playing a file
			r=WavPlay();
		else if (count)		// If there is more than 1 parameter, try recording
			r=WavRecord();
		
		//TestWaveformGenerator();
			
		}
	
	Fs.Close();
	
	Test.Next(_L("Close channels"));
	RxSoundDevice.Close();
	TxSoundDevice.Close();
	
	__KHEAP_MARKEND;

	// Now that both the channels are closed, unload the LDD and the PDDs.
	TestUnloadDrivers();

	Test(r==KErrNone);

	Test.End();
	Test.Close();

	Cleanup();
	
	__UHEAP_MARKEND;

	return(KErrNone);
	}
