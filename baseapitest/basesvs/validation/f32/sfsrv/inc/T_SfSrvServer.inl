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

This contains CT_SfSrvServer inline functions
*/

CT_SfSrvServer::CT_SfSrvBlock::CT_SfSrvBlock()
:	CTestBlockController()
	{
	}

CT_SfSrvServer::CT_SfSrvBlock::~CT_SfSrvBlock()
	{
	}

CDataWrapper* CT_SfSrvServer::CT_SfSrvBlock::CreateDataL(const TDesC& aData)
	{
	CDataWrapper*	wrapper = NULL;
	if( KRFs() == aData )
		{
		wrapper = CT_FsData::NewL();
		}
	else if( KRFile() == aData )
		{
		wrapper = CT_FileData::NewL();
		}
	else if( KRFormat() == aData )
		{
		wrapper = CT_FormatData::NewL();
		}
	else if( KRRawDisk() == aData )
		{
		wrapper = CT_RawDiskData::NewL();
		}
	else if( KTDriveUnit() == aData )
		{
		wrapper = CT_DriveUnitData::NewL();
		}
	else if( KCDir() == aData )
		{
		wrapper = CT_DirData::NewL();
		}
	else if( KCDirScan() == aData )
		{
		wrapper = CT_DirScanData::NewL();
		}
	else if( KCFileMan() == aData )
		{
		wrapper = CT_FileManData::NewL();
		}
	else if( KFileNamesIdentical() == aData )
		{
		wrapper = CT_FileNamesIdenticalData::NewL();
		}
	else if( KRDir() == aData )
		{
		wrapper = CT_RDirData::NewL();
		}
	else if( KTFileText() == aData )
		{
		wrapper = CT_FileTextData::NewL();
		}
	else if( KTEntry() == aData )
		{
		wrapper = CT_EntryData::NewL();
		}
	else if( KTEntryArray() == aData )
		{
		wrapper = CT_EntryArrayData::NewL();
		}
	else if( KTFindFile() == aData )
		{
		wrapper = CT_FindFileData::NewL();
		}
	else if( KTParse() == aData )
		{
		wrapper = CT_ParseData::NewL();
		}
	else if( KTParsePtrC() == aData )
		{
		wrapper = CT_ParsePtrCData::NewL();
		}
	else if( KTParsePtr() == aData )
		{
		wrapper = CT_ParsePtrData::NewL();
		}
	else if( KTOpenFileScan() == aData )
		{
		wrapper = CT_OpenFileScanData::NewL();
		}
	else if( KTVolumeInfo() == aData )
		{
		wrapper = CT_VolumeInfoData::NewL();
		}  
	return wrapper;
	}

CT_SfSrvServer::CT_SfSrvServer()
	{
	}

CT_SfSrvServer::~CT_SfSrvServer()
	{
	}

CTestBlockController* CT_SfSrvServer::CreateTestBlock()
	{
	return new CT_SfSrvBlock();
	}
