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
// e32test\debug\d_logtofile.cpp
// d_logtofile.ldd is kernel side of t_logtofile application that tests *
// trace handler hook (TTraceHandler). See t_logtofile.cpp for details. *
// 
//

#include "d_logtofile.h"
#include <kernel/kern_priv.h>

_LIT(PatternInfo, "Pattern:");
_LIT(BufferInfo, "Buffer Size:");
_LIT(FastCounterInfo, "Fast couner freq:");
const TInt MaxLogSize=300;
///////////////////////////////////////////////////////////////////////
class DLogToFile : public DLogicalChannelBase
	{
public:
	DLogToFile();
	~DLogToFile();
	static TBool Handler (const TDesC8& aText, TTraceSource aTraceSource);
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	TInt CreateChunk();
	void Start();
	TInt Stop();
	void RemoveChunk();

private:
    DChunk* iChunk;					/**Shared chunk*/
    TLinAddr iChunkKernelAddr;		/**Kernel base address of the chunk*/
    TUint32 iChunkMapAttr;			/**Mapping attributes of the chunk*/
	TBool iBufferFull;				/**Chunk full indication*/
	TText8* iCurrent;				/**Current pointer to the chunk space*/
	TChunkCreateStr iChunkCreateStr;/**Input arguments(matching pattern and the chunk size) from user side*/
	TTraceHandler iOldHook;			/**Previous trace logging hook*/
	};

DLogToFile* Logger;

DLogToFile::DLogToFile() 
	{
	}

DLogToFile::~DLogToFile()
	{
	Logger = NULL;
	}

/*
Trace handler hook.
Should be able to run in any content (including ISR).
@return EFalse, always and that way suppresses debug logging to serial port.
*/
TBool DLogToFile::Handler (const TDesC8& aText, TTraceSource aTraceSource)
	{
	if(Logger->iBufferFull)
		return ETrue;

	TInt irq=NKern::DisableAllInterrupts(); //Disable interrupts to prevent loggings overlapping each other.

	if (aText.Length() < Logger->iChunkCreateStr.iPattern.Length())
		{
		NKern::RestoreInterrupts(irq);
		return ETrue; //The log is too short to match the pattern
		}
	TPtrC8 ptr( aText.Ptr(), Logger->iChunkCreateStr.iPattern.Length());
	if (ptr != Logger->iChunkCreateStr.iPattern) //Compare the pattern against the start of the log
		{
		NKern::RestoreInterrupts(irq);
		return ETrue; //Does not match
		}

	TBuf8<20> fcBuffer;
	fcBuffer.AppendNum(NKern::FastCounter());//If it was a real tool, we should not do this here.

	memcpy (Logger->iCurrent, fcBuffer.Ptr(), fcBuffer.Length());
	Logger->iCurrent+=fcBuffer.Length();
	*(Logger->iCurrent++) = '\t';

	switch(aTraceSource)
		{
		case EKernelTrace:
			{
			*(Logger->iCurrent++) = 'K';
			*(Logger->iCurrent++) = '\t';
			memcpy (Logger->iCurrent, aText.Ptr(),aText.Length());
			break;	
			}

		case EPlatSecTrace:
			{
			*(Logger->iCurrent++) = 'P';
			*(Logger->iCurrent++) = '\t';
			//PlatSec log could be of any size. Additional checking required:
			if ((TInt)Logger->iCurrent > (TInt)(Logger->iChunkKernelAddr + Logger->iChunkCreateStr.iSize - aText.Length()))
				{
				Logger->iBufferFull = ETrue;
				break;
				}
			memcpy (Logger->iCurrent, aText.Ptr(),aText.Length());
			break;	
			}
		
		case EUserTrace:
			{
			*(Logger->iCurrent++) = 'U';
			*(Logger->iCurrent++) = '\t';
			memcpy(Logger->iCurrent, aText.Ptr(),aText.Length());
			break;	
			}
		
		default:
			break;
		}

	Logger->iCurrent+=aText.Length();
	*(Logger->iCurrent++) = '\n';

	if ((TInt)Logger->iCurrent > (TInt)(Logger->iChunkKernelAddr + Logger->iChunkCreateStr.iSize - MaxLogSize))
		Logger->iBufferFull = ETrue;

	NKern::RestoreInterrupts(irq);

	return ETrue;
	}

/**
Creates the channel
*/
TInt DLogToFile::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

/*
Creates chunk and opens another chunk handle for user side.
Returns user side handle.
*/
TInt DLogToFile::CreateChunk()
{
	TInt r;
    TChunkCreateInfo info;
    info.iType         = TChunkCreateInfo::ESharedKernelSingle;
    info.iMaxSize      = iChunkCreateStr.iSize;//This is hard coded to 64K. No need to round to page size.
#ifdef __EPOC32__
    info.iMapAttr      = (TInt)EMapAttrFullyBlocking; // Full caching
#endif
    info.iOwnsMemory   = ETrue; // Use memory from system's free pool
    info.iDestroyedDfc = NULL;

	NKern::ThreadEnterCS();
    if (KErrNone != (r = Kern::ChunkCreate(info, iChunk, iChunkKernelAddr, iChunkMapAttr)))
		{
		NKern::ThreadLeaveCS();
		return r;
		}
	r = Kern::ChunkCommit(iChunk,0,iChunkCreateStr.iSize);
    if(r!=KErrNone)
        {
		Kern::ChunkClose(iChunk);
		NKern::ThreadLeaveCS();
		return r;
		}
	r = Kern::MakeHandleAndOpen(NULL, iChunk); 
    if(r < KErrNone)
        {
		Kern::ChunkClose(iChunk);
		NKern::ThreadLeaveCS();
		return r;
		}
	NKern::ThreadLeaveCS();
	iCurrent = (TText8*)iChunkKernelAddr;
	return r;
}


/*
Logs input parameters info into chunk and starts logging.
*/
void DLogToFile::Start()
	{
	//Log pattern info
	memcpy (Logger->iCurrent, ((const TDesC&)PatternInfo).Ptr(), ((const TDesC&)PatternInfo).Length());
	iCurrent+=((const TDesC&)PatternInfo).Length();
	*(Logger->iCurrent++) = '\t';
	memcpy (Logger->iCurrent, iChunkCreateStr.iPattern.Ptr(), iChunkCreateStr.iPattern.Length());
	Logger->iCurrent += iChunkCreateStr.iPattern.Length();
	*(iCurrent++) = '\n';

	//Log buffern info
	memcpy (iCurrent, ((const TDesC&)BufferInfo).Ptr(), ((const TDesC&)BufferInfo).Length());
	iCurrent+=((const TDesC&)BufferInfo).Length();
	*(Logger->iCurrent++) = '\t';
	TBuf8<20> buf;
	buf.AppendNum(iChunkCreateStr.iSize);
	memcpy (iCurrent, buf.Ptr(), buf.Length());
	iCurrent+=buf.Length();
	*(iCurrent++) = '\n';

	//Log fast counter info
	memcpy (iCurrent, ((const TDesC&)FastCounterInfo).Ptr(), ((const TDesC&)FastCounterInfo).Length());
	iCurrent+=((const TDesC&)FastCounterInfo).Length();
	*(iCurrent++) = '\t';
	buf.SetLength(0);
	buf.AppendNum(NKern::FastCounterFrequency());
	memcpy (iCurrent, buf.Ptr(), buf.Length());
	iCurrent+=buf.Length();
	*(iCurrent++) = '\n';

	//Start logging	
	iOldHook = Kern::SetTraceHandler(DLogToFile::Handler);
	}

/*
Stops logging into chunk. Returns the size of the log.
*/
TInt DLogToFile::Stop()
	{
	//Setting the old hook is always risky. Is the old hook still valid?
	//We'll do it for the sake of testing.
	Kern::SetTraceHandler(iOldHook);
	return (TInt)Logger->iCurrent  - Logger->iChunkKernelAddr;
	}

/*
Closes the shared chunk
*/
void DLogToFile::RemoveChunk()
{
	NKern::ThreadEnterCS();
	//It has been a while since we removed the hook. It should be safe to close the chunk now. 
	Kern::ChunkClose(iChunk);
	NKern::ThreadLeaveCS();
}

/**
User side request entry point.
*/
TInt DLogToFile::Request(TInt aFunction, TAny* a1, TAny* /*a2*/)
	{
	TInt r = KErrNone;
	switch (aFunction)
	{
		case RLogToFileDevice::EControlCreateChunk:
			{
			kumemget(&iChunkCreateStr, a1,sizeof(TChunkCreateStr));
			r = CreateChunk();
			break;
			}
		case RLogToFileDevice::EControlStart:
			{
			Start();
			break;
			}
		case RLogToFileDevice::EControlStop:
			{
			r = Stop();
			break;
			}
		case RLogToFileDevice::EControlRemoveChunk:
			{
			RemoveChunk();
			break;
			}
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

//////////////////////////////////////////
class DTestFactory : public DLogicalDevice
	{
public:
	DTestFactory();
	// from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

DTestFactory::DTestFactory()
    {
    iParseMask = KDeviceAllowUnit;
    iUnitsMask = 0x3;
    }

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
    {
	Logger = new DLogToFile;
	aChannel = Logger;
	return (aChannel ? KErrNone : KErrNoMemory);
    }

TInt DTestFactory::Install()
    {
    return SetName(&KLogToFileName);
    }

void DTestFactory::GetCaps(TDes8& /*aDes*/) const
    {
    }

DECLARE_STANDARD_LDD()
	{
    return new DTestFactory;
	}
