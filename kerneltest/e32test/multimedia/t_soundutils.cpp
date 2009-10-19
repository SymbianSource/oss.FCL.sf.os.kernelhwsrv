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
// e32test\multimedia\t_soundutils.cpp
// 
//

/**
 @file Utilities used by the shared chunk sound driver test code.
*/

#include <e32test.h>
#include <e32math.h>
#include "t_soundutils.h"

const TInt SineAddressBits = 10;
const TInt SineAddressFractionBits = 32-SineAddressBits;
const TInt SineTableSize = 1<<SineAddressBits;

TUint32 ToneIndex = 0;
TUint32 ToneIndexStep = 0;

TInt32* SineTable = NULL;

_LIT(KSndDirCapsTitle, "Direction    :");
_LIT(KSndDirRecord, " Record");
_LIT(KSndDirPlay, " Playback");

_LIT(KSndRateCapsTitle,"Sample rates :");
_LIT(KSndRateConfigTitle,"Current rate :");
_LIT(KSndRate7350Hz," 7.35KHz");
_LIT(KSndRate8000Hz," 8KHz");
_LIT(KSndRate8820Hz," 8.82KHz");
_LIT(KSndRate9600Hz," 9.6KHz");
_LIT(KSndRate11025Hz," 11.025KHz");
_LIT(KSndRate12000Hz," 12KHz");
_LIT(KSndRate14700Hz," 14.7KHz");
_LIT(KSndRate16000Hz," 16KHz");
_LIT(KSndRate22050Hz," 22.05KHz");
_LIT(KSndRate24000Hz," 24KHz");
_LIT(KSndRate29400Hz," 29.4KHz");
_LIT(KSndRate32000Hz," 32KHz");
_LIT(KSndRate44100Hz," 44.1KHz");
_LIT(KSndRate48000Hz," 48KHz");

_LIT(KSndChanConfigCapsTitle,"Chan configs :");
_LIT(KSndChanConfigMono," Mono");
_LIT(KSndChanConfigStereo," Stereo");
_LIT(KSndChanConfig3Chan," 3Chan");
_LIT(KSndChanConfig4Chan," 4Chan");
_LIT(KSndChanConfig5Chan," 5Chan");
_LIT(KSndChanConfig6Chan," 6Chan");

_LIT(KSndEncodingCapsTitle,"Encodings    :");
_LIT(KSndEncodingConfigTitle,"Encoding     :");
_LIT(KSndEncoding8BitPCM," 8bit PCM");
_LIT(KSndEncoding16BitPCM," 16bit PCM");
_LIT(KSndEncoding24BitPCM," 24bit PCM");

_LIT(KSndDataFormatCapsTitle,"Data formats :");
_LIT(KSndDataFormatConfigTitle,"Data format  :");
_LIT(KSndDataFormatInterleaved," Interleaved");
_LIT(KSndDataFormatNonInterleaved," Non-interleaved");


GLDEF_C TInt BytesPerSample(TCurrentSoundFormatV02& aFormat)
	{
	TInt bytes = aFormat.iChannels;
	switch(aFormat.iEncoding)
		{
		case ESoundEncoding24BitPCM:
			bytes *= 3;
			break;	
		case ESoundEncoding16BitPCM:
			bytes *= 2;
			break;
		case ESoundEncoding8BitPCM:
			break;
		default:
			bytes=0;
			break;
		}
	return(bytes);
	}

GLDEF_C TInt RateInSamplesPerSecond(TSoundRate aRate)
	{
	switch(aRate)
		{
		case ESoundRate7350Hz: 	return(7350);
		case ESoundRate8000Hz: 	return(8000);
		case ESoundRate8820Hz: 	return(8820);
		case ESoundRate9600Hz: 	return(9600);
		case ESoundRate11025Hz: return(11025);
		case ESoundRate12000Hz: return(12000);
		case ESoundRate14700Hz:	return(14700);
		case ESoundRate16000Hz: return(16000);
		case ESoundRate22050Hz: return(22050);
		case ESoundRate24000Hz: return(24000);
		case ESoundRate29400Hz: return(29400);
		case ESoundRate32000Hz: return(32000);
		case ESoundRate44100Hz: return(44100);
		case ESoundRate48000Hz: return(48000);
		default: return(0);
		};
	}
	
GLDEF_C TInt BytesPerSecond(TCurrentSoundFormatV02& aFormat)
	{
	return(RateInSamplesPerSecond(aFormat.iRate) * BytesPerSample(aFormat));
	}
	
GLDEF_C TInt ValidBufferSize(TInt aSize, TInt aDeviceMinSize, TCurrentSoundFormatV02& aFormat)
	{
	// Keep the buffer length a multiple of the number of bytes per sample
	TInt bytesPerSample=BytesPerSample(aFormat);
	aSize-=(aSize%bytesPerSample);
	
	// Keep the buffer length valid for driver.
	if (aDeviceMinSize)
		aSize&=~(aDeviceMinSize-1); 	
	return(aSize);	
	}			
	
GLDEF_C TInt MakeSineTable(TCurrentSoundFormatV02& aPlayFormat)
	{
	delete SineTable;
	SineTable = (TInt32*)User::Alloc(SineTableSize*sizeof(TInt32));
	if (!SineTable)
		return(KErrNoMemory);

	TInt i;
	TReal scale;

	switch(aPlayFormat.iEncoding)
		{
		case ESoundEncoding8BitPCM:
			scale = 127;
			break;
		case ESoundEncoding16BitPCM:
			scale = 32767;
			break;
		case ESoundEncoding24BitPCM:
			scale = 8388607;
			break;
		default:
			return(KErrNotSupported);
		}
			
	for(i=0; i<SineTableSize; i++)
		{
		TReal r = KPi*2.0*(TReal)i/(TReal)SineTableSize;
		Math::Sin(r,r);
		r *= scale;
		Math::Int(SineTable[i],r);
		}
	
	return KErrNone;
	};

GLDEF_C TInt SetToneFrequency(TUint aFrequency,TCurrentSoundFormatV02& aPlayFormat)
	{
	TInt64 step(MAKE_TINT64(aFrequency,0));
	step /= RateInSamplesPerSecond(aPlayFormat.iRate);
	ToneIndexStep = I64LOW(step);
	return((I64HIGH(step)==0) ?  KErrNone : KErrGeneral);
	}

GLDEF_C void WriteTone(TDes8& aBuffer,TCurrentSoundFormatV02& aPlayFormat)
	{
	aBuffer.SetMax();

	TUint32 index = ToneIndex;
	TUint32 step = ToneIndexStep;
	TInt32* table = SineTable;

	switch(aPlayFormat.iEncoding)
		{
		case ESoundEncoding16BitPCM:
			{
			TInt16* ptr = (TInt16*)aBuffer.Ptr();
			TInt16* end = ptr+aBuffer.Length()/2;
			while(ptr<end)
				{
				*ptr++ = (TInt16)table[index>>SineAddressFractionBits];
				if (aPlayFormat.iChannels == 2)
					*ptr++ = (TInt16)table[index>>SineAddressFractionBits];
				index += step;
				}
			}
			break;
		case ESoundEncoding8BitPCM:
			{
			TUint8* ptr = (TUint8*)aBuffer.Ptr();
			TUint8* end = ptr+aBuffer.Length();
			while(ptr<end)
				{
				*ptr++ = (TUint8)table[index>>8];
				if (aPlayFormat.iChannels == 2)
					*ptr++ = (TInt8)table[index>>8];
				index += step;
				}
			}
			break;
		case ESoundEncoding24BitPCM:
			{
			TUint8* ptr = (TUint8*)aBuffer.Ptr();
			TUint8* end = ptr+aBuffer.Length();
			while(ptr<end)
				{
				*ptr++ = (TUint8)table[index>>24];
				if (aPlayFormat.iChannels == 2)
					*ptr++ = (TInt8)table[index>>24];
				index += step;
				}
			}
			break;

		default:
			break;
		}

	ToneIndex = index;
	}
	
GLDEF_C void Cleanup()
	{
	delete SineTable;
	}	
	
GLDEF_C void PrintCaps(TSoundFormatsSupportedV02& aCaps,RTest& aTest)
	{
	TBuf<128> buf;
	
	aTest.Printf(_L("**Sound Capabilities**\r\n"));
	
	// Display the data transfer direction
	buf.Zero();
	buf.Append(KSndDirCapsTitle);
	if (aCaps.iDirection==ESoundDirRecord)
		buf.Append(KSndDirRecord);
	else
		buf.Append(KSndDirPlay);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	
	// Display the channel configuration
	buf.Zero();
	buf.Append(KSndChanConfigCapsTitle);
	if (aCaps.iChannels & KSoundMonoChannel)
		buf.Append(KSndChanConfigMono);
	if (aCaps.iChannels & KSoundStereoChannel)
		buf.Append(KSndChanConfigStereo);
	if (aCaps.iChannels & KSoundThreeChannel)
		buf.Append(KSndChanConfig3Chan);
	if (aCaps.iChannels & KSoundFourChannel)
		buf.Append(KSndChanConfig4Chan);
	if (aCaps.iChannels & KSoundFiveChannel)
		buf.Append(KSndChanConfig5Chan);
	if (aCaps.iChannels & KSoundSixChannel)
		buf.Append(KSndChanConfig6Chan);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	
	// Display the supported sample rates
	buf.Zero();
	buf.Append(KSndRateCapsTitle);
	if (aCaps.iRates & KSoundRate7350Hz)
		buf.Append(KSndRate7350Hz);
	if (aCaps.iRates & KSoundRate8000Hz)
		buf.Append(KSndRate8000Hz);
	if (aCaps.iRates & KSoundRate8820Hz)
		buf.Append(KSndRate8820Hz);
	if (aCaps.iRates & KSoundRate9600Hz)
		buf.Append(KSndRate9600Hz);
	if (aCaps.iRates & KSoundRate11025Hz)
		buf.Append(KSndRate11025Hz);
	if (aCaps.iRates & KSoundRate12000Hz)
		buf.Append(KSndRate12000Hz);
	if (aCaps.iRates & KSoundRate14700Hz)
		buf.Append(KSndRate14700Hz);
	if (aCaps.iRates & KSoundRate16000Hz)
		buf.Append(KSndRate16000Hz);
	if (aCaps.iRates & KSoundRate22050Hz)
		buf.Append(KSndRate22050Hz);
	if (aCaps.iRates & KSoundRate24000Hz)
		buf.Append(KSndRate24000Hz);
	if (aCaps.iRates & KSoundRate29400Hz)
		buf.Append(KSndRate29400Hz);
	if (aCaps.iRates & KSoundRate32000Hz)
		buf.Append(KSndRate32000Hz);
	if (aCaps.iRates & KSoundRate44100Hz)
		buf.Append(KSndRate44100Hz);
	if (aCaps.iRates & KSoundRate48000Hz)
		buf.Append(KSndRate48000Hz);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	
	// Display the sound encodings supported
	buf.Zero();
	buf.Append(KSndEncodingCapsTitle);
	if (aCaps.iEncodings & KSoundEncoding8BitPCM)
		buf.Append(KSndEncoding8BitPCM);
	if (aCaps.iEncodings & KSoundEncoding16BitPCM)
		buf.Append(KSndEncoding16BitPCM);
	if (aCaps.iEncodings & KSoundEncoding24BitPCM)
		buf.Append(KSndEncoding24BitPCM);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	
	// Display the data formats supported
	buf.Zero();
	buf.Append(KSndDataFormatCapsTitle);
	if (aCaps.iDataFormats & KSoundDataFormatInterleaved)
		buf.Append(KSndDataFormatInterleaved);
	if (aCaps.iDataFormats & KSoundDataFormatNonInterleaved)
		buf.Append(KSndDataFormatNonInterleaved);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	
	// Display the minimum request size and the request alignment factor.
	aTest.Printf(_L("Min req size : %d\r\n"),aCaps.iRequestMinSize);
	aTest.Printf(_L("Req alignment: %d\r\n"),aCaps.iRequestAlignment);
	}
	
GLDEF_C void PrintConfig(TCurrentSoundFormatV02& aConfig,RTest& aTest)
	{
	TBuf<80> buf;
	
	aTest.Printf(_L("**Sound configuration**\r\n"));
	
	// Display the current channel configuration
	aTest.Printf(_L("Channels     : %d\r\n"),aConfig.iChannels);
	
	// Display the current sample rate
	buf.Zero();
	buf.Append(KSndRateConfigTitle);
	if (aConfig.iRate==ESoundRate7350Hz)
		buf.Append(KSndRate7350Hz);
	else if (aConfig.iRate==ESoundRate8000Hz)
		buf.Append(KSndRate8000Hz);
	else if (aConfig.iRate==ESoundRate8820Hz)
		buf.Append(KSndRate8820Hz);
	else if (aConfig.iRate==ESoundRate9600Hz)
		buf.Append(KSndRate9600Hz);
	else if (aConfig.iRate==ESoundRate11025Hz)
		buf.Append(KSndRate11025Hz);
	else if (aConfig.iRate==ESoundRate12000Hz)
		buf.Append(KSndRate12000Hz);
	else if (aConfig.iRate==ESoundRate14700Hz)
		buf.Append(KSndRate14700Hz);
	else if (aConfig.iRate==ESoundRate16000Hz)
		buf.Append(KSndRate16000Hz);
	else if (aConfig.iRate==ESoundRate22050Hz)
		buf.Append(KSndRate22050Hz);
	else if (aConfig.iRate==ESoundRate24000Hz)
		buf.Append(KSndRate24000Hz);
	else if (aConfig.iRate==ESoundRate29400Hz)
		buf.Append(KSndRate29400Hz);
	else if (aConfig.iRate==ESoundRate32000Hz)
		buf.Append(KSndRate32000Hz);
	else if (aConfig.iRate==ESoundRate44100Hz)
		buf.Append(KSndRate44100Hz);
	else if (aConfig.iRate==ESoundRate48000Hz)
		buf.Append(KSndRate48000Hz);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	
	// Display the current encoding
	buf.Zero();
	buf.Append(KSndEncodingConfigTitle);
	if (aConfig.iEncoding==ESoundEncoding8BitPCM)
		buf.Append(KSndEncoding8BitPCM);
	else if (aConfig.iEncoding==ESoundEncoding16BitPCM)
		buf.Append(KSndEncoding16BitPCM);
	else if (aConfig.iEncoding==ESoundEncoding24BitPCM)
		buf.Append(KSndEncoding24BitPCM);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	
	// Display the current data format
	buf.Zero();
	buf.Append(KSndDataFormatConfigTitle);
	if (aConfig.iDataFormat==ESoundDataFormatInterleaved)
		buf.Append(KSndDataFormatInterleaved);
	else if (aConfig.iDataFormat==ESoundDataFormatNonInterleaved)
		buf.Append(KSndDataFormatNonInterleaved);
	buf.Append(_L("\r\n"));
	aTest.Printf(buf);
	}
	
GLDEF_C void PrintBufferConf(TTestSharedChunkBufConfig& aBufConf,RTest& aTest)
	{
	TBuf<80> buf(0);
	
	aTest.Printf(_L("**Buffer configuration**\r\n"));
	
	// Display the buffer configuration
	buf.Format(_L("NumBufs:%d Size:%xH(%d)\r\n"),aBufConf.iNumBuffers,aBufConf.iBufferSizeInBytes,aBufConf.iBufferSizeInBytes);
	aTest.Printf(buf);
	if (aBufConf.iFlags & KScFlagBufOffsetListInUse)
		{
		buf.Format(_L(" Offsets[%08xH,%08xH,%08xH,%08xH]\r\n"),aBufConf.iBufferOffsetList[0],aBufConf.iBufferOffsetList[1],aBufConf.iBufferOffsetList[2],aBufConf.iBufferOffsetList[3]);
		aTest.Printf(buf);
		buf.Format(_L(" Offsets[%08xH,%08xH,%08xH,%08xH]\r\n"),aBufConf.iBufferOffsetList[4],aBufConf.iBufferOffsetList[5],aBufConf.iBufferOffsetList[6],aBufConf.iBufferOffsetList[7]);
		aTest.Printf(buf);
		}
	}
