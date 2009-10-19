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
// e32\drivers\pbus\mmc\sdcard\sdio\cisreader.h
// 
//

/**
 @file cisreader.h
 @publishedPartner
*/

#ifndef __CISREADER_H__
#define __CISREADER_H__

#include <drivers/sdio/sdio.h>
#include <drivers/sdio/function.h>

//
// General tuple parsing definitions
//
const TInt KSmallTplBufSize=16;
const TInt KLargeTplBufSize=257;



//// SDIO Tuple Definitions (Refer to SDIO Specification 16.5)
//// (CM - Common Mandatory, FM - Function Mandatory)

const TUint8 KSdioCisTplNull            =0x00; // Null tuple
const TUint8 KSdioCisTplCheckSum        =0x10; // Checksum control
const TUint8 KSdioCisTplVers1           =0x15; // Level 1 version/product information
const TUint8 KSdioCisTplAltStr          =0x16; // Alternate Language String tuple
const TUint8 KSdioCisTplManfId          =0x20; // Manufacturer Identification String tuple      (CM)
const TUint8 KSdioCisTplFuncId          =0x21; // Function Identification Tuple                 (FM)
const TUint8 KSdioCisTplFunce           =0x22; // Function Extensions                           (FM)
const TUint8 KSdioCisTplVendorSpecificS =0x80; // Vendor Unique Tuple (Start)
const TUint8 KSdioCisTplVendorSpecificE =0x8F; // Vendor Unique Tuple (End)
const TUint8 KSdioCisTplSdioStd         =0x91; // Adittional information for Standard Functions (FM)
const TUint8 KSdioCisTplSdioExt         =0x92; // Reserved
const TUint8 KSdioCisTplEnd             =0xFF; // End Of Chain Tuple                            (CM, FM)

//// Common Tuple Offsets

const TUint32 KSdioTupleOffsetType      =0x00; // Offset to Tuple ID code
const TUint32 KSdioTupleOffsetLink      =0x01; // Offset to Tuple Link
const TUint32 KSdioTupleOffsetData      =0x02; // Offset to Tuple Specific Data

//
const TUint8 KNonSpecificTpl = 0xff;
const TUint8 KInvalidConfOpt = 0xff;
//

//
// SDIO Specific Tuple Information
//

// ...for KSdioCisTplManfId
const TUint8 KSdioManfIdOffManfIdLo		= 0x02;
const TUint8 KSdioManfIdOffManfIdHi		= 0x03;
const TUint8 KSdioManfIdOffCardIdLo		= 0x04;
const TUint8 KSdioManfIdOffCardIdHi		= 0x05;

// ...for KSdioCisTplStd
const TUint8 KSdioStdOffFunctionId		= 0x02;
const TUint8 KSdioStdOffFunctionType	= 0x03;

// ...for KSdioCisTplExt (Common and Function)
const TUint8 KSdioExtOffIdent			= 0x02;
const TUint8 KSdioExtCmnIdent			= 0x00;	// Identifier for Common Extension
const TUint8 KSdioExtFuncIdent			= 0x01;	// Identifier for Function Extension

// ...for KSdioCisTplExt (Common)
const TUint8 KSdioCisTplExtCmnLen		= 0x04;

const TUint8 KSdioExtCmnOffFn0MBSLo		= 0x03;
const TUint8 KSdioExtCmnOffFn0MBSHi		= 0x04;
const TUint8 KSdioExtCmnOffMaxTranSpeed = 0x05;

// ...for KSdioCisTplExt (Function)
const TUint8 KSdioCisTplExtFuncLen1_0	= 0x1E;	// Length of Tuple for SDIO Ver 1.0
const TUint8 KSdioCisTplExtFuncLen1_1	= 0x23;	// Length of Tuple for SDIO Ver 1.1

const TUint8 KSdioExtFuncOffFuncInfo	= 0x03;
const TUint8 KSdioExtFuncOffRevision	= 0x04;
const TUint8 KSdioExtFuncOffSerialNo0	= 0x05;
const TUint8 KSdioExtFuncOffSerialNo1	= 0x06;
const TUint8 KSdioExtFuncOffSerialNo2	= 0x07;
const TUint8 KSdioExtFuncOffSerialNo3	= 0x08;
const TUint8 KSdioExtFuncOffCSASize0	= 0x09;
const TUint8 KSdioExtFuncOffCSASize1	= 0x0a;
const TUint8 KSdioExtFuncOffCSASize2	= 0x0b;
const TUint8 KSdioExtFuncOffCSASize3	= 0x0c;
const TUint8 KSdioExtFuncOffCSAProps	= 0x0d;
const TUint8 KSdioExtFuncOffMaxBlkSzLo	= 0x0e;
const TUint8 KSdioExtFuncOffMaxBlkSzHi	= 0x0f;
const TUint8 KSdioExtFuncOffOCR0		= 0x10;
const TUint8 KSdioExtFuncOffOCR1		= 0x11;
const TUint8 KSdioExtFuncOffOCR2		= 0x12;
const TUint8 KSdioExtFuncOffOCR3		= 0x13;
const TUint8 KSdioExtFuncOffMinPwrOp	= 0x14;
const TUint8 KSdioExtFuncOffAvePwrOp	= 0x15;
const TUint8 KSdioExtFuncOffMaxPwrOp	= 0x16;
const TUint8 KSdioExtFuncOffMinPwrStby	= 0x17;
const TUint8 KSdioExtFuncOffAvePwrStby	= 0x18;
const TUint8 KSdioExtFuncOffMaxPwrStby	= 0x19;
const TUint8 KSdioExtFuncOffMinBwLo		= 0x1a;
const TUint8 KSdioExtFuncOffMinBwHi		= 0x1b;
const TUint8 KSdioExtFuncOffOptBwLo		= 0x1c;
const TUint8 KSdioExtFuncOffOptBwHi		= 0x1d;
const TUint8 KSdioExtFuncOffEnableToLo	= 0x1e;	// Introduced at SDIO Ver 1.1
const TUint8 KSdioExtFuncOffEnableToHi	= 0x1f;	// Introduced at SDIO Ver 1.1
const TUint8 KSdioExtFuncOffAveHiPwrLo	= 0x20;	// Introduced at SDIO Ver 1.1
const TUint8 KSdioExtFuncOffAveHiPwrHi	= 0x21;	// Introduced at SDIO Ver 1.1
const TUint8 KSdioExtFuncOffMaxHiPwrLo	= 0x22;	// Introduced at SDIO Ver 1.1
const TUint8 KSdioExtFuncOffMaxHiPwrHi	= 0x23;	// Introduced at SDIO Ver 1.1

//

const TUint KReportErrors = 0x00002000;
const TUint KFindOnly     = 0x00004000;

const TInt KMaxTuplesPerCis = 256;

///////////////////////

class TCisReader
	{
public:
   	IMPORT_C TCisReader();   	
	IMPORT_C TInt SelectCis(TUint aSocket,TUint aStack,TUint aCard,TUint8 aFunction);
	IMPORT_C TInt Restart();
	IMPORT_C TInt FindReadTuple(TUint8 aDesiredTpl,TDes8 &aDes,TUint aFlag=0);
	IMPORT_C TInt ReadTuple(TDes8 &aDes);
	IMPORT_C TInt FindReadCommonConfig(TSDIOCardConfig& anInfo);
	IMPORT_C TInt FindReadFunctionConfig(TSDIOFunctionCaps& aCaps);
public:
	TInt DoSelectCis(TUint8 aCardFunc);
	void DoRestart();
	TInt DoFindReadTuple(TUint8 aDesiredTpl,TDes8 &aDes,TUint aFlag);
	TInt DoReadTuple(TDes8 &aDes);
	TInt ReadCis(TInt aPos,TDes8 &aDes,TInt aLen);
private:
	static TInt ParseConfigTuple(TDes8 &configTpl,TSDIOCardConfig &anInfo);
	static TInt ParseExtensionTupleCommon(TDes8 &configTpl,TSDIOCardConfig &anInfo);
	static TInt ParseExtensionTupleFunction(TDes8 &configTpl,TSDIOFunctionCaps& aCaps);
	static TInt ParseTupleStandardFunction(TDes8 &configTpl, TSDIOFunctionCaps& aCaps);
public:
	TUint8 iFunc;
	TUint32	iCisOffset;	
	TBool iRestarted; 
private:
	DMMCSocket* iSocketP;
	DMMCStack*  iStackP;
	TSDIOCard*  iCardP;
	
private:
    //
    // Reserved members to maintain binary compatibility
    TInt iReserved[2];	
	};

#endif
