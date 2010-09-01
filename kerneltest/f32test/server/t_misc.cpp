// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_misc.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

// If there is an NFE media driver present, then because of the way EDeleteNotify requests work,
// the data retrieved from a deleted file will not be a buffer full of zero's, but instead a buffer
// full of decrypted zero's
#define __NFE_MEDIA_DRIVER_PRESENT__

#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

GLDEF_D RTest test(_L("T_MISC"));

const TUint KBufLength = 0x100;

LOCAL_C void Test1()
//
// Open, write to and read from a file
//
	{

	test.Next(_L("Open, write to and read from a file"));
	TInt r=TheFs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	RFile file;
	r=file.Create(TheFs,_L("Hello.Wld"),EFileWrite);
	test(r==KErrNone);
	r=file.Write(_L8("Hello World"),11);
	test(r==KErrNone);
	file.Close();

	r=file.Open(TheFs,_L("Hello.Wld"),EFileRead);
	test(r==KErrNone);
	TBuf8<256> buf;
	r=file.Read(buf);
	test(r==KErrNone);
	test(buf==_L8("Hello World"));
	file.Close();
	}

LOCAL_C void Test2()
//
// Open and read from a file
//
	{

	test.Next(_L("Open and read from a file"));
	TInt r=TheFs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	RFile file;
	r=file.Open(TheFs,_L("Hello.Wld"),EFileRead);
	test(r==KErrNone);
	TBuf8<256> buf;
	r=file.Read(buf);
	test(r==KErrNone);
	test(buf==_L8("Hello World"));
	file.Close();
	r=TheFs.Delete(_L("HELLO.WLD"));
	test(r==KErrNone);
	}

LOCAL_C void Test3()
//
// Create nested directories
//
	{

	test.Next(_L("Create nested directories"));
	TInt r=TheFs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	TheFs.ResourceCountMarkStart();
//
	r=TheFs.MkDir(_L("\\F32-TST\\LEFT\\A.B"));
	test(r==KErrNone || r==KErrAlreadyExists);
	r=TheFs.MkDir(_L("\\F32-TST\\RIGHT\\"));
	test(r==KErrNone || r==KErrAlreadyExists);
//
	r=TheFs.MkDir(_L("\\F32-TST\\LEFT\\ONE\\"));
	test(r==KErrNone || r==KErrAlreadyExists);
	r=TheFs.MkDir(_L("\\F32-TST\\LEFT\\TWO\\"));
	test(r==KErrNone || r==KErrAlreadyExists);
	r=TheFs.MkDir(_L("\\F32-TST\\LEFT\\THREE\\"));
	test(r==KErrNone || r==KErrAlreadyExists);
	r=TheFs.MkDir(_L("\\F32-TST\\LEFT\\TWO\\BOTTOM\\"));
	test(r==KErrNone || r==KErrAlreadyExists);
//
	r=TheFs.MkDirAll(_L("\\F32-TST\\RIGHT\\TOP\\MID\\BOT\\"));
	test(r==KErrNone || r==KErrAlreadyExists);
	}

LOCAL_C void Test4()
//
// Test returned error values
//
	{

	test.Next(_L("Test returned error values"));
	TInt r=TheFs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	TheFs.ResourceCountMarkStart();
//
	r=TheFs.MkDir(_L("\\"));
	test(r==KErrAlreadyExists);
	r=TheFs.MkDir(_L("\\LEFT"));
	test(r==KErrAlreadyExists);
	r=TheFs.MkDir(_L("\\F32-TST\\LEFT\\"));
	test(r==KErrAlreadyExists);
	r=TheFs.MkDir(_L("\\F32-TST\\LEFT\\..\\NEWDIR\\"));
	test(r==KErrBadName);
	r=TheFs.MkDir(_L("\\F32-TST\\NEWDIR\\SUBDIR\\"));
	test(r==KErrPathNotFound);
//
	r=TheFs.RmDir(_L("\\"));
	test(r==KErrInUse);
	r=TheFs.RmDir(_L("\\PROG"));
	test(r==KErrInUse);
	r=TheFs.RmDir(_L("\\F32-TST\\"));
	test(r==KErrInUse);


	RDir dir;
	r=dir.Open(TheFs,_L("V:\\asdf"),KEntryAttNormal);
	test(r==KErrNone || r==KErrNotReady || r==KErrNotFound);
	if (r==KErrNone)
		dir.Close();
	r=dir.Open(TheFs,_L("L:\\asdf"),KEntryAttNormal);
	test(r==KErrNone || r==KErrNotReady || r==KErrNotFound);
	dir.Close();
//
	TEntry entry;
	r=TheFs.Entry(_L("z:\\NOTEXiSTS\\file.txt"),entry);
	test(r==KErrPathNotFound);
	r=TheFs.Entry(_L("z:\\NOTEXiSTS\\"),entry);
	test(r==KErrNotFound);
	r=TheFs.Entry(_L("z:\\SYSTEM\\"),entry);
	test(r==KErrNone);
	r=TheFs.Entry(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin)?_L("z:\\SYS\\BIN\\ESHELL.EXE"):_L("z:\\SYSTEM\\BIN\\ESHELL.EXE"),entry);
	test(r==KErrNone);

	r=dir.Open(TheFs,_L("\\*"),NULL);
	test(r==KErrNone);
	TEntry dirEntry;
	r=dir.Read(dirEntry);
	test(r==KErrNone || r==KErrEof);
	if (r==KErrNone)
		test.Printf(_L("%S\n"),&dirEntry.iName);
	dir.Close();

	r=dir.Open(TheFs,_L("A:\\*"),NULL);
	test(r==KErrNotReady || r==KErrNone);
	dir.Close();
	}

LOCAL_C void Test5()
//
// Read files directly from the rom
//

	{
	test.Next(_L("Read Files directly from the rom"));

	TInt pos=0;
	TInt r;
	_LIT(KTFileCpp, "Z:\\test\\T_FILE.CPP");
	_LIT(KTFsrvCpp, "Z:\\test\\T_FSRV.CPP");

	if ( TheFs.IsFileInRom(KTFileCpp) != NULL && TheFs.IsFileInRom(KTFsrvCpp) != NULL )
		{
		RFile f;
		r=f.Open(TheFs,KTFileCpp,EFileRead);
		test(r==KErrNone);
		r=f.Seek(ESeekAddress,pos);
		TText8* ptrPos=*(TText8**)&pos;
		test(r==KErrNone);
		TBuf8<1024> readBuf;
		r=f.Read(readBuf);
		test(r==KErrNone);
		test(readBuf.Length()==readBuf.MaxLength());
		TPtrC8 memBuf(ptrPos,readBuf.Length());
		test(memBuf==readBuf);

		ptrPos+=9913;
		pos=9913;
		r=f.Seek(ESeekStart,pos);
		test(r==KErrNone);
		readBuf.SetLength(0);
		r=f.Read(readBuf);
		test(r==KErrNone);
		test(readBuf.Length()==readBuf.MaxLength());
		memBuf.Set(ptrPos,readBuf.Length());
		test(memBuf==readBuf);

		RFile f2;
		pos=10;
		r=f2.Open(TheFs,KTFsrvCpp,EFileRead);

		test(r==KErrNone);
		r=f2.Seek(ESeekAddress,pos);
		ptrPos=*(TText8**)&pos;
		test(r==KErrNone);
		readBuf.SetLength(0);
		pos=10;
		r=f2.Seek(ESeekStart,pos);
		test(r==KErrNone);
		r=f2.Read(readBuf);
		test(r==KErrNone);
		test(readBuf.Length()==readBuf.MaxLength());
		memBuf.Set(ptrPos,readBuf.Length());
		test(memBuf==readBuf);

		ptrPos+=2445;
		pos=10+2445;
		r=f2.Seek(ESeekStart,pos);
		test(r==KErrNone);
		readBuf.SetLength(0);
		r=f2.Read(readBuf);
		test(r==KErrNone);
		test(readBuf.Length()==readBuf.MaxLength());
		memBuf.Set(ptrPos,readBuf.Length());
		test(memBuf==readBuf);

		pos=0;
		r=f.Seek(ESeekAddress,pos);
		ptrPos=*(TText8**)&pos;
		test(r==KErrNone);
		readBuf.SetLength(0);
		pos=0;
		r=f.Seek(ESeekStart,pos);
		test(r==KErrNone);
		r=f.Read(readBuf);
		test(r==KErrNone);
		test(readBuf.Length()==readBuf.MaxLength());
		memBuf.Set(ptrPos,readBuf.Length());
		test(memBuf==readBuf);

		ptrPos+=5245;
		pos=5245;
		r=f.Seek(ESeekStart,pos);
		test(r==KErrNone);
		readBuf.SetLength(0);
		r=f.Read(readBuf);
		test(r==KErrNone);
		test(readBuf.Length()==readBuf.MaxLength());
		memBuf.Set(ptrPos,readBuf.Length());
		test(memBuf==readBuf);

		f.Close();
		f2.Close();
		}
	}

LOCAL_C void Test6()
//
// Test rom return values
//
	{
	test.Next(_L("Test rom return values"));

	RFile f;
	TInt r=f.Replace(TheFs,_L("Z:\\Test\\T_Fsrv.Cpp"),EFileRead);
	test(r==KErrAccessDenied);
	r=f.Create(TheFs,_L("Z:\\Test\\newT_Fsrv.Cpp"),EFileRead);
	test(r==KErrAccessDenied);
	r=f.Open(TheFs,_L("Z:\\Test\\T_Fsrv.Cpp"),EFileRead);
	test(r==KErrNone);
	f.Close();
	r=f.Open(TheFs,_L("Z:\\Test\\T_Fsrv.Cpp"),EFileRead|EFileWrite);
	test(r==KErrAccessDenied);
	}

LOCAL_C void Test7()
//
// Test cache
//
	{

	test.Next(_L("Test cache updated when writing to a file"));
	TUidType uid1(TUid::Uid(1),TUid::Uid(2),TUid::Uid(3));
	TBuf8<32> contents1=_L8("asdf asdf asdf");
	TBuf<32> file1=_L("\\TMISC\\CACHE.FILE");
	MakeFile(file1,uid1,contents1);

	TEntry entry;
	TInt r=TheFs.Entry(file1,entry);
	test(r==KErrNone);
	test(entry.iType==uid1);

	TUidType uid2(TUid::Uid(4),TUid::Uid(5),TUid::Uid(6));
	TCheckedUid checkedUid(uid2);
	TPtrC8 uidData((TUint8*)&checkedUid,sizeof(TCheckedUid));
	RFile f;
	r=f.Open(TheFs,file1,EFileRead|EFileWrite);
	test(r==KErrNone);
	r=f.Write(uidData);
	test(r==KErrNone);
	r = f.Flush();
	test(r==KErrNone);

	r=TheFs.Entry(file1,entry);
	test(r==KErrNone);
	test(entry.iType==uid2);

	f.Close();
	r=TheFs.Entry(file1,entry);
	test(r==KErrNone);
	test(entry.iType==uid2);
	}

LOCAL_C void Test8()
//
// Test IsValidName
//
	{
	test.Next(_L("Test RFs::IsValidName(TDesC)"));

	// tests calling IsValidName() with invalid name as first call to session
	// see defect EXT-57KH9K
	_LIT(KInvalidName, "test\\i1.jpg");
	RFs fs;
	test(KErrNone==fs.Connect());
	test(fs.IsValidName(KInvalidName)==EFalse);
	fs.Close();

	test(TheFs.IsValidName(_L("*"))==EFalse);
	test(TheFs.IsValidName(_L("?"))==EFalse);
	test(TheFs.IsValidName(_L(">"))==EFalse);
	test(TheFs.IsValidName(_L("<"))==EFalse);
	test(TheFs.IsValidName(_L(":"))==EFalse);
	test(TheFs.IsValidName(_L("\""))==EFalse);
	test(TheFs.IsValidName(_L("/"))==EFalse);
	test(TheFs.IsValidName(_L("|"))==EFalse);
	test(TheFs.IsValidName(_L("\\"))==EFalse);

	test(TheFs.IsValidName(_L("xx*yy"))==EFalse);
	test(TheFs.IsValidName(_L("xx?yy"))==EFalse);
	test(TheFs.IsValidName(_L("xx>yy"))==EFalse);
	test(TheFs.IsValidName(_L("xx<yy"))==EFalse);
	test(TheFs.IsValidName(_L("xx:yy"))==EFalse);
	test(TheFs.IsValidName(_L("xx\"yy"))==EFalse);
	test(TheFs.IsValidName(_L("xx/yy"))==EFalse);
	test(TheFs.IsValidName(_L("xx|yy"))==EFalse);

	test(TheFs.IsValidName(_L("C:\\*\\group\\release.txt"))==EFalse);
	test(TheFs.IsValidName(_L("C:\\?\\group\\release.txt"))==EFalse);
	test(TheFs.IsValidName(_L("C:\\>\\group\\release.txt"))==EFalse);
	test(TheFs.IsValidName(_L("C:\\<\\group\\release.txt"))==EFalse);
	test(TheFs.IsValidName(_L("C:\\:\\group\\release.txt"))==EFalse);
	test(TheFs.IsValidName(_L("C:\\\"\\group\\release.txt"))==EFalse);
	test(TheFs.IsValidName(_L("C:\\/\\group\\release.txt"))==EFalse);
	test(TheFs.IsValidName(_L("C:\\|\\group\\release.txt"))==EFalse);

	test(TheFs.IsValidName(_L(""))==EFalse); // must be a name or extension present
	test(TheFs.IsValidName(_L(".ext")));
	test(TheFs.IsValidName(_L("C:\\asdf.blarg\\"))==EFalse);
	test(TheFs.IsValidName(_L("\\"))==EFalse);

	test(TheFs.IsValidName(_L("as(){}@~#;!\xA3$%^&()df.blarg")));	//	Valid names
	test(TheFs.IsValidName(_L("C:\\asdf.blarg\\asdf.blarg")));	//	Valid names
	test(TheFs.IsValidName(_L("\'")));							//	Valid names

	test.Next(_L("Test RFs::IsValidName(TDesC, TDes) overload"));

	TText testChar;
	test(TheFs.IsValidName(_L("*"),testChar)==EFalse);
	test(testChar=='*');
	test(TheFs.IsValidName(_L("?"),testChar)==EFalse);
	test(testChar=='?');
	test(TheFs.IsValidName(_L(">"),testChar)==EFalse);
	test(testChar=='>');
	test(TheFs.IsValidName(_L("<"),testChar)==EFalse);
	test(testChar=='<');
	test(TheFs.IsValidName(_L(":"),testChar)==EFalse);
	test(testChar==':');
	test(TheFs.IsValidName(_L("\""),testChar)==EFalse);
	test(testChar=='\"');	//	Tests that " is illegal
	test(TheFs.IsValidName(_L("/"),testChar)==EFalse);
	test(testChar=='/');
	test(TheFs.IsValidName(_L("|"),testChar)==EFalse);
	test(testChar=='|');
	test(TheFs.IsValidName(_L("\\"),testChar)==EFalse);
	test(testChar==' ');

	test(TheFs.IsValidName(_L("xx*yy"),testChar)==EFalse);
 	test(testChar=='*');
	test(TheFs.IsValidName(_L("xx?yy"),testChar)==EFalse);
	test(testChar=='?');
	test(TheFs.IsValidName(_L("xx>yy"),testChar)==EFalse);
	test(testChar=='>');
	test(TheFs.IsValidName(_L("xx<yy"),testChar)==EFalse);
	test(testChar=='<');
	test(TheFs.IsValidName(_L("xx:yy"),testChar)==EFalse);
	test(testChar==':');
	test(TheFs.IsValidName(_L("xx\"yy"),testChar)==EFalse);
	test(testChar=='\"');	//	Tests that " is illegal
	test(TheFs.IsValidName(_L("xx/yy"),testChar)==EFalse);
	test(testChar=='/');
	test(TheFs.IsValidName(_L("xx|yy"),testChar)==EFalse);
	test(testChar=='|');

	test(TheFs.IsValidName(_L("C:\\*\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='*');
	test(TheFs.IsValidName(_L("C:\\?\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='?');
	test(TheFs.IsValidName(_L("C:\\..\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='.');	//	Only one "." returned however many are in filename
	test(TheFs.IsValidName(_L("C:\\.\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='.');
	test(TheFs.IsValidName(_L("C:\\>\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='>');
	test(TheFs.IsValidName(_L("C:\\<\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='<');
	test(TheFs.IsValidName(_L("C:\\HelloWorld\\:\\group\\release.txt"),testChar)==EFalse);
	test(testChar==':');


	test(TheFs.IsValidName(_L("C::\\group\\release.txt"),testChar)==EFalse);
	test(testChar==':');
	test(TheFs.IsValidName(_L(">\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='>');
	test(TheFs.IsValidName(_L("C|group\\release.txt"),testChar)==EFalse);
	test(testChar=='|');
	test(TheFs.IsValidName(_L("C\\|\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='|');

	test(TheFs.IsValidName(_L("\\>"),testChar)==EFalse);
	test(testChar=='>');
	test(TheFs.IsValidName(_L("C:\\|group\\release.txt"),testChar)==EFalse);
	test(testChar=='|');

	test(TheFs.IsValidName(_L("C:\\\"\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='\"');
	test(TheFs.IsValidName(_L("C:\\/\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='/');
	test(TheFs.IsValidName(_L("C:\\|\\group\\release.txt"),testChar)==EFalse);
	test(testChar=='|');
	test(TheFs.IsValidName(_L("C:\\ \\group\\release.txt"),testChar)==EFalse); // must be a name or extension present
	test(testChar==' ');

//	Test that \ is not allowed in filenames
	TFileName filename;
	filename=_L("C:\\HelloWorld\\\\\\group\\release.txt");
	TPtr pChar(&testChar,sizeof(TText),sizeof(TText));
	test(TheFs.IsValidName(filename,testChar)==EFalse);
	test(pChar.Find(_L("\\"))!=KErrNotFound);
	filename=_L("C:\\\\\\group\\release.txt");
	test(TheFs.IsValidName(filename,testChar)==EFalse);
	test(pChar.Find(_L("\\"))!=KErrNotFound);
	filename=_L("C:\\Hello World\\group\\release.txt");
	filename[8]=KPathDelimiter;	//	Makes C:\\Hello World\\group\\release.txt
	test(TheFs.IsValidName(filename,testChar));
	filename=_L("C:\\HelloWorld\\::\\group\\release.txt");
	test(TheFs.IsValidName(filename,testChar)==EFalse);
	test(pChar.Find(_L(":"))!=KErrNotFound);

	filename=_L("C:\\>>\\group\\release.txt");
	test(TheFs.IsValidName(filename,testChar)==EFalse);
	test(pChar.Find(_L(">"))!=KErrNotFound);

	test(TheFs.IsValidName(_L(""),testChar)==EFalse); // Must be a name
	test(testChar==' ');
	test(TheFs.IsValidName(_L(".ext"),testChar));
	test(TheFs.IsValidName(_L("C:\\asdf.blarg\\"),testChar)==EFalse);
	test(testChar==' ');	//	Must be a name else testChar is set to blank
	test(TheFs.IsValidName(_L("C:\\asdf.blarg"),testChar));

	test(TheFs.IsValidName(_L("C:\\asdf..blarg\\"),testChar)==EFalse);
	test(testChar==' ');	//	Must be a name else testChar is set to blank
	test(TheFs.IsValidName(_L("C:\\asdf..blarg"),testChar));

	test(TheFs.IsValidName(_L("\\"),testChar)==EFalse);
	test(testChar==' ');

//	Test multiple evil characters - parsing occurs from right to left
//	except that wildcarded characters take priority and are picked out first
	test(TheFs.IsValidName(_L("abc>def|ghi?jkl:mno<pqr*stu"),testChar)==EFalse);
	test(testChar=='*');
	test(TheFs.IsValidName(_L("abc>def|ghi<jkl:mno?pqr"),testChar)==EFalse);
	test(testChar=='?');
	test(TheFs.IsValidName(_L("abc>def|ghi<jkl:mno"),testChar)==EFalse);
	test(testChar==':');
	test(TheFs.IsValidName(_L("abc>def|ghi<jkl"),testChar)==EFalse);
	test(testChar=='<');
	test(TheFs.IsValidName(_L("abc>def|ghi"),testChar)==EFalse);
	test(testChar=='|');
	test(TheFs.IsValidName(_L("abc>def"),testChar)==EFalse);
	test(testChar=='>');

	test(!TheFs.IsValidName(_L("C:\\v123456.."),testChar));				//	Valid name
	test(TheFs.IsValidName(_L("abc"),testChar));						//	Valid name
	test(TheFs.IsValidName(_L("as(){}@~#;!\xA3$%^&()df.blarg"),testChar));	//	Valid name
	test(TheFs.IsValidName(_L("C:\\asdf.blarg\\asdf.blarg"),testChar));	//	Valid name
	test(TheFs.IsValidName(_L("\'"),testChar));							//	Valid name

	//PDEF133084: The wild character in the extension was not being detected.
	_LIT( KTestString, "E:\\My Videos\\Downloads\\1\\1\\Name.3gp?" );
    TBuf<50> path;
    path = KTestString;
    TBool validName( EFalse );
    TText badChar;
    TInt badCharLoc( KErrNotFound );

    while ( ! validName )
        {
        validName = TheFs.IsValidName( path, badChar );

        if ( ! validName )
            {
            badCharLoc = path.LocateReverse( badChar );

            if ( KErrNotFound != badCharLoc )
                {
                path[badCharLoc] = '_';
                }
            }
        }
	}

LOCAL_C void Test9()
//
// Test IsFileInRom
//
	{

	test.Next(_L("Test RFs::IsFileInRom"));

	CFileMan* fMan=CFileMan::NewL(TheFs);
	TInt r=fMan->Copy(_L("Z:\\TEST\\T_FILE.CPP"),_L("C:\\T_FILE.CPP"));
	test(r==KErrNone || r==KErrAccessDenied);
	delete fMan;
	TUint8* addr=TheFs.IsFileInRom(_L("C:\\ESHELL.EXE"));
	test(addr==NULL);
	addr=TheFs.IsFileInRom(_L("Z:\\TEST\\T_FILE.CPP"));
	if (addr!=NULL)
		{
		test(addr!=NULL);
		TPtrC8 startOfFile(addr,12);
		test(startOfFile==_L8("// Copyright"));
		}
	else
		{
		test (addr==NULL);
		}
	}

LOCAL_C void Test10()
//
// Test drive names
//
	{

	test.Next(_L("Test Drive Names"));
	TFileName driveName;
	TInt i;
	for(i=0;i<KMaxDrives;i++)
		{
		TInt r=TheFs.GetDriveName(i,driveName);
		test(r==KErrNone);
		if (driveName.Length())
			test.Printf(_L("Default name of %c: == %S\n"),'A'+i,&driveName);
		}

	TBuf<64> drive0=_L("Dilbert");
	TBuf<64> drive4=_L("Dogbert");
	TBuf<64> drive17=_L("Flibble");
	TBuf<64> drive25=_L("RAMDRIVE");
	TInt r=TheFs.SetDriveName(0,drive0);
	test(r==KErrNone);
	r=TheFs.SetDriveName(4,drive4);
	test(r==KErrNone);
	r=TheFs.SetDriveName(17,drive17);
	test(r==KErrNone);
	r=TheFs.SetDriveName(25,drive25);
	test(r==KErrNone);

	r=TheFs.GetDriveName(0,driveName);
	test(r==KErrNone);
	test(driveName==drive0);
	r=TheFs.GetDriveName(4,driveName);
	test(r==KErrNone);
	test(driveName==drive4);
	r=TheFs.GetDriveName(17,driveName);
	test(r==KErrNone);
	test(driveName==drive17);
	r=TheFs.GetDriveName(25,driveName);
	test(r==KErrNone);
	test(driveName==drive25);

	drive0=_L("askdjflsdfourewoqiuroiuaksjdvx,cvsdhwjhjhalsjhfshfkjhslj");
	r=TheFs.SetDriveName(0,drive0);
	test(r==KErrNone);
	r=TheFs.GetDriveName(0,driveName);
	test(r==KErrNone);
	test(driveName==drive0);

//	Test with illegal characters in drive name
	drive0=_L("Dil>bert");
	drive4=_L("Dog?bert");
	drive17=_L("Fli*bble");
	drive25=_L("RAMD//RIVE");

	r=TheFs.SetDriveName(0,drive0);
	test(r==KErrBadName);
	r=TheFs.SetDriveName(4,drive4);
	test(r==KErrBadName);
	r=TheFs.SetDriveName(17,drive17);
	test(r==KErrBadName);
	r=TheFs.SetDriveName(25,drive25);
	test(r==KErrBadName);

//	Test that it is OK to set the name to no characters

	drive0=_L("");
	drive4=_L("");
	drive17=_L("");
	drive25=_L("");

	r=TheFs.SetDriveName(0,drive0);
	test(r==KErrNone);
	r=TheFs.SetDriveName(4,drive4);
	test(r==KErrNone);
	r=TheFs.SetDriveName(17,drive17);
	test(r==KErrNone);
	r=TheFs.SetDriveName(25,drive25);
	test(r==KErrNone);

	r=TheFs.GetDriveName(0,driveName);
	test(r==KErrNone);
	test(driveName==drive0);
	r=TheFs.GetDriveName(4,driveName);
	test(r==KErrNone);
	test(driveName==drive4);
	r=TheFs.GetDriveName(17,driveName);
	test(r==KErrNone);
	test(driveName==drive17);
	r=TheFs.GetDriveName(25,driveName);
	test(r==KErrNone);
	test(driveName==drive25);


	}

LOCAL_C void Test11()
//
// Miscellaneous tests
//
	{

	test.Next(_L("Miscellaneous tests"));
	TVolumeInfo vol;
	TInt r=TheFs.Volume(vol);
	test.Printf(_L("VolumeName = %S\n"),&vol.iName);
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\asdfasdf.\\"));
	test(r==KErrBadName);
	r=TheFs.MkDir(_L("\\asdfasdf.\\"));
	test(r==KErrBadName);
	}

LOCAL_C void Test12()
//
// Test SetNotifyUser and GetNotifyUser
//
	{

	test.Next(_L("Test Set and GetNotifyUser"));
	TBool notifyState=TheFs.GetNotifyUser();
	test(notifyState);
	notifyState=EFalse;
	TheFs.SetNotifyUser(notifyState);
	notifyState=TheFs.GetNotifyUser();
	test(notifyState==EFalse);
	notifyState=ETrue;
	TheFs.SetNotifyUser(notifyState);
	notifyState=TheFs.GetNotifyUser();
	test(notifyState);
	}

LOCAL_C void Test13()
//
// Test return values from RFs::Volume on cf-cards
//
	{

	test.Next(_L("Test RFs::Volume"));
	TVolumeInfo vol;
	TInt r=TheFs.Volume(vol,EDriveB);
	test(r==KErrNotReady || r==KErrNone || KErrPathNotFound);
	test.Printf(_L("RFs::Volume EDriveB returned %d\n"),r);

	r=TheFs.Volume(vol,EDriveC);
	test(r==KErrNotReady || r==KErrNone || KErrPathNotFound);
	test.Printf(_L("RFs::Volume EDriveC returned %d\n"),r);

	r=TheFs.Volume(vol,EDriveD);
	test(r==KErrNotReady || r==KErrNone || KErrPathNotFound);
	test.Printf(_L("RFs::Volume EDriveD returned %d\n"),r);

	r=TheFs.Volume(vol,EDriveE);
	test(r==KErrNotReady || r==KErrNone || KErrPathNotFound);
	test.Printf(_L("RFs::Volume EDriveE returned %d\n"),r);

	r=TheFs.Volume(vol,EDriveF);
	test(r==KErrNotReady || r==KErrNone || KErrPathNotFound);
	test.Printf(_L("RFs::Volume EDriveF returned %d\n"),r);
	}


void    DoTest14(TInt aDrvNum);
TInt    CreateStuffedFile(RFs& aFs, const TDesC& aFileName, TUint aFileSize);
TInt    CreateEmptyFile(RFs& aFs, const TDesC& aFileName, TUint aFileSize);
TBool   CheckFileContents(RFs& aFs, const TDesC& aFileName);
#ifndef __NFE_MEDIA_DRIVER_PRESENT__
TBool   CheckBufferContents(const TDesC8& aBuffer, TUint aPrintBaseAddr=0);
#endif

/**
Testing unallocated data initialization vulnerability in RFile
This test is performed on RAM drives and non-removable media that supports DeleteNotify (KMediaAttDeleteNotify flag)
e.g. XSR NAND
*/
LOCAL_C void Test14()
{
	TInt nRes;

	test.Next(_L("Testing unallocated data initialization vulnerability in RFile"));

	TDriveList driveList;
	TDriveInfo driveInfo;

	//-- 1. get drives list
	nRes=TheFs.DriveList(driveList);
    test(nRes == KErrNone);

	//-- 2. walk through all drives, performing the test only on suitable ones
	for (TInt drvNum=0; drvNum<KMaxDrives; ++drvNum)
    {
	    if(!driveList[drvNum])
	        continue;   //-- skip unexisting drive

	    //-- get drive info
	    test(TheFs.Drive(driveInfo, drvNum) == KErrNone);

	    //-- select a suitable drive for the testing. It shall be RAM drive, of FLASH but not removable
	    //-- and not read only, if it is FLASH, it shall support "Delete Notify" facility
        switch(driveInfo.iType)
        {
        //-- RAM drive, OK
        case EMediaRam:
        break;

        //-- FLASH drive, OK
        case EMediaFlash:
        case EMediaNANDFlash:
            if(driveInfo.iMediaAtt & KMediaAttDeleteNotify)
                break; //-- this type of media shall support DeleteNotify flag, otherwise this test is inconsistent
            else continue;

        //break; //unreacable

        default:
            continue;
        }//switch(driveInfo.iType)

		if (driveInfo.iDriveAtt	& KDriveAttSubsted)
			{
			// skip subst drives.
			continue;
			}

        TBool readOnly = driveInfo.iMediaAtt & KMediaAttWriteProtected;
        if(readOnly)
            continue; //-- nothing to do, can't create any file etc.

        //-- skip test on the emulator's C: drive, doesn't make any sense because
        //-- in this case we deal with WIN32 API and filesystem.
        #ifdef __WINS__
        if(drvNum == 2)
        {
             test.Printf(_L("Skipping test on emulator's C: drive\n"));
             continue;
        }
        #endif

        DoTest14(drvNum);

    }// for (TInt drvNum=0; ...

}

//--------------------------------------------------------

/**
    Actually perform the test on a drive aDrvNum.
    @param  aDrvNum drive number
*/
void DoTest14(TInt aDrvNum)
{

    TFileName fileName;
    fileName.Format(_L("Testing drive %c:"), 'A'+aDrvNum);
    test.Next(fileName);

    const TInt  KFileSize = 0x1000; //-- size of the files t be created
    TInt        nRes;

    fileName.Format(_L("%c:\\TestFile.bin"), aDrvNum+'A');
    TheFs.Delete(fileName); //-- just in case

    //==============================
    //== Scenario 1.
    //== Create an empty file; AllocateSingleClusterL, ExtendClusterListL will be involved.
    //== Check that the file doesn't contain any meaningful information
    //==============================
    test.Printf(_L("Testing scenario 1\n"));

    //-- 1. create an empty file
    nRes = CreateEmptyFile(TheFs, fileName, KFileSize);
    test(nRes == KErrNone);

#ifndef __NFE_MEDIA_DRIVER_PRESENT__	// can't easily check for illegitimate information if drive is encrypted
    //-- 1.1  check that this file doesn't contain illegitimate information.
    nRes = CheckFileContents(TheFs, fileName);
    test(nRes == KErrNone);
#endif

    //-- 1.2 delete the empty file
    nRes = TheFs.Delete(fileName);
    test(nRes == KErrNone);

    //==============================
    //== Scenario 2.
    //== Create file, filling it with some pattern.
    //== Delete this file, FreeClusterListL() will be involved.
    //== Create an empty file supposedly of the place of just deleted one
    //== Check that the file doesn't contain any meaningful information
    //==============================
    test.Printf(_L("Testing scenario 2\n"));

    //-- 2. create file filled with some data pattern
    nRes = CreateStuffedFile(TheFs, fileName, KFileSize);
    test(nRes == KErrNone);

    //-- 2.1 delete this file
    TheFs.Delete(fileName);

    //-- 2.1 create an empty file on the place of just deleted one (hopefully)
    nRes = CreateEmptyFile(TheFs, fileName, KFileSize);
    test(nRes == KErrNone);

    //-- 2.2  check that this file doesn't contain illegitimate information.
    nRes = CheckFileContents(TheFs, fileName);
    test(nRes == KErrNone);

    //-- 2.3 delete this file
    TheFs.Delete(fileName);

}

LOCAL_C void Test15()
//
// Test IsValidName
//
	{
	test.Next(_L("Test RFs::IsValidName(TDesC& ,RFs::TNameValidParam& )"));
	TBool useDefaultSessionPath = EFalse;
	//tests under this loop are run twice
	//first, when the sessionPath is not used.
	//second, when the sessionPath is used.
	for(TInt i = 0; i<2; i++)
		{
		RFs::TNameValidParam param(useDefaultSessionPath);
		test(TheFs.IsValidName(_L("*"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 1);
	
		test(TheFs.IsValidName(_L("?"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 1);
		
		test(TheFs.IsValidName(_L(">"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 1);
	
		test(TheFs.IsValidName(_L("<"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 1);
		
		test(TheFs.IsValidName(_L(":"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 1);
	
		test(TheFs.IsValidName(_L("\""),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 1);
	
		test(TheFs.IsValidName(_L("/"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 1);
	
		test(TheFs.IsValidName(_L("|"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 1);
	
		test(TheFs.IsValidName(_L("xx*yy"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);
		
		test(TheFs.IsValidName(_L("xx?yy"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);
	
		test(TheFs.IsValidName(_L("xx>yy"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);
	
		test(TheFs.IsValidName(_L("xx<yy"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);

		test(TheFs.IsValidName(_L("xx:yy"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);
		
		test(TheFs.IsValidName(_L("xx\"yy"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);
		
		test(TheFs.IsValidName(_L("xx/yy"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);
			
		test(TheFs.IsValidName(_L("xx|yy"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);
			
		test(TheFs.IsValidName(_L("C:\\*\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
	
		test(TheFs.IsValidName(_L("C:\\?\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
	
		test(TheFs.IsValidName(_L("C:\\..\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
	
		test(TheFs.IsValidName(_L("C:\\.\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
		
		test(TheFs.IsValidName(_L("C:\\>\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
	
		test(TheFs.IsValidName(_L("C:\\<\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
	
		test(TheFs.IsValidName(_L("C:\\HelloWorld\\:\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 15);
		
		test(TheFs.IsValidName(_L("C::\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);
			
		test(TheFs.IsValidName(_L(">\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 1);
			
		test(TheFs.IsValidName(_L("C|group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 2);
			
		test(TheFs.IsValidName(_L("C\\|\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 3);
			
		test(TheFs.IsValidName(_L("\\>"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 2);
			
		test(TheFs.IsValidName(_L("C:\\|group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
				
		test(TheFs.IsValidName(_L("C:\\\"\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
		
		test(TheFs.IsValidName(_L("C:\\/\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
	
		test(TheFs.IsValidName(_L("C:\\|\\group\\release.txt"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
		
		test(TheFs.IsValidName(_L("C:\\ \\group\\release.txt"),param)==EFalse);//intermediate directory names cannot be blank
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadName);
			
		//	Test that \ is not allowed in filenames
		TFileName filename;
		filename=_L("C:\\HelloWorld\\\\\\group\\release.txt");
		
		test(TheFs.IsValidName(filename,param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 22);
		
		filename=_L("C:\\\\\\group\\release.txt");
		test(TheFs.IsValidName(filename,param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 11);
		
		filename=_L("C:\\Hello World\\group\\release.txt");
		filename[8]=KPathDelimiter;
		test(TheFs.IsValidName(filename,param));
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
		
		filename=_L("C:\\HelloWorld\\::\\group\\release.txt");
		test(TheFs.IsValidName(filename,param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 16);
			
		filename=_L("C:\\>>\\group\\release.txt");
		test(TheFs.IsValidName(filename,param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 5);
		
		test(TheFs.IsValidName(_L(""),param)==EFalse); // Must be a name
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadName);
		
		test(TheFs.IsValidName(_L(".ext"),param));
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
		
		test(TheFs.IsValidName(_L("C:\\asdf.blarg"),param));
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
		
		test(TheFs.IsValidName(_L("C:\\asdf..blarg"),param));
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
		
		//	Test multiple evil characters - parsing occurs from right to left
		//	except that wildcarded characters take priority and are picked out first
		
		test(TheFs.IsValidName(_L("abc>def|ghi?jkl:mno<pqr*stu"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 24);
	
		test(TheFs.IsValidName(_L("abc>def|ghi<jkl:mno?pqr"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 20);
		
		test(TheFs.IsValidName(_L("abc>def|ghi<jkl:mno"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 16);
		
		test(TheFs.IsValidName(_L("abc>def|ghi<jkl:mno"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 16);
	
		test(TheFs.IsValidName(_L("abc>def|ghi<jkl"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 12);
				
		test(TheFs.IsValidName(_L("abc>def|ghi<jkl"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 12);
	
		test(TheFs.IsValidName(_L("abc>def|ghi"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 8);
			
		test(TheFs.IsValidName(_L("abc>def|ghi"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 8);
	
		test(TheFs.IsValidName(_L("abc>def"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
	
		test(TheFs.IsValidName(_L("abc>def"),param)==EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 4);
	
		test(!TheFs.IsValidName(_L("C:\\v123456.."),param));
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 11);
		
		test(!TheFs.IsValidName(_L("C:\\v123456.."),param));
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadCharacter);
		test(param.InvalidCharPos() == 11);
	
		test(TheFs.IsValidName(_L("abc"),param));						//	Valid name
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
	
		test(TheFs.IsValidName(_L("abc"),param));						//	Valid name
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
	
		test(TheFs.IsValidName(_L("as(){}@~#;!\xA3$%^&()df.blarg"),param));	//	Valid name
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
	
		test(TheFs.IsValidName(_L("as(){}@~#;!\xA3$%^&()df.blarg"),param));	//	Valid name
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
	
		test(TheFs.IsValidName(_L("C:\\asdf.blarg\\asdf.blarg"),param));	//	Valid name
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
	
		test(TheFs.IsValidName(_L("C:\\asdf.blarg\\asdf.blarg"),param));	//	Valid name
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
	
		test(TheFs.IsValidName(_L("\'"),param));							//	Valid name
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
		
		test(TheFs.IsValidName(_L("\'"),param));							//	Valid name
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
		
		//testing directory names
		test(TheFs.IsValidName(_L("\\"),param));
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);           // Valid Name
		
		test(TheFs.IsValidName(_L("C:\\asdf.blarg\\"),param));
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);           // Valid Name
		
		
		test(TheFs.IsValidName(_L("C:\\asdf..blarg\\"),param));
		test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);           // Valid Name
		
		test(TheFs.IsValidName(_L("file1.txt\\\\"),param) == EFalse);
		test(param.ErrorCode() == RFs::TNameValidParam::ErrBadName);
	
		// test name which exceeds KMaxFileName only on prepending the session path
		_LIT(KNameLength250, "AAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAABBBBBBBBBBAAAAAAAAAA");
		if(useDefaultSessionPath)
			{
			test(TheFs.IsValidName(KNameLength250, param)==EFalse);
			test(param.ErrorCode() == RFs::TNameValidParam::ErrNameTooLong);
			break;
			}
		else
			{
			test(TheFs.IsValidName(KNameLength250, param));
			test(param.ErrorCode() == RFs::TNameValidParam::ErrNone);
			}
		useDefaultSessionPath = ETrue;
		}
	}



void TestGetMediaSerialNumber()
    {
	test.Next(_L("Test RFs::GetMediaSerialNumber"));	
    TInt theDrive;
    TInt r = TheFs.CharToDrive(gDriveToTest,theDrive);
    test(r == KErrNone);
    TMediaSerialNumber serNum;
    r = TheFs.GetMediaSerialNumber(serNum, theDrive);
	if (r) test.Printf(_L("RFs::GetMediaSerialNumber returned error %d"), r);
    test(r == KErrNotSupported || r == KErrNotReady || r == KErrNone);
    if (r == KErrNotSupported)
        {
        test.Printf(_L("MediaSerialNumber: Not Supported\n"));
        }
    else
        {
        test.Printf(_L("MediaSerialNumber: length=%d\n"), serNum.Length());
        TBuf<20> str;
        _LIT(KNumberString, "%02X");
        _LIT(KNewLine, "\n");
        TInt i;
        for (i = 0; i < serNum.Length(); i++)
            {
            str.AppendFormat(KNumberString, serNum[i]);
            if (i%8 == 7)
                {
                str.Append(KNewLine);
                test.Printf(_L("%S"), &str);
                str.SetLength(0);
                }
            }
        if (i%8 != 7)
            {
            test.Printf(KNewLine);
            }
        }
    }


//--------------------------------------------------------

/**
    Create an empty file of specified size.
    @param  aFs		    ref. to the FS
    @param  aFileName   name of the file
    @param  aFileSize   size of the file to be created
    @return    KErrNone on success, system-wide error code otherwise
*/
TInt CreateEmptyFile(RFs& aFs, const TDesC& aFileName, TUint aFileSize)
{
    RFile   file;
	TInt    nRes;

	nRes = file.Create(aFs, aFileName, EFileRead|EFileWrite);
    if(nRes != KErrNone)
        return nRes;

	nRes = file.SetSize(aFileSize);
    if(nRes != KErrNone)
        return nRes;

    file.Close();

    return KErrNone;
}

//--------------------------------------------------------

/**
    Create a file of specified size filled with some data pattern.
    @param  aFs		    ref. to the FS
    @param  aFileName   name of the file
    @param  aFileSize   size of the file to be created
    @return    KErrNone on success, system-wide error code otherwise
*/
TInt CreateStuffedFile(RFs& aFs, const TDesC& aFileName, TUint aFileSize)
{
	TInt    nRes;
    RFile   file;

	//-- create a buffer with some data
	TBuf8<KBufLength> buffer;
	buffer.SetLength(KBufLength);

    TUint i;

	for(i = 0; i < KBufLength; i++)
		buffer[i] = static_cast<TUint8> (i) ;

	//-- create a file
	nRes = file.Create(aFs, aFileName, EFileRead|EFileWrite);
    if(nRes != KErrNone)
        return nRes;

    const TUint n1 = aFileSize / KBufLength;
    const TUint n2 = aFileSize % KBufLength;

    //-- fill the file with the data from buffer
    for(i=0; i<n1; ++i)
    {
        nRes = file.Write(buffer);
        if(nRes != KErrNone)
            return nRes;
    }

    if(n2)
    {
        nRes = file.Write(buffer, n2); //-- write the rest of the data
        if(nRes != KErrNone)
            return nRes;
    }

    file.Close();

    return KErrNone;
}

//--------------------------------------------------------

/**
    Check if the specified file contains illegitimate information i.e. something different from 0x00, 0xff, 0x03, 0xcc

    @param  aFs		    ref. to the FS
    @param  aFileName   name of the file
    @return    KErrNone on success, KErrCorrupt otherwise
*/
TInt   CheckFileContents(RFs& aFs, const TDesC& aFileName)
{
	TInt    nRes = KErrNone;
    RFile   file;

	TBuf8<KBufLength> buffer;
    buffer.SetLength(0);

    //-- open the file
    nRes = file.Open(aFs, aFileName, EFileRead);
    test(nRes == KErrNone);

    //-- check file contents
    TUint nFilePos=0;
    for(;;)
    {
        //-- read data from the file into the buffer
        nRes = file.Read(buffer);
        test(nRes == KErrNone);

        if(buffer.Length() == 0)
        {
            nRes = KErrNone; //-- read all the file, no illegitimate information found
            break; //EOF
        }

#ifdef __NFE_MEDIA_DRIVER_PRESENT__
		// check the buffer doesn't contain the same pattern written to it by CreateStuffedFile()
		TUint i;
		for(i = 0; i < KBufLength; i++)
			if (buffer[i] != static_cast<TUint8> (i))
				break;
		if (i == KBufLength)
			{
            nRes = KErrCorrupt; //-- indicate that the read buffer contains illegitimate information
            break; //-- comment this out if you need a full dump of the file
			}
#else
        //-- check if the buffer contains only allowed data (RAM page initialisation data, etc. e.g. 0x00, 0xff, 0x03, 0xcc)
        if(!CheckBufferContents(buffer, nFilePos))
        {
            test.Printf(_L("\nCheckFileContents failed ! The file contains illegitimate information!\n"));
            nRes = KErrCorrupt; //-- indicate that the read buffer contains illegitimate information
            break; //-- comment this out if you need a full dump of the file
        }
#endif

        nFilePos+=buffer.Length();
    }

    file.Close();
    return nRes;
}

//--------------------------------------------------------

/**
    Check if the buffer contains illegitimate information i.e. something different from 0x00, 0xff, 0x03, 0xcc

    @param  aBuffer         buffer descriptor to check
    @param  aPrintBaseAddr  dump base address, used for dumping buffer only
    @return ETrue on success
*/
TBool CheckBufferContents(const TDesC8& aBuffer, TUint aPrintBaseAddr/*=0*/)
{
    TBool bRes = ETrue;

    //-- check if the buffer filled with allowable data (RAM page initialisation data or something similar)
    //-- but not something meaningful.
    //-- allowable bytes: 0x00, 0x03, 0xff, 0xcc
    for(TInt i=0; i<aBuffer.Size(); ++i)
    {
        TUint8 byte = aBuffer[i];
        if(byte != 0x00 && byte != 0x03 && byte != 0xff && byte != 0xcc )
        {
            bRes = EFalse;
            break;
        }
    }

    //-- dump the buffer if it contains anything different than allowed data
	if (!bRes)
	{
		for (TInt n=0; n<aBuffer.Size(); )
			{
			TBuf16<3> byteBuffer;
			TBuf16<256> lineBuffer;
			lineBuffer.Format(_L("%08X: "), aPrintBaseAddr+n);
			for (TInt m=0; m<16 && n<aBuffer.Size(); m++, n++)
				{
				byteBuffer.Format(_L("%02X "), aBuffer[n]);
				lineBuffer.Append(byteBuffer);
				}
			test.Printf(lineBuffer);
			}
	}

    return bRes;
}


GLDEF_C void CallTestsL()
//
// Call tests that may leave
//
	{

	Test1();
	Test2();
	Test3();
	Test4();
	Test5();
	Test6();
	Test7();
	Test8();
	Test9();
	Test10();
	Test11();
	Test12();
	Test13();
	Test14();
	Test15();
    TestGetMediaSerialNumber();
	}
