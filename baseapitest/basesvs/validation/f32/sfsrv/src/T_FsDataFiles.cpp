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

This contains CT_FsData
*/

//	User includes
#include "T_FsData.h"
#include "FileserverUtil.h"

//	EPOC includes
#include <e32cmn.h>

/*@{*/

//	Parameters
_LIT(KAttMask,					"attmask" );
_LIT(KPath,						"path");
_LIT(KDrive,					"drive");
_LIT(KDir,						"dir");
_LIT(KFile,						"file");
_LIT(KName,						"name");
_LIT(KNewName,					"newname");
_LIT(KOldName,					"oldname");
_LIT(KLongName,					"longname");
_LIT(KShortName,				"shortname");
_LIT(KLengthBuffer,				"length_buffer");
_LIT(KLengthRead,				"length_read");
_LIT(KEntrySortKey,				"sortkey");
_LIT(KEntrySetAttMask,			"setattmask");
_LIT(KEntryClearAttMask,		"clearattmask");
_LIT(KTime,						"time");
_LIT(KOffset,					"offset");
_LIT(KTUidType,					"uidtype");
_LIT(KIsDirList,				"isdirlist");
_LIT(KRelated,					"related");
_LIT(KBadChar,					"badchar");
_LIT(KIsBadChar,				"isbadchar");
_LIT(KCompareName,				"comparename");
_LIT(KComparePath,				"comparepath");
_LIT(KParseName,				"parsename");
_LIT(KParseDrive,				"parsedrive");
_LIT(KParsePath,				"parsepath");
_LIT(KParseExt,					"parseext");
_LIT(KIsExist,					"isexist");
_LIT(KIsSetAtt,					"issetatt");
_LIT(KSize,						"size");
_LIT(KIsOpen,					"isopen");
_LIT(KIsValid,					"isvalid");
_LIT(KIsInRom,					"isinrom");
_LIT(KDirEntryArray,			"dirEntryObject");
_LIT(KDirDirectoryArray,		"dirDirectoryObject");
_LIT(KObjectEntry,				"entry" );

//	Attributes for compare
_LIT(KIsAttReadOnly, 			"isattreadonly");
_LIT(KIsAttHidden, 				"isatthidden");
_LIT(KIsAttSystem, 				"isattsystem");
_LIT(KIsAttDir, 				"isattdir");
_LIT(KIsAttArchive, 			"isattarchive");
_LIT(KIsAttAllowUid, 			"isattuid");

//	Commands
_LIT(KCmdRealName,				"RealName");
_LIT(KCmdSessionPath,			"SessionPath");
_LIT(KCmdSetSessionPath,		"SetSessionPath");
_LIT(KCmdParse,					"Parse");
_LIT(KCmdMkDir,					"MkDir");
_LIT(KCmdMkDirAll,				"MkDirAll");
_LIT(KCmdRmDir,					"RmDir");
_LIT(KCmdGetDir,				"GetDir");
_LIT(KCmdDelete,				"Delete");
_LIT(KCmdRename,				"Rename");
_LIT(KCmdReplace,				"Replace");
_LIT(KCmdAtt,					"Att");
_LIT(KCmdSetAtt,				"SetAtt");
_LIT(KCmdModified,				"Modified");
_LIT(KCmdSetModified,			"SetModified");
_LIT(KCmdEntry,					"Entry");
_LIT(KCmdSetEntry,				"SetEntry");
_LIT(KCmdReadFileSection,		"ReadFileSection");
_LIT(KCmdIsFileOpen,			"IsFileOpen");
_LIT(KCmdGetShortName,			"GetShortName");
_LIT(KCmdGetLongName,			"GetLongName");
_LIT(KCmdIsFileInRom,			"IsFileInRom");
_LIT(KCmdIsValidName,			"IsValidName");
_LIT(KCmdSetSessionToPrivate,	"SetSessionToPrivate");
_LIT(KCmdPrivatePath,			"PrivatePath");
_LIT(KCmdCreatePrivatePath,		"CreatePrivatePath");

//	Sort key
_LIT(KESortNone,				"ESortNone");
_LIT(KESortByName,				"ESortByName");
_LIT(KESortByExt,				"ESortByExt");
_LIT(KESortBySize,				"ESortBySize");
_LIT(KESortByDate,				"ESortByDate");
_LIT(KESortByUid,				"ESortByUid");
_LIT(KEDirsAnyOrder,			"EDirsAnyOrder");
_LIT(KEDirsFirst,				"EDirsFirst");
_LIT(KEDirsLast,				"EDirsLast");
_LIT(KEAscending,				"EAscending");
_LIT(KEDescending,				"EDescending");
_LIT(KEDirDescending,			"EDirDescending");

// Attributes
_LIT(KEntryAttUnknown, 					"KEntryAttUnknown");
_LIT(KEntryAttReadOnlyStr, 				"KEntryAttReadOnly");
_LIT(KEntryAttHiddenStr, 				"KEntryAttHidden");
_LIT(KEntryAttSystemStr, 				"KEntryAttSystem");
_LIT(KEntryAttVolumeStr,				"KEntryAttVolume");
_LIT(KEntryAttDirStr, 					"KEntryAttDir");
_LIT(KEntryAttArchiveStr, 				"KEntryAttArchive");
_LIT(KEntryAttAllowUidStr,				"KEntryAttAllowUid");
_LIT(KEntryAttXIPStr, 					"KEntryAttXIP");

// Constants
_LIT(KTimeFormat,				"%D%M%Y%/0%1%/1%2%/2%3%/3 %-B%:0%J%:1%T%:2%S%:3%+B"); //SIZE 30
#define KTimeFormatSize 		30
#define KShortNameSize	 		13
#define KLongNameSize	 		256

/*@}*/

/**
* Process a files related command read from the ini file
*
* @param aCommand	the command to process
* @param aSection		the entry in the ini file requiring the command to be processed
*
* @return ETrue if the command is processed
*/
TBool CT_FsData::DoCommandFilesL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt  /*aSynchronous*/)
	{
	TBool retVal = ETrue;

	if (aCommand == KCmdAtt)
		{
		DoCmdAtt(aSection);
		}
	else if (aCommand == KCmdCreatePrivatePath)
		{
		DoCmdCreatePrivatePath(aSection);
		}
	else if (aCommand == KCmdDelete)
		{
		DoCmdDelete(aSection);
		}
	else if (aCommand == KCmdEntry)
		{
		DoCmdEntryL(aSection);
		}
	else if (aCommand == KCmdGetDir)
		{
		DoCmdGetDir(aSection);
		}
	else if (aCommand == KCmdGetShortName)
		{
		DoCmdGetShortName(aSection);
		}
	else if (aCommand == KCmdGetLongName)
		{
		DoCmdGetLongName(aSection);
		}
	else if (aCommand == KCmdIsFileInRom)
		{
		DoCmdIsFileInRom(aSection);
		}
	else if (aCommand == KCmdIsFileOpen)
		{
		DoCmdIsFileOpen(aSection);
		}
	else if (aCommand == KCmdIsValidName)
		{
		DoCmdIsValidName(aSection);
		}
	else if (aCommand == KCmdMkDir)
		{
		DoCmdMkDir(aSection);
		}
	else if (aCommand == KCmdMkDirAll)
		{
		DoCmdMkDirAll(aSection);
		}
	else if (aCommand == KCmdModified)
		{
		DoCmdModified(aSection);
		}
	else if (aCommand == KCmdParse)
		{
		DoCmdParse(aSection);
		}
	else if (aCommand == KCmdPrivatePath)
		{
		DoCmdPrivatePath(aSection);
		}
	else if (aCommand == KCmdReadFileSection)
		{
		DoCmdReadFileSectionL(aSection);
		}
	else if (aCommand == KCmdRealName)
		{
		DoCmdRealName(aSection);
		}
	else if (aCommand == KCmdRename)
		{
		DoCmdRename(aSection);
		}
	else if (aCommand == KCmdReplace)
		{
		DoCmdReplace(aSection);
		}
	else if (aCommand ==  KCmdRmDir)
		{
		DoCmdRmDir(aSection);
		}
	else if (aCommand == KCmdSessionPath)
		{
		DoCmdSessionPath(aSection);
		}
	else if (aCommand == KCmdSetAtt)
		{
		DoCmdSetAtt(aSection);
		}
	else if (aCommand == KCmdSetEntry)
		{
		DoCmdSetEntry(aSection);
		}
	else if (aCommand == KCmdSetModified)
		{
		DoCmdSetModified(aSection);
		}
	else if (aCommand == KCmdSetSessionPath)
		{
		DoCmdSetSessionPath(aSection);
		}
	else if (aCommand == KCmdSetSessionToPrivate)
		{
		DoCmdSetSessionToPrivate(aSection);
		}
	else
		{
		retVal = EFalse;
		}

	return retVal;
	}

void CT_FsData::DoCmdRealName(const TDesC& aSection)
	{
	//	Gets name of file from ini file.
	TPtrC	name;
	if (GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		//	Gets the real name of a file.
		TFileName	realName;
		TInt		err = iFs->RealName(name, realName);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("RealName error %d"), err);
			SetError(err);
			}
		else
			{
			//	Prints real name.
			INFO_PRINTF2(_L("Real name: %S"), &realName);

			//	Compares realname from ini file and realname from function.
			TPtrC compareName;
			if (GET_OPTIONAL_STRING_PARAMETER(KCompareName(), aSection, compareName))
				{
				if (compareName.CompareC(realName, 0, NULL) != 0)
					{
					ERR_PRINTF3(_L("Error compare %S != %S"), &realName, &compareName);
					SetBlockResult(EFail);
					}
				else
					{
					INFO_PRINTF3(_L("%S == %S"), &realName, &compareName);
					}
				}
			}
		}
	}

void CT_FsData::DoCmdSessionPath(const TDesC& aSection)
	{
	//	Gets the session path.
	TBuf<KMaxTestExecuteCommandLength> path;
	TInt err = iFs->SessionPath(path);
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("SessionPath error %d"), err);
		SetError(err);
		}
	else
		{
		//	Prints session path.
		INFO_PRINTF2(_L("Session path is %S"), &path);

		//	Compares path from ini file and path from function.
		TPtrC comparePath;
		if (GET_OPTIONAL_STRING_PARAMETER(KComparePath(), aSection, comparePath))
			{
			if (comparePath.CompareC(path, 0, NULL) != 0)
				{
				ERR_PRINTF3(_L("Error compare %S != %S"), &path, &comparePath);
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF3(_L("%S == %S"), &path, &comparePath);
				}
			}
		}
	}

void CT_FsData::DoCmdSetSessionPath(const TDesC& aSection)
	{
	//	Gets path from ini file.
	TPtrC path;
	if (GET_MANDATORY_STRING_PARAMETER(KPath(), aSection, path))
		{
		//	Sets the session path for the current file server client.
		TInt err = iFs->SetSessionPath(path);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetSessionPath error %d"), err);
			SetError(err);
			}
		else
			{
			//	Prints session path.
			INFO_PRINTF2(_L("Path is set into %S"), &path);
			}
		}
	}

void CT_FsData::DoCmdParse(const TDesC& aSection)
	{
	//	Gets name of file from ini file.
	TPtrC name;
	if (GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		TInt err;
		TParse parse;
		TPtrC related;

		//	Reads the related file specification.
		if (GET_OPTIONAL_STRING_PARAMETER(KRelated(), aSection, related))
			{
			INFO_PRINTF2(_L("Related parameter: %S"), &related);

	    	//	Parses a filename specification, specifying related file path components.
			err = iFs->Parse(name, related, parse);
			}
		else
			{
			//	Parses a filename specification.
			err = iFs->Parse(name, parse);
			}

		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Parse error %d"), err);
			SetError(err);
			}
		else
			{
			//	Writes parsed components to log file.
			TPtrC parsedResult[] =
				{
				parse.Drive(),	//parsedDrive,
				parse.Path(),	//parsedPath,
				parse.Name(),	//parsedName,
				parse.Ext() 	//parsedExt,
				};// size 4

			TBuf<KMaxTestExecuteCommandLength> tempStore;
			tempStore.Format(_L("Parsed %S, Drive: %S, Path: %S, Name: %S, Ext: %S"),  	&name,
																						&parsedResult[0], //Drive
																						&parsedResult[1], //Path
																						&parsedResult[2], //Name
																						&parsedResult[3]);//Ext
			INFO_PRINTF1(tempStore);


			//	Compares parsed components from ini file.
			TPtrC iniParsed[] =
				{
				KParseDrive(),
				KParsePath(),
				KParseName(),
				KParseExt()
				};// size 4

			TInt size = sizeof(iniParsed) / sizeof(TPtrC);
			for (TInt i = 0; i < size; i++)
				{
				// Reads components from ini
				TPtrC compareName;
				if (GET_OPTIONAL_STRING_PARAMETER(iniParsed[i], aSection, compareName))
					{
					if (parsedResult[i].CompareC(compareName, 0, NULL) != 0)
						{
						ERR_PRINTF3(_L("Error compare %S != %S"), &parsedResult[i], &compareName);
						SetBlockResult(EFail);
						}
					else
						{
						INFO_PRINTF3(_L("%S == %S"), &parsedResult[i], &compareName);
						}
					}
				}
			}
		}
	}

void CT_FsData::DoCmdMkDir(const TDesC& aSection)
	{
	//	Gets name of directiry from ini file.
	TPtrC	dir;
	if (GET_MANDATORY_STRING_PARAMETER(KDir(), aSection, dir))
		{
		//	Makes a directory.
		TInt	err = iFs->MkDir(dir);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("MkDir error %d"), err);
			SetError(err);
			}
		}
	}

void CT_FsData::DoCmdMkDirAll(const TDesC& aSection)
	{
	//	Gets name of directiry from ini file.
	TPtrC	dir;
	if (GET_MANDATORY_STRING_PARAMETER(KDir(), aSection, dir))
		{
		//	Makes one or more directories.
		TInt	err = iFs->MkDirAll(dir);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("MkDirAll error %d"), err);
			SetError(err);
			}
		}
	}

void CT_FsData::DoCmdRmDir(const TDesC& aSection)
	{
	//	Gets name of directiry from ini file.
	TPtrC	dir;
	if (GET_MANDATORY_STRING_PARAMETER(KDir(), aSection, dir))
		{
		//	Removes a directory.
		TInt	err = iFs->RmDir(dir);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("RmDir error %d"), err);
			SetError(err);
			}
		}
	}

void CT_FsData::DoCmdGetDir(const TDesC& aSection)
	{
	TBool	dataOk = ETrue;

	//	Reads name of directory from ini file.
	TPtrC	name;
	if (!GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		dataOk = EFalse;
		}

	//	Reads sort key from ini file.
	TPtrC	entrySortKey;
	TUint	sortKey = ESortByName;
	if (GET_MANDATORY_STRING_PARAMETER(KEntrySortKey(), aSection, entrySortKey))
		{
		if ( !ConvertToSortKey(entrySortKey, sortKey) )
			{
			TInt	intTemp;
			if ( GET_MANDATORY_INT_PARAMETER(KEntrySortKey(), aSection, intTemp) )
				{
				sortKey=intTemp;
				}
			else
				{
				dataOk = EFalse;
				}
			}
		}
	else
		{
		dataOk = EFalse;
		}

	// 	Reads attribute mask from ini file
	// 	if not exist
	// 	Reads uidtype from ini file

	TInt		intUIDType = 0;
	TUidType	uidType = KNullUid;
	TBool		isUidType = EFalse;
	
	TUint		attMask = KEntryAttNormal;

	TBool		isDirList = FALSE;

	if (FileserverUtil::GetAttMask(*this, aSection, KAttMask(), attMask))
    	{
		//	Reads boolean of dirlist from ini file.
		GET_OPTIONAL_BOOL_PARAMETER(KIsDirList(), aSection, isDirList);
		}
	else if (GET_MANDATORY_INT_PARAMETER(KTUidType(), aSection, intUIDType))
		{
    	TUid id = TUid::Uid(intUIDType);
	   	uidType = TUidType(id);
	   	INFO_PRINTF2(_L("UID type set to %d"), uidType[0].iUid);
		}
	else
		{
		dataOk = EFalse;
		ERR_PRINTF2(_L("attmask or %S must be declared !!!"), &KTUidType);
		}
		
	// If all data was read
	if (dataOk)
		{
		//	Gets a filtered list of a directory's contents.
		TInt err = KErrNone;
		CT_DirData* dirWrapperEntry = NULL;
		CT_DirData* dirWrapperDirectory = NULL;
		CDir*		entryArray = NULL;
		CDir*		dirArray = NULL;
		TPtrC		dirEntryArray;
		TPtrC		dirDirectoryArray;
		
		if (GET_OPTIONAL_STRING_PARAMETER(KDirEntryArray(), aSection, dirEntryArray))
			{
			TRAP(err, dirWrapperEntry = static_cast<CT_DirData*>(GetDataWrapperL(dirEntryArray)));
			}
			
		if (GET_OPTIONAL_STRING_PARAMETER(KDirDirectoryArray(), aSection, dirDirectoryArray))
			{
			TRAP(err, dirWrapperDirectory = static_cast<CT_DirData*>(GetDataWrapperL(dirDirectoryArray)));
			}

		if (isUidType)
			{
			// Gets a filtered list of a directory's contents by UID type.
			err = iFs->GetDir(name, uidType, sortKey, entryArray);
			}
		else
			{
			if (isDirList)
				{
				// Gets a filtered list of the directory and file entries contained in a directory,
				// and a list of the directory entries only.
				err = iFs->GetDir(name, attMask, sortKey, entryArray, dirArray);
				}
			else
				{
				// Gets a filtered list of a directory's contents.
				err = iFs->GetDir(name, attMask, sortKey, entryArray);
				}
			}
			
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("GetDir error %d"), err);
			SetError(err);
			}
		else
			{	
			//	Writes sorted entries to log file.
			INFO_PRINTF2(_L("PATH %S"), &name);
			for (TInt i = 0; i < entryArray->Count(); i++)
				{
				TBuf<KMaxTestExecuteCommandLength> tempStore;
				tempStore.Format(_L("%d) %S"), i+1, &(*entryArray)[i].iName);
				INFO_PRINTF1(tempStore);
				}

			//	If "_comparename" of parameter is set in INI file, then
			//	try to find this name in the list of entries.
			TPtrC compareName;
			if (GET_OPTIONAL_STRING_PARAMETER(KCompareName(), aSection, compareName))
				{
				// Find compare name in list of entries.
				TBool isFind = FALSE;
				for (TInt i = 0; i < entryArray->Count(); i++)
					{
					if (compareName.CompareC((*entryArray)[i].iName, 0, NULL) == 0)
						{
						isFind = TRUE;
						}
					}

				//	Reads a flag which specifies a name should be existing or not be existing
				//	specified in parameter "KCompareName".
				TBool isExist = TRUE;
				GET_OPTIONAL_BOOL_PARAMETER(KIsExist(), aSection, isExist);

				TPtrC strIsExist = isExist ? _L("EXIST") : _L("NOT EXIST");
				INFO_PRINTF3(_L("%S must be %S"), &compareName, &strIsExist);

				TPtrC strIsFind = isFind ? _L("FOUND") : _L("NOT FOUND");
				INFO_PRINTF3(_L("%S is %S"), &compareName, &strIsFind);

				//	If name has been found when this name must not be existing
				//	or when name has been not found when name must be existing
				//	then test case fail.
				if (isExist != isFind)
					{
					ERR_PRINTF1(_L("Expected exist does not match actual"));
					SetBlockResult(EFail);
					}
				}
			}

		//	Delete list of data
		if (entryArray)
			{
			if(dirWrapperEntry)
				{
				dirWrapperEntry->SetObjectL(entryArray);
				}
			else
			    {
			    delete entryArray;
			    entryArray = NULL;			        
			    }
			}
		if (dirArray)
			{
			if(dirWrapperDirectory)
				{
				dirWrapperDirectory->SetObjectL(dirArray);
				}
			else
			    {
			    delete dirArray;
			    dirArray = NULL;   
			    }
			}
		}
	}

void CT_FsData::DoCmdDelete(const TDesC& aSection)
	{
	//	Reads name of file from ini file.
	TPtrC	name;
	if (GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		//	Deletes a single file.
		TInt	err = iFs->Delete(name);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Delete error %d"), err);
			SetError(err);
			}
		}
	}

void CT_FsData::DoCmdRename(const TDesC& aSection)
	{
	TBool	dataOk=ETrue;

	//	Reads the old name of file or directory from ini file.
	TPtrC	oldName;
	if ( !GET_MANDATORY_STRING_PARAMETER(KOldName(), aSection, oldName) )
		{
		dataOk=EFalse;
		}

	//	Reads the new name of file or directory from ini file.
	TPtrC	newName;
	if ( !GET_MANDATORY_STRING_PARAMETER(KNewName(), aSection, newName) )
		{
		dataOk=EFalse;
		}

	if ( dataOk )
		{
		//	Renames a single file or directory.
		TInt	err = iFs->Rename(oldName, newName);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Rename error %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF3(_L("File %S was renamed to %S successfully"), &oldName, &newName);
			}
		}
	}

void CT_FsData::DoCmdReplace(const TDesC& aSection)
	{
	TBool	dataOk=ETrue;

	//	Reads the old name of file from ini file.
	TPtrC	oldName;
	if ( !GET_MANDATORY_STRING_PARAMETER(KOldName(), aSection, oldName) )
		{
		dataOk=EFalse;
		}

	//	Reads the new name of file from ini file.
	TPtrC	newName;
	if ( !GET_MANDATORY_STRING_PARAMETER(KNewName(), aSection, newName) )
		{
		dataOk=EFalse;
		}

	if ( dataOk )
		{
		//	Replaces a single file with another.
		TInt err = iFs->Replace(oldName, newName);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Replace error %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF3(_L("File %S was replaced to %S successfully"), &oldName, &newName);
			}
		}
	}

void CT_FsData::DoCmdAtt(const TDesC& aSection)
	{
	//	Reads the name of file from ini file.
	TPtrC	name;
	if (GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		//	Gets a file's attributes.
		TUint	attValue;
		TInt	err = iFs->Att(name, attValue);
		INFO_PRINTF2(_L("RFs::Att = 0x%X"), attValue);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Att error %d"), err);
			SetError(err);
			}
		else
			{
			TUint	arrAttMask[]=
				{
				KEntryAttReadOnly,
				KEntryAttHidden,
				KEntryAttSystem,
				KEntryAttVolume,
				KEntryAttDir,
				KEntryAttArchive,
				KEntryAttXIP
				};//size 7

		    //	Writes what attributes has a file.
			INFO_PRINTF1(_L("--- START --- print all attributes"));

			TInt size = sizeof(arrAttMask) / sizeof(TUint);
			for (TInt i = 0; i < size; i++)
				{
				if (attValue & arrAttMask[i])
					{
					INFO_PRINTF2(_L("Attribute %S is set"), &ConvertToStrAttMask(arrAttMask[i]));
					}
				else
					{
					INFO_PRINTF2(_L("Attribute %S is not set"), &ConvertToStrAttMask(arrAttMask[i]));
					}
				}
			INFO_PRINTF1(_L("--- END --- print all attributes"));

			//	Reads atribute wich is testing.
			TPtrC testAttMaskStr;
			if(GET_OPTIONAL_STRING_PARAMETER(KEntrySetAttMask(), aSection, testAttMaskStr))
				{
				TUint	testAttMask = 0;
				if (FileserverUtil::GetAttMask(*this, aSection, KEntrySetAttMask(), testAttMask))
					{
				    //	Reads a flag which specifies a attribute should be set or not set
				    //	specified in parameter "KEntrySetAttMask".
				    TBool	testIsSet = TRUE;
					if (GET_OPTIONAL_BOOL_PARAMETER(KIsSetAtt(), aSection, testIsSet))
						{
						INFO_PRINTF2(_L("TEST attributes %S must be set"), &ConvertToStrAttMask(testAttMask));
						}
					else
						{
						INFO_PRINTF2(_L("TEST attributes %S must not be set"), &ConvertToStrAttMask(testAttMask));
						}
					if ( (attValue&testAttMask)==testAttMask )
						{
						if ( !testIsSet )
							{
							ERR_PRINTF2(_L("All bits not set %S"), &ConvertToStrAttMask(testAttMask));
							SetBlockResult(EFail);
							}
						}
					else if ( (attValue&testAttMask)==0 )
						{
						if ( testIsSet )
							{
							ERR_PRINTF2(_L("Some bits set %S"), &ConvertToStrAttMask(testAttMask));
							SetBlockResult(EFail);
							}
						}
					else
						{
						ERR_PRINTF2(_L("Some bits set %S"), &ConvertToStrAttMask(testAttMask));
						SetBlockResult(EFail);
						}
					}
				else
					{
					ERR_PRINTF1(_L("Unknown attribute!"));
			        SetBlockResult(EFail);
					}
				}
			}
		}
	}

void CT_FsData::DoCmdSetAtt(const TDesC&  aSection)
	{
	//	Reads attributes to be set from ini file.
	TUint	setAttMask = 0;
	
	if (!FileserverUtil::GetAttMask(*this, aSection, KEntrySetAttMask(), setAttMask))
		{
		ERR_PRINTF1(_L("Unknown attribute!"));
		SetBlockResult(EFail);	
		}

	//	Reads attributes to be clear from ini file.
	TUint	clearAttMask = 0;
	if (!FileserverUtil::GetAttMask(*this, aSection, KEntryClearAttMask(), clearAttMask))
		{
		ERR_PRINTF1(_L("Unknown attribute!"));
		SetBlockResult(EFail);
		}

	//	Reads the name of file or directory from ini file.
	TPtrC	name;
	if (GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		//	Sets or clears the attributes of a single file or directory.
		TInt err = iFs->SetAtt(name, setAttMask, clearAttMask);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetAtt error %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF1(_L("SetAtt() is successful"));
			}
		}
	}

void CT_FsData::DoCmdModified(const TDesC& aSection)
	{
	//	Reads the name of file or directory from ini file.
	TPtrC name;
	if (GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		//	Gets the last modification date and time of a file or a directory, in UTC.
		TTime time;
		TInt err = iFs->Modified(name, time);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Modified error %d"), err);
			SetError(err);
			}
		else
			{
			//	Write to log modified date and time.
			TBuf<KTimeFormatSize> dateString;
			TRAP (err, time.FormatL(dateString, KTimeFormat));
			if (err != KErrNone)
				{
				ERR_PRINTF2(_L("time.FormatL() error %d"), err);
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF3(_L("File %S modified: %S"), &name, &dateString);

				//	Reads time in ini file which must be returned from function "Modified()".
				TPtrC inputTime;
				if (GET_OPTIONAL_STRING_PARAMETER(KTime(), aSection, inputTime))
					{
					//	Convert input string in "KTime" from ini to TTime object.
					//	Assigns a date and time contained in a descriptor.
					TTime iniTime;
					TInt err = iniTime.Set(inputTime);
					if (err == KErrNone)
						{
						//	Compares time from ini file and time returned from function.
						if (iniTime == time)
							{
							INFO_PRINTF1(_L("Time equal"));
							}
						else
							{
							ERR_PRINTF3(_L("Time not equal, %S != %S"), &inputTime, &dateString);
							SetBlockResult(EFail);
							}
						}
					else
						{
						ERR_PRINTF2(_L("Fail set time, error %d"), err);
						SetBlockResult(EFail);
						}
					}
				}
			}
		}
	}

void CT_FsData::DoCmdSetModified(const TDesC& aSection)
	{
	//	Reads the time from ini file.
	TTime	time(0);
	TPtrC	inputTime;
	if (GET_OPTIONAL_STRING_PARAMETER(KTime(), aSection, inputTime))
		{
		//	Assigns a date and time contained in a descriptor.
		TInt err = time.Set(inputTime);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Time set failed, error %d"), err);
			SetBlockResult(EFail);
			}
		}

	//	Reads the name of file or directory from ini file.
	TPtrC	name;
	if (GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		//	Sets the date and time that the contents of a file or directory were modified, in UTC.
		TInt err = iFs->SetModified(name, time);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetModified error %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF1(_L("SetModified() OK"));
			}
		}
	}

void CT_FsData::DoCmdEntryL(const TDesC& aSection)
	{	
	TBool isEntryNew = EFalse;
	
	//	Reads the name of file or directory from ini file.
	TPtrC name;
	if (GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		TEntry *entry = NULL;
		
		//	Gets the entry details for a file or directory.
		TPtrC entryObjectName;
		if( GET_OPTIONAL_STRING_PARAMETER( KObjectEntry, aSection, entryObjectName ) )
			{
			INFO_PRINTF1( _L( "Get TEntry class instance." ) );
			TRAPD( err, entry = (TEntry*)GetDataObjectL(entryObjectName));
			if ( err != KErrNone )
				{
				ERR_PRINTF3( _L( "Unrecognized object name parameter value: %S. Error %d"), &entryObjectName, err );
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF1( _L( "TEntry class instance accepted OK." ) );
				}				
			}
		else
			{
			INFO_PRINTF1( _L( "Create new temporary TEntry() class instance." ) );
			TRAPD( err, entry = new (ELeave) TEntry() );
			if ( err!=KErrNone )
				{
				ERR_PRINTF2( _L( "new TEntry() error %d" ), err );
				SetBlockResult(EFail);
				}
			else
				{
				isEntryNew = ETrue;
				}
			}
		
		if ( entry != NULL )
			{				
			TInt err = iFs->Entry(name, *entry);
			if (err != KErrNone)
				{
				ERR_PRINTF2(_L("Entry error %d"), err);
				SetError(err);
				}
			else
				{
				// 	Prints all entry details for the file or directory.
				//	and compares these entry fith entry in ini file
				TPtrC arrNames[] =
					{
					KEntryAttDirStr(),
					KEntryAttArchiveStr(),
					KEntryAttHiddenStr(),
					KEntryAttReadOnlyStr(),
					KEntryAttSystemStr(),
					KEntryAttAllowUidStr()
					};//size 6

				TInt64 arrFunctions[] =
					{
					entry->IsDir() > 0,
					entry->IsArchive() > 0,
					entry->IsHidden() > 0,
					entry->IsReadOnly() > 0,
					entry->IsSystem() > 0,
					entry->IsTypeValid() > 0
					};//size 6

				TPtrC iniParam[] =
					{
					KIsAttDir(),
					KIsAttArchive(),
					KIsAttHidden(),
					KIsAttReadOnly(),
					KIsAttSystem(),
					KIsAttAllowUid(),
					};//size 6

				TInt size = sizeof(arrNames) / sizeof(TPtrC);
				for	(TInt i = 0; i < size; i++)
					{
					TBuf<KMaxTestExecuteCommandLength>	tempStore;
					if (arrFunctions[i])
						{
						tempStore.Format(_L("%S - attribute %S is SET"), &name, &arrNames[i]);
						}
					else
						{
						tempStore.Format(_L("%S - attribute %S is NOT SET"), &name, &arrNames[i]);
						}
					INFO_PRINTF1(tempStore);

					//	If iniParam[] is set in ini then compare  with attribute returned from function.
					TBool iniCompare = FALSE;
					if (GET_OPTIONAL_BOOL_PARAMETER(iniParam[i], aSection, iniCompare))
						{
						//	Compares entry in ini file and entry returned from function.
						if (arrFunctions[i] != iniCompare)
							{
							ERR_PRINTF2(_L("Error compare attribute %S"), &iniParam[i]);
							SetBlockResult(EFail);
							}
						}
					}

				//	Prints size of file.
				INFO_PRINTF3(_L("%S - size %d"), &name, entry->iSize);

				//	Compare size of file returned from function with size from ini.
				TInt iniSize = 0;
				if (GET_OPTIONAL_INT_PARAMETER(KSize(), aSection, iniSize))
					{
					if (entry->iSize != iniSize)
						{
						ERR_PRINTF3(_L("Size from ini file not equal with size returned from function (%d != %d)"), iniSize, entry->iSize);
						SetBlockResult(EFail);
						}
					}

				//	Prints the system time of last modification.
				TBuf<KTimeFormatSize>	dateString;
				entry->iModified.FormatL(dateString, KTimeFormat);
				INFO_PRINTF3(_L("%S - modified %S"), &name, &dateString);

				//	Compare time from ini with time returned from function.
				TPtrC	iniTimeStr;
				if (GET_OPTIONAL_STRING_PARAMETER(KTime(), aSection, iniTimeStr))
					{
					//	Assigns a date and time contained in a descriptor.
					TTime	iniTime;
					
					TInt error = iniTime.Set(iniTimeStr);
					if(error == KErrNone)
						{
						//	Compares size in ini file and size returned from function.
						if (entry->iModified != iniTime)
							{
							ERR_PRINTF1(_L("Time from ini file not equal with time returned from function."));
							SetBlockResult(EFail);
							}
						}
					else
						{
						ERR_PRINTF3(_L("Cannot convert %S to TTime type. Error: %d"), &iniTimeStr, error);
						SetBlockResult(EFail);
						}
					}

				// Prints the file's UIDtype.
				INFO_PRINTF3(_L("%S - UIDtype %d"), &name, entry->MostDerivedUid().iUid);
				
				if (isEntryNew)
					{
					INFO_PRINTF1(_L("Delete temporary TEntry() class instance."));
					delete entry;
					entry = NULL;
					}
				}
			}
		}
	}

void CT_FsData::DoCmdSetEntry(const TDesC& aSection)
	{
	TBool	dataOk=ETrue;

	TUint	setAttMask = 0;
	if (!FileserverUtil::GetAttMask(*this, aSection, KEntrySetAttMask(), setAttMask))
		{
		dataOk = EFalse;
		ERR_PRINTF1(_L("Unknown attribute!"));
		SetBlockResult(EFail);
		}
		
	//	Reads attributes to be clear from ini file.
	TUint	clearAttMask = 0;
	if (!FileserverUtil::GetAttMask(*this, aSection, KEntryClearAttMask(), clearAttMask))
		{
		dataOk = EFalse;
		ERR_PRINTF1(_L("Unknown attribute!"));
		SetBlockResult(EFail);
		}
	//	Reads the time from ini file.
	TTime	time(0);
	TPtrC	inputTime;
	if (GET_OPTIONAL_STRING_PARAMETER(KTime(), aSection, inputTime))
		{
		//	Assigns a date and time contained in a descriptor.
		TInt err = time.Set(inputTime);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("Time set failed, error %d"), err);
			SetBlockResult(EFail);
			}
		}

	//	Reads the name of file or directory from ini file.
	TPtrC	name;
	if (!GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		dataOk = EFalse;
		}

	if ( dataOk )
		{
		//	Sets both the attributes and the last modified date and time for a file or directory.
		TInt	err = iFs->SetEntry(name, time, setAttMask, clearAttMask);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetEntry error %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF1(_L("SetEntry() is successful."));
			}
		}
	}

void CT_FsData::DoCmdReadFileSectionL(const TDesC& aSection)
	{
	TBuf<KMaxTestExecuteCommandLength>	tempStore;
	TBool dataOk = ETrue;


	//	Reads the offset, in bytes, from the start of the file where reading is to start.
	TInt offset = 0;
	if (!GET_MANDATORY_INT_PARAMETER(KOffset(), aSection, offset))
		{
		dataOk = EFalse;
		}

	//	Reads the number of bytes to be read from the file.
	TInt lengthRead = 0;
	if (!GET_MANDATORY_INT_PARAMETER(KLengthRead(), aSection, lengthRead))
		{
		dataOk = EFalse;
		}

	//	Reads the number of bytes to be alocated.
	TInt lengthBuffer = 0;
	if (!GET_MANDATORY_INT_PARAMETER(KLengthBuffer(), aSection, lengthBuffer))
		{
		dataOk = EFalse;
		}

	//	Reads the name of file from ini file.
	TPtrC name;
	if (!GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		dataOk = EFalse;
		}

	if ( dataOk )
		{
		//	Creates 8-bit heap descriptor.
		HBufC8*	buffer = NULL;
		TRAPD (err, buffer = HBufC8::NewL(lengthBuffer));
		if (err == KErrNone)
			{
			//	Pushes descriptor onto the cleanup stack.
			CleanupStack::PushL(buffer);

			//	Create memory for 16-bit bufer.
			HBufC*	buffer16 = NULL;
			TRAP (err, buffer16 = HBufC::NewL(lengthBuffer));
			if (err != KErrNone)
				{
				ERR_PRINTF2(_L("The 16-bit heap descriptor cannot be created, err %d"), err);
				SetBlockResult(EFail);
				CleanupStack::PopAndDestroy(buffer);
				}
			else
				{
				//	Pushes descriptor onto the cleanup stack.
				CleanupStack::PushL(buffer16);

				//	Reads data from a file without opening it.
				TPtr8 bufferPtr = buffer->Des();
				err = iFs->ReadFileSection(name, offset, bufferPtr, lengthRead);
				if (err != KErrNone)
					{
					ERR_PRINTF2(_L("ReadFileSection error %d"), err);
					SetError(err);
					}
				else
					{
					//	Writes data from a file.
					tempStore.Format(_L("The data from a file %S, offset =%d, length_buffer =%d, length_read =%d"), &name, offset, lengthBuffer, lengthRead);
					INFO_PRINTF1(tempStore);
					// Converts 8-bit to 16-bit.
					buffer16->Des().Copy(bufferPtr);
					TPtr16 bufferPtr16 =  buffer16->Des();
					INFO_PRINTF2(_L("%S"), &bufferPtr16);
					}

				//	Clean memory buffer and buffer16
				CleanupStack::PopAndDestroy(2, buffer); // buffer, buffer16.
				}
			}
		else
			{
			ERR_PRINTF2(_L("The 8-bit heap descriptor cannot be created, err %d"), err);
			SetBlockResult(EFail);
			}
		}
	}

void CT_FsData::DoCmdIsFileOpen(const TDesC& aSection)
	{
	//	Reads the name of file from ini file.
	TPtrC file;
	if (GET_MANDATORY_STRING_PARAMETER(KFile(), aSection, file))
		{
		//	Tests whether a file is open.
		TBool isOpen;
		TInt err = iFs->IsFileOpen(file, isOpen);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("IsFileOpen error %d"), err);
			SetError(err);
			}
		else
			{
			if (isOpen)
				{
				INFO_PRINTF2(_L("The File %S is OPEN"), &file);
				}
			else
				{
				INFO_PRINTF2(_L("The FILE %S is CLOSED"), &file);
				}

			//	Compare parameter from ini
			TBool iniIsOpen;
			if (GET_OPTIONAL_BOOL_PARAMETER(KIsOpen(), aSection, iniIsOpen))
				{
				if (isOpen != iniIsOpen)
					{
					ERR_PRINTF2(_L("Error compare parameter %S"), &KIsOpen);
					SetBlockResult(EFail);
					}
				}
			}
		}
	}

void CT_FsData::DoCmdGetShortName(const TDesC& aSection)
	{
	//	Reads the long name of file from ini file.
	TPtrC longName;
	if (GET_MANDATORY_STRING_PARAMETER(KLongName(), aSection, longName))
		{
		//	Gets the short filename associated with a VFAT long filename.
		TBuf<KShortNameSize> shortName;
		TInt err = iFs->GetShortName(longName, shortName);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("GetShortName error %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF2(_L("Short name: %S"), &shortName);

			//	Compares name from ini file and name from function.
			TPtrC compareName;
			if (GET_OPTIONAL_STRING_PARAMETER(KShortName(), aSection, compareName))
				{
				if (shortName.CompareC(compareName, 0, NULL) != 0)
					{
					ERR_PRINTF3(_L("%S != %S"), &shortName, &compareName);
					SetBlockResult(EFail);
					}
				else
					{
					INFO_PRINTF3(_L("%S == %S"), &shortName, &compareName);
					}
				}
			}
		}
	}

void CT_FsData::DoCmdGetLongName(const TDesC& aSection)
	{
	//	Reads the short name of file from ini file.
	TPtrC shortName;
	if (GET_MANDATORY_STRING_PARAMETER(KShortName(), aSection, shortName))
		{
		//	Gets the long filename associated with a short (8.3) filename.
		TBuf<KLongNameSize> longName;
		TInt err = iFs->GetLongName(shortName, longName);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("GetLongName error %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF2(_L("Long name: %S"), &longName);

			//	Compares name from ini file and name from function.
			TPtrC compareName;
			if (GET_OPTIONAL_STRING_PARAMETER(KLongName(), aSection, compareName))
				{
				if (longName.CompareC(compareName, 0, NULL) != 0)
					{
					ERR_PRINTF3(_L("%S != %S"), &longName, &compareName);
					SetBlockResult(EFail);
					}
				else
					{
					INFO_PRINTF3(_L("%S == %S"), &longName, &compareName);
					}
				}
			}
		}
	}

void CT_FsData::DoCmdIsFileInRom(const TDesC& aSection)
	{
	//	Reads the name of file from ini file.
	TPtrC file;
	if (GET_MANDATORY_STRING_PARAMETER(KFile(), aSection, file))
		{
		//	Gets a pointer to the specified file, if it is in ROM.
		iIsFileInRom=iFs->IsFileInRom(file);
		INFO_PRINTF2(_L("Fs->IsFileInRom = 0x%X"), iIsFileInRom);

		TBool	isInROM = (iIsFileInRom!=NULL);
		if ( isInROM )
			{
			INFO_PRINTF2(_L("File %S in ROM"), &file);
			}
		else
			{
			INFO_PRINTF2(_L("File %S not in ROM"), &file);
			}

		//	Compare parameter from ini
		TBool iniIsInROM = FALSE;
		if (GET_OPTIONAL_BOOL_PARAMETER(KIsInRom(), aSection, iniIsInROM))
			{
			if (isInROM != iniIsInROM)
				{
				ERR_PRINTF1(_L("Expected result does not match actual"));
				SetBlockResult(EFail);
				}
			}
		}
	}

void CT_FsData::DoCmdIsValidName(const TDesC& aSection)
	{
	//	Reads the name of file from ini file.
	TPtrC	name;
	if (GET_MANDATORY_STRING_PARAMETER(KName(), aSection, name))
		{
		TBool	isValid = FALSE;
		TBool	isBadChar;
		if (GET_OPTIONAL_BOOL_PARAMETER(KIsBadChar(), aSection, isBadChar))
			{
			//	Tests whether a filename and path are syntactically correct.
			//	badChar = on return, contains any illegal character within name,
			//	if the path is valid, badChar is blank.

			TText	badChar;
			isValid = iFs->IsValidName(name, badChar);
			INFO_PRINTF2(_L("BadChar: %c"), badChar);

			TPtrC	iniBadChar;
			if (GET_OPTIONAL_STRING_PARAMETER(KBadChar(), aSection, iniBadChar))
				{
				TText	expectedBadChar=iniBadChar[0];
				if (badChar != expectedBadChar)
					{
					ERR_PRINTF3(_L("Error compare badchar %c != %c"), badChar, expectedBadChar);
					SetBlockResult(EFail);
					}
				else
					{
					INFO_PRINTF3(_L("Badchar's has equal value %c == %c"), badChar, expectedBadChar);
					}
				}
			}
		else
			{
			//	Tests whether a filename and path are syntactically correct.
			isValid = iFs->IsValidName(name);
			}

		if (isValid)
			{
			INFO_PRINTF2(_L("Name %S is valid"), &name);
			}
		else
			{
			INFO_PRINTF2(_L("Name %S is not valid"), &name);
			}

		//	Compare parameter from ini
		TBool	iniIsValid;
		if (GET_OPTIONAL_BOOL_PARAMETER(KIsValid(), aSection, iniIsValid))
			{
			if (isValid != iniIsValid)
				{
				ERR_PRINTF1(_L("Error compare with ini parameter"));
				SetBlockResult(EFail);
				}
			}
		}
	}

void CT_FsData::DoCmdSetSessionToPrivate(const TDesC& aSection)
	{
	//	Reads the name of drive from ini file.
	TDriveNumber driveNumber;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("Error read parameter %S"), &KDrive());
		SetBlockResult(EFail);
		}
	else
		{
		//	Sets the session path to point to the private path on the specified drive.
		TInt err = iFs->SetSessionToPrivate(driveNumber);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("SetSessionToPrivate error %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF1(_L("Function DoCmdSetSessionToPrivate() OK"));
			}
		}
	}

void CT_FsData::DoCmdPrivatePath(const TDesC& aSection)
	{
	//	Creates the text defining the private path for a process.
	TBuf<KMaxTestExecuteCommandLength> privatePath;
	TInt err = iFs->PrivatePath(privatePath);
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("PrivatePath error %d"), err);
		SetError(err);
		}
	else
		{
		INFO_PRINTF2(_L("PrivatePath: %S"), &privatePath);

		//	Compares path from ini file and path from function.
		TPtrC comparePath;
		if (GET_OPTIONAL_STRING_PARAMETER(KComparePath(), aSection, comparePath))
			{
			if (comparePath.CompareC(privatePath, 0, NULL) != 0)
				{
				ERR_PRINTF3(_L("%S != %S"), &privatePath, &comparePath);
				SetBlockResult(EFail);
				}
			else
				{
				INFO_PRINTF3(_L("%S == %S"), &privatePath, &comparePath);
				}
			}
		}
	}

void CT_FsData::DoCmdCreatePrivatePath(const TDesC& aSection)
	{
	//	Reads the name of drive from ini file.
	TDriveNumber driveNumber;
	if (!GetDriveNumberFromConfig(aSection, KDrive(), driveNumber))
		{
		ERR_PRINTF2(_L("Error read parameter %S"), &KDrive());
		SetBlockResult(EFail);
		}
	else
		{
		//	Creates the private path for a process on the specified drive.
		TInt err = iFs->CreatePrivatePath(driveNumber);
		if (err != KErrNone)
			{
			ERR_PRINTF2(_L("PrivatePath error %d"), err);
			SetError(err);
			}
		else
			{
			INFO_PRINTF1(_L("Function DoCmdCreatePrivatePath() OK"));
			}
		}
	}

TBool CT_FsData::ConvertToSortKey(const TDesC& aSortKeyStr, TUint& aSortKey)
	{
	TBool ret = ETrue;

	if (aSortKeyStr == KESortNone)
		{
		aSortKey = ESortNone;
		}
	else if (aSortKeyStr == KESortByName)
		{
		aSortKey = ESortByName;
		}
	else if (aSortKeyStr == KESortByExt)
		{
		aSortKey = ESortByExt;
		}
	else if (aSortKeyStr == KESortBySize)
		{
		aSortKey = ESortBySize;
		}
	else if (aSortKeyStr == KESortByDate)
		{
		aSortKey = ESortByDate;
		}
	else if (aSortKeyStr == KESortByUid)
		{
		aSortKey = ESortByUid;
		}
	else if (aSortKeyStr == KEDirsAnyOrder)
		{
		aSortKey = EDirsAnyOrder;
		}
	else if (aSortKeyStr == KEDirsFirst)
		{
		aSortKey = EDirsFirst;
		}
	else if (aSortKeyStr == KEDirsLast)
		{
		aSortKey = EDirsLast;
		}
	else if (aSortKeyStr == KEAscending)
		{
		aSortKey = EAscending;
		}
	else if (aSortKeyStr == KEDescending)
		{
		aSortKey = EDescending;
		}
	else if (aSortKeyStr == KEDirDescending)
		{
		aSortKey = EDirDescending;
		}
	else if (aSortKeyStr.Match((_L("*|*"))) != KErrNotFound)
		{
		TUint tmpSortKey;

		TInt location = aSortKeyStr.Match(_L("*|*"));
		//Converting Left part of the data
		TPtrC left = aSortKeyStr.Left(location);
		if (ConvertToSortKey(left, tmpSortKey))
			{
			aSortKey = tmpSortKey;
			}
		else
			{
			ret = EFalse;
			}

		//Converting right data can be with another "|"
		TPtrC right = aSortKeyStr.Mid(location + 1);

		if (ConvertToSortKey(right, tmpSortKey))
			{
			aSortKey = aSortKey | tmpSortKey;
			}
		else
			{
			ret = EFalse;
			}
		}
	else
		{
		ret = EFalse;
		}

	return ret;
	}

const TDesC& CT_FsData::ConvertToStrAttMask(TUint aAttMask)
	{
	switch(aAttMask)
		{
	case KEntryAttReadOnly:
		return KEntryAttReadOnlyStr;
	case KEntryAttHidden:
		return KEntryAttHiddenStr;
	case KEntryAttSystem:
		return KEntryAttSystemStr;
	case KEntryAttVolume:
		return KEntryAttVolumeStr;
	case KEntryAttDir:
		return KEntryAttDirStr;
	case KEntryAttArchive:
		return KEntryAttArchiveStr;
	case KEntryAttXIP:
		return KEntryAttXIPStr;
	default:
		break;
		};
	return KEntryAttUnknown;
	}
