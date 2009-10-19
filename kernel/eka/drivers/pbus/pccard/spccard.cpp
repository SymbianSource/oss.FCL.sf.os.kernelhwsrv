// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\pbus\pccard\spccard.cpp
// 
//

#include <pccard.h>
#include "cis.h"

LOCAL_D const TPccdAccessSpeed CisDevSpeedTable[8] =
	{EAcSpeedInValid,EAcSpeed250nS,EAcSpeed200nS,EAcSpeed150nS,
	EAcSpeed100nS,EAcSpeedInValid,EAcSpeedInValid,EAcSpeedInValid};
LOCAL_D const TUint32 CisDevSizeInBytesTable[8] =
	{0x00000200,0x00000800,0x00002000,0x00008000,0x00020000,0x00080000,0x00200000,0};
LOCAL_D const TInt CisMantisaTable[0x10] =
	{10,12,13,15,20,25,30,35,40,45,50,55,60,70,80,90};
LOCAL_D const TInt CisSpeedExponentTable[8] =
	{0,1,10,100,1000,10000,100000,1000000};

GLDEF_C void PcCardPanic(TPcCardPanic aPanic)
	{
	Kern::Fault("PCCARD",aPanic);
	}

LOCAL_C TPccdAccessSpeed DevSpeedFromExtended(TInt aSpeedInNanoSecs)
	{

	if (aSpeedInNanoSecs<=100) return(EAcSpeed100nS);
	if (aSpeedInNanoSecs<=150) return(EAcSpeed150nS);
	if (aSpeedInNanoSecs<=200) return(EAcSpeed200nS);
	if (aSpeedInNanoSecs<=250) return(EAcSpeed250nS);
	if (aSpeedInNanoSecs<=300) return(EAcSpeed300nS);
	if (aSpeedInNanoSecs<=450) return(EAcSpeed450nS);
	if (aSpeedInNanoSecs<=600) return(EAcSpeed600nS);
	if (aSpeedInNanoSecs<=750) return(EAcSpeed750nS);
	return(EAcSpeedExtended);
	}

LOCAL_C TMemDeviceType DevType(TInt aTypeCode)
	{
	if ( aTypeCode>=KTpDiDTypeNull && aTypeCode<=KTpDiDTypeDram )
		return( (TMemDeviceType)aTypeCode );
	else if (aTypeCode>=KTpDiDTypeFuncSpec)
		return(EDeviceFunSpec);
	else
		return(EDeviceInvalid);
	}

LOCAL_C TInt ExtendedSpeedToNanoSeconds(TUint8 aVal)
//
// Converts extended device speed field to speed in nS.
//
	{

	TInt mant=(aVal&KCisTplMantM)>>KCisTplMantFO;
	TInt s=(mant==0)?0:CisMantisaTable[mant-1];
	s*=CisSpeedExponentTable[aVal&KCisTplExponM];
	return(s);
	}
	 
LOCAL_C TInt PwrTplToMicroAmps(TUint aVal,TUint anExt)
//
// Converts a power tuple into an integer value - units uA.
//
	{
	TInt p=CisMantisaTable[(aVal&KCisTplMantM)>>KCisTplMantFO];
	p*=10;
	if (anExt<=99)
		p+=anExt;	// Add on the extension
	switch ( aVal&KCisTplExponM )
		{
		case 7: return(p*=10000);   case 6: return(p*=1000); 
		case 5: return(p*=100);	 	case 4: return(p*=10); 
		case 3: return(p);  		case 2: return(p/=10);
		case 1: return(p/=100);
		default: return(0); // Anything else is too small to worry about 
		}
	}

LOCAL_C TInt PwrTplToMilliVolts(TUint aVal,TUint anExt)
//
// Converts a power tuple into a integer value - units mV.
//
	{
	return(PwrTplToMicroAmps(aVal,anExt)/10);
	}

LOCAL_C TInt ParseConfigTuple(TDes8 &configTpl,TPcCardConfig &anInfo,TInt &aLastEntry)
//
// Parse a KCisTplConfig tuple.
// (Always alters iConfigBaseAddr and iRegPresent).  
//
	{

	anInfo.iConfigBaseAddr=0;
	anInfo.iRegPresent=0;

	// Get the sizes of the ConfReg base addr & ConfReg present fields
	TInt rasz=((configTpl[2]&KTpCcRaszM)>>KTpCcRaszFO)+1;
	TInt rmsz=((configTpl[2]&KTpCcRmszM)>>KTpCcRmszFO)+1;
	if ( (configTpl.Size()-4) < (rasz+rmsz) )
		return(KErrNotSupported); // Size of fields longer than tuple length.
	aLastEntry=configTpl[3];

	// Read Config. Reg. base address.
	TInt i;
	for (i=0;i<rasz;i++)
		anInfo.iConfigBaseAddr += (configTpl[4+i]<<(8*i));

	// Read Config. Reg. present mask
	if (rmsz>4) rmsz=4;	  // We only have 32bit field
	for (i=0;i<rmsz;i++)
		anInfo.iRegPresent += (configTpl[4+rasz+i]<<(8*i));
	return(KErrNone); // Ignore custom interface subtuples
	}

LOCAL_C TInt ParsePowerEntry(const TUint8 *aTplPtr,TInt *aVMax,TInt *aVMin,TInt *aPeakI,TInt *aPdwnI)
//
// Parse a Power descriptor in a KCisTplCfTableEntry tuple. Returns the 
// number of bytes we have parsed. 
//
	{
	const TUint8 *initPtr = aTplPtr;
	TUint8 present = *aTplPtr++;
	TBuf8<16> pwr;
	pwr.FillZ(16);	  // Important

	TInt i;
	for (i=0;i<16;i+=2,present>>=1)
		{
		if (present&0x01)
			{
			pwr[i]=(TUint8)((*aTplPtr)&(~KCisTplExt));
			if (*aTplPtr++ & KCisTplExt)
				{
				pwr[i+1]=(TUint8)((*aTplPtr)&(~KCisTplExt));	// Extension tuple
				while( *aTplPtr++ & KCisTplExt );				// Jump past any more extensions
				}
			}
		}

	if (aVMin && aVMax)
		{
		if (pwr[0])						 // NomV (assume +/-5%)
			{
			(*aVMin)=(*aVMax)=PwrTplToMilliVolts(pwr[0],pwr[1]);
			(*aVMin) = ((*aVMin)*95)/100;
			(*aVMax) = ((*aVMax)*105)/100;
			}
		if (pwr[2])							// MinV
			*aVMin=PwrTplToMilliVolts(pwr[2],pwr[3]);
  		if (pwr[4])							// MaxV
			*aVMax=PwrTplToMilliVolts(pwr[4],pwr[5]);
		}
	// We'll settle for average/static if no peak.
	if (aPeakI && (pwr[10]||pwr[8]||pwr[6]) )
		{
		if (pwr[6])
			*aPeakI = PwrTplToMicroAmps(pwr[6],pwr[7]);
		if (pwr[8])
			*aPeakI = PwrTplToMicroAmps(pwr[8],pwr[9]);
		if (pwr[10])
			*aPeakI = PwrTplToMicroAmps(pwr[10],pwr[11]); // Last one overides others
		}
	if (aPdwnI && pwr[12])
		*aPdwnI = PwrTplToMicroAmps(pwr[12],pwr[13]); 

	return(aTplPtr-initPtr);
	}

LOCAL_C TInt ParseTimingEntry(const TUint8 *aTplPtr)
//
// Parse a timing descriptor in a KCisTplCfTableEntry tuple. Returns the 
// number of bytes we have parsed. 
//
	{
	// We ignore this information - just jump past this field
	const TUint8 *initPtr=aTplPtr;

	TUint8 present=*aTplPtr++;	  // First the timing present field

	if ((present & KTpCeTimWaitM) != KTpCeTimWaitM)
		while( *aTplPtr++ & KCisTplExt ); // Wait time (jump past any extensions)
	if ((present & KTpCeTimRdyM) != KTpCeTimRdyM)
		while( *aTplPtr++ & KCisTplExt ); // Ready time (jump past any extensions)
	if ((present & KTpCeTimResM) != KTpCeTimResM)
		while( *aTplPtr++ & KCisTplExt ); // Reserved time (jump past any extensions)
	return(aTplPtr-initPtr);
	}

LOCAL_C TInt ParseIoEntry(const TUint8 *aTplPtr,TPccdChnk *aChnk,TInt &aNextChnkNum)
//
// Parse an IO space descriptor in a KCisTplCfTableEntry tuple. Returns the 
// number of bytes we have parsed (or a negative error value). Also returns the 
// number of config chunk entries used ('aNextChunkNum').
//
	{
	TPccdMemType memType;
	TInt bytesParsed = 1; // Must be a minimum of a single byte descriptor here.

	// Always at least one I/O space descriptor
	switch( (*aTplPtr & KTpCeBus16_8M) >> KTpCeBus16_8FO )
		{
		case 1: case 2:
			memType = EPccdIo8Mem;	 // Card supports 8bit I/O only.
			break;
		case 3:
			memType = EPccdIo16Mem;	// Card supports 8 & 16 bit I/O.
			break;
		default:
			return(KErrCorrupt);	
		}
	TUint ioLines = (*aTplPtr & KTpCeIoLinesM) >> KTpCeIoLinesFO;

	TInt ranges=1; // We always specify one chunk even if no range descriptors follow
	TInt addrInBytes=0; 
	TInt lenInBytes=0;
	// Are there any IO Range description bytes to follow
	if (*aTplPtr++ & KTpCeRangePresM)
		{
		ranges = ((*aTplPtr & KTpCeIoRangesM) >> KTpCeIoRangesFO)+1;
		addrInBytes = (*aTplPtr & KTpCeIoAddrSzM) >> KTpCeIoAddrSzFO;
		lenInBytes = (*aTplPtr & KTpCeIoAddrLenM) >> KTpCeIoAddrLenFO;
		aTplPtr++;

		// There could be multiple range descriptors
		if ((ranges+aNextChnkNum)<=KMaxChunksPerConfig)
			bytesParsed += (ranges * (addrInBytes + lenInBytes))+1;
		else
			return(KErrNotSupported);	// Too many descriptors for us
		}

	aChnk+=aNextChnkNum;
	for (;ranges>0;ranges--,aChnk++,aNextChnkNum++)
		{
		TInt j;
		aChnk->iMemType=memType;	 // I/O memory type

		// Lets get the IO start address
		aChnk->iMemBaseAddr=0;
		if (addrInBytes)
			{
			for (j=0;j<addrInBytes;j++)
				aChnk->iMemBaseAddr += (*aTplPtr++) << (8*j);
			}

		// Finally, lets get the IO length
		if (lenInBytes)
			{
			for (j=0,aChnk->iMemLen=0;j<lenInBytes;j++)
		   		aChnk->iMemLen += (*aTplPtr++) << (8*j);
			(aChnk->iMemLen)++;
			}
		else
			{
			if (ioLines)
				aChnk->iMemLen = 0x01<<ioLines;
			else
				return(KErrCorrupt); // No ioLines and no length, it's invalid.   
			}
		}
	return(bytesParsed);
	}

LOCAL_C TInt ParseMemEntry(const TUint8 *aTplPtr,TInt aFeatureVal,TPccdChnk *aChnk,TInt &aNextChnkNum)
//
// Parse a memory space descriptor in a KCisTplCfTableEntry tuple. Returns
// the number of bytes we have parsed (or a negative error value). Also returns the 
// number of config chunk entries used ('aNextChunkNum').
//
	{

	const TUint8 *initPtr=aTplPtr;
	TInt windows=0; 		
	TInt lenInBytes=0;  	
	TInt addrInBytes=0;		
	TBool hostAddr=EFalse;
	switch (aFeatureVal)
		{
		case 3:   // Memory space descriptor
			windows=(*aTplPtr & KTpCeMemWindowsM)+1;
			lenInBytes=(*aTplPtr & KTpCeMemLenSzM) >> KTpCeMemLenSzFO;
			addrInBytes=(*aTplPtr & KTpCeMemAddrSzM) >> KTpCeMemAddrSzFO;
			hostAddr=(*aTplPtr & KTpCeMemHostAddrM);
			aTplPtr++;
			break;
		case 2:			// Length(2byte) and base address(2byte) specified.
			addrInBytes=2; 
		case 1:			// Single 2-byte length specified.
			lenInBytes=2;
			windows=1;
			break;
		}

	if ((windows+aNextChnkNum)>KMaxChunksPerConfig)
		return(KErrNotSupported);	// Too many descriptors for us

	aChnk+=aNextChnkNum;
	TInt i;
	for (;windows>0;windows--,aChnk++,aNextChnkNum++)
		{
		aChnk->iMemType=EPccdCommon16Mem;
		aChnk->iMemLen=0;
		if (lenInBytes)
			{
			for (i=0;i<lenInBytes;i++)
				aChnk->iMemLen += (*aTplPtr++) << ((8*i)+8);  	// in 256 byte pages
			}
		aChnk->iMemBaseAddr=0;
		if (addrInBytes)
			{
			for (i=0;i<addrInBytes;i++)
				aChnk->iMemBaseAddr += (*aTplPtr++) << ((8*i)+8);// in 256 byte pages
			}
		if (hostAddr)
			{
			for (i=0;i<addrInBytes;i++)
				aTplPtr++; // Dont record this, just advance the tuple pointer
			}
		}
	return(aTplPtr-initPtr);
	}

LOCAL_C TInt ParseMiscEntry(const TUint8 *aTplPtr, TBool &aPwrDown)
//
// Parse a miscellaneous features field in a KCisTplCfTableEntry tuple.
// Returns the number of bytes we have parsed. 
//
	{
	aPwrDown=(*aTplPtr&KTpCePwrDownM);

	TInt i;
	for (i=1;*aTplPtr & KCisTplExt;i++,aTplPtr++);
	return(i);
	}

LOCAL_C TInt ParseConfigEntTuple(TDes8 &cTpl,TPcCardConfig &anInfo)
//
// Parse a KCisTplCfTableEntry tuple. anInfo contains default values on 
// entry so this routine only adds data it finds in the tuple.
//
	{

	// Parse the Index byte.
	const TUint8 *tplPtr=cTpl.Ptr()+2; // First tuple after link
	anInfo.iConfigOption=(*tplPtr & KTpCeOptionM);
	anInfo.iIsDefault=(*tplPtr & KTpCeIsDefaultM);

	// Check if there is an interface description field to follow
	if (*tplPtr++ & KTpCeIntfPresM)
		{
 		anInfo.iIsIoAndMem=(*tplPtr&KTpCeIntfTypeM);
		anInfo.iActiveSignals=*tplPtr&(KTpCeBvdM|KTpCeWpM|KTpCeReadyM|KTpCeWaitM);
		tplPtr++;
		}

	// Next byte should be the feature selection byte.
	TUint8 features=*tplPtr++;

	// Next might be 0-3 power description structures. 1st one is always VCC info.
	TInt entry=(features & KTpCePwrPresM)>>KTpCePwrPresFO;
	if (entry)
		{
		tplPtr += ParsePowerEntry(tplPtr,&anInfo.iVccMaxInMilliVolts,&anInfo.iVccMinInMilliVolts,
							      &anInfo.iOperCurrentInMicroAmps,&anInfo.iPwrDwnCurrentInMicroAmps);
		entry--;
		}
	
	// We only support a single Vpp supply. However we need to parse both (Vpp1+Vpp2)
	// in order to advance the tuple pointer.
	while ( entry-- )
		tplPtr += ParsePowerEntry(tplPtr,&anInfo.iVppMaxInMilliVolts,&anInfo.iVppMinInMilliVolts,NULL,NULL);

	// Next might be timing info.
	if (features & KTpCeTimPresM)
		tplPtr += ParseTimingEntry(tplPtr);

	// Next might be IO space description.
	TInt ret;
	TInt nextFreeChunk=0;
	if (features & KTpCeIoPresM)
		{
		if((ret=ParseIoEntry(tplPtr,&(anInfo.iChnk[0]),nextFreeChunk))<0)
			return(ret);
		anInfo.iValidChunks=nextFreeChunk;
		tplPtr += ret;
		}

	// Next might be IRQ description.
	if (features & KTpCeIrqPresM)
		{
		anInfo.iInterruptInfo=*tplPtr&(KPccdIntShare|KPccdIntPulse|KPccdIntLevel);
		tplPtr+=(*tplPtr&KTpCeIrqMaskM)?3:1; // Ignore mask bytes if present
		}

	// Next might be memory space description.
	entry=((features & KTpCeMemPresM) >> KTpCeMemPresFO);
	if (entry)
		{
		if ((ret=ParseMemEntry(tplPtr,entry,&(anInfo.iChnk[0]),nextFreeChunk))<0)
			return(ret);
		anInfo.iValidChunks=nextFreeChunk;
		tplPtr+=ret;
		}

	// And finally there might be a miscellaneous features field
	if (features & KTpCeMiscPresM)
		tplPtr+=ParseMiscEntry(tplPtr,anInfo.iPwrDown);

	// Check that we haven't been reading beyond the tuple.
	if ((tplPtr-cTpl.Ptr()) > (cTpl[1]+2))
		return(KErrCorrupt);

	return(KErrNone);
	}

LOCAL_C TInt ParseDeviceInfo(const TUint8 *aTplPtr,TPcCardRegion &anInfo)
//
// Parse a device info field in a KCisTplDeviceX tuple.
// Returns the number of bytes we have parsed (or a negative error value). 
//
	{

	const TUint8 *initPtr=aTplPtr;
	TInt val;
	// Device ID - device type field
	val=((*aTplPtr & KTpDiDTypeM) >> KTpDiDTypeFO);
	if (val==KTpDiDTypeExtend)
		return(KErrNotSupported);	   // Don't support extended device type
	anInfo.iDeviceType=DevType(val);

	// Device ID - write protect field
	if (!(*aTplPtr&KTpDiWpsM))
		anInfo.iActiveSignals|=KSigWpActive;

	// Device ID - device speed field
	val=(*aTplPtr & KTpDiDSpeedM);
	if (val==KTpDiDSpeedExt)
		{
		aTplPtr++;
		anInfo.iExtendedAccSpeedInNanoSecs=ExtendedSpeedToNanoSeconds(*aTplPtr);
		anInfo.iAccessSpeed=DevSpeedFromExtended(anInfo.iExtendedAccSpeedInNanoSecs);
		while(*aTplPtr++ & KCisTplExt); // Jump past any (further) extended speed fields
		}
	else
		{
		anInfo.iExtendedAccSpeedInNanoSecs=0;
		anInfo.iAccessSpeed=CisDevSpeedTable[val];
		aTplPtr++;
		}

	// Now the Device size
	TInt size,numUnits;
	size=((*aTplPtr & KTpDiDSizeM) >> KTpDiDSizeFO);
	numUnits=((*aTplPtr++ & KTpDiDUnitsM) >> KTpDiDUnitsFO)+1;
	if (size>KTpDiDSize2M)
		return(KErrCorrupt);	 
	anInfo.iChnk.iMemLen=numUnits*CisDevSizeInBytesTable[size];
	return(aTplPtr-initPtr);
	}
/*
LOCAL_C TInt SocketIsInRange(TSocket aSocket)
//
// Check socket is valid for this machine
//
	{

//	return(aSocket>=0&&aSocket<ThePcCardController->TotalSupportedBuses());
	return (aSocket>=0 && aSocket<KMaxPBusSockets && TheSockets[aSocket]!=NULL);
	}
*/
EXPORT_C TCisReader::TCisReader()
//
// Constructor.
//
	: iFunc(0),iCisOffset(0),iLinkOffset(0),iMemType(EPccdAttribMem),
	  iLinkFlags(0),iRestarted(EFalse),iRegionCount(0),
	  iConfigCount(0)
	{
	iSocket=NULL;
	}

EXPORT_C TInt TCisReader::SelectCis(TSocket aSocket,TInt aCardFunc)
//
//  Assign the CIS reader to a socket and function.
//
	{
	// We need to have read the CIS format
	__KTRACE_OPT(KPBUS1,Kern::Printf(">CisReader:SelectCis(S:%d F:%d)",aSocket,aCardFunc));
	DPcCardSocket* pS=(DPcCardSocket*)TheSockets[aSocket];
	if (pS->CardIsReadyAndVerified()!=KErrNone)
		return KErrNotReady;
	iSocket=pS;
	return(DoSelectCis(aCardFunc));
	}

TInt TCisReader::DoSelectCis(TInt aCardFunc)
//
//  Actually assign the CIS reader to a socket and function.
//
	{

	// Check that the function is valid
	TInt r;
	if (!iSocket->IsValidCardFunc(aCardFunc))
		{
		iSocket=NULL;
		r=KErrNotFound;
		}
	else
		{
		iFunc=aCardFunc;
		DoRestart();
		iConfigCount=0;
		r=KErrNone;
		}
	__KTRACE_OPT(KPBUS1,Kern::Printf("<CisReader:DoSelectCis(F:%d)-%d",aCardFunc,r));
	return(r);
	}

EXPORT_C TInt TCisReader::Restart()
//
// Restart the CIS reader back to the start of the CIS, and re-initialise
// config entry parsing.
//
	{
	if (iSocket==NULL)
		return(KErrGeneral);
	DoRestart();
	iConfigCount=0;
	return(KErrNone);
	}

void TCisReader::DoRestart()
//
//  Restart the CIS reader back to the start of the CIS
//
	{

	TPcCardFunction *func=iSocket->CardFunc(iFunc);
	iCisOffset=func->InitCisOffset();	
	iLinkOffset=0;	
	iMemType=func->InitCisMemType();
	iLinkFlags=0;
	iRestarted=ETrue;
	iRegionCount=0;
	__KTRACE_OPT(KPBUS1,Kern::Printf("<CisReader:DoRestart"));
	}

EXPORT_C TInt TCisReader::FindReadTuple(TUint8 aDesiredTpl,TDes8 &aDes,TUint aFlag)
//
// Find a specified tuple from the CIS and read it.
//
	{                                 
	__ASSERT_ALWAYS(iSocket!=NULL,PcCardPanic(EPcCardCisReaderUnInit)); 

	// We're going to read the card itself so it must be ready.
	if ( iSocket->CardIsReadyAndVerified()!=KErrNone )
		return(KErrNotReady);

	return(DoFindReadTuple(aDesiredTpl,aDes,aFlag));
	}

TInt TCisReader::DoFindReadTuple(TUint8 aDesiredTpl,TDes8 &aDes,TUint aFlag)
//
// Actually find a specified tuple from the CIS and read it.
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">CisReader:DoFindReadTuple(T:%xH)",aDesiredTpl));

	TBuf8<KSmallTplBufSize> tpl;
	TBuf8<KSmallTplBufSize> linkAddr;
	TInt i,j,err;

	// Read the previous tuple
	if ((err=iSocket->ReadCis(iMemType,iCisOffset,tpl,2))!=KErrNone)
		return(err);

	for (j=0;j<KMaxTuplesPerCis;j++)
		{
		// Adjust CIS offset beyond last tuple read (unless we've just restarted)
		if (iRestarted)
			iRestarted=EFalse;
		else
			{
			if (tpl[0]!=KCisTplEnd && tpl[1]!=0xff)
				iCisOffset+=(tpl[0]==KCisTplNull)?1:(tpl[1]+2); // A null tuple has no link field
			else
				{
				// End of chain tuple
				if ((err=FollowLink(aFlag&KPccdReportErrors))!=KErrNone)
					return(err);
				}
			}

		// Read the next tuple
		if ((err=iSocket->ReadCis(iMemType,iCisOffset,tpl,2))!=KErrNone)
			return(err);

		// Check for a link tuple (need to store next chain addr. for later)
		switch(tpl[0])
			{
			case KCisTplLongLinkA:
				iLinkFlags |= KPccdLinkA;
				if ((err= iSocket->ReadCis(iMemType,iCisOffset+2,linkAddr,4)) != KErrNone)
					return(err);
				for (iLinkOffset=0,i=0 ; i<4 ; i++)
					iLinkOffset += linkAddr[i] << (8*i);
				break;
			case KCisTplLongLinkC:
				iLinkFlags |= KPccdLinkC;
				if ((err= iSocket->ReadCis(iMemType,iCisOffset+2,linkAddr,4)) != KErrNone)
					return(err);
				for (iLinkOffset=0,i=0 ; i<4 ; i++)
					iLinkOffset += linkAddr[i] << (8*i);
				break;
			case KCisTplLongLinkMfc:
				iLinkFlags |= KPccdLinkMFC;
				break;
			case KCisTplNoLink:
				iLinkFlags |= KPccdNoLink;
			default:
				break;
			}

		// Check if we have found the specified tuple
		if (aDesiredTpl==KPccdNonSpecificTpl || aDesiredTpl==tpl[0])
			{
			// The following are ignored unless KPccdReturnLinkTpl is set. 
			if ((tpl[0]==KCisTplNull)||
				(tpl[0]==KCisTplEnd)||
				(tpl[0]==KCisTplLongLinkA)||
				(tpl[0]==KCisTplLongLinkC)||
				(tpl[0]==KCisTplLongLinkMfc)||
				(tpl[0]==KCisTplNoLink)||
				(tpl[0]==KCisTplLinkTarget))
				{
				if (aFlag&KPccdReturnLinkTpl)
					break;
				}
			else
				break;
			}
		}

	// We got a result (or we've wandered off into the weeds)
	if (j>=KMaxTuplesPerCis)
		return( (aFlag&KPccdReportErrors)?KErrCorrupt:KErrNotFound );
	else
		return((aFlag&KPccdFindOnly)?KErrNone:DoReadTuple(aDes));
	}

EXPORT_C TInt TCisReader::ReadTuple(TDes8 &aDes)
//
// Read the tuple at the current CIS offset.
//
	{
	__ASSERT_ALWAYS(iSocket!=NULL,PcCardPanic(EPcCardCisReaderUnInit)); 

	// We're going to read the card itself so it must be ready.
	if ( iSocket->CardIsReadyAndVerified()!=KErrNone )
		return(KErrNotReady);

	return(DoReadTuple(aDes));
	}

TInt TCisReader::DoReadTuple(TDes8 &aDes)
//
// Actually read the tuple at the current CIS offset.
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">CisReader:DoReadTuple"));
	TInt err;

	// Read the tuple type and link
	TBuf8<KSmallTplBufSize> tpl;
	if ((err= iSocket->ReadCis(iMemType,iCisOffset,tpl,2)) != KErrNone)
		return(err);

	TInt tplLen ;
	if ((tpl[0] == KCisTplNull) || (tpl[0] == KCisTplEnd))
		tplLen = 1 ;			// These tuples dont have a link.
	else
		tplLen = (tpl[1]+2) ;
	if ( tplLen>aDes.MaxLength() )   // We dont want a panic if aDes too small
		return(KErrArgument);

	// Lets copy the tuple
	if ((err= iSocket->ReadCis(iMemType,iCisOffset,aDes,tplLen)) != KErrNone)
		return(err);
	else
		return(KErrNone);
	}

TInt TCisReader::FollowLink(TUint aFullErrorReport)
//
// Called at the end of a tuple chain, this moves CIS pointer to the next
// CIS chain if a long link has been detected.
//
	{

	TInt err;
	switch (iLinkFlags)
		{
		case 0: // Haven't found anything so assume longlink to 0 in common.
			iLinkOffset=0;
		case KPccdLinkC:
			iCisOffset=iLinkOffset;
			iMemType=EPccdCommon8Mem;
			iLinkOffset=0;
			if ((err=VerifyLinkTarget())!=KErrNone)
				{
				DoRestart(); // Leave pointers somewhere safe.
				if (iLinkFlags==0||!aFullErrorReport)
					err=KErrNotFound; // Above assumption wrong
				}
			break;
		case KPccdLinkA:
			iCisOffset=iLinkOffset;
			iMemType=EPccdAttribMem;
			iLinkOffset=0;
			if ((err=VerifyLinkTarget())!=KErrNone)
				{
				iCisOffset>>=1; // Check if the link offset is wrong
				if (VerifyLinkTarget()!=KErrNone)
					{
					DoRestart(); // Leave pointers somewhere safe.
					if (!aFullErrorReport)
						err=KErrNotFound;
					}
				else
					err=KErrNone;
				}
			break;
		case KPccdNoLink:
		case KPccdLinkMFC: // Can't follow a multi-function link
			DoRestart(); // Leave pointers somewhere safe.
			err=KErrNotFound;
			break;
		default:	// Shouldn't have more than 1 link per chain
			DoRestart(); // Leave pointers somewhere safe.
			err=(aFullErrorReport)?KErrCorrupt:KErrNotFound;   
		}
	iLinkFlags=0;
	return(err);
	}

TInt TCisReader::VerifyLinkTarget()
//
//	Verify a new tuple chain starts with a valid link target tuple
//
	{
	TBuf8<KSmallTplBufSize> tpl;
	TInt err;
	if ((err=iSocket->ReadCis(iMemType,iCisOffset,tpl,5))!=KErrNone)
		return(err);
	if ( (tpl[0]!=KCisTplLinkTarget) || (tpl[1]<3) || (tpl.Find(_L8("CIS"))!=2) )
		return(KErrCorrupt);
	return(KErrNone);
	}

EXPORT_C TInt TCisReader::FindReadRegion(TPccdSocketVcc aSocketVcc,TPcCardRegion &anInfo,TUint8 aDesiredTpl)
//
// Read region info from the CIS on the specified Socket/Function. Can
// be called multiple times to read all regions (eventually
// returns KErrNotFound). 
// If the function returns an error value then ignore anInfo.
//
	{

	if (!aDesiredTpl)
		aDesiredTpl=(aSocketVcc==EPccdSocket_5V0)?KCisTplDevice:KCisTplDeviceOC;
	__KTRACE_OPT(KPBUS1,Kern::Printf(">CisReader:FindReadRegion(TPL:%xH)",aDesiredTpl));

	TInt ret;
	TBuf8<KLargeTplBufSize> devTpl;
	if (!iRegionCount)				 // Count of regions processed in tuple
		ret=FindReadTuple(aDesiredTpl,devTpl);
	else
		ret=ReadTuple(devTpl);
	if (ret!=KErrNone)
		return(ret);
	const TUint8 *tplPtr=devTpl.Ptr();
	const TUint8 *tplE=tplPtr+devTpl.Length(); 
	tplPtr+=2; // First tuple after link

	if (aDesiredTpl==KCisTplDeviceOC||aDesiredTpl==KCisTplDeviceOA)
		{
		// Process the Other Conditions info.
		anInfo.iChnk.iMemType=(aDesiredTpl==KCisTplDeviceOA)?EPccdAttribMem:EPccdCommon16Mem;
		anInfo.iActiveSignals=(*tplPtr & KTpDoMWaitM)?KSigWaitRequired:0;
		switch( (*tplPtr & KTpDoVccUsedM) >> KTpDoVccUsedFO )
			{
			case 3: anInfo.iVcc=EPccdSocket_yVy; break;
			case 2: anInfo.iVcc=EPccdSocket_xVx; break;
			case 1: anInfo.iVcc=EPccdSocket_3V3; break;
			default: anInfo.iVcc=EPccdSocket_5V0; break;
			}
		while (*tplPtr++ & KCisTplExt);	 // Ignore any extensions
		}
	else
		{ // KCisTplDevice
		anInfo.iChnk.iMemType=(aDesiredTpl==KCisTplDeviceA)?EPccdAttribMem:EPccdCommon16Mem;
		anInfo.iVcc=EPccdSocket_5V0;
		anInfo.iActiveSignals=0;
		}

	// Now start on the Device Info fields
	anInfo.iAccessSpeed=EAcSpeedInValid;
	anInfo.iChnk.iMemBaseAddr = anInfo.iChnk.iMemLen = 0;
	for (TInt regions=1;*tplPtr!=0xFF&&tplPtr<tplE;tplPtr+=ret,regions++)
		{
		// Add length of previous region to give new base address.
		anInfo.iChnk.iMemBaseAddr+=anInfo.iChnk.iMemLen;

		if ((ret=ParseDeviceInfo(tplPtr,anInfo)) < 0)
			return(ret);

		// Check if we have new region to report (dont report null regions)
		if (anInfo.iDeviceType!=EDeviceNull && regions>iRegionCount)
			{
			iRegionCount=regions; // Save for next time
			return(KErrNone);
			}
		}
	return(KErrNotFound); 
	}

EXPORT_C TInt TCisReader::FindReadConfig(TPcCardConfig &anInfo)
//
// Read configuration info from the CIS on the specified Socket/Function. Can
// be called multiple times to read all configuration options (eventually
// returns KErrNotFound). Uses previous configuration option value to mark
// where we are in a configuration table.
// If the function returns an error value then ignore anInfo.
//
	{

	__KTRACE_OPT(KPBUS1,Kern::Printf(">CisReader:FindReadConfig(%d)",iConfigCount));
	__ASSERT_ALWAYS(iSocket!=NULL,PcCardPanic(EPcCardCisReaderUnInit)); 

	DoRestart();	  // Start from beginning of CIS each time (dont reset iConfigCount though).

	// Create an initial default configuration
	TPcCardConfig defaultConfInfo;
	defaultConfInfo.iVccMaxInMilliVolts=5250;	// 5V+5%		
	defaultConfInfo.iVccMinInMilliVolts=4750;	// 5V-5%					
	defaultConfInfo.iAccessSpeed=DEF_IO_ACSPEED;
	defaultConfInfo.iActiveSignals=0;
					  
	TBuf8<KLargeTplBufSize> configTpl;
	TInt lastEntryIndex;
	TBool foundLast=EFalse;
	TInt err;
	TInt i=0;
	if (
		 (err=FindReadTuple(KCisTplConfig,configTpl))==KErrNone &&
		 (err=ParseConfigTuple(configTpl,defaultConfInfo,lastEntryIndex))==KErrNone
	   )
		{
		// Start of new configuration table
		for (; (err=FindReadTuple(KCisTplCfTableEntry,configTpl))==KErrNone && i<KMaxCfEntriesPerCis ; i++)
			{
			anInfo=defaultConfInfo; 		// Entries assume values from last default entry
			err=ParseConfigEntTuple(configTpl,anInfo);
			if (anInfo.iConfigOption==lastEntryIndex)
				foundLast=ETrue;
			else
				{
				if (foundLast)
					{
					err=KErrNotFound; // We've passed the last entry
					break;
					}
				}
			if (iConfigCount==i)
				break;
			if (err==KErrNone && anInfo.iIsDefault)
				defaultConfInfo=anInfo;
			}
		}
	if (i>=KMaxCfEntriesPerCis)
		err=KErrCorrupt;
	if (err==KErrNone)
		iConfigCount++;
	__KTRACE_OPT(KPBUS1,Kern::Printf("<CisReader:FindReadConfig-%d",err));
	return(err);
	}

TPcCardFunction::TPcCardFunction(TUint32 anOffset,TPccdMemType aMemType)
//
// Constructor
//
	: iFuncType(EUnknownCard),iInitCisOffset(anOffset),iInitCisMemType(aMemType),
	  iConfigBaseAddr(0),iConfigRegMask(0),iConfigIndex(KInvalidConfOpt),iConfigFlags(0)
	{
	iClientID=NULL;
	}

void TPcCardFunction::SetConfigOption(TInt anIndex,DBase *aClientID,TUint aConfigFlags)
//
// Save configuration index and client ID
//
	{

	iConfigIndex=anIndex;
	iClientID=aClientID;
	iConfigFlags=aConfigFlags;
	}

TInt TPcCardFunction::ConfigRegAddress(TInt aRegOffset,TInt &anAddr)
//
// Provide the specified configuration register address.
//
	{

	// Must be configured or we wont have the ConfigReg base address
	if (!IsConfigured())
		return(KErrGeneral);
	anAddr=(iConfigBaseAddr + (aRegOffset<<1));

	// Return an error if the register isn't present
	if ( !(iConfigRegMask & (0x01<<aRegOffset)) )
		return(KErrNotSupported);
	else
		return(KErrNone);
	}

EXPORT_C TPccdChnk::TPccdChnk()
//
// Constructor
//
	: iMemType(EPccdAttribMem),iMemBaseAddr(0),iMemLen(0)
	{}

EXPORT_C TPccdChnk::TPccdChnk(TPccdMemType aType,TUint32 aBaseAddr,TUint32 aLen)
//
// Constructor
//
	: iMemType(aType),iMemBaseAddr(aBaseAddr),iMemLen(aLen)
	{}

EXPORT_C TPcCardConfig::TPcCardConfig()
//
// Constructor (iConfigOption to KInvalidConfOpt guarentees that we start with
// 1st configuration entry).
//
	: iAccessSpeed(EAcSpeedInValid),iActiveSignals(0),iVccMaxInMilliVolts(0),
	  iVccMinInMilliVolts(0),iValidChunks(0),iIsIoAndMem(FALSE),iIsDefault(FALSE),
	  iPwrDown(FALSE),iVppMaxInMilliVolts(0),iVppMinInMilliVolts(0),iOperCurrentInMicroAmps(0),
	  iPwrDwnCurrentInMicroAmps(0),iInterruptInfo(0),iConfigOption(KInvalidConfOpt),iConfigBaseAddr(0),
	  iRegPresent(0)
	{}

EXPORT_C TBool TPcCardConfig::IsMachineCompatible(TSocket aSocket,TInt aFlag)
//
// Return ETrue if this configuration is compatible with this machine
//
	{

	DPcCardSocket* pS=(DPcCardSocket*)TheSockets[aSocket];
	DPcCardVcc* pV=(DPcCardVcc*)pS->iVcc;
	TInt nomSocketVcc=DPcCardVcc::SocketVccToMilliVolts(pV->VoltageSetting());
    if (!(aFlag&KPccdCompatNoVccCheck))
        {
	    // Check Vcc level compatibility
	    if (iVccMaxInMilliVolts<nomSocketVcc||iVccMinInMilliVolts>nomSocketVcc)
		    {
		    __KTRACE_OPT(KPBUS1,Kern::Printf("MachineCompatible-Bad Vcc"));
		    return(EFalse);
		    }
        }

	TPcCardSocketInfo si;
	pS->SocketInfo(si);
    if (!(aFlag&KPccdCompatNoVppCheck))
		{
		// Check Vpp level compatibility
		if (iVppMaxInMilliVolts<si.iNomVppInMilliVolts||iVppMinInMilliVolts>si.iNomVppInMilliVolts) 
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("MachineCompatible-Bad Vpp"));
			return(EFalse);
			} 
		}

    if (!(aFlag&KPccdCompatNoPwrCheck))
		{
		// Check the configurations power requirements can be supported
		if (iOperCurrentInMicroAmps>pV->MaxCurrentInMicroAmps())
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("MachineCompatible-Bad Pwr"));
			return(EFalse);
			}
		}

	// If wait requested then check its supported
	if ((iActiveSignals&KSigWaitRequired)&&!(si.iSupportedSignals&KSigWaitSupported))
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("MachineCompatible-Bad Wait-sig"));
		return(EFalse);
		}
	// Dealt with WAIT - mask out any other signls which aren't supported - not reason to reject though
	iActiveSignals&=si.iSupportedSignals;
	return(ETrue);
	}

EXPORT_C TPcCardRegion::TPcCardRegion()
//
// Constructor (iDeviceType to EDeviceInvalid guarentees that we start with
// 1st device information entry).
//
	: iAccessSpeed(EAcSpeedInValid),iActiveSignals(0),iVcc(EPccdSocket_Invalid),
	  iDeviceType(EDeviceInvalid),iExtendedAccSpeedInNanoSecs(0)
	{}

EXPORT_C TBool TPcCardRegion::IsMachineCompatible(TSocket aSocket)
//
// Return ETrue if this configuration is compatible with this machine
//
	{

	DPcCardSocket* pS=(DPcCardSocket*)TheSockets[aSocket];
	TPccdSocketVcc vcc=pS->VccSetting();
	// Check Vcc level compatibility
	if (iVcc!=vcc)
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("MachineCompatible-Bad Vcc"));
		return(EFalse);
		}

	// If wait requested then check its supported
	TPcCardSocketInfo si;
	pS->SocketInfo(si);
	TBool waitReq=(iActiveSignals&KSigWaitRequired);
	if (waitReq&&!(si.iSupportedSignals&KSigWaitSupported))
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("MachineCompatible-Bad Wait-sig"));
		return(EFalse);
		}
	// Dealt with WAIT - mask out any other signls which aren't supported - not reason to reject though
	iActiveSignals&=si.iSupportedSignals;

	// Check requested access speed (ie not too slow for us)
	TPccdAccessSpeed as=__IS_ATTRIB_MEM(iChnk.iMemType)?si.iMaxAttribAccSpeed:si.iMaxCommonIoAccSpeed;
	if (iAccessSpeed>as && !waitReq)
		{
		__KTRACE_OPT(KPBUS1,Kern::Printf("MachineCompatible-Bad speed"));
		return(EFalse);
		}
	return(ETrue);
	}

EXPORT_C TPccdType::TPccdType()
//
// Constructor
//
	: iFuncCount(0)
	{
	for (TInt i=0;i<(TInt)KMaxFuncPerCard;i++)
		iFuncType[i]=EUnknownCard;
	}


