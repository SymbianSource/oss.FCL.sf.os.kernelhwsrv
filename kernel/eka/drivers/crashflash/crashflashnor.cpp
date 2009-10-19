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
// e32\drivers\crashflashnor.cpp
// 
//

#include <crashflashnor.h>

void CrashFlashNor::StartTransaction()
	{
	}

void CrashFlashNor::EndTransaction()
	{
#ifdef _CRASHLOG_COMPR
	// Ensure any buffered data is output not all the data will be valid but 
	// iWriteTotal should reflect this by only increasing by no of valid/buffered bytes
	if (iWriteBufBytes)
		{
		DoWrite(iWriteBuf);
		}
#endif //_CRASHLOG_COMPR
	}

TUint CrashFlashNor::BytesWritten()
	{
	return iWriteTotal + iWriteBufBytes;
	}

void CrashFlashNor::SetReadPos(TUint aPos)
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Setting read position to %d", aPos));
	if( (aPos%sizeof(TCFIWord)) == 0)
		{
		iReadPos = aPos;
		iReadBufBytes = 0;
		iReadBuf = 0;
		}
	else
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Invalid read position requested, ignoring."));
		}
	}


TInt CrashFlashNor::Initialise()
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CrashFlashNor::Initialise()"));
	TInt ret = VariantInitialise();
	if(ret)
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("CrashFlashNor::VariantInitialise() failed"));
		return ret;
		}
	// start writing after the crash log header	
	
#ifdef CDS_CRASH_LOGGER
	iWritePos = 0;
#else
	iWritePos = KCrashLogHeaderSize;
#endif //CDS_CRASH_LOGGER
	
	SetReadPos(0);
	iWriteTotal = 0;

	return KErrNone;
	}

void CrashFlashNor::SetWritePos(TUint aPos)
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CrashFlashNor::SetWritePos"));
	if( (aPos%sizeof(TCFIWord)) == 0)
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Setting write position to %d", aPos));
		iWritePos = aPos;
		iWriteTotal = 0;
		iWriteBufBytes = 0;
		iWriteBuf = 0;
		}
	else
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Invalid read position requested, ignoring."));
		}
	}
void CrashFlashNor::Write(const TDesC8& aDes)
	{	
	if (iWritePos >= KMaxCrashLogSize)
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Write: log limit already exceeded"));
		return;
		}
	
	TInt size = aDes.Size();
#ifndef _CRASHLOG_COMPR	
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Write: %S, size: %d",&aDes,size));
#else	
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Write: writing %d bytes",size));
#endif
	const TUint8* ptr8 = aDes.Ptr();
	
	TInt truncated = EFalse;
#ifndef _CRASHLOG_COMPR	
	// If this will take us too close to (or past) the end of the crash log, discard the current string
	// and write the truncation notice instead
	if (iWritePos+size > KMaxCrashLogSize-KCrashLogTruncated().Size())
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Write: truncating log"));
		size = KCrashLogTruncated().Size();
		ptr8 = KCrashLogTruncated().Ptr();
		truncated = ETrue;
		}
#else
	// If this will take us past the end of the crash log, then truncate it
	if (iWritePos+size > KMaxCrashLogSize)
		{
		// no. of bytes left in crash log sector
		TUint tmp=KMaxCrashLogSize - iWritePos;
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Write: truncating log, limiting output to %d bytes as iWritePos=%d",tmp,iWritePos));
		size = tmp;
		truncated = ETrue;
		}
#endif

	const TUint8* end = ptr8 + size;

	for(; ptr8<end; ptr8++)
		{
		switch(iWriteBufBytes)
			{
			case 0:
				iWriteBuf |= (*ptr8);
				break;
#if defined(TCFI_2BYTE_WORD) || defined(TCFI_4BYTE_WORD)
			case 1:
				iWriteBuf |= (*ptr8)<<8;
				break;
#if defined(TCFI_4BYTE_WORD)
			case 2:
				iWriteBuf |= (*ptr8)<<16;
				break;
			case 3:
				iWriteBuf |= (*ptr8)<<24;
				break;
#endif
#endif
			}
		iWriteBufBytes++;
		if(iWriteBufBytes == (sizeof(TCFIWord)))
			{
			DoWrite(iWriteBuf);
			iWritePos+=sizeof(TCFIWord);
			iWriteTotal+=sizeof(TCFIWord);
			iWriteBuf = 0;
			}
		//equiv to iWriteBufBytes%=sizeof(TCFIWord) as long as TCFIWord is
		//a power of 2
		iWriteBufBytes&=sizeof(TCFIWord)-1;
		}
	
	// If the log was truncated, skip the write position ahead so we don't try to write any more
	if (truncated)
		{
		iWritePos = KMaxCrashLogSize;
		}
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Write: total %d, position %d", iWriteTotal, iWritePos));
	}

void CrashFlashNor::WriteSignature(const TDesC8& aDes)
	{
	if (iWriteBufBytes > 0)
		{
		DoWrite(iWriteBuf);
		iWriteBufBytes=0;
		iWriteBuf=0;
		}
	iWritePos = 0;
	Write(aDes);
	}

void CrashFlashNor::Read(TDes8& aDes)
	{
	TUint8* ptr8 = const_cast<TUint8*>(aDes.Ptr());
	const TUint8* end = ptr8 + aDes.Size();
	for( ; ptr8<end; ptr8++)
		{
		switch(iReadBufBytes)
			{
			case 0:
				iReadBuf = DoRead();
				iReadPos+=sizeof(TCFIWord);
				*ptr8 = (TUint8)(iReadBuf);
				break;
#if defined(TCFI_2BYTE_WORD) || defined(TCFI_4BYTE_WORD)
			case 1:
				*ptr8 = (TUint8)(iReadBuf>>8);
				break;
#if defined(TCFI_4BYTE_WORD)
			case 2:
				*ptr8 = (TUint8)(iReadBuf>>16);
				break;
			case 3:
				*ptr8 = (TUint8)(iReadBuf>>24);
				break;
#endif
#endif
			}
		iReadBufBytes++;
		//equiv to iReadBufBytes%=sizeof(TCFIWord) as long as TCFIWord is
		//a power of 2
		iReadBufBytes&=sizeof(TCFIWord)-1;
		}
	}

void CrashFlashNor::EraseLogArea()
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Erasing crash log area..."));
	for(TUint erased = 0; erased < KMaxCrashLogSize; erased += iEraseBlockSize)
		{		
		DoEraseBlock(erased);
		}
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("Finished erasing area for crash log."));	
	}

void CrashFlashNor::EraseFlashBlock(TUint aBlock)
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CrashFlashNor::Erasing crash flash block offset [0x%X]", aBlock));
	
	if(aBlock%iEraseBlockSize != 0 || aBlock > KMaxCrashLogSize)
		{
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Invalid Block Address - Not deleting [0x%X]", aBlock));
		return;
		}
	
	DoEraseBlock(aBlock);
	}

#ifdef _CRASHLOG_COMPR	
TUint CrashFlashNor::GetOutputLimit()
	{
	return KMaxCrashLogSize-KCrashLogHeaderSize;
	}
	
TUint CrashFlashNor::GetLogOffset(void)
	{
	return 0;
	}
#endif

