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
// e32test\debug\d_logtofile.h
// 
//

#ifndef __D_LOGTOFILE_H__
#define __D_LOGTOFILE_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

/**
Data structure for CreateChunk call to ldd
*/
struct TChunkCreateStr
{
TInt iSize;			/**The size of the chunk to be created*/
TBuf8<10> iPattern; /**The matching pattern*/
};

/**
The user side class for controlling trace handler hook.
*/
class RLogToFileDevice : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlCreateChunk,
		EControlStart,
		EControlStop,
		EControlRemoveChunk,
		};
public:
#ifndef __KERNEL_MODE__
	
	inline TInt Open(); 
	inline TInt CreateChunk(TChunkCreateStr* aStr);
	inline void Start();
	inline TInt Stop();
	inline void RemoveChunk();
#endif //__KERNEL_MODE__
	};

_LIT(KLogToFileName,"d_logtofile");

#ifndef __KERNEL_MODE__
/** Opens a channel*/
inline TInt RLogToFileDevice::Open()
	{return DoCreate(KLogToFileName,TVersion(1,0,0),KNullUnit,NULL,NULL);}
/**Creates a chunk. Returns user side handle*/
inline TInt RLogToFileDevice::CreateChunk(TChunkCreateStr* aStr)
	{return DoControl(EControlCreateChunk, reinterpret_cast<TAny*>(aStr));}
/**Starts logging into the chunk*/
inline void RLogToFileDevice::Start()
	{DoControl(EControlStart);}
/**Stops logging into the chunk. Returns the size of the log.*/
inline TInt RLogToFileDevice::Stop()
	{return DoControl(EControlStop);}
/*Destroys the chunk*/
inline void RLogToFileDevice::RemoveChunk()
	{DoControl(EControlRemoveChunk);}
#endif //__KERNEL_MODE__
#endif //__D_LOGTOFILE_H__
