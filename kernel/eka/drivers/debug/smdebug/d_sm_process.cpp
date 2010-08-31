// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Responsible for dealing with process lists in the SM framework
//

/**
 * @file
 * @internalComponent
 * @prototype
 */

#include <sm_debug_api.h>

using namespace Debug;

/**
 * Stop Mode routine to retrieve the process list and place it in the response buffer
 * @param aItem List item describing the list
 * @return One of the system wide error codes
 */
TInt StopModeDebug::GetProcessList(const TListItem* aItem, bool aCheckConsistent)
	{
	Kern::Printf("\nDumping the process list");

	if(aItem->iListScope != EScopeGlobal)
		{
		return KErrArgument; //No other scope makes sense for process list
		}

	//Check that the process obj con lock is ok if required
	DObjectCon* objCon = Kern::Containers()[EProcess];

	if(aCheckConsistent && objCon->Lock()->iHoldCount)
		{
		return KErrNotReady;
		}

	//Look at the buffer
	TUint8* buffer = (TUint8*)aItem->iBufferAddress;
	TUint8* bufferPos = Align4(buffer + sizeof(TListReturn));
	TUint8* bufferEnd = buffer + aItem->iBufferSize;

	TListReturn* listResp = (TListReturn*)buffer;	
	listResp->iReqNo = EProcess;
	listResp->iNumberItems = 0;
	listResp->iDataSize = 0;

	DProcess** firstProcess = (DProcess**)objCon->iObjects;

	TInt numProcesses = objCon->Count();
	for(TInt cnt = 0; cnt < numProcesses; cnt++)
		{
		//Get process and see if we want to list it
		DProcess* proc = firstProcess[cnt];
		if(proc && (proc->iId >= aItem->iStartElement) )
			{
			TUint32 sizeOfProc = 0;
			TInt err = AppendProcessToBuffer(proc, bufferPos, bufferEnd, sizeOfProc);
			if(KErrNone != err)
				{
				return err;
				}
			++(listResp->iNumberItems);
			listResp->iDataSize += sizeOfProc;
			bufferPos += sizeOfProc;
			}
		}

	return KErrNone;
	}

/**
 * Writes the required process details to the stop mode response buffer in the form of a 
 * TProcessListEntry structure
 * @param aProc Process to write
 * @param aBuffer Buffer to write to
 * @param aBufferEnd Where the buffer ends
 * @return TInt KErrTooBig if there is no more space in buffer or one of the other system wide error codes
 */
TInt StopModeDebug::AppendProcessToBuffer(DProcess* aProc, TUint8* aBuffer, TUint8* aBufferEnd, TUint32& aProcSize)
	{
	TFullName procName;
	GetObjectFullName(aProc, procName);
	TUint32	dynamicNameLength = procName.Length();

	DCodeSeg* codeSeg = aProc->iCodeSeg;
	TUint16 fileNameLength = (codeSeg) ? (*codeSeg->iFileName).Length() : 0;

	//Struct size is unicode so the filenames are twice as long, plus the size of the struct minus one character that 
	//lives inside the struct itself. Also, this is word aligned
	TUint32 structSize = Align4( (2*fileNameLength) + (2*dynamicNameLength) + sizeof(TProcessListEntry) - sizeof(TUint16));
	aProcSize = structSize;

	//Is there space to write this to the buffer
	if(aBuffer + structSize < aBufferEnd)
		{
		TProcessListEntry& entry = *(TProcessListEntry*)(aBuffer);

		entry.iProcessId = (TUint64)aProc->iId;
		entry.iFileNameLength = fileNameLength;
		entry.iDynamicNameLength = dynamicNameLength;
		entry.iUid3 = aProc->iUids.iUid[2].iUid;
		entry.iAttributes = aProc->iAttributes;

		//Write the filename
		if(codeSeg)
			{
			//create TPtr to where the file name should be written
			TPtr name = TPtr((TUint8*)&(entry.iNames[0]), fileNameLength*2, fileNameLength*2);

			//copy the file name
			TInt err = CopyAndExpandDes(*codeSeg->iFileName, name);
			if(KErrNone != err)
				{
				return KErrGeneral;
				}
			}

		//create TPtr to where the dynamic name should be written
		TPtr name = TPtr((TUint8*)(&(entry.iNames[0]) + fileNameLength), dynamicNameLength*2, dynamicNameLength*2);

		//copy the dynamic name
		TInt err = CopyAndExpandDes(procName, name);
		if(KErrNone != err)
			{
			return KErrGeneral;
			}	

		return KErrNone;
		}
	else
		{
		return KErrTooBig;
		}
	}


