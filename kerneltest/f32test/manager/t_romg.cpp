// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test/manager/t_romg.cpp
// 
//

#define	__E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32rom.h>
#include "../server/t_server.h"

const TInt KBufSize=0x10000;
const TInt KFillerSize=0x100;
const TInt KRomFileHeaderSize=0x100;
const TInt KRomFileHeaderNameSize=0x10;

class TRomFileHeaderBase
	{
public:
	TText8 iName[KRomFileHeaderNameSize];
	TText8 iVersionStr[4];
	TText8 iBuildNumStr[4];
	TInt iRomSize;
	TInt iHeaderSize;
	};

class TRomFileHeader : public TRomFileHeaderBase
	{
public:
	TUint8 iFiller[KRomFileHeaderSize-sizeof(TRomFileHeaderBase)];
	};

class CMemDir;
class CMemEntry : public CBase
	{
public:
	static CMemEntry* New(const TEntry& anEntry);
	~CMemEntry();
	TInt EntrySize();
	void Write();
	void WriteFile();
	inline TBool IsDir() {return(iEntry.iAtt&KEntryAttDir);}
	inline CMemDir& Dir() {return(*iDir);}
	inline void SetDir(CMemDir& aDir) {iDir=(&aDir);}
	inline TInt Size() {return(iSize);}
	inline TLinAddr Base() {return(iBase);}
	inline void SetBase();
	inline const TDesC& Name() {return(*iName);}
	inline static TInt LinkOffset() {return(_FOFF(CMemEntry,iLink));}
protected:
	CMemEntry(const TEntry& anEntry);
private:
	TLinAddr iBase;
	TInt iSize;
	TRomEntry iEntry;
	HBufC* iName;
	CMemDir* iDir;
	TDblQueLink iLink;
	};

class CMemDir : public CBase
	{
public:
	static CMemDir* NewL();
	~CMemDir();
	void LoadDirL(const TDesC& aPath);
	void SetBaseDirs();
	void SetBaseFiles();
	void WriteDirs();
	void WriteFiles();
	void Write();
	TFileName Name(const TDesC& aName);
	inline TLinAddr Base() {return(iBase);}
protected:
	CMemDir();
private:
	TInt iSize;
	TInt iCount;
	TLinAddr iBase;
	HBufC* iPath;
	TDblQue<CMemEntry> iEntryQ;
	};

class RFileX : public RFile
	{
public:
	RFileX() : RFile() {}
	TInt Write(const TDesC8& aDes);
	TInt Write(const TDesC8& aDes,TInt aLength);
	};

GLDEF_D RTest test(_L("T_ROMG"));
LOCAL_D RFileX TheFile;
LOCAL_D TRomHeader TheRomHeader;
LOCAL_D TLinAddr TheCurrentBase;
LOCAL_D CMemDir* TheRootDir;
LOCAL_D TInt TheLevel;
LOCAL_D TBuf8<KBufSize> TheBuf;
LOCAL_D TBuf8<KFillerSize> TheFiller;
#if defined(_UNICODE)
LOCAL_D const TPtrC TheFileName=_L("ROMFILEU.BIN");
#else
LOCAL_D const TPtrC TheFileName=_L("ROMFILE.BIN");
#endif

TInt RFileX::Write(const TDesC8& aDes)
//
// Write and update the file pos
//
	{

	TheCurrentBase+=aDes.Size();
	return(RFile::Write(aDes));
	}

TInt RFileX::Write(const TDesC8& aDes,TInt aLength)
//
// Write and update the file pos
//
	{

	TheCurrentBase+=aLength;
	return(RFile::Write(aDes,aLength));
	}

CMemEntry* CMemEntry::New(const TEntry& anEntry)
//
// Create a new entry.
//
	{

	CMemEntry* pE=new(ELeave) CMemEntry(anEntry);
	pE->iName=anEntry.iName.Alloc();
	test(pE->iName!=NULL);
	return(pE);
	}

#pragma warning( disable : 4705 )	// statement has no effect
CMemEntry::CMemEntry(const TEntry& anEntry)
//
// Constructor.
//
	{

	iEntry.iAtt=(TUint8)(anEntry.iAtt|KEntryAttReadOnly); // All rom files are read only
	iEntry.iSize=anEntry.iSize;
	iEntry.iNameLength=(TUint8)anEntry.iName.Size();
	iSize=Align4(anEntry.iSize);
	__DECLARE_NAME(_S("CMemEntry"));
	}
#pragma warning( default : 4705 )

CMemEntry::~CMemEntry()
//
// Destruct.
//
	{

	if (iLink.iNext!=NULL)
		iLink.Deque();
	delete iName;
	if (IsDir())
		delete iDir;
	}

void CMemEntry::SetBase()
//
// Set the entries base address.
//
	{

	iBase=TheCurrentBase;
	iEntry.iAddressLin=TheCurrentBase;
	}

TInt CMemEntry::EntrySize()
//
// Calculate the entries size.
//
	{

	return(Align4(KRomEntrySize+iEntry.iNameLength));
	}

void CMemEntry::Write()
//
// Write the current entry.
//
	{

	test(TheFile.Write(TPtrC8((TUint8*)&iEntry,KRomEntrySize))==KErrNone);
	test(TheFile.Write(TPtrC8((TUint8*)iName->Ptr(),iName->Size()))==KErrNone);
	TInt rem=EntrySize()-iName->Size()-KRomEntrySize;
	if (rem)
		test(TheFile.Write(TheFiller,rem)==KErrNone);
	}

void CMemEntry::WriteFile()
//
// Write the current entries file.
//
	{

	test(iBase==TheCurrentBase);
	TAutoClose<RFile> f;
	TFileName n=Dir().Name(*iName);
	test(f.iObj.Open(TheFs,n,EFileStream|EFileRead)==KErrNone);
	TInt size=0;
	do
		{
		test(f.iObj.Read(TheBuf)==KErrNone);
		test(TheFile.Write(TheBuf)==KErrNone);
		size+=TheBuf.Length();
		} while (TheBuf.Length()==TheBuf.MaxLength());
	test(iEntry.iSize==size);
	TInt rem=iSize-size;
	if (rem)
		test(TheFile.Write(TheFiller,rem)==KErrNone);
	}

CMemDir* CMemDir::NewL()
//
// Create a new directory.
//
	{

	return(new(ELeave) CMemDir);
	}

CMemDir::CMemDir()
//
// Constructor.
//
	: iEntryQ(CMemEntry::LinkOffset())
	{

	__DECLARE_NAME(_S("CMemDir"));
	}

CMemDir::~CMemDir()
//
// Destruct.
//
	{

	while (!iEntryQ.IsEmpty())
		{
		CMemEntry* pE=iEntryQ.First();
		delete pE;
		}
	delete iPath;
	}

void CMemDir::SetBaseDirs()
//
// Set the base address of the directory and any sub-directories.
//
	{

	iBase=TheCurrentBase;
	iSize=sizeof(TInt);
	TDblQueIter<CMemEntry> q(iEntryQ);
	CMemEntry* pE;
	while ((pE=q++)!=NULL)
		{
		iCount++;
		iSize+=pE->EntrySize();
		}
	TheCurrentBase+=iSize;
	q.SetToFirst();
	while ((pE=q++)!=NULL)
		{
		if (pE->IsDir())
			{
			pE->SetBase();
			pE->Dir().SetBaseDirs();
			}
		}
	}

void CMemDir::SetBaseFiles()
//
// Set the base address of each file.
//
	{

	TDblQueIter<CMemEntry> q(iEntryQ);
	CMemEntry* pE;
	while ((pE=q++)!=NULL)
		{
		if (!pE->IsDir())
			{
			pE->SetBase();
			TheCurrentBase+=pE->Size();
			}
		}
	q.SetToFirst();
	while ((pE=q++)!=NULL)
		{
		if (pE->IsDir())
			pE->Dir().SetBaseFiles();
		}
	}

void CMemDir::WriteDirs()
//
// Write the directory and any sub-directories.
//
	{

	TheLevel++;
	TFileName name=Name(_L(""));
	test.Printf(_L("%*p%S\n"),TheLevel<<1,&name);
	Write();
	TDblQueIter<CMemEntry> q(iEntryQ);
	CMemEntry* pE;
	while ((pE=q++)!=NULL)
		{
		if (pE->IsDir())
			pE->Dir().WriteDirs();
		}
	TheLevel--;
	}

void CMemDir::WriteFiles()
//
// Set the base address of each file.
//
	{

	TheLevel++;
	TFileName name=Name(_L(""));
	test.Printf(_L("%*p%S\n"),TheLevel<<1,&name);
	TDblQueIter<CMemEntry> q(iEntryQ);
	CMemEntry* pE;
	while ((pE=q++)!=NULL)
		{
		if (!pE->IsDir())
			{
			test.Printf(_L("%*p%S\n"),(TheLevel<<1)+2,&pE->Name()),
			pE->WriteFile();
			}
		}
	q.SetToFirst();
	while ((pE=q++)!=NULL)
		{
		if (pE->IsDir())
			pE->Dir().WriteFiles();
		}
	TheLevel--;
	}

void CMemDir::Write()
//
// Write the current directory.
//
	{

	test(iBase==TheCurrentBase);
	TInt size=iSize-sizeof(TInt);
	test(TheFile.Write(TPtrC8((TUint8*)&size,sizeof(TInt)))==KErrNone);
	TDblQueIter<CMemEntry> q(iEntryQ);
	CMemEntry* pE;
	while ((pE=q++)!=NULL)
		pE->Write();
	TInt sz=(TheCurrentBase-iBase);
	test(sz==iSize);
	}

TFileName CMemDir::Name(const TDesC& aName)
//
// Make a full path name from aName.
//
	{

	TFileName n(*iPath);
	n+=_L("\\");
	n+=aName;
	return(n);
	}

void CMemDir::LoadDirL(const TDesC& aPath)
//
// Load a directory.
//
	{

	TheLevel++;
	iPath=aPath.AllocL();
	TFileName name=Name(_L("*.*"));
	test.Printf(_L("%*p%S\n"),TheLevel<<1,&name);
	CDir* pD;
	test(TheFs.GetDir(Name(_L("*.*")),KEntryAttMatchMask,EDirsFirst|ESortByName,pD)==KErrNone);
	TInt count=pD->Count();
	TInt i=0;
	while (i<count)
		{
		const TEntry& e=(*pD)[i++];
		TParse parse;
		parse.Set(e.iName,NULL,NULL);
		if (!parse.Ext().CompareF(_L(".NCB")))
			continue; // Ignore .ncb files - cannot open/read them.
		CMemEntry* pE=CMemEntry::New(e);
		iEntryQ.AddLast(*pE);
		}
	delete pD;
	TDblQueIter<CMemEntry> q(iEntryQ);
	CMemEntry* pE;
	while ((pE=q++)!=NULL)
		{
		if (pE->IsDir())
			{
			CMemDir* pM=CMemDir::NewL();
			pE->SetDir(*pM);
			pM->LoadDirL(Name(pE->Name()));
			}
		else
			pE->SetDir(*this);
		}
	TheLevel--;
	}

LOCAL_C void buildRomImageL()
//
// Build the ROM image.
//
	{

	test.Start(_L("Parse command line"));
	HBufC* buf=HBufC::New(RProcess().CommandLineLength());
   	test(buf!=NULL);
 	TPtr cmd = buf->Des();
 	RProcess().CommandLine(cmd);
	TLex lex(*buf);
	TFileName n=lex.NextToken();
	if (n.Length()==0)
		n=_L("\\F32");
	test(n.Length()!=0);
	if (n.Right(1)==_L("\\"))
		n.SetLength(n.Length()-1);
//
	test.Next(_L("Create root mem dir"));
	TRAPD(r,TheRootDir=CMemDir::NewL());
	test_KErrNone(r);
//
	test.Next(_L("Load directory structure"));
	TheLevel=(-1);
	TRAP(r,TheRootDir->LoadDirL(n));
	test_KErrNone(r);
	test(TheLevel==(-1));
//
	delete buf;
	test.End();
	}

LOCAL_C void baseRomImage()
//
// Set base addresses for the ROM image.
//
	{

	test.Start(_L("Setting up the header"));
	Mem::FillZ(&TheRomHeader,sizeof(TRomHeader));
	test.Printf(_L("1"));
	TheRomHeader.iVersion=TVersion(1,0,1);
	test.Printf(_L("2"));
	TTime t;
	t.HomeTime();
	test.Printf(_L("3"));
	TheRomHeader.iTime=t.Int64();
	test.Printf(_L("4"));
	TheRomHeader.iRomBase=UserSvr::RomHeaderAddress();
	test.Printf(_L("5"));
	TheRomHeader.iRomRootDirectoryList=TheCurrentBase=UserSvr::RomHeaderAddress()+sizeof(TRomHeader);
	test.Printf(_L("6"));
//
	test.Next(_L("Set dirs base"));
	TheRootDir->SetBaseDirs();
//
	test.Next(_L("Set files base"));
	TheRootDir->SetBaseFiles();
	TheRomHeader.iRomSize=TheCurrentBase-UserSvr::RomHeaderAddress();
//
	test.End();
	}

LOCAL_C void writeRomImage()
//
// Write the ROM image.
//
	{

	test.Start(_L("Write rom file header"));
//
	TPckgBuf<TRomFileHeader> fileHeadB;
	fileHeadB.FillZ(fileHeadB.MaxLength());
	TRomFileHeader& fileHead=fileHeadB();
	Mem::Copy(&fileHead.iName[0],"EPOC468 ROM     ",KRomFileHeaderNameSize);
	Mem::Copy(&fileHead.iVersionStr[0],"0.01",4);
	Mem::Copy(&fileHead.iBuildNumStr[0],"   1",4);
	fileHead.iRomSize=TheRomHeader.iRomSize;
	fileHead.iHeaderSize=KRomFileHeaderSize;
	test(TheFile.Write(fileHeadB)==KErrNone);
//
	test.Next(_L("Write rom header"));
	TheFiller.FillZ(TheFiller.MaxLength());
	TPckgC<TRomHeader> head(TheRomHeader);
	test(TheFile.Write(head)==KErrNone);
	TheCurrentBase=UserSvr::RomHeaderAddress()+sizeof(TheRomHeader);
//
	test.Next(_L("Write directories"));
	TheLevel=(-1);
	TheRootDir->WriteDirs();
	test(TheLevel==(-1));
//
	test.Next(_L("Write files"));
	TheLevel=(-1);
	TheRootDir->WriteFiles();
	test(TheLevel==(-1));
//
	test.End();
	}

GLDEF_C void CallTestsL(void)
//
// Test the file server.
//
    { 	
	test.Title();

#if defined(__WINS__) 
	test.Printf(_L("Running on WINS.  Skipping ROM test"));
	return;
#endif
//
	test.Start(_L("Testing rom"));

	TInt r=TheFs.Delete(TheFileName);
	if (r==KErrAccessDenied)
		{
		test.Printf(_L("Error: Cannot access %S"),&TheFileName);
		test.End();
		return;
		}
	test_Value(r, r == KErrNone || r==KErrNotFound);
//
	test.Next(_L("Generating ROM image"));
	TRAP(r,buildRomImageL());
	test_KErrNone(r);
//
	test.Next(_L("Basing the rom image"));
	baseRomImage();
//
	TBuf<0x80> b=_L("Writing ROM file ");
	b+=TheFileName;
	test.Next(b);
	r=TheFile.Replace(TheFs,TheFileName,EFileStream|EFileWrite);
	test_KErrNone(r);
	writeRomImage();
	TheFile.Close();
	delete TheRootDir;

	test.End();
	return;
    }

