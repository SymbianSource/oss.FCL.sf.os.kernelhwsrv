// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <drivers/sdio/cisreader.h>
#include <drivers/sdio/sdiocard.h>
#include <drivers/sdio/regifc.h>
#include "utraceepbussdio.h"

TInt TCisReader::ParseConfigTuple(TDes8 &configTpl,TSDIOCardConfig &anInfo)
/**
Parse a KSdioCisTplConfig tuple.
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "CisReader:ParseConfigTuple")); // @SymTraceDataInternalTechnology
	
	anInfo.iManufacturerID = 0;
	anInfo.iCardID = 0;

	anInfo.iManufacturerID = (TUint16)((configTpl[KSdioManfIdOffManfIdHi] << 8) | configTpl[KSdioManfIdOffManfIdLo]);
	anInfo.iCardID		   = (TUint16)((configTpl[KSdioManfIdOffCardIdHi] << 8) | configTpl[KSdioManfIdOffCardIdLo]);

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iManufacturerID = 0x%04x", anInfo.iFn0MaxBlockSize)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iCardID         = 0x%04x", anInfo.iMaxTranSpeed)); // @SymTraceDataInternalTechnology
	
	return(KErrNone);
	}

TInt TCisReader::ParseExtensionTupleCommon(TDes8 &configTpl,TSDIOCardConfig &anInfo)
/**
Parse an extension tuple (common)
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "CisReader:ParseExtensionTupleCommon")); // @SymTraceDataInternalTechnology

	if(configTpl[KSdioExtOffIdent] != KSdioExtCmnIdent)
		return KErrCorrupt;

	anInfo.iFn0MaxBlockSize = (TUint16)((configTpl[KSdioExtCmnOffFn0MBSHi] << 8) | configTpl[KSdioExtCmnOffFn0MBSLo]);
	anInfo.iMaxTranSpeed	= configTpl[KSdioExtCmnOffMaxTranSpeed];
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iFn0MaxBlockSize = 0x%04x", anInfo.iFn0MaxBlockSize)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMaxTranSpeed 	= 0x%02x", anInfo.iMaxTranSpeed)); // @SymTraceDataInternalTechnology
	
	return(KErrNone);
	}

TInt TCisReader::ParseExtensionTupleFunction(TDes8 &configTpl,TSDIOFunctionCaps& aCaps)
/**
Parse an extension tuple (function)
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "CisReader:ParseExtensionTupleFunction")); // @SymTraceDataInternalTechnology

	if(configTpl[KSdioExtOffIdent] != KSdioExtFuncIdent)
		return KErrCorrupt;

	if(configTpl.Length() < KSdioCisTplExtFuncLen1_0)
		return KErrCorrupt;

	aCaps.iFunctionInfo  = configTpl[KSdioExtFuncOffFuncInfo];
	aCaps.iRevision		 = configTpl[KSdioExtFuncOffRevision];
	
	aCaps.iSerialNumber	 = (configTpl[KSdioExtFuncOffSerialNo3] << 24) | (configTpl[KSdioExtFuncOffSerialNo2] << 16) | 
						   (configTpl[KSdioExtFuncOffSerialNo1] << 8)  |  configTpl[KSdioExtFuncOffSerialNo0];
	
	aCaps.iCSASize		 = (configTpl[KSdioExtFuncOffCSASize3] << 24) | (configTpl[KSdioExtFuncOffCSASize2] << 16) | 
						   (configTpl[KSdioExtFuncOffCSASize1] << 8)  |  configTpl[KSdioExtFuncOffCSASize0];

	aCaps.iCSAProperties = configTpl[KSdioExtFuncOffCSAProps];

	aCaps.iMaxBlockSize	 = (TUint16)((configTpl[KSdioExtFuncOffMaxBlkSzHi] << 8) | configTpl[KSdioExtFuncOffMaxBlkSzLo]);

	aCaps.iOCR 			 = (configTpl[KSdioExtFuncOffOCR3] << 24) | (configTpl[KSdioExtFuncOffOCR2] << 16) | 
						   (configTpl[KSdioExtFuncOffOCR1] << 8)  |  configTpl[KSdioExtFuncOffOCR0];

	aCaps.iMinPwrOp		 = configTpl[KSdioExtFuncOffMinPwrOp];
	aCaps.iAvePwrOp		 = configTpl[KSdioExtFuncOffAvePwrOp];
	aCaps.iMaxPwrOp		 = configTpl[KSdioExtFuncOffMaxPwrOp];
	aCaps.iMinPwrStby	 = configTpl[KSdioExtFuncOffMinPwrStby];
	aCaps.iAvePwrStby	 = configTpl[KSdioExtFuncOffAvePwrStby];
	aCaps.iMaxPwrStby	 = configTpl[KSdioExtFuncOffMaxPwrStby];

	aCaps.iMinBandwidth	 = (TUint16)((configTpl[KSdioExtFuncOffMinBwHi] << 8) | configTpl[KSdioExtFuncOffMinBwLo]);
	aCaps.iOptBandwidth	 = (TUint16)((configTpl[KSdioExtFuncOffOptBwHi] << 8) | configTpl[KSdioExtFuncOffOptBwLo]);
	
	// The following values were added in SDIO Rev 1.1
	if(configTpl.Length() > KSdioCisTplExtFuncLen1_0 && configTpl.Length() >= KSdioCisTplExtFuncLen1_1)
		{
		aCaps.iEnableTimeout = (TUint16)((configTpl[KSdioExtFuncOffEnableToHi] << 8) | configTpl[KSdioExtFuncOffEnableToLo]);
		aCaps.iAveHiPwr		 = (TUint16)((configTpl[KSdioExtFuncOffAveHiPwrHi] << 8) | configTpl[KSdioExtFuncOffAveHiPwrLo]);
		aCaps.iMaxHiPwr		 = (TUint16)((configTpl[KSdioExtFuncOffMaxHiPwrHi] << 8) | configTpl[KSdioExtFuncOffMaxHiPwrLo]);
		}
	else
		{
		aCaps.iEnableTimeout = 0;
		aCaps.iAveHiPwr		 = 0;
		aCaps.iMaxHiPwr		 = 0;
		}
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iFunctionInfo:  0x%02X", aCaps.iFunctionInfo)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iRevision:      0x%02X", aCaps.iRevision)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iSerialNumber:  0x%08X", aCaps.iSerialNumber)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iCSASize:       0x%08X", aCaps.iCSASize)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iCSAProperties: 0x%02X", aCaps.iCSAProperties)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMaxBlockSize:  0x%04X", aCaps.iMaxBlockSize)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iOCR:           0x%08X", aCaps.iOCR)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMinPwrStby:    0x%02X", aCaps.iMinPwrStby)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iAvePwrStby:    0x%02X", aCaps.iAvePwrStby)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMaxPwrStby:    0x%02X", aCaps.iMaxPwrStby)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMinPwrOp:      0x%02X", aCaps.iMinPwrOp)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iAvePwrOp:      0x%02X", aCaps.iAvePwrOp)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMaxPwrOp:      0x%02X", aCaps.iMaxPwrOp)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMinBandwidth:  0x%04X", aCaps.iMinBandwidth)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iOptBandwidth:  0x%04X", aCaps.iOptBandwidth)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iEnableTimeout: 0x%02X", aCaps.iEnableTimeout)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iAveHiPwr:      0x%02X", aCaps.iAveHiPwr)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMaxHiPwr:      0x%02X", aCaps.iMaxHiPwr)); // @SymTraceDataInternalTechnology

	return(KErrNone);
	}

TInt TCisReader::ParseTupleStandardFunction(TDes8 &configTpl, TSDIOFunctionCaps& aCaps)
/**
Parse an extension tuple (function)
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:ParseTupleStandardFunction")); // @SymTraceDataInternalTechnology

	aCaps.iStandardFunctionID   = configTpl[KSdioStdOffFunctionId];
	aCaps.iStandardFunctionType = configTpl[KSdioStdOffFunctionType];
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"iStandardFunctionID:   0x%02x", aCaps.iStandardFunctionID)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"iStandardFunctionType: 0x%02x", aCaps.iStandardFunctionType)); // @SymTraceDataInternalTechnology

	return(KErrNone);
	}

EXPORT_C TCisReader::TCisReader()
/**
Constructor.
*/
	: iFunc(0),
	  iCisOffset(0),
	  iRestarted(EFalse)
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisReaderConstructor, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	
	iSocketP = NULL;
	iStackP = NULL;
	iCardP = NULL;

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisReaderConstructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	}

EXPORT_C TInt TCisReader::SelectCis(TUint aSocket,TUint aStack,TUint aCard,TUint8 aCardFunc)
/**
Assign the CIS reader to a socket and function.
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisSelectCis, reinterpret_cast<TUint32>(this), aCard); // @SymTraceDataPublishedTvk

	// We need to have read the CIS format
	//
	// Obtain the appropriate card from the socket/stack
	//
	iSocketP = static_cast<DMMCSocket*>(DPBusSocket::SocketFromId(aSocket));
	if(iSocketP == NULL)
		{
		TRACE1(TTraceContext(EError), UTraceModuleEPBusSDIO::ESDIOSocketOOM, reinterpret_cast<TUint32>(this));
		return KErrNoMemory;
		}
	
	iStackP = static_cast<DSDIOStack*>(iSocketP->Stack(aStack));
	if(iStackP == NULL)
		{
		TRACE1(TTraceContext(EError), UTraceModuleEPBusSDIO::ESDIOStackOOM, reinterpret_cast<TUint32>(this));
		return KErrNoMemory;
		}

	iCardP = static_cast<TSDIOCard*>(iStackP->CardP(aCard));
	if(iCardP == NULL)
		{
		TRACE1(TTraceContext(EError), UTraceModuleEPBusSDIO::ESDIOCardOOM, reinterpret_cast<TUint32>(this));
		return KErrNoMemory;
		}

	TInt ret = DoSelectCis(aCardFunc);
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisSelectCisReturning, reinterpret_cast<TUint32>(this), ret); // @SymTraceDataPublishedTvk
	
	return(ret);
	}

TInt TCisReader::DoSelectCis(TUint8 aCardFunc)
/**
Actually assign the CIS reader to a socket and function.
@internalTechnology
*/
	{
	// Check that the function is valid
	TInt r = KErrNone;
	iFunc=aCardFunc;
	DoRestart();

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:DoSelectCis(F:%d)-%d",aCardFunc,r)); // @SymTraceDataInternalTechnology
	return(r);
	}

EXPORT_C TInt TCisReader::Restart()
/**
Restart the CIS reader back to the start of the CIS, and re-initialise
config entry parsing.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisRestart, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	DoRestart();

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisRestart, reinterpret_cast<TUint32>(this), KErrNone); // @SymTraceDataPublishedTvk
	
	return(KErrNone);
	}

void TCisReader::DoRestart()
/**
Restart the CIS reader back to the start of the CIS
@internalTechnology
*/
	{
	if(iFunc != 0)
		{
		TSDIOFunction* functionP = iCardP->IoFunction(iFunc);
		if(functionP == NULL)
			{
			iRestarted = EFalse;
			return;
			}
			
		iCisOffset = functionP->CisPtr();
		}
	else
		{
		iCisOffset = iCardP->CommonConfig().CisPtr();
		}
		
	iRestarted=ETrue;

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:DoRestart")); // @SymTraceDataInternalTechnology
	}

EXPORT_C TInt TCisReader::FindReadTuple(TUint8 aDesiredTpl,TDes8 &aDes,TUint aFlag)
/**
Find a specified tuple from the CIS and read it.
*/
	{        
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisFindReadTuple, reinterpret_cast<TUint32>(this), aDesiredTpl); // @SymTraceDataPublishedTvk
	
	TInt ret = DoFindReadTuple(aDesiredTpl,aDes,aFlag);

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisFindReadTupleReturning, reinterpret_cast<TUint32>(this), ret); // @SymTraceDataPublishedTvk

	return(ret);
	}

TInt TCisReader::DoFindReadTuple(TUint8 aDesiredTpl,TDes8 &aDes,TUint aFlag)
/**
Actually find a specified tuple from the CIS and read it.
@internalTechnology
*/
	{

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:DoFindReadTuple(T:%xH)",aDesiredTpl)); // @SymTraceDataInternalTechnology

	TBuf8<KSmallTplBufSize> tpl;
	TInt j,err;

	if ((err=ReadCis(iCisOffset,tpl,2))!=KErrNone)
		return(err);

	for (j=0;j<KMaxTuplesPerCis;j++)
		{
		// Adjust CIS offset beyond last tuple read (unless we've just restarted)
		if (iRestarted)
			iRestarted=EFalse;
		else
			{
			if (tpl[0]!=KSdioCisTplEnd && tpl[1]!=0xff)
				iCisOffset += (tpl[0] == KSdioCisTplNull) ? 1 : (tpl[1] + 2); // A null tuple has no link field
			else
				{
				return(KErrNotFound);
				}
			}

		// Read the next tuple
		if ((err=ReadCis(iCisOffset,tpl,2))!=KErrNone)
			return(err);

		// Check if we have found the specified tuple
		if (aDesiredTpl==KNonSpecificTpl || aDesiredTpl==tpl[0])
			{
			// The following are ignored.
			if ((tpl[0]==KSdioCisTplNull)||
				(tpl[0]==KSdioCisTplEnd))
				{
				}
			else
				break;
			}
		}

	// We got a result (or we've wandered off into the weeds)
	if (j >= KMaxTuplesPerCis)
		{
		return( (aFlag&KReportErrors) ? KErrCorrupt : KErrNotFound );
		}
	else
		{
		return((aFlag&KFindOnly) ? KErrNone : DoReadTuple(aDes));
		}
	}

EXPORT_C TInt TCisReader::ReadTuple(TDes8 &aDes)
/**
Read the tuple at the current CIS offset.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisReadTuple, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	TInt ret = DoReadTuple(aDes);

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisReadTupleReturning, reinterpret_cast<TUint32>(this), ret); // @SymTraceDataPublishedTvk
	
	return (ret);
	}

TInt TCisReader::DoReadTuple(TDes8 &aDes)
/**
Actually read the tuple at the current CIS offset.
@internalTechnology
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:DoReadTuple")); // @SymTraceDataInternalTechnology
	
	TInt err;

	// Read the tuple type and link
	TBuf8<KSmallTplBufSize> tpl;
	if ((err= ReadCis(iCisOffset,tpl,2)) != KErrNone)
		return(err);

	TInt tplLen ;
	if ((tpl[0] == KSdioCisTplNull) || (tpl[0] == KSdioCisTplEnd))
		tplLen = 1 ;			// These tuples dont have a link.
	else
		tplLen = (tpl[1]+2) ;
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:TupleLength %d",tplLen)); // @SymTraceDataInternalTechnology
	
	if ( tplLen>aDes.MaxLength() )   // We dont want a panic if aDes too small
		return(KErrArgument);

	// Lets copy the tuple
	if ((err= ReadCis(iCisOffset,aDes,tplLen)) != KErrNone)
		return(err);
	else
		return(KErrNone);
	}


EXPORT_C TInt TCisReader::FindReadCommonConfig(TSDIOCardConfig &anInfo)
/**
Searches for the Card and Common Function Identification Tuples.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisFindReadCommonConfig, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:FindReadCommonConfig")); // @SymTraceDataInternalTechnology

	// Copy stuff from the CCCR that doesn't come from the CIS (iRevision, etc.) 
	anInfo = iCardP->CommonConfig();

	DoRestart();	  // Start from beginning of CIS each time

	TBuf8<KLargeTplBufSize> configTpl;
	TInt err;

	// Look for the Manufacturer ID Tuple
	if ((err = FindReadTuple(KSdioCisTplManfId, configTpl)) == KErrNone &&
		(err = ParseConfigTuple(configTpl, anInfo)) == KErrNone)
		{
		// Although these are manditory, they may not exist in
		// some cards so we should carry on if not found.
		}

	DoRestart(); // Start from beginning of CIS
	
	// Now look for the Function Identification Tuple.  This is followed
	// by the Function Extension Tuples, which contain information about each
	// function.  Here, we are interested in Function 0 (Common) info.
	if ((err = FindReadTuple(KSdioCisTplFuncId, configTpl)) == KErrNone)
		{
		if(configTpl[2] == 0x0C)
			{
			DoRestart(); // Start from beginning of CIS	
			
			// This is an SDIO Function.  Follow up the extended data
			if ((err = FindReadTuple(KSdioCisTplFunce, configTpl)) == KErrNone)
				{
				if(configTpl[2] == 0x00) // Common Function Tuple Info.
					{
					err = ParseExtensionTupleCommon(configTpl,anInfo);
					}
				else
					{
					Printf(TTraceContext(EError),"TCisReader::FindReadCommonConfig, SDIO Function not supported");
					return(KErrNotSupported);
					}
				}
			}
		}

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:FindReadCommonConfig-%d",err)); // @SymTraceDataInternalTechnology

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisFindReadCommonConfigReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	
	return(err);
	}

EXPORT_C TInt TCisReader::FindReadFunctionConfig(TSDIOFunctionCaps& aCaps)
/**
Searches for the Function Identification Tuple.  This is followed
by the Function Extension Tuples, which contain information about each
function.  Here, we are interested in Function 1:7 (Function) info.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisFindReadFunctionConfig, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:FindReadFunctionConfig")); // @SymTraceDataInternalTechnology

	TInt err = KErrNone;
	
	// Copy stuff from the FBR that doesn't come from the CIS (iNumber, etc.) 
	TSDIOFunction* pFunction = iCardP->IoFunction(iFunc);
	if(pFunction)
		aCaps = pFunction->Capabilities();

	DoRestart(); // Start from beginning of CIS	
	
	TBuf8<KLargeTplBufSize> configTpl;
	if ((err = FindReadTuple(KSdioCisTplFunce, configTpl)) == KErrNone)
		{
		if(configTpl[2] == 0x01) // Function Tuple Info.
			{
			err = ParseExtensionTupleFunction(configTpl, aCaps);
			}
		else
			{
			return(KErrNotSupported);
			}
		}

	if(err != KErrNone)
		{
		return(err);
		}

	// If we have found the function tuple, follow up the SDIO Standard Code Tuple
	// (This is mandatory for cards that conform to the standard SDIO interface API's)
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:FindReadFunctionConfig DoRestart()")); // @SymTraceDataInternalTechnology
	DoRestart(); // Start from beginning of CIS	
	
	if ((err = FindReadTuple(KSdioCisTplSdioStd, configTpl)) == KErrNone)
		{	
		err=ParseTupleStandardFunction(configTpl, aCaps);
		}
	else if(err == KErrNotFound)
		{
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:KErrNotFound!")); // @SymTraceDataInternalTechnology
		err = KErrNone;
		}				
		
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"CisReader:FindReadFunctionConfig-%d",err)); // @SymTraceDataInternalTechnology
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTCisFindReadFunctionConfig, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk
	return(err);
	}

const TInt KReadCisBufferSize=0x80;   // 128 Bytes
TInt TCisReader::ReadCis(TInt aPos,TDes8 &aDes,TInt aLen)
/**
Read from CIS
@internalTechnology 
*/
	{

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"TCisReader::ReadCis(LE:%xH PO:0x%08x)",aLen,aPos)); // @SymTraceDataInternalTechnology

	TInt err = KErrNone;

	TInt cisE=(aPos+aLen);
	aDes.Zero();
	TText8 buf[KReadCisBufferSize];
	TInt s;
	for (;aPos<cisE;aPos+=s)
		{
		s=Min(KReadCisBufferSize,(cisE-aPos));

		// It is faster to do read direct than multiple for one byte
		if(s>1)
			{
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"ReadMultiple8")); // @SymTraceDataInternalTechnology
			err = iCardP->CommonRegisterInterface()->ReadMultiple8(aPos, buf, s);
			}
		else
			{
			SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals),"Read8")); // @SymTraceDataInternalTechnology
			err = iCardP->CommonRegisterInterface()->Read8(aPos, buf);
			}

		if(err != KErrNone)
			{
			return err;
			}

		for (TInt i=0;i<s;i++)
			{
			aDes.Append((TChar)buf[i]);   
			}
		} 

	return(err);
	}

