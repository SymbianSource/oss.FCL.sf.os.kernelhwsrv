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
// e32test\multimedia\t_soundutils.h
// 
//

/**
 @file Definitions for utilities used by the shared chunk sound driver test code.
*/

#ifndef __T_SOUNDUTILS_H__
#define __T_SOUNDUTILS_H__

#include <d32soundsc.h>


const TInt KTestMaxSharedChunkBuffers=16;
class TTestSharedChunkBufConfig : public TSharedChunkBufConfigBase
	{
public:
	TInt iBufferOffsetList[KTestMaxSharedChunkBuffers];
	};

GLREF_C TInt BytesPerSample(TCurrentSoundFormatV02& aFormat);
GLREF_C TInt RateInSamplesPerSecond(TSoundRate aRate);
GLREF_C TInt BytesPerSecond(TCurrentSoundFormatV02& aFormat);
GLREF_C TInt ValidBufferSize(TInt aSize,TInt aDeviceMinSize,TCurrentSoundFormatV02& aFormat);
GLREF_C TInt MakeSineTable(TCurrentSoundFormatV02& aPlayFormat);
GLREF_C TInt SetToneFrequency(TUint aFrequency,TCurrentSoundFormatV02& aPlayFormat);
GLREF_C void WriteTone(TDes8& aBuffer,TCurrentSoundFormatV02& aPlayFormat);
GLREF_C void Cleanup();
GLREF_C void PrintCaps(TSoundFormatsSupportedV02& aCaps,RTest& aTest);
GLREF_C void PrintConfig(TCurrentSoundFormatV02& aConfig,RTest& aTest);
GLREF_C void PrintBufferConf(TTestSharedChunkBufConfig& aBufConf,RTest& aTest);	

//a panic category which the limited capability process t_sound_api_helper
//uses to communicate back to t_sound_api.
_LIT(KSoundAPICaps, "SoundAPICaps");

_LIT(KHelperExeBase, "t_sound_api_helper.exe");
_LIT(KHelperExe, "t_sound_api_helper_caps.exe");
_LIT(KSysBin, "c:\\sys\\bin");
const TInt KSlot=1; //the RProcess data slot to use when creating t_sound_api_helper_caps.exe
#endif	// __T_SOUNDUTILS_H__ 
