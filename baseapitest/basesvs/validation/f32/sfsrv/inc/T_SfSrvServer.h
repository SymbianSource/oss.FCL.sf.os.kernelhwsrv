/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


/**
@test
@internalComponent

This contains CT_SfSrvServer
*/

#ifndef __T_SFSRV_SERVER_H__
#define __T_SFSRV_SERVER_H__

#include <test/testblockcontroller.h>
#include <test/testserver2.h>

#include "T_FsData.h"
#include "T_FileData.h"
#include "T_FormatData.h"
#include "T_FileManData.h"
#include "T_DriveUnitData.h"
#include "T_RawDiskData.h"
#include "T_DirData.h"
#include "T_DirScanData.h"
#include "T_FileNamesIdenticalData.h"
#include "T_RDirData.h"
#include "T_FileTextData.h"
#include "T_EntryData.h"
#include "T_EntryArrayData.h"
#include "T_FindFileData.h"
#include "T_ParseData.h"
#include "T_ParsePtrCData.h"
#include "T_ParsePtrData.h"
#include "T_OpenFileScanData.h"
#include "T_VolumeInfoData.h"


_LIT(KRFs, 					"RFs");
_LIT(KRFile,				"RFile");
_LIT(KRFormat,				"RFormat");
_LIT(KRRawDisk,				"RRawDisk");
_LIT(KTDriveUnit,			"TDriveUnit");
_LIT(KCDir,					"CDir");
_LIT(KCDirScan,				"CDirScan");
_LIT(KCFileMan,				"CFileMan");
_LIT(KFileNamesIdentical,	"FileNamesIdentical");
_LIT(KRDir,					"RDir");
_LIT(KTFileText,			"TFileText");
_LIT(KTEntry,				"TEntry");
_LIT(KTEntryArray,			"TEntryArray");
_LIT(KTFindFile,			"TFindFile");
_LIT(KTParse,				"TParse");
_LIT(KTParsePtr,			"TParsePtr");
_LIT(KTParsePtrC,			"TParsePtrC");
_LIT(KTOpenFileScan,		"TOpenFileScan");
_LIT(KTVolumeInfo,			"TVolumeInfo");

class CT_SfSrvServer : public CTestServer2
	{
private:
	class CT_SfSrvBlock : public CTestBlockController
		{
	public:
		inline CT_SfSrvBlock();
		inline ~CT_SfSrvBlock();

		inline CDataWrapper*	CreateDataL(const TDesC& aData);
		};

public:
	static CT_SfSrvServer* NewL();
	inline ~CT_SfSrvServer();

	inline CTestBlockController*	CreateTestBlock();

protected:
	inline CT_SfSrvServer();
	};

#include "T_SfSrvServer.inl"

#endif // __T_SFSRV_SERVER_H__
