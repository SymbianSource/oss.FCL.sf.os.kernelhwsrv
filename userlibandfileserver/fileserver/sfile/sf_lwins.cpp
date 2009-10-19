// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_lwins.cpp
// 
//

#include "sf_std.h"
#include <f32image.h>
#include "sf_image.h"
#include <e32uid.h>
#include <emulator.h>
#include <e32wins.h>
#include <stdlib.h>
#include <hal.h>

_LIT(KDirSystemPrograms,"\\System\\Programs\\");
_LIT(KDirSystemLibs,"\\System\\Libs\\");
_LIT8(KRomSystemLibs,"z:\\system\\libs\\");

_LIT(KDirSysBin,"\\sys\\bin\\");
_LIT8(KRomSysBin,"z:\\sys\\bin\\");
_LIT(KDirSystemBin,"\\System\\Bin\\");
_LIT8(KRomSystemBin,"z:\\system\\Bin\\");

#ifdef _DEBUG
extern TRequestStatus* ProcessDestructStatPtr;
extern TBool ProcessCreated;
#endif


/******************************************************************************
 * Executable file find routines for WINS
 ******************************************************************************/

TInt GetPEInfo(TProcessCreateInfo& aInfo)
//
// Extract the Uids, etc from the PE file
//
	{
	TBuf<MAX_PATH> ifilename;
	ifilename.Copy(aInfo.iFileName);
	TBuf<MAX_PATH> filename;
	TInt r = MapEmulatedFileName(filename, ifilename);
	if (r == KErrNone)
		{
		Emulator::RImageFile pefile;
		r = pefile.Open(filename.PtrZ());
		if (r == KErrNone)
			{
			pefile.GetInfo(aInfo);
			// Overide capabilities in image
			for(TInt i=0; i<SCapabilitySet::ENCapW; i++)
				{
				aInfo.iS.iCaps[i] |= DisabledCapabilities[i];
				aInfo.iS.iCaps[i] &= AllCapabilities[i];
				}
			pefile.Close();
			}
		}
	return r;
	}

TBool CheckIsDirectoryName(TProcessCreateInfo& aInfo)
//
// Attempt to get file attributes from Windows if attributes can't be found 
// just assume not a directory otherwise check the directory bit
// Only intended for use by FindBin, FindDll and FindExe
//
	{	
	TBuf<MAX_PATH> ifilename;
	ifilename.Copy(aInfo.iFileName);
	TBuf<MAX_PATH> filename;
	if (MapEmulatedFileName(filename, ifilename) != KErrNone)
		return EFalse; // just return EFalse as the error will be picked up later
	
	DWORD attr = Emulator::GetFileAttributes(filename.PtrZ());
	if (attr != -1 && (attr & FILE_ATTRIBUTE_DIRECTORY))
		return ETrue;
	return EFalse;
	}

TInt FindBin(E32Image& aImage)
//
// WINS find Binary in system bin
// System directories on all drives Y-A,Z
//
	{
	__IF_DEBUG(Printf("FindBin"));

	TInt len=aImage.iFileName.Length();
	// check if it's a bare drive letter with no path
	if (len>=3 && aImage.iFileName[1]==':' && aImage.iFileName[2]!='\\')
		{
		if (len + KRomSysBin().Length()-2 <= aImage.iFileName.MaxLength())
			aImage.iFileName.Insert(2,KRomSysBin().Mid(2));
		}
	
	// Ensure consistent error codes with h/w, see DEF092502
	// Unlike Symbian on h/w targets, on Windows searching 
	// for a driectory, including "." and "..", will return KErrorAccessDenied
	if (CheckIsDirectoryName(aImage))
		return KErrNotFound;	
	
	// first try and find it in the given directory
	TInt r=GetPEInfo(aImage);

	// Next : if looking at z:\sys\bin then look for it in emulator path
	if (r == KErrNotFound || r == KErrPathNotFound)
		{
		if (aImage.iFileName.FindF(KRomSysBin) == 0)
			{
			aImage.iFileName.Delete(0, KRomSysBin().Length());
			r = GetPEInfo(aImage);
			}

		if (r==KErrNotFound || r == KErrPathNotFound)
			{
			//	Now try finding it in the EPOC scheme of things
			TBuf<MAX_PATH> ifilename;
			ifilename.Copy(aImage.iFileName);
			TFindFile ff(gTheLoaderFs);
			r=ff.FindByDir(ifilename,KDirSysBin);
			if (r==KErrNone)
				{
				aImage.iFileName.Copy(ff.File());
				__IF_DEBUG(Printf("Found file %S",&aImage.iFileName));
				r=GetPEInfo(aImage);
				}
			else
				{
				// Last chance look in emulator path for driveless things
				if (aImage.iFileName.FindF(KRomSysBin().Mid(2)) == 0)
					{
					aImage.iFileName.Delete(0, KRomSysBin().Length()-2);
					r = GetPEInfo(aImage);
					}
				else 
					{
					__IF_DEBUG(Printf("Filename %S not found in ?:\\sys\\bin",&aImage.iFileName));
					r=KErrNotFound;
					}
				}
			}
		}

	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		return r;

	// Next : if looking at z:\system\bin then look for it in emulator path
	if (r == KErrNotFound || r == KErrPathNotFound)
		{
		if (aImage.iFileName.FindF(KRomSystemBin) == 0)
			{
			aImage.iFileName.Delete(0, KRomSystemBin().Length());
			r = GetPEInfo(aImage);
			}

		if (r==KErrNotFound || r == KErrPathNotFound)
			{
			//	Now try finding it in the EPOC scheme of things
			TBuf<MAX_PATH> ifilename;
			ifilename.Copy(aImage.iFileName);
			TFindFile ff(gTheLoaderFs);
			r=ff.FindByDir(ifilename,KDirSystemBin);
			if (r==KErrNone)
				{
				aImage.iFileName.Copy(ff.File());
				__IF_DEBUG(Printf("Found file %S",&aImage.iFileName));
				r=GetPEInfo(aImage);
				}
			else
				{
				__IF_DEBUG(Printf("Filename %S not found",&aImage.iFileName));
				r=KErrNotFound;
				}
			}
		}
	return r;
	}


TInt FindExe(E32Image& aImage)
//
// WINS find executable
// System directories on all drives Y-A,Z
//
	{
	__IF_DEBUG(Printf("FindExe"));

	TInt len  = aImage.iFileName.Length();
	// check if it's a bare drive letter with no path
	if (len >= 3 && aImage.iFileName[1]==':' && aImage.iFileName[2]!='\\')
		{
		if (len + KRomSysBin().Length()-2 <= aImage.iFileName.MaxLength())
			aImage.iFileName.Insert(2,KRomSysBin().Mid(2));
		}
	// Ensure consistent error codes with h/w, see DEF092502
	// Unlike Symbian on h/w targets, on Windows searching 
	// for a driectory, including "." and "..", will return KErrorAccessDenied
	if (CheckIsDirectoryName(aImage))
		return KErrNotFound;
	
	// first try and find it in the given directory
	TInt r=GetPEInfo(aImage);

	// Next : if looking at z:\sys\bin then look for it in emulator path
	if (r == KErrNotFound || r == KErrPathNotFound)
		{
		if (aImage.iFileName.FindF(KRomSysBin) == 0)
			{
			aImage.iFileName.Delete(0, KRomSysBin().Length());
			r = GetPEInfo(aImage);
			}

		if (r==KErrNotFound || r == KErrPathNotFound)
			{
			//	Now try finding it in the EPOC scheme of things
			TBuf<MAX_PATH> ifilename;
			ifilename.Copy(aImage.iFileName);
			TFindFile ff(gTheLoaderFs);
			r=ff.FindByDir(ifilename,KDirSysBin);
			if (r==KErrNone)
				{
				aImage.iFileName.Copy(ff.File());
				__IF_DEBUG(Printf("Found file %S",&aImage.iFileName));
				r=GetPEInfo(aImage);
				}
			else
				{
				__IF_DEBUG(Printf("Filename %S not found in ?:\\sys\\bin",&aImage.iFileName));
				r=KErrNotFound;
				}
			}
		}

	if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		return r;

	// Next : if looking at z:\system\libs then look for it in emulator path
	if (r == KErrNotFound || r == KErrPathNotFound)
		{
		if (aImage.iFileName.FindF(KRomSystemLibs) == 0)
			{
			aImage.iFileName.Delete(0, KRomSystemLibs().Length());
			r = GetPEInfo(aImage);
			}

		if (r==KErrNotFound || r == KErrPathNotFound)
			{
			//	Now try finding it in the EPOC scheme of things
			TBuf<MAX_PATH> ifilename;
			ifilename.Copy(aImage.iFileName);
			TFindFile ff(gTheLoaderFs);
			r=ff.FindByDir(ifilename,KDirSystemPrograms);
			if (r==KErrNone)
				{
				aImage.iFileName.Copy(ff.File());
				__IF_DEBUG(Printf("Found file %S",&aImage.iFileName));
				r=GetPEInfo(aImage);
				}
			else
				{
				__IF_DEBUG(Printf("Filename %S not found",&aImage.iFileName));
				r=KErrNotFound;
				}
			}
		}
	return r;
	}

// WINS FindDll
TInt FindDll(E32Image& aImage, const TDesC8* aPath)
//
// Search for a dll in the following sequence ...
// 1. Supplied path parameter
// 2. System directories on all drives
//
	{

	TInt len = aImage.iFileName.Length();
	// check if it's a bare drive letter with no path
	if (len >=3 && aImage.iFileName[1]==':' && aImage.iFileName[2]!='\\')
		{
		if (len + KRomSysBin().Length()-2 <= aImage.iFileName.MaxLength())
			aImage.iFileName.Insert(2,KRomSysBin().Mid(2));
		}
	
	// Ensure consistent error codes with h/w, see DEF092502
	// Unlike Symbian on h/w targets, on Windows searching 
	// for a driectory, including "." and "..", will return KErrorAccessDenied
	if (CheckIsDirectoryName(aImage))
		return KErrNotFound;
	
	TInt r=GetPEInfo(aImage);

	// Next : if looking at z:\system\libs then look for it in emulator path
	if (r == KErrNotFound || r == KErrPathNotFound)
		{
		if (aImage.iFileName.FindF(KRomSysBin) == 0)
			{
			aImage.iFileName.Delete(0, KRomSysBin().Length());
			r = GetPEInfo(aImage);
			}

		if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
			if (r == KErrNotFound || r == KErrPathNotFound)
				if (aImage.iFileName.FindF(KRomSystemLibs) == 0)
					{
					aImage.iFileName.Delete(0, KRomSystemLibs().Length());
					r = GetPEInfo(aImage);
					}

		TBuf<MAX_PATH> ifilename;
		ifilename.Copy(aImage.iFileName);
		if (r==KErrNotFound || r == KErrPathNotFound)
			{
			//	Now try finding it in the EPOC scheme of things
			TFindFile ff(gTheLoaderFs);
			__IF_DEBUG(Printf("FindDll aDllName %S, aPath %S",&aImage.iFileName,aPath?aPath:&KNullDesC8));
			if (aPath && aPath->Length()!=0)
				{
				TBuf<MAX_PATH> ipath;
				ipath.Copy(*aPath);
				r=ff.FindByPath(ifilename, &ipath);
				if (r==KErrNone)
					{
					aImage.iFileName.Copy(ff.File());
					__IF_DEBUG(Printf("Found file %S",&aImage.iFileName));
					r=GetPEInfo(aImage);
					}
				}

			if (r!=KErrNone)
				{
				r=ff.FindByDir(ifilename,KDirSysBin);
				if (r==KErrNone)
					{
					aImage.iFileName.Copy(ff.File());
					__IF_DEBUG(Printf("Found file %S",&aImage.iFileName));
					r=GetPEInfo(aImage);
					}
				}

			if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
				if (r!=KErrNone)
					{
					r=ff.FindByDir(ifilename,KDirSystemLibs);
					if (r==KErrNone)
						{
						aImage.iFileName.Copy(ff.File());
						__IF_DEBUG(Printf("Found file %S",&aImage.iFileName));
						r=GetPEInfo(aImage);
						}
					}

			if(r!=KErrNone)
				{
				__IF_DEBUG(Printf("Filename %S not found",&aImage.iFileName));
				r=KErrNotFound;
				}
			}
		}
	return r;
	}

/******************************************************************************
 * WINS specific E32Image functions
 ******************************************************************************/

TInt E32Image::ProcessFileName()
//
// Get the properly capitalised file name and root name
//
	{

	TFileNameInfo fni;
	TInt r = fni.Set(iFileName, 0);
	if (r!=KErrNone)
		return r;
	iRootNameOffset = fni.iBasePos;
	iRootNameLength = fni.iLen - fni.iBasePos;
	iExtOffset = fni.iExtPos;
//	TBuf<MAX_PATH> filename;
//	TInt r=MapEmulatedFileName(filename, iFileName);
//	if (r!=KErrNone)
//		return r;
//	WIN32_FIND_DATA w32fd;
//	HANDLE h=Emulator::FindFirstFile(filename.PtrZ(), &w32fd);
//	if (h==0)
//		return Emulator::LastError();
//	TPtrC real_filename((const TText*)&w32fd.cFileName[0]);
//	FindClose(h);
//	iFileName.SetLength(slash+1);
//	iFileName+=real_filename;
	__IF_DEBUG(Printf("ProcessFileName: %S,%d,%d,%d",&iFileName,iRootNameOffset,iRootNameLength,iExtOffset));
	return KErrNone;
	}

TInt E32Image::LoadProcess(const RLdrReq& aReq)
	{
	__IF_DEBUG(Printf("E32Image::LoadProcess %S",&aReq.iFileName));

	iFileName=*aReq.iFileName;
	TInt r=FindExe(*this);
	if (r!=KErrNone)
		r=FindBin(*this);
	if (r==KErrNone)
		r=ProcessFileName();
	if (r==KErrNone)
		r=CheckUids(iUids, aReq.iRequestedUids);
	if (r!=KErrNone)
		return r;
	r = aReq.iMsg->Client((RThread&)aReq.iClientThread);
	if (r!=KErrNone)
		return r;
	iClientHandle=aReq.iClientThread.Handle();
#ifdef _DEBUG
	iDestructStat = ProcessDestructStatPtr;
#endif
	iFlags |= EDataUnpaged;	// Data paging is not supported on the emulator
	r=E32Loader::ProcessCreate(*this, aReq.iCmd);
	__IF_DEBUG(Printf("Done E32Loader::ProcessCreate %d",r));
	if (r!=KErrNone)
		return r;
#ifdef _DEBUG
	ProcessCreated = ETrue;
#endif
	iClientProcessHandle=iProcessHandle;
	r=E32Loader::ProcessLoaded(*this);
	return r;
	}

// Load a code segment, plus all imports if main loadee
TInt E32Image::LoadCodeSeg(const RLdrReq& aReq)
	{
	__IF_DEBUG(Printf("E32Image::LoadCodeSeg %S",aReq.iFileName));

	const TDesC8& reqName=*aReq.iFileName;
	const TDesC8* searchPath=aReq.iPath;
		
	iFileName=reqName;
	TInt r=FindDll(*this, searchPath);
	if (r!=KErrNone)
		r=FindBin(*this);
	// Hack to support EXEDLLs which are DLLs but have EXE file extentions
	if(r==KErrNotFound)
		{
		if(iFileName.Right(4).CompareF(_L8(".DLL"))==0)
			{
			TUint8* p = (TUint8*)iFileName.Ptr() + iFileName.Length() - 3;
			*p++ = 'E';
			*p++ = 'X';
			*p++ = 'E';
			r=FindDll(*this, searchPath);
			if (r!=KErrNone)
				r=FindBin(*this);
			}
		}
	if (r==KErrNone)
		r=ProcessFileName();
	if (r==KErrNone)
		r=CheckUids(iUids, aReq.iRequestedUids);

	if(r==KErrNone)
		{
		if(this == iMain)
			{
			// Check that we can legally load the DLL into the process
			r=aReq.CheckSecInfo(iS);
			}
		}
	if (r!=KErrNone)
		return r;
		
	__IF_DEBUG(Printf("Checking uids for %S", &iFileName));
	if (iUids[0]!=KDynamicLibraryUid && iUids[0]!=KExecutableImageUid)
	    return KErrNotSupported;
	r=CheckAlreadyLoaded();
	if (r!=KErrNone || iAlreadyLoaded)
		{
		__IF_DEBUG(Printf("<LoadCodeSeg AlreadyLoaded %d",r));
		return r;		// already loaded, either share or give up
		}
	__IF_DEBUG(Printf("CodeSeg create"));
	r=E32Loader::CodeSegCreate(*this);
	if (r==KErrNone)
		r=E32Loader::CodeSegLoaded(*this);

	__IF_DEBUG(Printf("<LoadCodeSeg, r=%d",r));
	return r;
	}

//__DATA_CAGING__
TInt ReadCapabilities(RLdrReq& aReq)
//	
//	Wins version
//
	{
	E32Image* e=new E32Image;
	if (!e)
		return KErrNoMemory;
	e->iMain=e;
	e->iFileName=*aReq.iFileName;
	TInt r=GetPEInfo(*e);
	if (r==KErrNotFound || r == KErrPathNotFound)
		{
		//	z:\sys\bin\* may be found in emulator path
		if (e->iFileName.FindF(KRomSysBin) == 0)
			{
			e->iFileName.Delete(0, KRomSysBin().Length());
			r = GetPEInfo(*e);
			}
		}

	if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
		if (r==KErrNotFound || r == KErrPathNotFound)
			{
			//	z:\system\bin\* may be found in emulator path
			if (e->iFileName.FindF(KRomSystemBin) == 0)
				{
				e->iFileName.Delete(0, KRomSystemBin().Length());
				r = GetPEInfo(*e);
				}
			}
	TPtrC8 caps((const TUint8*)&e->iS.iCaps, sizeof(e->iS.iCaps));
	if (r==KErrNone)
		r=aReq.iMsg->Write(2, caps);
	delete e;
	return r;
	}

TInt GetModuleInfo(RLdrReq& aReq)
//
//	Read capabilities from file found
//
	{
	__IF_DEBUG(Printf("ReadModuleInfo %S",aReq.iFileName));
	TFileNameInfo& fi = aReq.iFileNameInfo;
	TInt r = KErrNotSupported;

	// must specify a fully qualified name
	if (fi.DriveLen() && fi.PathLen())
		{
		E32Image* e=new E32Image;
		if (!e)
			return KErrNoMemory;
		e->iMain = e;
		e->iFileName = *aReq.iFileName;
		r = GetPEInfo(*e);
		if (r==KErrNotFound || r == KErrPathNotFound)
			{
			//	z:\system\bin\* may be found in emulator path
			if (e->iFileName.FindF(KRomSysBin) == 0)
				{
				e->iFileName.Delete(0, KRomSysBin().Length());
				r = GetPEInfo(*e);
				}

			if(!PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
				if(r!=KErrNone)
					if (e->iFileName.FindF(KRomSystemBin) == 0)
						{
						e->iFileName.Delete(0, KRomSystemBin().Length());
						r = GetPEInfo(*e);
						}
			}
		if (r == KErrNone)
			{
			RLibrary::TInfo ret_info;
			memclr(&ret_info,sizeof(ret_info));
			ret_info.iModuleVersion = e->iModuleVersion;
			ret_info.iUids = e->iUids;
			*(SSecurityInfo*)&ret_info.iSecurityInfo = e->iS;
			TPckgC<RLibrary::TInfo> ret_pckg(ret_info);
			r = aReq.iMsg->Write(2, ret_pckg);
			}
		delete e;
		}
	return r;
	}

TInt GetInfoFromHeader(const RLoaderMsg& /*aMsg*/)
	{
	TInt r;
	r = KErrNotSupported;
	return r;
	}



