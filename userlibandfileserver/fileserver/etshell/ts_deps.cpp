// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\etshell\ts_deps.cpp
// 
//

#include "ts_std.h"
#include "f32image.h"
#include <e32uid.h>
#include "sf_deflate.h"
#include "sf_image.h"

//#define _DETAILED_CHECK	//	Select for detailed dependency check

#if defined(_DETAILED_CHECK)	//	Prints information to screen
#define __PRINT(t) {CShell::TheConsole->Printf(t);}
#define __PRINT1(t,a) {CShell::TheConsole->Printf(t,a);}
#define __PRINT2(t,a,b) {CShell::TheConsole->Printf(t,a,b);}
#define __PRINTWAIT(t){CShell::TheConsole->Printf(t);CShell::TheConsole->Getch();}
#else
#define __PRINT(t)
#define __PRINT1(t,a)
#define __PRINT2(t,a,b)
#define __PRINTWAIT(t)
#endif

/*
  
CDllChecker::GetImportDataL(aFilename) reads the Image Header, Import Section 
and all import data for aFilename.  If aFilename is a ROM dll, and thus has no 
import data, or if the file contains no import data for some other reason the 
function leaves with KErrGeneral. If a file is compressed, function calls 
appropriate decompression routine to inflate the import data.
The function then calls GetDllTableL function which 
reads the first import block and enters a "for" loop. The Dll's name and Uid3 are obtained 
from CDllChecker::GetFileNameAndUid().  If the Dll name does not occur in the 
array of previously checked Dlls, CDllChecker::FindDll() is called.  If the Dll 
is found,the function then calls CDllChecker::GetImportDataL on the filename acquired by 
GetFileNameAndUid()(recursive call). 

If the Dll contains no import data or cannot be found, or if the Uid is invalid,
the next import is checked.

The Uid3 value is checked by calling CDllChecker::CheckUid.  This compares the 
Uid3 value found in the image header of the file, with that found by 
GetFileNameAndUid().  If there are any discrepancies,these are noted.

Each potential import is added to the array by dllAppendL(TResultCheck) to 
indicate its import status.

CDllChecker::ListArray() lists the contents of the array when all imports
have been checked.
*/


CDllChecker::CDllChecker()
//
//	Constructor
//
	{
	iDllArray=NULL;
	iCalls=0;
	}



void CDllChecker::ConstructL()
//
//	Creates an array to hold DLLs referenced by this executable	
//
	{
	iDllArray = new(ELeave)CArrayFixFlat<SDllInfo>(4);			
	__PRINT(_L(" Successfully created iDllArray\n"));	
	}


CDllChecker::~CDllChecker()
//
//	Destructor
//
	{
    delete iDllArray;
	}


GLDEF_C void Get16BitDllName(TDes8& aDllName,TDes& aFileName)
//
//	Convert an 8 bit name to a 16 bit name - zero padded automatically
//	No effect in 8 bit builds - just sets aFileName to aDllName
//
	{
	aFileName.SetLength(aDllName.Length());
	aFileName.Copy(aDllName);
	}

void FileCleanup(TAny* aPtr)
	{
	TFileInput* f=(TFileInput*)aPtr;
	f->Cancel();
	delete f;
	}

void CDllChecker::LoadFileInflateL(E32ImageHeaderComp* aHeader,TUint8* aRestOfFileData,TUint32 aRestOfFileSize)
	{
	TInt pos = aHeader->TotalSize();
	User::LeaveIfError(iFile.Seek(ESeekStart,pos)); // Start at beginning of compressed data

	TFileInput* file = new (ELeave) TFileInput(iFile);
	CleanupStack::PushL(TCleanupItem(&FileCleanup,file));
	CInflater* inflater=CInflater::NewLC(*file);
	
	if (aHeader->iCodeSize)
		{
		TUint8* CodeData = (TUint8*)User::AllocLC(aHeader->iCodeSize );
		TInt count=inflater->ReadL((TUint8*)CodeData ,aHeader->iCodeSize,&Mem::Move);
		if(count!=aHeader->iCodeSize)
			User::Leave(KErrCorrupt);
		CleanupStack::PopAndDestroy(CodeData);
		}
	
	if (aRestOfFileSize)
		{
		TUint32 count=inflater->ReadL(aRestOfFileData,aRestOfFileSize,&Mem::Move);
		if(count!=aRestOfFileSize)
			User::Leave(KErrCorrupt);
		}
	CleanupStack::PopAndDestroy(2,file);
	}

void CDllChecker::LoadFileNoCompressL(E32ImageHeaderComp* aHeader,TUint8* aRestOfFileData,TUint32 aRestOfFileSize)
	{
	TInt pos = (aHeader->TotalSize()+aHeader->iCodeSize);
	if (aRestOfFileSize)
		{
		User::LeaveIfError(iFile.Seek(ESeekStart,pos)); // Start at beginning of import table
		TPtr8 ptrToData((TText8*)(aRestOfFileData),aRestOfFileSize,aRestOfFileSize);
		User::LeaveIfError(iFile.Read(ptrToData, (TInt)aRestOfFileSize));	
		}
	}	
//function loads file's import information calling decompression routine if needed
TInt CDllChecker::LoadFile(TUint32 aCompression,E32ImageHeaderComp* aHeader,TUint8* aRestOfFileData,TUint32 aRestOfFileSize)
	{
	TInt r=KErrNone;
	if(aCompression==KFormatNotCompressed)
		{
		TRAP(r,LoadFileNoCompressL(aHeader,aRestOfFileData,aRestOfFileSize));		
		}
	else if(aCompression==KUidCompressionDeflate)
		{
		TRAP(r,LoadFileInflateL(aHeader,aRestOfFileData,aRestOfFileSize));
		}
	else
		r=KErrNotSupported;

	return r;
	}
								
//function iterates through the list of libraries the current executable depends on
//for each dependency in the list the function checks whether the .dll being checked is already added to the array of dependencies
//if not, the function adds dependency being checked to the array of dependencies and calls GetImportDataL function recursively
void CDllChecker::GetDllTableL(TUint8* aImportData, TInt aDllRefTableCount,TUint aFlags)
	{
	E32ImportBlock* block = (E32ImportBlock*)((TUint8*)aImportData+sizeof(E32ImportSection));

	//*********Beginning of the loop*********
	for (TInt i=0; i<aDllRefTableCount; i++)	
		{

		SDllInfo dllInfo;		
		TText8* dllName=(aImportData+block->iOffsetOfDllName);
		TPtrC8 dllNamePtr(dllName, User::StringLength(dllName));
		GetFileNameAndUid(dllInfo,dllNamePtr);//Gets name and Uid of dependency
		TUid* pointer=&dllInfo.iUid;		
		__PRINT(_L(" Check Dll has not already been imported\n"));		
		TKeyArrayFix key(0,ECmpFolded);	
		TInt pos;		
		TInt r=iDllArray->Find(dllInfo,key,pos);//	Search array by Dll Name		
		if (r==KErrNone)	//	**********IT IS ALREADY IN THE ARRAY***********
			{
			__PRINT1(_L(" Dll %S has already been checked\n"),&dllInfo.iDllName);

			//Check its Uid3 against that noted in the array		
			if (iDllArray->At(pos).iUid!=dllInfo.iUid)	
				{
			//	Uid3 is different to that of the same named Dll
			//	Add it to the array for comparative purposes			
				__PRINT2(_L(" Uid3 [%08x] for %S is different from that noted previously\n"),dllInfo.iUid, &dllInfo.iDllName);
				dllInfo.iResult=EUidDifference;
			
			//	Add this entry to iDllArray
				DllAppendL(dllInfo);							
				}
			}//	Run to the end of the "for" loop for this i value
		else	//	**********THE FILE IS NOT YET IN THE ARRAY**********				
			{
			__PRINT(_L(" Dll has not previously been checked\n"));

		//	Check through it and add the relevant information to the array
		
		//	Search for the DLL
			TPath aPath=(TPath)CShell::currentPath;
			TFileName fileName;
			TName dll16BitName;

#if defined(UNICODE)
			Get16BitDllName(dllInfo.iDllName,dll16BitName);
#else
			dll16BitName=(dllInfo.iDllName);
#endif
			r=FindDll(dll16BitName,fileName,aPath);
			
			__PRINT1(_L("dll16BitName=%S\n"),&dll16BitName);
			__PRINTWAIT(_L(" Press any key to continue\n"));

			if (r==KErrNotFound)	//	Could not find Dll
				{
				dllInfo.iResult=ENotFound;
				}//	Run to the end of the "for" loop for this i value

			else	//	File was located 
					
//	Go recursive.  Call GetImportDataL on the new dll, if it imports anything.
//	ROM dlls have no import data so this is never called for ROM dlls.
//	This *will* terminate.  It is only called on "new" dlls not in the array.
		
				{				
				__PRINT(_L(" ****Go recursive****\n"));
				__PRINTWAIT(_L(" Press any key to continue\n"));
								
				iCalls++;
				
				TRAP(r,GetImportDataL(fileName, pointer));				
			//	Pass in (parsed) fileName and Uid3 (from file header)
				switch(r)		
					{
				case(KErrGeneral):	//	No import data
						{
						dllInfo.iResult=ENoImportData;
						break;
					//	Run to the end of the for loop for this i value
						}
				case(EUidNotSupported):
						{
						dllInfo.iResult=EUidNotSupported;
						break;
						}
				case(KErrNone):	//	Import data was read
						{
						dllInfo.iResult=EFileFoundAndUidSupported;
						break;
					//	Run to the end of the for loop for this i value
						}
				case(KErrInUse):
						{
						__PRINT2(_L(" File %S is already open\n"),&fileName,r);
						dllInfo.iResult=EAlreadyOpen;
						break;
						}
				case(KErrCorrupt):
						{
						__PRINT2(_L(" File %S has unexpected format\n"),&fileName,r);
						dllInfo.iResult=EAlreadyOpen;
						break;
						}
				default:
						{
						__PRINT1(_L(" File %S could not be opened \n"),&fileName);
						dllInfo.iResult=ECouldNotOpenFile;
						break;
						}
					//	Run to the end of the for loop for this i value							
					}	
				}
		
		//	Add the information about the dependency to iDllArray
			DllAppendL(dllInfo);			
			}
//	Runs to here when all import data has been read for this i value

		__PRINT(_L(" Go to next dll to be imported...\n"));
		__PRINTWAIT(_L(" Press any key to continue\n"));
			
		block = (E32ImportBlock*)block->NextBlock(E32ImageHeader::ImpFmtFromFlags(aFlags));
		
		}//	The end of the loop.  Every DLL has been located	

	}


void CDllChecker::GetImportDataL(const TDesC& aFileName, TUid* aPointer)
	{
	TInt cleanupCount=0;
	//	Check that the file is not a ROM dll.  These have no import data	
	if (ShellFunction::TheShell->TheFs.IsFileInRom(aFileName))
		{		
		User::Leave(KErrGeneral);
		}
	
	//open file for reading and push it to autoclose stack
	TAutoClose<RFile> autoFile;
	User::LeaveIfError(autoFile.iObj.Open(CShell::TheFs,aFileName,EFileStream));
	autoFile.PushL();
	cleanupCount++;
	iFile=autoFile.iObj;
		
	//Create a pointer to an Image Header
	//reserve enough memory for compressed file header because we don't know whether the file is compressed or not 
	E32ImageHeaderComp* imageHeader=new(ELeave)E32ImageHeaderComp;
	CleanupStack::PushL(imageHeader);
	cleanupCount++;
	
	//read file header
	TPtr8 ptrToImageHeader((TText8*)(imageHeader),sizeof(E32ImageHeaderComp),sizeof(E32ImageHeaderComp));
	User::LeaveIfError(iFile.Read(ptrToImageHeader,sizeof(E32ImageHeaderComp)));
	
	if (imageHeader->iImportOffset==0)//	File contains no import data
		{	
		User::Leave(KErrGeneral);
		}	

	TUint32 compression = imageHeader->CompressionType();
	TInt restOfFileSize=0;
	TUint8* restOfFileData=NULL;
	//detect the size of import information
	if (compression != KFormatNotCompressed)
		{
		// Compressed executable
		// iCodeOffset	= header size for format V or above
		//				= sizeof(E32ImageHeader) for format J
		restOfFileSize = imageHeader->UncompressedFileSize() - imageHeader->iCodeOffset;
		}
	else
		{
		TInt FileSize;
		iFile.Size(FileSize); 		
		restOfFileSize = FileSize-imageHeader->TotalSize();
		}	
	restOfFileSize -= imageHeader->iCodeSize; // the size of the exe less header & code
	
	//allocate memory for import information
	if (restOfFileSize >0)
		{
		restOfFileData = (TUint8*)User::AllocLC(restOfFileSize );		
		cleanupCount++;
		}

	User::LeaveIfError(LoadFile(compression,imageHeader,restOfFileData,restOfFileSize)); // Read import information in
			
	TInt32 uid3=imageHeader->iUid3;
	if(iCalls!=0)	//	Only check Uid3 of dependencies (ie only after first 
		{			//	call of recursive function)
		TInt r=CheckUid3(uid3,*aPointer);

		if (r!=KErrNone) //	Dll's Uid3 is not valid
			User::Leave(EUidNotSupported);						
		}
				
	TInt bufferOffset=imageHeader->iImportOffset-(imageHeader->iCodeOffset + imageHeader->iCodeSize);
	if( TInt(bufferOffset+sizeof(E32ImportSection))>restOfFileSize)
		User::Leave(KErrCorrupt);		
	//get the table of dependencies
	GetDllTableL(restOfFileData+bufferOffset,imageHeader->iDllRefTableCount,imageHeader->iFlags);

	CleanupStack::PopAndDestroy(cleanupCount);	
	}


TUint8* CDllChecker::NextBlock(TUint8* aBlock)
	{
	E32ImportBlock* block;	
	//	Advance the pointer to the next block	
	block=(E32ImportBlock*)aBlock;
	aBlock=(aBlock+sizeof(E32ImportBlock)+((block->iNumberOfImports)*sizeof(TUint)));		
	return (aBlock);
	}


TFileNameInfo::TFileNameInfo()
	{
	memclr(this, sizeof(TFileNameInfo));
	}

TInt TFileNameInfo::Set(const TDesC8& aFileName, TUint aFlags)
	{
	iUid = 0;
	iVersion = 0;
	iPathPos = 0;
	iName = aFileName.Ptr();
	iLen = aFileName.Length();
	iExtPos = aFileName.LocateReverse('.');
	if (iExtPos<0)
		iExtPos = iLen;
	TInt osq = aFileName.LocateReverse('[');
	TInt csq = aFileName.LocateReverse(']');
	if (!(aFlags & EAllowUid) && (osq>=0 || csq>=0))
		{
		return KErrBadName;
		}
	if (osq>=iExtPos || csq>=iExtPos)
		{
		return KErrBadName;
		}
	TInt p = iExtPos;
	if ((aFlags & EAllowUid) && p>=10 && iName[p-1]==']' && iName[p-10]=='[')
		{
		TPtrC8 uidstr(iName + p - 9, 8);
		TLex8 uidlex(uidstr);
		TUint32 uid = 0;
		TInt r = uidlex.Val(uid, EHex);
		if (r==KErrNone && uidlex.Eos())
			iUid = uid, p -= 10;
		}
	iUidPos = p;
	TInt ob = aFileName.LocateReverse('{');
	TInt cb = aFileName.LocateReverse('}');
	if (ob>=iUidPos || cb>=iUidPos)
		{
		return KErrBadName;
		}
	if (ob>=0 && cb>=0 && p-1==cb)
		{
		TPtrC8 p8(iName, p);
		TInt d = p8.LocateReverse('.');
		TPtrC8 verstr(iName+ob+1, p-ob-2);
		TLex8 verlex(verstr);
		if (ob==p-10 && d<ob)
			{
			TUint32 ver = 0;
			TInt r = verlex.Val(ver, EHex);
			if (r==KErrNone && verlex.Eos())
				iVersion = ver, p = ob;
			}
		else if (d>ob && p-1>d && (aFlags & EAllowDecimalVersion))
			{
			TUint32 maj = 0;
			TUint32 min = 0;
			TInt r = verlex.Val(maj, EDecimal);
			TUint c = (TUint)verlex.Get();
			TInt r2 = verlex.Val(min, EDecimal);
			if (r==KErrNone && c=='.' && r2==KErrNone && verlex.Eos() && maj<32768 && min<32768)
				iVersion = (maj << 16) | min, p = ob;
			}
		}
	iVerPos = p;
	if (iLen>=2 && iName[1]==':')
		{
		TUint c = iName[0];
		if (c!='?' || !(aFlags & EAllowPlaceholder))
			{
			c |= 0x20;
			if (c<'a' || c>'z')
				{
				return KErrBadName;
				}
			}
		iPathPos = 2;
		}
	TPtrC8 pathp(iName+iPathPos, iVerPos-iPathPos);
	if (pathp.Locate('[')>=0 || pathp.Locate(']')>=0 || pathp.Locate('{')>=0 || pathp.Locate('}')>=0 || pathp.Locate(':')>=0)
		{
		return KErrBadName;
		}
	iBasePos = pathp.LocateReverse('\\') + 1 + iPathPos;
	return KErrNone;
	}

void TFileNameInfo::GetName(TDes8& aName, TUint aFlags) const
	{
	if (aFlags & EIncludeDrive)
		aName.Append(Drive());
	if (aFlags & EIncludePath)
		{
		if (PathLen() && iName[iPathPos]!='\\')
			aName.Append('\\');
		aName.Append(Path());
		}
	if (aFlags & EIncludeBase)
		aName.Append(Base());
	if ((aFlags & EForceVer) || ((aFlags & EIncludeVer) && VerLen()) )
		{
		aName.Append('{');
		aName.AppendNumFixedWidth(iVersion, EHex, 8);
		aName.Append('}');		
		}
	if ((aFlags & EForceUid) || ((aFlags & EIncludeUid) && UidLen()) )
		{
		aName.Append('[');
		aName.AppendNumFixedWidth(iUid, EHex, 8);
		aName.Append(']');
		}
	if (aFlags & EIncludeExt)
		aName.Append(Ext());
	}


void CDllChecker::GetFileNameAndUid(SDllInfo &aDllInfo, const TDesC8 &aExportName)
//	
//	Gets filename and UID 
//
	{	
	TFileNameInfo filename;
	filename.Set(aExportName,TFileNameInfo::EAllowUid|TFileNameInfo::EAllowDecimalVersion);
	filename.GetName(aDllInfo.iDllName,TFileNameInfo::EIncludeBaseExt);
	aDllInfo.iUid=TUid::Uid(filename.Uid());
	}


TInt CDllChecker::CheckUid3(TInt32 aUid3,TUid aUid)
//
//	Check that Uid3 is the same in the iDllName and as noted by the Image Header
//	aUid3 is the value found by the image header
//	aUid is the value found by parsing the result of block->dllname 
//	using GetFileNameAndUid()
	{

	if ((aUid.iUid)==aUid3)
		{

		__PRINT(_L(" Uid3 is valid\n"));

		return KErrNone;
		}
	else
		{

		__PRINT(_L(" Uid3 value is not supported\n"));

		return (EUidNotSupported);
		}
	}



TInt CDllChecker::FindDll(TDes& aDllName,TFileName& aFileName, TPath& aPath)
//
// Search for a dll in the following sequence ...
// 1. Supplied path parameter
// 2. System directories on all drives
//
	{
	TFindFile findFile(CShell::TheFs);
	TInt r=findFile.FindByPath(aDllName,&aPath);
	if (r==KErrNone)
		{
		aFileName=findFile.File();	

		__PRINT1(_L(" Dependency %S was found (supplied path)\n"),&aFileName);

		return(r);
		}

	r=findFile.FindByDir(aDllName,_L("\\Sys\\Bin\\"));
	if (r==KErrNone)
		{
		aFileName=findFile.File();	

		__PRINT1(_L(" Dependency %S was found (system directory)\n"),&aFileName);

		return(r);
		}

	if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		{
		r=findFile.FindByDir(aDllName,_L("\\System\\Bin\\"));
		if (r==KErrNone)
			{
			aFileName=findFile.File();	

			__PRINT1(_L(" Dependency %S was found (system directory)\n"),&aFileName);

			return(r);
			}
		}

	__PRINT1(_L(" Dependency %S was not found\n"),&aDllName);

	return(KErrNotFound);
	}




void CDllChecker::DllAppendL(const SDllInfo& aDllInfo)
	{
	TInt leaveCode=KErrNone;				
	TRAP(leaveCode,iDllArray->AppendL(aDllInfo));	//	Add it to iDllArray
	
	if (leaveCode!=KErrNone)
		{
		__PRINT(_L(" Could not add Dll to the array\n"));
		__PRINTWAIT(_L(" Press any key to continue\n"));
		
		User::Leave(leaveCode);	
		}
	else
		{
		__PRINT(_L(" Added this information to the array\n"));
		__PRINT1(_L(" Number of elements in array=%d\n"),iDllArray->Count());

		}
	}



void CDllChecker::ListArray()
	{
	TInt elements=iDllArray->Count();
	
	CShell::TheConsole->Printf(_L(" Number of dependencies checked = %d\n"),elements);

	for (TInt i=0;i<elements; i++)
		{
//		Prints filename and result of CDllChecker::GetImportDataL for 
//		each element in iDllArray
#if defined (UNICODE)	
		TFileName filename;
		Get16BitDllName(iDllArray->At(i).iDllName,filename);
		CShell::TheConsole->Printf(_L(" %d: %-15S  Uid3: [%08x] "),(i+1),&(filename),(iDllArray->At(i).iUid));
#else
		CShell::TheConsole->Printf(_L(" %d: %-15S  Uid3: [%08x] "),(i+1),&(iDllArray->At(i).iDllName),(iDllArray->At(i).iUid));
#endif
		switch(iDllArray->At(i).iResult)
			{
		case(ENoImportData):
			CShell::TheConsole->Printf(_L("--- No import data\n"));
			break;

		case(EUidNotSupported):
			CShell::TheConsole->Printf(_L("--- Uid3 is not supported\n"));
			break;

		case(ENotFound):
			CShell::TheConsole->Printf(_L("--- File was not found\n"));
			break;

		case(ECouldNotOpenFile):
			CShell::TheConsole->Printf(_L("--- File could not be opened\n"));
			break;

		case(EUidDifference):
			CShell::TheConsole->Printf(_L("--- File already noted with different Uid\n"));
			break;

		case(EAlreadyOpen):
			CShell::TheConsole->Printf(_L("--- File already open\n"));
			break;
		
		case(EFileFoundAndUidSupported):
			CShell::TheConsole->Printf(_L("--- File was found, Uid3 is supported\n"));
			break;
		default:	//	Will never reach here
			CShell::TheConsole->Printf(_L("--- Undefined\n"));
			break;
			}
		}
	}

