// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\cfileman\t_cfileman_aux.cpp
// 
//

#include "t_cfileman_aux.h"

CFileMan* gFileMan = NULL;
RPointerArray<RFile>* gFileHandles = NULL;
TBool gAsynch = EFalse;
TRequestStatus gStat;
TBool testingInvalidPathLengths;

void InitialiseL()
	{
	gFileMan=CFileMan::NewL(TheFs);
	}

void RmDir(const TDesC& aDirName)
	{
	TFileName filename_dir = aDirName;
	TInt r = 0;
	r = TheFs.SetAtt(filename_dir, 0, KEntryAttReadOnly);
	r=gFileMan->RmDir(filename_dir);
	test(r==KErrNone || r==KErrNotFound || r==KErrPathNotFound);
	}

// Cleanup test variables
void Cleanup()
	{
	delete gFileMan;
	}

//Test that the contents of two directories are identical
TBool CompareL(const TDesC& aDir1,const TDesC& aDir2)
	{
	TBool rel = ETrue;
	CDirScan* scanDir1=CDirScan::NewL(TheFs);
	scanDir1->SetScanDataL(aDir1,KEntryAttMaskSupported,ESortByName);
	CDirScan* scanDir2=CDirScan::NewL(TheFs);
	scanDir2->SetScanDataL(aDir2,KEntryAttMaskSupported,ESortByName);

	FOREVER
		{
		CDir* entryList1;
		CDir* entryList2;

		scanDir1->NextL(entryList1);
		scanDir2->NextL(entryList2);

		if (entryList1==NULL || entryList2==NULL)
			{
			if (!(entryList1==NULL && entryList2==NULL))
				{
				delete entryList1, 
				delete entryList2, 
				delete scanDir1, 
				delete scanDir2;
				return(EFalse);
				}
			break;
			}

		TFileName abbPath1=scanDir1->AbbreviatedPath();
		TFileName abbPath2=scanDir2->AbbreviatedPath();
		if (!(abbPath1==abbPath2))
			{
			delete entryList1, 
			delete entryList2, 
			delete scanDir1, 
			delete scanDir2;
			return(EFalse);
			}

		TInt count1=entryList1->Count();
		TInt count2=entryList2->Count();
		if (!(count1==count2))
			{
			delete entryList1, 
			delete entryList2, 
			delete scanDir1, 
			delete scanDir2;
			return(EFalse);
			}

		while(count1--)
			{
			TEntry entry1=(*entryList1)[count1];
			TEntry entry2=(*entryList2)[count1];
			if (!(entry1.iName==entry2.iName))
				{
				delete entryList1, 
				delete entryList2, 
				delete scanDir1, 
				delete scanDir2;
				return(EFalse);
				}
			if (!(entry1.iAtt==entry2.iAtt))
				{
				delete entryList1, 
				delete entryList2, 
				delete scanDir1, 
				delete scanDir2;
				return(EFalse);
				}

			}

		delete entryList1;
		delete entryList2;
		}

	delete scanDir1;
	delete scanDir2;
	
	return rel;
	}

/**
    Parsing Dir Data Block
    @param  aDataBlock		data block in TInt[] for parsing	
    @param  aDirDataArray	returning dir data array after parsing

    @panic 					if data setup error

*/
void ParsingDirDataBlock(const TInt aDataBlock[], RArray<TInt>& aDirDataArray)
	{
	TInt err = KErrNone;
	aDirDataArray.Reset();

	if (aDataBlock[0] == EOB)
		{
		return;
		}

	TInt i = 1;
	FOREVER
		{
		TInt lastItem = aDataBlock[i-1];
		TInt currentItem = aDataBlock[i];
		
		// check currentItem
		if (currentItem == EOB)
			{
			if (lastItem == CON || lastItem > LAST)
			//check last
				{
				test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
				test(EFalse);
				}
			else
			// passed, insert last, break
				{
				err = aDirDataArray.InsertInOrder(lastItem);
				if (err != KErrNone && err != KErrAlreadyExists)
					{
					test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
					test(EFalse);
					}
				break;
				}
			}
		
		else if (currentItem == CON)
		// if current == CON
			{
			if (lastItem == CON || lastItem >= LAST)
			// check last item
				{
				test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
				test(EFalse);
				}
			else // last < LAST, last != CON
				{
				// check next item
				TInt nextItem = aDataBlock[i+1];
				if (nextItem <= 0 || nextItem > LAST || lastItem >= nextItem)
					{
					test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
					test(EFalse);
					}
				else
					{
					// all valid
					for (TInt j = lastItem; j < nextItem; j++)
						{
						err = aDirDataArray.InsertInOrder(j);
						if (err != KErrNone && err != KErrAlreadyExists)
							{
							test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
							test(EFalse);
							}
						}
					}
				}
			i++;
			}

		else if (0 <= currentItem && currentItem <= LAST)
		// if current == normal item
			{
			if (lastItem == CON)
				{
				i++;
				continue;
				}
			else if (lastItem >= LAST)
			// check last item
				{
				test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
				test(EFalse);
				}
			else
			// passed, insert last
				{
				err = aDirDataArray.InsertInOrder(lastItem);
				if (err != KErrNone && err != KErrAlreadyExists)
					{
					test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
					test(EFalse);
					}
				}
			i++;
			}
		
		else	// invalid input
			{
			test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
			test(EFalse);
			}
		}
	}

	
/**
	Setup attribs:
	@param  filenamedir   		root path of dir data or a file data
	@param  mode			    value of an mode to be set(Normal/Open/Shared..)
*/
void OpenFile(TDesC& aFile, TFileMode aFileMode)
	{
	RFile* file = new (ELeave) RFile;
	gFileHandles->Append(file);
	TInt ret = 0;
	ret = file->Open(TheFs, aFile, aFileMode);
	test(ret==KErrNone);
	}
	

/**
    Setup dir structure for testing and verifying functional results
    @param	datastr			data structure to setup directory
    @param  iOperation   	Operation to be performed 
    @param  SrcDrive		Source drive
    @param	Targetdrive		Target drive input
    @panic					if data structure definition is incorrect
*/
void SetupDirFiles(const TDesC& aPath, const TDirSetupFiles& aDirFiles)
	{
	TFileName path = aPath;
	if (path.Length() == 0)
		{
		test.Printf(_L("ERROR<SetupDirFiles()>: Zero length src path!\n"));
		test(EFalse);
		}
	
	RmDir(path);
	MakeDir(path);

	RArray<TInt> addBlockDataArray;
	RArray<TInt> deductBlockDataArray;
	
	ParsingDirDataBlock(aDirFiles.iAddingBlock, addBlockDataArray);
	ParsingDirDataBlock(aDirFiles.iDeductBlock, deductBlockDataArray);
	
	if (addBlockDataArray.Count() == 0)
	// empty dir setup
		{
		return;
		}
	else
		{
		for (TInt i = 0; i < deductBlockDataArray.Count(); ++i)
			{
			TInt idxToDelete = addBlockDataArray.FindInOrder(deductBlockDataArray[i]);
			if (idxToDelete >= 0)
				{
				addBlockDataArray.Remove(idxToDelete);
				}
			else if (idxToDelete == KErrNotFound)
				{
				continue;
				}
			else
				{
				test.Printf(_L("ERROR<<SetupDir>>: wrong dir data setup! err=%d\n"), idxToDelete);
				test(EFalse);
				}
			}
		}

	if (addBlockDataArray.Count() > 0)
		{
		for (TInt i = 0; i < addBlockDataArray.Count(); ++i)
			{
			TInt idx = addBlockDataArray[i];
			path = aPath;
			path += gDirPatterns[idx];
			if (path[path.Length() - 1] == '\\')
				{
				MakeDir(path);
				}
			else
				{
				MakeFile(path, _L8("blahblah"));

				}
			}
		}

	addBlockDataArray.Reset();
	deductBlockDataArray.Reset();
	}

/**
    Print out all items in aPath
    @param  aPath 		root path for scanning

    @panic				SetScanData error
*/
void PrintDir(const TDesC& aPath, const TChar& aDrv)
	{
	TFileName fn;
	if (aPath.Length() == 0)
		{
		return;
		}
	else
		{
		fn = aPath;
		fn[0] = (TUint16)aDrv;
		test.Printf(_L("==============================\n"));
		test.Printf(_L("<<PrintDir>>: root = \"%S\"\n"), &fn);
		}

	CDirScan* scan = CDirScan::NewL(TheFs);
	TUint entryAttMask = KEntryAttMaskSupported;
	TUint entrySortMask = EDirsLast|EAscending;
	CDirScan::TScanDirection scanDirection = CDirScan::EScanDownTree;


	TRAPD(err, scan->SetScanDataL(fn, entryAttMask, entrySortMask, scanDirection));
	test(err == KErrNone);

	CDir* dir = NULL;
	TInt i = 0;
	TBool temp_val = ETrue;
	while (temp_val)
		{
		scan->NextL(dir);
		if (dir != NULL)
			{
			for (TInt j = 0; j < dir->Count(); ++j)
				{
				TFileName item(fn.Left(aPath.Length() - 1));
				item.Append(scan->AbbreviatedPath());
				TEntry dummy = (*dir)[j];
				item.Append(dummy.iName);
				if (dummy.iAtt & KEntryAttDir)
					{
					item.Append('\\');
					}
				test.Printf(_L("<<PrintDir>>: item %d: \"%S\"\n"), i, &item);
				++i;
				}
			delete dir;
			dir = NULL;
			}
		else
			{
			break;
			}
		}
	delete scan;
	}
