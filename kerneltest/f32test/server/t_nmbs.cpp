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
//

#include <f32file.h>
#include <e32test.h>
#include "t_server.h"
#include "t_chlffs.h"

GLDEF_D RTest test(_L("T_NMBS"));

LOCAL_C void testMkDirRmDir()
//
// Test the MkDir functions.
//
	{

	test.Next(_L("Test MkDir"));

	TInt r=TheFs.RmDir(_L("\\F32-TST\\TNMBS\\"));
	test(r==KErrNone);
	r=TheFs.MkDir(_L("\\F32-TST\\TNMBS\\TEST\\"));
	test(r==KErrPathNotFound);
	r=TheFs.MkDir(_L("\\F32-TST\\TNMBS\\"));
	test(r==KErrNone);
	r=TheFs.MkDir(_L("\\F32-TST\\TNMBS\\TEST\\"));
	test(r==KErrNone);

	test.Next(_L("Test RmDir 1.0"));
	r=TheFs.RmDir(_L("\\F32-TST\\TNMBS\\TEST\\"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\TNMBS\\"));
	test(r==KErrNone);

	test.Next(_L("Test MkDirAll 1.0"));
	r=TheFs.MkDirAll(_L("\\F32-TST\\TNMBS\\TEST\\"));
	test(r==KErrNone);

	test.Next(_L("Test RmDir 2.0"));
	r=TheFs.RmDir(_L("\\F32-TST\\TNMBS\\TEST\\"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\TNMBS\\"));
	test(r==KErrNone);

	test.Next(_L("Test MkDirAll 2.0"));
	r=TheFs.MkDirAll(_L("\\F32-TST\\TNMBS\\"));
	test(r==KErrNone);
	r=TheFs.MkDirAll(_L("\\F32-TST\\TNMBS\\TEST\\"));
	test(r==KErrNone);

	test.Next(_L("Test RmDir 3.0"));
	r=TheFs.RmDir(_L("\\F32-TST\\TNMBS\\TEST\\"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\TNMBS\\"));
	test(r==KErrNone);

	test.Next(_L("Test mkdir and rmdir on root"));
	r=TheFs.RmDir(_L("\\File.TXT"));
	test(r==KErrInUse);
	r=TheFs.MkDir(_L("\\asdf.ere"));
	test(r==KErrAlreadyExists);
	r=TheFs.MkDirAll(_L("\\asdf.ere"));
	test(r==KErrAlreadyExists);

	test.Next(_L("Test error code return values"));
	r=TheFs.MkDir(_L("\\F32-TST\\\\ABCDEF\\"));

	test(r==KErrBadName);

	test.Next(_L("Test MkDir with trailing spaces"));
	r=TheFs.MkDir(_L("\\F32-TST\\TESTMKDIR    \\"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\TESTMKDIR\\"));
	test(r==KErrNone);
	r=TheFs.MkDirAll(_L("\\F32-TST\\TESTMKDIR    \\NOTCREATED\\NORTHIS   \\"));
	test(r==KErrPathNotFound);
	r=TheFs.RmDir(_L("\\F32-TST\\TESTMKDIR\\NOTCREATED\\"));
	test(r==KErrNotFound);
	r=TheFs.RmDir(_L("\\F32-TST\\TESTMKDIR\\"));
	test(r==KErrNone);

	r=TheFs.MkDirAll(_L("\\F32-TST\\TNMBS\\"));
	test(r==KErrNone);
	}

LOCAL_C void testRename()
//
// Test the rename function.
//
	{

	test.Next(_L("Test rename directories"));
	TInt r=TheFs.MkDirAll(_L("\\F32-TST\\ABCDEF\\GPQ\\"));
	test(r==KErrNone || r==KErrAlreadyExists);
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF\\"),_L("\\F32-TST\\ABCDEF\\LMED"));
	test(r==KErrInUse);
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF\\GPQ"),_L("\\F32-TST\\LMED"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\LMED\\"));
	test(r==KErrNone);

	MakeDir(_L("\\F32-TST\\ABC\\"));
	MakeFile(_L("\\F32-TST\\ABCDEF\\GPQ\\asdf.txt"));
	MakeFile(_L("\\F32-TST\\asdf.txt"));
	
	test.Next(_L("Test rename directory while subfile is open"));
	RFile f;
	r=f.Open(TheFs,_L("\\F32-TST\\ABCDEF\\GPQ\\asdf.txt"),EFileRead);
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF"),_L("\\F32-TST\\xxxyyy"));
	test(r==KErrInUse);
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF"),_L("\\F32-TST\\xxxyyy"));
	test(r==KErrInUse);
	r=TheFs.Rename(_L("\\F32-TST\\ABC"),_L("\\F32-TST\\XXX"));
	test(r==KErrNone);
	f.Close();
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF"),_L("\\F32-TST\\xxxyyy"));
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\XXX"),_L("\\F32-TST\\ABC"));
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\"),_L("\\BLARG"));
	test(r==KErrBadName);

	r=f.Open(TheFs,_L("\\F32-TST\\asdf.txt"),EFileRead);
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\xxxyyy"),_L("\\F32-TST\\ABCDEF"));
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\"),_L("\\ABCDEF"));
	test(r==KErrInUse);
	f.Close();

	r=TheFs.Delete(_L("\\F32-TST\\ABCDEF\\GPQ\\asdf.txt"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\asdf.txt"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\ABCDEF\\GPQ\\"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\ABC\\"));
	test(r==KErrNone);

	r=TheFs.Rename(_L("\\TST-E32\\123456"),_L("\\F32-TST\\ABCDEF"));
	test(r==KErrPathNotFound);
	r=TheFs.Rename(_L("\\F32-TST\\123456"),_L("\\F32-TST\\ABCDEF"));
	test(r==KErrNotFound);
	r=TheFs.Rename(_L("\\TST-E32\\123456"),_L("\\F32-TST\\FEDCBA"));
	test(r==KErrPathNotFound);
	r=TheFs.Rename(_L("\\F32-TST\\FEDCBA"),_L("\\TST-E32\\123456"));
	test(r==KErrNotFound);
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF"),_L("\\F32-TST\\123456"));
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\123456"),_L("\\F32-TST\\XYZABC"));
	test(r==KErrNone);

	test.Next(_L("Test rename files"));
	r=f.Create(TheFs,_L("\\F32-TST\\XYZABC\\OLDNAME.TXT"),EFileRead|EFileWrite);
	test(r==KErrNone);
	f.Close();
	r=TheFs.Rename(_L("\\F32-TST\\XYZABC\\OLDNAME.TXT"),_L("\\F32-TST\\XYZABC\\NEWNAME.TXT"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\XYZABC\\NEWNAME.TXT"));
	test(r==KErrNone);

	test.Next(_L("Test rename checks for duplicate entries"));
	r=TheFs.MkDirAll(_L("\\F32-TST\\ABCDEF\\"));
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF"),_L("\\F32-TST\\XYZABC"));
	test(r==KErrAlreadyExists);
	r=f.Create(TheFs,_L("\\F32-TST\\XYZABC\\OLDNAME.TXT"),EFileRead|EFileWrite);
	test(r==KErrNone);
	f.Close();
	r=f.Create(TheFs,_L("\\F32-TST\\XYZABC\\NEWNAME.TXT"),EFileRead|EFileWrite);
	test(r==KErrNone);
	f.Close();
	r=TheFs.Rename(_L("\\F32-TST\\XYZABC\\OLDNAME.TXT"),_L("\\F32-TST\\XYZABC\\NEWNAME.TXT"));
	test(r==KErrAlreadyExists);

	test.Next(_L("Test rename across directories"));
	r=TheFs.Rename(_L("\\F32-TST\\XYZABC\\NEWNAME.TXT"),_L("\\F32-TST\\ABCDEF\\OLDNAME.TXT"));
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\XYZABC\\NEWNAME.TXT"),_L("\\F32-TST\\ABCDEF\\OLDNAME.TXT"));
	test(r==KErrNotFound);
	r=TheFs.Rename(_L("\\F32-TST\\XYZABC\\NEWNAME.TXT"),_L("\\F32-TST\\ABCDEF\\DIFNAME.TXT"));
	test(r==KErrNotFound);
	r=TheFs.Rename(_L("\\F32-TST\\XYZABC"),_L("\\F32-TST\\ABCDEF\\XYZABC"));
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF\\XYZABC\\OLDNAME.TXT"),_L("\\F32-TST\\ABCDEF\\NEWNAME.TXT"));
	test(r==KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF\\NewNAME.TXT"),_L("\\F32-TST\\ABCDEF\\XYZABC\\OLDNAME.TXT"));
	test(r==KErrNone);
	test.Next(_L("Test rename across drive error code"));
	r=TheFs.Rename(_L("Z:\\BLEG"),_L("C:\\FRUM"));
	test(r==KErrArgument);	
	test.Next(_L("Test rename to identical names"));
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF\\XYZABC\\OLDNAME.TXT"),_L("\\F32-TST\\ABCDEF\\XYZABC\\OLDNAME.TXT"));
	test(r==KErrNone);
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\XYZABC\\OLDNAME.TXT"),KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF\\XYZABC\\OLDNAME.TXT"),_L("\\F32-TST\\ABCDEF\\XYZABC\\OLdnAME.TXT"));
	test(r==KErrNone);
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\XYZABC\\OLDNAME.TXT"),KErrNone,EFalse);
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\XYZABC\\OLdnAME.TXT"),KErrNone,ETrue);

	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF\\XYZABC\\OLDNAME.TXT"),_L("\\F32-TST\\ABCDEF\\NEWNAME.TXT"));
	test(r==KErrNone);
	test.Next(_L("Test RmDir"));
	r=TheFs.Delete(_L("\\F32-TST\\ABCDEF\\NEWNAME.TXT"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\ABCDEF\\OLDNAME.TXT"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\ABCDEF\\XYZABC\\"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\ABCDEF\\"));
	test(r==KErrNone);
	}
LOCAL_C void TestLongFileName()
	{
	#ifndef __EPOC32__ //emulator
	if (gDriveToTest.GetLowerCase()=='c')
		return;//don't perform this test for c: in emulator as emulator uses windows system calls
		//windows doesn't create a directory with length more than 244 characters
	#endif 
	
	 test.Next(_L("Test renaming 257 characters directories"));
	_LIT(KLongFileName256, "256dir_IncludingBackslash_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	_LIT(KLongFileName257, "257dir_IncludingBackslash_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	TBuf<260> Path;
	TBuf<10> baseDir;
			
	baseDir.Format(_L("%c:\\"),(TUint)(gDriveToTest));
	Path.Copy(baseDir);
	Path.Append(KLongFileName256);
	Path.Append(_L("\\"));
	
	TInt 	result = KErrNone;
	//-- 1. create directory in Root which total path length is 256 symbols.	result = TheFs.MkDirAll(Path);
	result=TheFs.MkDirAll(Path);
	test((result==KErrNone)||(result==KErrAlreadyExists));
	
	test.Next(_L("Renaming a 265 char directory to a 257 char directory"));
	result=TheFs.SetSessionPath(baseDir);
	test(result==KErrNone);	
	TheFs.SessionPath(baseDir);
	test.Printf(_L("Session path was set to: %S"), &baseDir);

	//-- 2. try to rename this directory to one with 257 character total path length
	test.Printf(_L("Renaming 256-char directory to 257"));
	result = KErrNone;
	Path.Copy(KLongFileName257);
	result = TheFs.Rename(KLongFileName256, Path);
	test(result==KErrBadName);
	//-- 3. try to rename this directory to one with 258 character total path length

	Path.Append(_L("z"));
	result = TheFs.Rename(KLongFileName256, Path);
	test(result==KErrBadName);

	}
LOCAL_C void testRenameCase()
//
// Test renaming "AGENDA" to "agenda"
//
	{

	test.Next(_L("Test rename case"));
	MakeDir(_L("\\F32-TST\\RENAMECASE\\"));
	TInt r=TheFs.SetSessionPath(_L("\\F32-TST\\RENAMECASE\\"));
	test(r==KErrNone);

	MakeFile(_L("file1.txt"));
	MakeFile(_L("FILE2.TXT"));
	MakeFile(_L("AGENDA"));
	MakeFile(_L("agenda.one"));

	TEntry entry;
	r=TheFs.Rename(_L("FILE1.TXT"),_L("FILE1.TXT"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("File1.txt"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("FILE1.TXT"));

	r=TheFs.Rename(_L("file2.txt"),_L("file2.txt"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("File2.txt"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("file2.txt"));

	r=TheFs.Rename(_L("agenda."),_L("agenda.two"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("Agenda.two"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("agenda.two"));

	r=TheFs.Rename(_L("AGENDA.ONE"),_L("AGENDA.ONE"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("Agenda.one"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("AGENDA.ONE"));

	r=TheFs.Rename(_L("FILE1.TXT"),_L("file1.txt"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("File1.txt"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("file1.txt"));

	r=TheFs.Rename(_L("file2.txt"),_L("FILE2.TXT"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("File2.txt"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("FILE2.TXT"));

	r=TheFs.Rename(_L("agenda.two"),_L("AGENDA"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("Agenda"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("AGENDA"));

	r=TheFs.Rename(_L("AGENDA.ONE"),_L("agenda.one"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("Agenda.one"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("agenda.one"));

	r=TheFs.Rename(_L("FILE1.TXT"),_L("FILe1.txt"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("File1.txt"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("FILe1.txt"));

	r=TheFs.Rename(_L("file2.txt"),_L("FILE3.TXT"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("File3.txt"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("FILE3.TXT"));

	r=TheFs.Rename(_L("agenda."),_L("AGENDA1"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("Agenda1"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("AGENDA1"));

	r=TheFs.Rename(_L("AGENDA.ONE"),_L("Agenda.One"));
	test(r==KErrNone);
	r=TheFs.Entry(_L("Agenda.one"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("Agenda.One"));

	r=TheFs.Delete(_L("file1.txt"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("file3.txt"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("Agenda1"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("AGENDA.ONE"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\RENAMECASE\\"));
	test(r==KErrNone);
	r=TheFs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	}

LOCAL_C void testReplace()
//
// Test the replace function
//
	{

	test.Next(_L("Test Replace"));
	TInt r=TheFs.MkDirAll(_L("\\F32-TST\\ABCDEF\\"));
	test(r==KErrNone);
	r=TheFs.Replace(_L("\\TST-E32\\123456"),_L("\\F32-TST\\ABCDEF"));
	test(r==KErrAccessDenied); // Do not replace directories
	r=TheFs.Replace(_L("\\F32-TST\\123456"),_L("\\F32-TST\\ABCDEF"));
	test(r==KErrAccessDenied);
	r=TheFs.Replace(_L("\\TST-E32\\123456"),_L("\\F32-TST\\FEDCBA"));
	test(r==KErrPathNotFound);
	r=TheFs.Replace(_L("\\F32-TST\\ABCDEF"),_L("\\F32-TST\\123456"));
	test(r==KErrAccessDenied);

	test.Next(_L("Replace a file with itself (8.3 filename)"));
	MakeFile(_L("\\F32-TST\\ABCDEF\\TEST1.SPR"));
	r=TheFs.Replace(_L("\\F32-TST\\ABCDEF\\TEST1.SPR"),_L("\\F32-TST\\ABCDEF\\TEST1.SPR"));
	test(r==KErrNone);
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\TEST1.SPR"),KErrNone);
	r=TheFs.Replace(_L("\\F32-TST\\ABCDEF\\TEST1.SPR"),_L("\\F32-TST\\ABCDEF\\test1.spr"));
	test(r==KErrNone);
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\TEST1.SPR"),KErrNone,ETrue); // Replace does not rename existing file
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\test1.spr"),KErrNone,EFalse);

	test.Next(_L("Replace a file with itself (vfat filename)"));
	MakeFile(_L("\\F32-TST\\ABCDEF\\TEST_SHEET(01).SPR"));
	r=TheFs.Replace(_L("\\F32-TST\\ABCDEF\\TEST_SHEET(01).SPR"),_L("\\F32-TST\\ABCDEF\\TEST_SHEET(01).SPR"));
	test(r==KErrNone);
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\TEST_SHEET(01).SPR"),KErrNone);
	r=TheFs.Replace(_L("\\F32-TST\\ABCDEF\\TEST_SHEET(01).SPR"),_L("\\F32-TST\\ABCDEF\\test_sheet(01).spr"));
	test(r==KErrNone);
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\TEST_SHEET(01).SPR"),KErrNone,ETrue); // Replace does not rename existing file
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\test_sheet(01).spr"),KErrNone,EFalse);

	test.Next(_L("Replace preserves file contents (8.3 filename)"));
	MakeFile(_L("\\F32-TST\\ABCDEF\\SHEET1.SPR"),_L8("Original Data"));
	MakeFile(_L("\\F32-TST\\ABCDEF\\TEMP0001.SPR"),_L8("NewData"));
	r=TheFs.Replace(_L("\\F32-TST\\ABCDEF\\TEMP0001.SPR"),_L("\\F32-TST\\ABCDEF\\SHEET1.SPR"));
	test(r==KErrNone);
	CheckFileContents(_L("\\F32-TST\\ABCDEF\\SHEET1.SPR"),_L8("NewData"));
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\TEMP0001.SPR"),KErrNotFound);

	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF\\SHEET1.SPR"),_L("\\F32-TST\\ABCDEF\\TEMP0001.SPR"));
	test(r==KErrNone);
    r=TheFs.Replace(_L("\\F32-TST\\ABCDEF\\TEMP0001.SPR"),_L("\\F32-TST\\ABCDEF\\SHEET1.SPR"));
	test(r==KErrNone);
	CheckFileContents(_L("\\F32-TST\\ABCDEF\\SHEET1.SPR"),_L8("NewData"));
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\TEMP0001.SPR"),KErrNotFound);

	test.Next(_L("Replace preserves file contents (vfat fileName)"));
	MakeFile(_L("\\F32-TST\\ABCDEF\\SHEET_TEST1.SPR"),_L8("Original Data"));
	MakeFile(_L("\\F32-TST\\ABCDEF\\NEW_TEMP0001.SPR"),_L8("NewData"));
	r=TheFs.Replace(_L("\\F32-TST\\ABCDEF\\NEW_TEMP0001.SPR"),_L("\\F32-TST\\ABCDEF\\SHEET_TEST1.SPR"));
	test(r==KErrNone);
	CheckFileContents(_L("\\F32-TST\\ABCDEF\\SHEET_TEST1.SPR"),_L8("NewData"));
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\NEW_TEMP0001.SPR"),KErrNotFound);

	r=TheFs.Rename(_L("\\F32-TST\\ABCDEF\\SHEET_TEST1.SPR"),_L("\\F32-TST\\ABCDEF\\NEW_TEMP0001.SPR"));
	test(r==KErrNone);
    r=TheFs.Replace(_L("\\F32-TST\\ABCDEF\\NEW_TEMP0001.SPR"),_L("\\F32-TST\\ABCDEF\\SHEET_TEST1.SPR"));
	test(r==KErrNone);
	CheckFileContents(_L("\\F32-TST\\ABCDEF\\SHEET_TEST1.SPR"),_L8("NewData"));
	CheckFileExists(_L("\\F32-TST\\ABCDEF\\NEW_TEMP0001.SPR"),KErrNotFound);

	r=TheFs.RmDir(_L("\\F32-TST\\ABCDEF\\"));
	test(r==KErrInUse);
	r=TheFs.RmDir(_L("\\F32-TST\\ABCDEF\\SHEET1.SPR\\"));
	test(r==KErrPathNotFound);
	r=TheFs.Delete(_L("\\F32-TST\\ABCDEF\\SHEET1.SPR"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\ABCDEF\\SHEET_TEST1.SPR"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\ABCDEF\\TEST1.SPR"));
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\ABCDEF\\TEST_SHEET(01).SPR"));
	test(r==KErrNone);
	r=TheFs.RmDir(_L("\\F32-TST\\ABCDEF\\"));
	test(r==KErrNone);

	test.Next(_L("Check file date is retained"));
	MakeFile(_L("OldFile.Old"));
	MakeFile(_L("NewFile.new"));

	TDateTime newDate(1998,(TMonth)2,3,11,12,0,0);
	TTime newTime(newDate);
	
	r=TheFs.SetEntry(_L("NewFile.new"),newTime,0,0);
	test(r==KErrNone);
	
	TEntry entry;
	r=TheFs.Entry(_L("NewFile.new"),entry);
	test(r==KErrNone);
	
	TTime checkReturnedTime=entry.iModified;
	TDateTime dateTime=checkReturnedTime.DateTime();
	
	test(entry.iModified==newTime);

	TDateTime oldDate(1996,(TMonth)2,3,23,0,0,0);
	TTime oldTime(oldDate);
	r=TheFs.SetEntry(_L("OldFile.old"),oldTime,0,0);
	test(r==KErrNone);

	TheFs.Replace(_L("NewFile.new"),_L("OldFile.old"));
	test(r==KErrNone);
	TTime check;
	r=TheFs.Modified(_L("OldFile.old"),check);
	test(r==KErrNone);
	TDateTime checkDateTime=check.DateTime();
	
	test(checkDateTime.Year()==dateTime.Year());
	test(checkDateTime.Month()==dateTime.Month());
	test(checkDateTime.Day()==dateTime.Day());
	test(checkDateTime.Hour()==dateTime.Hour());
	test(checkDateTime.Minute()==dateTime.Minute());
	test(checkDateTime.Second()==dateTime.Second());

	test.Next(_L("Replace 'Agenda' with 'Agenda.'"));
	MakeFile(_L("Agenda"));
	r=TheFs.Replace(_L("Agenda"),_L("Agenda."));
	test(r==KErrNone);
	CheckFileExists(_L("Agenda"),KErrNone,ETrue);
	CheckFileExists(_L("Agenda."),KErrNone,ETrue);
	CheckFileExists(_L("AGENDA"),KErrNone,EFalse);
	CheckFileExists(_L("AGENDA."),KErrNone,EFalse);

	r=TheFs.Replace(_L("Agenda"),_L("Agenda."));
	test(r==KErrNone);
	CheckFileExists(_L("Agenda"),KErrNone,ETrue);
	CheckFileExists(_L("Agenda."),KErrNone,ETrue);
	CheckFileExists(_L("AGENDA"),KErrNone,EFalse);
	CheckFileExists(_L("AGENDA."),KErrNone,EFalse);

	r=TheFs.Replace(_L("Agenda."),_L("AGENDA"));
	test(r==KErrNone);
	CheckFileExists(_L("Agenda"),KErrNone,ETrue);  // Replace does not rename existing file
	CheckFileExists(_L("Agenda."),KErrNone,ETrue);
	CheckFileExists(_L("AGENDA"),KErrNone,EFalse);
	CheckFileExists(_L("AGENDA."),KErrNone,EFalse);

	r=TheFs.Replace(_L("AGENDA."),_L("AGENDA.")); // Unchanged, ie still 'Agenda'
	test(r==KErrNone);
	CheckFileExists(_L("Agenda"),KErrNone,ETrue);
	CheckFileExists(_L("Agenda."),KErrNone,ETrue);
	CheckFileExists(_L("AGENDA"),KErrNone,EFalse);
	CheckFileExists(_L("AGENDA."),KErrNone,EFalse);
	}

LOCAL_C void testEntry()
//
// Test RFs::Entry(..) function
//
	{

	test.Next(_L("Get directory entry"));
	TEntry entry;
	TInt r=TheFs.Entry(_L("\\BLEERRG\\"),entry);
	test(r==KErrNotFound); // BLEERRG does not exist
	r=TheFs.Entry(_L("\\F32-TST"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("F32-TST")||entry.iName==_L("F32-TST."));
	if (IsTestingLFFS())
		{
		r=TheFs.Rename(_L("\\F32-TST.\\"),_L("\\F32-TST\\"));
		test(r==KErrBadName);
		r=TheFs.Entry(_L("\\F32-TST"),entry);
		test(r==KErrNone);
		}
	test(entry.iName==_L("F32-TST"));
	test(entry.IsDir());

	test.Next(_L("Get file entry"));
	RFile f;
	r=f.Replace(TheFs,_L("ENTRY.TXT"),EFileStream);
	test(r==KErrNone);
	r=f.Write(_L8("Entry data"));
	test(r==KErrNone);
	f.Close();
	r=TheFs.Entry(_L("\\F32-TST\\TNMBS\\ENTRY.TXT"),entry);
	test(r==KErrNone);
	test(entry.iName==_L("ENTRY.TXT"));
	test(!entry.IsDir());

	test.Next(_L("Get the root directory"));
	r=TheFs.Entry(_L("\\"),entry);
	test(r==KErrBadName);
	}

LOCAL_C void testRenameRegression()
//
// Regression tests for rename
//
	{

	test.Next(_L("More rename tests"));
	MakeFile(_L("\\F32-TST\\asdf"));
	TInt r=TheFs.Rename(_L("\\F32-TST\\asdf"),_L("*"));
	test(r==KErrBadName);
	r=TheFs.Rename(_L("\\F32-TST\\"),_L("*"));
	test(r==KErrBadName);
	r=TheFs.Rename(_L("\\F32-TST\\"),_L("\\F32-TST.\\"));
	test(r==KErrBadName);
	CheckFileExists(_L("\\F32-TST\\asdf"),KErrNone);
	r=TheFs.Rename(_L("\\F32-TST\\asdf"),_L("\\F32-TST\\Asdf."));
	test(r==KErrNone);
	CheckFileExists(_L("\\F32-TST\\asdf"),KErrNone,EFalse);
	CheckFileExists(_L("\\F32-TST\\Asdf"),KErrNone,ETrue);

	TBuf<4> shortName;
	shortName.SetLength(1);
	shortName[0]=0xff;
	r=TheFs.Rename(_L("\\F32-TST\\asdf"),shortName);
	test(r==KErrNone);
	r=TheFs.Delete(_L("\\F32-TST\\Asdf"));
	test(r==KErrNotFound);
	r=TheFs.Delete(shortName);
	test(r==KErrNone);
	}

LOCAL_C void testMaxNameLength()
//
// Create files and directories with the maximum name length
//
	{

	test.Next(_L("Test max name length"));
	TBuf<128> longNameBase=_L("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
	TFullName longName=_L("\\F32-TST\\MAXNAMELEN\\");

	longName.Append(longNameBase);
	longName.Append(_L("\\"));
	longName.Append(longNameBase);
	longName.Append(_L("\\"));

	TInt r=TheFs.MkDirAll(longName);
	test(r==KErrNone);

	TInt i=0;
	FOREVER
		{
		longName.Append(_L("X"));
		longName.Append(_L("\\"));
		r=TheFs.MkDirAll(longName);
		if (r!=KErrNone)
			break;
		i++;
		}
	
	longName.SetLength(longName.Length()-2);
	r=TheFs.RmDir(longName);
	test(r==KErrNone); // Make room for file
	longName.SetLength(longName.Length()-2);

	TFullName oldSessionPath;
	r=TheFs.SessionPath(oldSessionPath);
	test(r==KErrNone);
	r=TheFs.SetSessionPath(longName);
	test(r==KErrNone);
	r=TheFs.SessionPath(longName);
	test(r==KErrNone);

	test.Printf(_L("MaxPathLength achieved = %d chars\n"),longName.Length());
	TBuf<32> fileName=_L("012345678901234567890");
	RFile f;
	while (fileName.Length())
		{
		r=f.Replace(TheFs,fileName,EFileWrite);
		if (r==KErrNone)
			break;
		fileName.SetLength(fileName.Length()-1);
		}
	
	f.Close();
	test.Printf(_L("Added filename %S\n"),&fileName);
	r=f.Open(TheFs,fileName,EFileRead);
	test(r==KErrNone);
	f.Close();

	CFileMan* fMan=CFileMan::NewL(TheFs);
	r=fMan->RmDir(_L("\\F32-TST\\MAXNAMELEN\\"));
	test(r==KErrNone);
	delete fMan;

	r=f.Open(TheFs,fileName,EFileRead);
	test(r==KErrPathNotFound);
	r=TheFs.SetSessionPath(oldSessionPath);
	test(r==KErrNone);
	}

LOCAL_C void testErrorReturnValues()
//
// Test error return values
//
	{

    TInt r;
	test.Next(_L("Create folder with the name of an existing file"));
	MakeFile(_L("\\BLUE"));
    if (!IsTestingLFFS())
        { //FIXME: Bad error codes from LFFS
	    r=TheFs.MkDir(_L("\\BLUE\\"));
        test(r==KErrAccessDenied);
        MakeFile(_L("\\RED\\BLUE"));
        r=TheFs.MkDir(_L("\\RED\\BLUE\\"));
        test(r==KErrAccessDenied);
        r=TheFs.MkDirAll(_L("\\BLUE\\GREEN\\ORANGE\\"));
        test(r==KErrAccessDenied);
        }

	test.Next(_L("Create folder with the name of an existing folder"));
	MakeDir(_L("\\VOLVO\\"));
	r=TheFs.MkDir(_L("\\VOLVO\\"));
	test(r==KErrAlreadyExists);
	MakeDir(_L("\\FORD\\VOLKSWAGEN\\"));
	r=TheFs.MkDirAll(_L("\\ford\\volkswagen\\"));
	test(r==KErrAlreadyExists);

	RFile f;
    if (!IsTestingLFFS())
        { //FIXME Bad error codes from LFFS
	    test.Next(_L("Create a file with the name of an existing folder"));
	    r=f.Create(TheFs,_L("\\VOLVO"),EFileRead|EFileWrite);
	    test(r==KErrAccessDenied);
	    r=f.Replace(TheFs,_L("\\VOLVO"),EFileRead|EFileWrite);
	    test(r==KErrAccessDenied);
	    r=f.Open(TheFs,_L("\\VOLVO"),EFileRead|EFileWrite);
	    test(r==KErrAccessDenied);
	    r=f.Create(TheFs,_L("\\FORD\\VOLKSWAGEN"),EFileRead|EFileWrite);
	    test(r==KErrAccessDenied);
	    r=f.Replace(TheFs,_L("\\FORD\\VOLKSWAGEN"),EFileRead|EFileWrite);
	    test(r==KErrAccessDenied);
	    r=f.Open(TheFs,_L("\\FORD\\VOLKSWAGEN"),EFileRead|EFileWrite);
	    test(r==KErrAccessDenied);
        }

	test.Next(_L("Create a file with the name of an existing file"));
	r=f.Create(TheFs,_L("\\BLUE"),EFileRead|EFileWrite);
	test(r==KErrAlreadyExists);
	}

LOCAL_C void testSetEntry()
//
// Test set entry
//
	{

	test.Next(_L("Test SetEntry"));
//	TDateTime dateTime(1968,ENovember,19,12,59,0,0); Year must be > 1980
	TDateTime dateTime(1981,ENovember,19,12,59,0,0);
	TTime time(dateTime);
	MakeFile(_L("DUMENTRY."));
	
	RFile f;
	TInt r=f.Open(TheFs,_L("dumentry"),EFileRead);
	test(r==KErrNone);
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttReadOnly,KEntryAttArchive);
	test(r==KErrInUse);
	f.Close();
	
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttReadOnly,KEntryAttArchive);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),KEntryAttReadOnly,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,0,KEntryAttReadOnly);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttDir,0);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttVolume,0);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttRemote,0);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttDir|KEntryAttVolume,0);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttDir|KEntryAttRemote,0);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttVolume|KEntryAttRemote,0);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttDir|KEntryAttVolume|KEntryAttRemote,0);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,0,KEntryAttDir);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,0,KEntryAttVolume);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,0,KEntryAttRemote);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,0,KEntryAttDir|KEntryAttVolume);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,0,KEntryAttDir|KEntryAttRemote);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,0,KEntryAttVolume|KEntryAttRemote);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,0,KEntryAttDir|KEntryAttVolume|KEntryAttRemote);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttDir,KEntryAttVolume);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttDir,KEntryAttRemote);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));	
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttVolume,KEntryAttDir);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttVolume,KEntryAttRemote);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttRemote,KEntryAttDir);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=TheFs.SetEntry(_L("dumentry."),time,KEntryAttRemote,KEntryAttVolume);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));

	r=f.Open(TheFs,_L("dumentry"),EFileWrite);
	test(r==KErrNone);	

	r=f.SetAtt(KEntryAttDir,0);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	r=f.SetAtt(KEntryAttVolume,KEntryAttDir);
	test(r==KErrNone);
	CheckEntry(_L("DUMEntry"),0,TTime(dateTime));
	f.Close();
	r=TheFs.Delete(_L("dumEntry."));
	test(r==KErrNone);

	MakeDir(_L("\\DumEntry\\"));
	r=TheFs.SetEntry(_L("\\dumentry\\"),time,KEntryAttReadOnly,KEntryAttArchive);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttReadOnly|KEntryAttDir,TTime(dateTime));
	r=TheFs.SetEntry(_L("\\dumentry."),time,0,KEntryAttReadOnly);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttDir,TTime(dateTime));
	r=TheFs.SetEntry(_L("\\dumentry"),time,KEntryAttDir,0);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttDir,TTime(dateTime));
	r=TheFs.SetEntry(_L("\\dumentry"),time,KEntryAttVolume,0);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttDir,TTime(dateTime));
	r=TheFs.SetEntry(_L("\\dumentry"),time,KEntryAttVolume|KEntryAttDir,0);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttDir,TTime(dateTime));
	r=TheFs.SetEntry(_L("\\dumentry"),time,0,KEntryAttVolume|KEntryAttDir);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttDir,TTime(dateTime));
	r=TheFs.SetEntry(_L("\\dumentry"),time,0,KEntryAttVolume);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttDir,TTime(dateTime));
	r=TheFs.SetEntry(_L("\\dumentry"),time,0,KEntryAttDir);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttDir,TTime(dateTime));
	r=TheFs.SetEntry(_L("\\dumentry"),time,KEntryAttVolume,KEntryAttDir);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttDir,TTime(dateTime));
	r=TheFs.SetEntry(_L("\\dumentry"),time,KEntryAttDir,KEntryAttVolume);
	test(r==KErrNone);
	CheckEntry(_L("\\DUMEntry"),KEntryAttDir,TTime(dateTime));
	r=TheFs.RmDir(_L("\\dumEntry\\"));
	test(r==KErrNone);
	}

	
LOCAL_C void testSetFileAttributes()
//
// Test RFile::SetAtt() and RFile::Set()
//
	{
//	First test RFile::SetAtt() function
	
	test.Next(_L("Test RFile::SetAtt()"));

//	Create a file "TEMPFILE.TMP" and set attributes to hidden	
	RFile file;
	TInt r=file.Replace(TheFs,_L("TEMPFILE.TMP"),0);
	test(r==KErrNone || r==KErrPathNotFound);	
	r=file.SetAtt(KEntryAttHidden,0);
	test(r==KErrNone);
	file.Close();

//	Check attributes are as set
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	TUint atts;
	r=file.Att(atts);
	test(r==KErrNone);
	file.Close();
	test(atts&KEntryAttHidden);

//	Change attributes from hidden to system	
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.SetAtt(KEntryAttSystem,KEntryAttHidden);
	test(r==KErrNone);
	file.Close();

//	Check attributes have been changed
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	file.Close();
	test(atts&KEntryAttSystem);

//	Change attributes to normal
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.SetAtt(KEntryAttNormal,KEntryAttSystem|KEntryAttArchive);
	test(r==KErrNone);
	file.Close();

//	Check attributes have been changed
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	file.Close();
	test(atts==KEntryAttNormal);

//	Attempt to change attributes from normal file to directory	
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.SetAtt(KEntryAttDir,KEntryAttNormal);
	test(r==KErrNone);	//	Returns KErrNone but DOESN'T change the file to a directory
	file.Close();

//	Check the file has not been changed to a directory
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	file.Close();
	test((TInt)(atts&KEntryAttDir)==KErrNone);

//	Change the attributes from normal file to hidden file
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.SetAtt(KEntryAttHidden,KEntryAttNormal);
	test(r==KErrNone);
	file.Close();

//	Check the attributes have been changed
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	file.Close();
	test(atts&KEntryAttHidden);

//	Try to change the attributes from hidden file to volume	
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.SetAtt(KEntryAttVolume,KEntryAttHidden);
	test(r==KErrNone);	//	Returns KErrNone but DOESN'T change the file to a volume
	file.Close();

//	Check that the hidden file has not been changed to a volume
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	file.Close();
	test((TInt)(atts&KEntryAttVolume)==KErrNone);

//	Test RFile::Set() function	
	
	test.Next(_L("Test RFile::Set()"));	

//	Check attributes 
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	file.Close();
	test(atts==KEntryAttNormal);

//	Change attributes from hidden to system	- and change modification time
	TDateTime dateTime(1998,EMay,25,18,23,0,0);
	TTime modTime1(dateTime);
	TTime retTime;
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Set(modTime1,KEntryAttSystem,KEntryAttNormal);
	test(r==KErrNone);
	file.Close();

//	Check attributes have been changed
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	r=file.Modified(retTime);
	test(r==KErrNone);
	file.Close();
	test(atts&KEntryAttSystem);
	test(retTime==modTime1);

//	Change attributes to normal - and change modification time
	dateTime.Set(1997,EMay,24,17,25,0,0);
	TTime modTime2(dateTime);
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Set(modTime2,KEntryAttNormal,KEntryAttSystem|KEntryAttArchive);
	test(r==KErrNone);
	file.Close();

//	Check attributes have been changed
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	r=file.Modified(retTime);
	test(r==KErrNone);
	file.Close();
	test(atts==KEntryAttNormal);
	test(retTime==modTime2);

//	Attempt to change attributes from normal file to directory	
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Set(modTime1,KEntryAttDir,KEntryAttNormal);
	test(r==KErrNone);	//	Returns KErrNone but DOESN'T change the file to a directory
	file.Close();

//	Check the file has not been changed to a directory
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	r=file.Modified(retTime);
	test(r==KErrNone);
	file.Close();
	test((TInt)(atts&KEntryAttDir)==KErrNone);
	test(retTime==modTime1);//	Modification time should have been set successfully

//	Change the attributes from normal file to hidden file - and change modification time
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Set(modTime1,KEntryAttHidden,KEntryAttNormal);
	test(r==KErrNone);
	file.Close();

//	Check the attributes have been changed
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	r=file.Modified(retTime);
	file.Close();
	test(atts&KEntryAttHidden);
	test(retTime==modTime1);

//	Try to change the attributes from hidden file to volume	
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Set(modTime2,KEntryAttVolume,KEntryAttHidden);
	test(r==KErrNone);	//	Returns KErrNone but DOESN'T change the file to a volume
	file.Close();

//	Check that the hidden file has not been changed to a volume
	file.Open(TheFs,_L("TEMPFILE.TMP"),EFileWrite);
	r=file.Att(atts);
	test(r==KErrNone);
	r=file.Modified(retTime);
	test(r==KErrNone);
	file.Close();
	test((TInt)(atts&KEntryAttVolume)==KErrNone);
	test(retTime==modTime2);	//	Modification time should have been set successfully
	
	r=TheFs.Delete(_L("TEMPFILE.TMP"));
	
	}	


GLDEF_C void CallTestsL()
//
// Do tests relative to session path
//
	{

	TurnAllocFailureOff();
	CreateTestDirectory(_L("\\F32-TST\\TNMBS\\"));
	testMkDirRmDir(); // Must come first
	testSetEntry();
	testSetFileAttributes();
	testRename();
	TestLongFileName();
	testRenameRegression();
	testRenameCase();
	testMaxNameLength();
	testEntry();
	testReplace();
	testErrorReturnValues();
	DeleteTestDirectory();
	}

