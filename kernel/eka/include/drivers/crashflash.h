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
// e32/include/drivers/crashflash.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __CRASH_FLASH_H__
#define __CRASH_FLASH_H__

#include <e32def.h>
#include <e32ver.h>
#include <e32cmn.h>

///////////////////////////////////////////////////////////////////////////////
//	Crash log is stored in flash in the format:
//		Crash log size (TUint), i.e. bytes stored in crash log crash sector
//		Crash log signature (string KCrashLogSignature)
//		Uncompressed log size (TUint), if log is compressed
//		Flags (TUint8) - bits 0-3 indicate log data format, bit 4 set when log 
//						had to be truncated.
//		If nand implementation, some white space is inserted to fill up
//  	the whole of the signature block/sector
//		Actual log data in specified format
//*******WARNING*******: Crash log header each field must start/end on 4 byte 
//						boundary, otherwise NOR flash implementations will hang
///////////////////////////////////////////////////////////////////////////////

/** Maximum size of a crash log.
 * @internalTechnology
 */
const TUint KMaxCrashLogSize = 0x00100000;// 1Meg of crashflash

/** The size in bytes of the Crash Log Signature
 * @internalTechnology
 */
const TUint KCrashLogSignatureBytes = 20;

/** The size in bytes of the total size of the crash log (including itself and
 * the crash log signature)
 * @internalTechnology
 */
const TUint KCrashLogSizeFieldBytes = 4;

/* The crash log signature.
 * @internalTechnology
 */
_LIT8(KCrashLogSignature, "Symbian Crash Logger");

/** The size in bytes of the total size of the crash log once it has been uncompressed
 * @internalTechnology
 */
const TUint KCrashLogUncompSizeFieldBytes = 4;

/** The flags to indicate to the crash reader the data format of the log and if 
 *	it was truncated or not.  16 MSBs used to indicate the offset of the start of the
 *	log data from the signature, required for NAND flash implementations where signarture
 *	occupyies a whole flash sector/page filling unused space with white space
 * @internalTechnology
 */
const TUint KCrashLogFlagsFieldBytes = 4;

const TUint8 KCrashLogFlagUncompr = 0x0;/** No compression performed on log */
const TUint8 KCrashLogFlagGzip = 0x1; 	/** Log compressed using Gzip compatible output*/
const TUint8 KCrashLogFlagTypeBits = 4; /** No. of bits for the type*/

const TUint8 KCrashLogFlagTruncated = 0x10; /** The log had to be truncated*/

const TUint32 KCrashLogFlagOffShift = 16; /**place offset in 16 MSBs of flags field*/

/** Total size of the crash log header in bytes.  Must be less than the size of a 
 *	single NAND flash sector/page for current crashflashnand implementations
 *	@internalTechnology
 */
#ifndef _CRASHLOG_COMPR
const TUint KCrashLogHeaderSize = KCrashLogSignatureBytes + KCrashLogSizeFieldBytes;
#else
const TUint KCrashLogHeaderSize = KCrashLogSignatureBytes + KCrashLogSizeFieldBytes + 
									KCrashLogUncompSizeFieldBytes + KCrashLogFlagsFieldBytes;
#endif //_CRASHLOG_COMPR


/** The string to output when the log had to be truncated
 *	@internalTechnology
 */
_LIT8(KCrashLogTruncated,"\r\nLog truncated due to end of crash log partition");

/** Abstract class defining interface to all CrashFlash classes.  This is used
 * by the CrashLogger to log to a specific type of flash.
 *@internalTechnology
 */
class CrashFlash
    {
public:
	/** Called first.  Should initialise underlying crash flash device to the
	 * state that it can read, write, and erase.
	 * @return KErrNone if successful.  Else, a system wide error code.
	 */
	virtual TInt Initialise()=0;

	/** Called second.  Allows underlying implementation to set any flags
	 * required to indicate that a transaction has started.
	 */
	virtual void StartTransaction()=0;

	/** Called third.  Performs the operations necessary to erase a block of
	 * flash large enough to store a log of KMaxCrashLogSize.
	 */
	virtual void EraseLogArea()=0;

	/** Called last.  Commits any buffered data and sets the flag for the
	 * underlying crash flash device indicating that the transaction finished
	 * succesfully.
	 */
	virtual void EndTransaction()=0;

	/** Writes aDes to the underlying crash flash device.  The underlying 
	 * implementation may buffer as required.
	 */
	virtual void Write(const TDesC8& aDes)=0;

	/** Writes aDes to the signature section of the underlying cras flash device.
	 * The descriptor should include both the signature and the length written.
	 */
	virtual void WriteSignature(const TDesC8& aDes)=0;

	/** Reads the next aDes.Length() characters and places them in aDes starting
	 * from aDes[0].  The read position is modifiable using SetReadPos().  The
	 * underlying implementation may buffer as required.
	 */
	virtual void Read(TDes8& aDes)=0;

	/** Sets the internal state such that the next read will take place at
	 * aPos bytes from the base address.
	 */
	virtual void SetReadPos(TUint aPos) = 0;
	
	/** Sets the internal state of the write position
	* aPos bytes from the base address.
	*/
	virtual void SetWritePos(const TUint aPos) = 0;

	/** Returns the number of bytes written to the flash.  This is used by
	 * reading programs to figure out how much to read back.
	 * @return The current total number of bytes written to flash.
	 */
	virtual TUint BytesWritten()=0;
	
	/**
	 * Erases the data in a given flash block
	 * @param aBlock The block to be erased
	 */
	virtual void EraseFlashBlock(const TUint aBlock) = 0;
	
#ifdef _CRASHLOG_COMPR
	/** Get the amount of space availiable for crash log data only.
		Not including the space required to output the signature message
		@return The number of bytes allocated to the crash log data
	*/
	virtual TUint GetOutputLimit(void)=0;
	
	/** Get the offset from the end of the signature to the log data, if any.
		@return The number of bytes after the signature that the log data starts
	*/
	virtual TUint GetLogOffset(void)=0;
#endif
	
	};

#endif
