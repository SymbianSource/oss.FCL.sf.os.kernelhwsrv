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
// Provides utilities to build SCSI commands
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __SCSICMDBUILDER_H__
#define __SCSICMDBUILDER_H__

#include <e32std.h>
#include <f32file.h>

const TInt KScsiCmdMaxLen 	= 10;
const TInt KCbwLength       = 31;
const TInt KCswLength       = 13;
const TInt KKiloBytes		= 1024;

GLREF_D TBuf8<KScsiCmdMaxLen> scsiCmdBuf;

/**
 builds read/write SCSI command
 */
GLREF_C void BuildReadWrite(TInt aFlag, TInt aLogicalBlkAddr, TInt aTotalBlks);

/**
 Builds prevent/allow medium removal SCSI command
 
 @param aFlag: indicating if the command is to allow or prevent medium removal
 */ 
GLREF_C void BuildMediumRemoval(TInt aFlag);

/**
 Builds a test unit ready SCSI command
 */
GLREF_C void BuildTestUnitReady();

/**
 Builds a start/stop unit command
 
 @param aFlag: indicating if the command is to start or stop unit
 */ 
GLREF_C void BuildStartStopUnit(TInt aFlag);

/**
 Copy an int. Little endian
 
 @param dest the destination
 @param source the source
 */
GLREF_C void fillInt(TUint8* dest, TInt source);

/**
 Extracts an integer from a buffer. Assume little endian 
 */
GLREF_C TInt extractInt(const TUint8* aBuf);

/**
 Builds a CBW message
 
 @param aCbw:            stores CBW
 @param aDCBWTag:        a command block tag sent by the host. Used to associates a CSW
         				 with corresponding CBW
 @param aDataTranferLen: the number of bytes the host expects to transfer
 @param aInOutFlag:      value for bmCBWFlags field, indicating the direction of transfer
 @param aCBWCB:			 the actual command to be wrapped
 @param aTestLun:	 	 local unit number
 */
GLREF_C void createCBW(TDes8& aCbw, TInt aDCBWTag, TInt aDataTransferLen, TUint8 aInOutFlag, TDes8& aCBWCB, TUint8 aTestLun);

#endif // __SCSICMDBUILDER_H__

