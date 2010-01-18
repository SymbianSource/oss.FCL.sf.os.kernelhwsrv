// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <f32dbg.h>
#include <e32test.h>
#include "t_server.h"
#include "t_chlffs.h"
#include "plugincommon.h"
#include "premodifier_plugin.h"
#include "modifier_plugin.h"
#include "observer_plugin.h"
#include "combinational_plugin.h"
#include "combinational2_plugin.h"
#include "stacked_plugin.h"
#include "stacked2_plugin.h"
#include "stacked3_plugin.h"
#include "drivec_plugin.h"
#include "drivez_plugin.h"
#include "allsupporteddrives_plugin.h"
#include "exclusiveaccess_plugin.h"
#include "unremovable_plugin.h"

#include <e32def.h>
#include <e32def_private.h>

RTest test(_L("Plugin Framework Version 2 Test"));

#define _plugin_test(r)		if(!r) { dismountAllPlugins(); test(r); }


class MyRPlugin : public RPlugin
	{
public:
	void DoRequest(TInt aReqNo,TRequestStatus& aStatus) const;
	void DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1) const;
	void DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1,TDes8& a2) const;
	TInt DoControl(TInt aFunction) const;
	TInt DoControl(TInt aFunction,TDes8& a1) const;
	TInt DoControl(TInt aFunction,TDes8& a1,TDes8& a2) const;
	void DoCancel(TUint aReqMask) const;
	};

void dismountAllPlugins()
	{
	TheFs.DismountPlugin(KObserverPluginName);
	TheFs.DismountPlugin(KModifierPluginName);
	TheFs.DismountPlugin(KPreModifierPluginName);
	TheFs.DismountPlugin(KCombinationalPluginName);
	TheFs.DismountPlugin(KCombinational2PluginName);
	TheFs.DismountPlugin(KAllSupportedDrivesPluginName);
	TheFs.DismountPlugin(KDriveZPluginName);
	TheFs.DismountPlugin(KDriveCPluginName);
	TheFs.DismountPlugin(KStackedPluginName);
	TheFs.DismountPlugin(KStacked2PluginName);
	TheFs.DismountPlugin(KStacked3PluginName);
	TheFs.DismountPlugin(KExclusiveAccessPluginName);
	TheFs.DismountPlugin(KUnremovablePluginName);

	TheFs.RemovePlugin(KObserverPluginName);
	TheFs.RemovePlugin(KModifierPluginName);
	TheFs.RemovePlugin(KPreModifierPluginName);
	TheFs.RemovePlugin(KCombinationalPluginName);
	TheFs.RemovePlugin(KCombinational2PluginName);
	TheFs.RemovePlugin(KAllSupportedDrivesPluginName);
	TheFs.RemovePlugin(KDriveZPluginName);
	TheFs.RemovePlugin(KDriveCPluginName);
	TheFs.RemovePlugin(KStackedPluginName);
	TheFs.RemovePlugin(KStacked2PluginName);
	TheFs.RemovePlugin(KStacked3PluginName);
	TheFs.RemovePlugin(KExclusiveAccessPluginName);
	TheFs.RemovePlugin(KUnremovablePluginName);
	}

inline void safe_test(TInt aError, TInt aLine, TText* aName)
	{
	if(aError!=KErrNone)
		{
		test.Printf(_L("Error: %d receieved on line %d\n"),aError,aLine);
		dismountAllPlugins();
		test.operator()(aError==KErrNone,aLine,(TText*)aName);
		}
	}

inline void plugin_test(RTest& test, TInt aUniquePluginPos, TInt aLine, TText* aName)
	{
	__UHEAP_MARK;
	MyRPlugin rplugin;
	TInt error = KErrNone;
	TInt lineNumber;
	TPckg<TInt> errCodePckg(error);
	TPckg<TInt> lineNumberPckg(lineNumber);

	test.Next(_L("Open RPlugin connection"));
	TInt r = rplugin.Open(TheFs,aUniquePluginPos);
	if(r!=KErrNone)
		{
		dismountAllPlugins();
		}

	//This next lines aren't needed though I'm leaving them here
	//for reference.
	// ...
	//const TText name[16] = (const TText *) Expand(__FILE__);
	//TPckg<TText[16]> namePckg(name);
	//test.operator()(r==KErrNone,aLine,(TText*)&namePckg());
	// ...

	//test for Open
	test.operator()(r==KErrNone,aLine,(TText*)aName);

	test.Next(_L("Check plugin for any errors"));
	r = rplugin.DoControl(KPluginGetError,errCodePckg,lineNumberPckg);
	//test for DoControl
	if(r!=KErrNone)
		{
		dismountAllPlugins();
		test.operator()(r==KErrNone,aLine,(TText*)aName);
		}

	if(error != KErrNone)
		{
		test.Printf(_L("Error code == %d, from lineNumber == %d"),error,lineNumber);
		}

	//test for actual error
	if(r!=KErrNone)
		{
		dismountAllPlugins();
		test.operator()(error==KErrNone,aLine,(TText*)aName);
		}
	rplugin.Close();
	__UHEAP_MARKEND;
	}



void MyRPlugin::DoRequest(TInt aReqNo,TRequestStatus& aStatus) const
	{
	RPlugin::DoRequest(aReqNo,aStatus);
	}
void MyRPlugin::DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1) const
	{
	RPlugin::DoRequest(aReqNo,aStatus,a1);
	}
void MyRPlugin::DoRequest(TInt aReqNo,TRequestStatus& aStatus,TDes8& a1,TDes8& a2) const
	{
	RPlugin::DoRequest(aReqNo,aStatus,a1,a2);
	}
TInt MyRPlugin::DoControl(TInt aFunction) const
	{
	return RPlugin::DoControl(aFunction);
	}
TInt MyRPlugin::DoControl(TInt aFunction,TDes8& a1) const
	{
	return RPlugin::DoControl(aFunction,a1);
	}
TInt MyRPlugin::DoControl(TInt aFunction,TDes8& a1,TDes8& a2) const
	{
	return RPlugin::DoControl(aFunction,a1,a2);
	}
void MyRPlugin::DoCancel(TUint aReqMask) const
	{
	RPlugin::DoCancel(aReqMask);
	}

TInt ReplaceFiles(TDes16 &aPath, TDes8 &aContent, TInt aLine)
	{
	RFile file;
	TInt r=KErrNone;

	r = file.Replace(TheFs, aPath, EFileWrite);
	test.operator()(r==KErrNone,aLine,_S("t_plugin_v2.cpp"));

	r = file.Write(aContent);
	test.operator()(r==KErrNone,aLine,_S("t_plugin_v2.cpp"));
	file.Close();

	return r;
	}


void SetupTestFiles()
	{
	test.Next(_L("Setting up test files"));

	TBuf<40> path1;
	TBuf<40> path2;
	TBuf<40> path3;

	TInt theDrive;
	TInt r=TheFs.CharToDrive(gDriveToTest,theDrive);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	path1.Append(gDriveToTest);
	path1.Append(_L(":\\data\\oldtest.tst"));

	path2.Append(gDriveToTest);
	path2.Append(_L(":\\data\\"));

	path3.Append(gDriveToTest);
	path3.Append(_L(":\\data2\\"));


	test.Next(_L("Setting up test files"));

	r = TheFs.Delete(path1);
	test(r==KErrNone || r==KErrNotFound || r==KErrPathNotFound);

	r = TheFs.MkDir(path2);
	test(r==KErrNone || r==KErrAlreadyExists);

	r = TheFs.MkDir(path3);
	test(r==KErrNone || r==KErrAlreadyExists);

	}


void LoadAndMountPlugins()
	{
	TInt r = KErrNone;

	test.Next(_L("Loading Observer plugin"));
	r = TheFs.AddPlugin(KObserverPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.MountPlugin(KObserverPluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Loading PreModifier plugin"));
	r = TheFs.AddPlugin(KPreModifierPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.MountPlugin(KPreModifierPluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Loading Modifier plugin"));
	r = TheFs.AddPlugin(KModifierPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.MountPlugin(KModifierPluginName,KPluginAutoAttach);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.MountPlugin(KModifierPluginName,20);
	if(r==KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));


	//Use RPlugin to communicate to the plugins which drive they should be
	//testing on.
	//This is needed because sometimes a plugin may open a different file etc and will need
	//to know which drive it should be testing on.

	MyRPlugin rplugin;
	TPckg<TChar> drivePckg(gDriveToTest);

	test.Next(_L("Open RPlugin connection for ModifierPlugin"));
	r = rplugin.Open(TheFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Send drive letter to test down to plugin"));
	r = rplugin.DoControl(KPluginSetDrive,drivePckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	test.Next(_L("Open RPlugin connection for PreModifierPlugin"));
	r = rplugin.Open(TheFs,KPreModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Send drive letter to test down to plugin"));
	r = rplugin.DoControl(KPluginSetDrive,drivePckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	rplugin.Close();

	// As an extra test, open an RPlugin handle but don't close to test subsession cleanup on session closure...

	RFs myFs;
	r = myFs.Connect();
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = rplugin.Open(myFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	
	// Don't close:	rplugin.Close();

	myFs.Close();
	
	// Test the memory cleanup for asynchronous request

	r = rplugin.Open(TheFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	TRequestStatus aStatus;
	rplugin.DoRequest(1,aStatus,drivePckg);
	User::WaitForRequest(aStatus);
	rplugin.Close();
	
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1336
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to read data directly from a file
//! @SYMPREQ					REQ7902
//! @SYMTestPriority			High
//! @SYMTestActions				TestReadFileDirect() disables intercepts on any mounted plugin.
//!								Following this, it opens a test file and sends a read request directly
//!								to the fileserver.
//!
//!								1.	Disable intercepts.
//!								2.  Set up test file name and path.
//!								3.	Open file for read access.
//!								4.	Read data from test file.
//!								5.	Close test file.
//!								6.	Enable intercepts.
//!
//!
//! @SYMTestExpectedResults		1.	Intercepts disabled without any panic
//!								2.	Test file created without any panic
//!								3.	File opened without any panic
//!								4.	Read request completes by bypassing plug-in without any panic
//!								5.	File close without any panic.
//!								6.	Intercepts enabled.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestReadFileDirect()
	{

	//As the plugin is installed at this point, rather than removing it
	//or not mounting it until after, let's disable intercepts
	//and enable them at the end of this function.
	MyRPlugin rplugin;
	TInt r = rplugin.Open(TheFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	TBool interceptsStatus;
	TPckg<TBool> interceptsStatusDes(interceptsStatus);
	test.Next(_L("RPlugin: toggle intercepts (Modifier)"));
	r = rplugin.DoControl(KPluginToggleIntercepts,interceptsStatusDes);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Ensure that intercepts are now disabled"));
	if(!interceptsStatus) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	interceptsStatus = ETrue;

	r = rplugin.Open(TheFs,KObserverPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("RPlugin: toggle intercepts (Observer)"));
	r = rplugin.DoControl(KPluginToggleIntercepts,interceptsStatusDes);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Ensure that intercepts are now disabled"));
	if(!interceptsStatus) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();


	RFile file;
	TBuf8<4> narrow_buffer;
	TBuf16<4> wide_buffer;
	TBuf<256> filename;
	TBuf8<10> content;

	//setting up test files
	filename.Append(gDriveToTest);
	filename.Append(_L(":\\data\\test.txt"));
	content.Copy(_L8("1234567890"));
	ReplaceFiles(filename, content,__LINE__);

	r = file.Open(TheFs, filename, EFileRead);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Reading from a file directly first time"));
	r = file.Read(narrow_buffer); // -> returns data from pos 0 to 3
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	wide_buffer.Copy(narrow_buffer);
	test.Printf(_L("read: %S\n"), &wide_buffer);

	test.Next(_L("Reading from a file directly second time"));
	r = file.Read(narrow_buffer); // -> returns data from pos 4 to 8
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	wide_buffer.Copy(narrow_buffer);
	test.Printf(_L("read: %S\n"), &wide_buffer);

	test.Next(_L("Reading from a file directly complete"));
	file.Close();

	//Turn intercepts back on.
    interceptsStatus = EFalse;

	r = rplugin.Open(TheFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("RPlugin: toggle intercepts (Observer)"));
	r = rplugin.DoControl(KPluginToggleIntercepts,interceptsStatusDes);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Ensure that intercepts are now enabled"));
	if(interceptsStatus) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	interceptsStatus = EFalse;

	r = rplugin.Open(TheFs,KObserverPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("RPlugin: toggle intercepts (Observer)"));
	r = rplugin.DoControl(KPluginToggleIntercepts,interceptsStatusDes);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Ensure that intercepts are now enabled"));
	if(interceptsStatus) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1337
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to read data via a plugin from a file
//! @SYMPREQ					REQ7902
//! @SYMTestPriority			High
//! @SYMTestActions				TestReadFileDirect() opens a file for read access.
//!								Following this, it sends a read request via a plugin to the fileserver.
//!
//!								1.	Set up test file name and path.
//!								2.  Open file for read access.
//!								3.	Read data from test file.
//!								4.  Get current size of file.
//!								5.  Read data from test file .
//!								6.	Verify that the correct data was read from the test file.
//!								7.	Close test file.
//!
//!
//!
//! @SYMTestExpectedResults		1.	Test file created without any panic.
//!								2.	File opened without any panic.
//!								3.	Read request completes by bypassing plug-in without any panic.
//!								4.	Size of file returned without any panic.
//!								5.	Read request completes by bypassing plug-in without any panic.
//!								6.	Correct data read by from test file.
//!								7.	File close without any panic.
//!
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestReadFileViaPlugin()
	{
	RFile file;
	TBuf8<4> narrow_buffer;
	narrow_buffer.FillZ(4);
	TBuf16<4> wide_buffer;
	wide_buffer.FillZ(4);

	TBuf<256> filename;
	TBuf8<10> content;

	//setting up test files
	filename.Append(gDriveToTest);
	filename.Append(_L(":\\data\\test.tst"));
	content.Copy(_L8("1234567890"));
	ReplaceFiles(filename, content,__LINE__);

	TInt r = file.Open(TheFs, filename, EFileRead);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Reading from a file via a plugin using CFsPlugin::FileRead first time"));
	r = file.Read(narrow_buffer); // -> returns data from pos 0 to 3
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	wide_buffer.Copy(narrow_buffer);
	RDebug::Print(_L("read: %S\n"), &wide_buffer);
	TInt size=0;
	r = file.Size(size);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	RDebug::Print(_L("RFile::size = %d"),size);

	test.Next(_L("Reading from a file via a plugin using CFsPlugin::FileRead second time"));
	r = file.Read(narrow_buffer); // -> returns data from pos 4 to 8
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	wide_buffer.Copy(narrow_buffer);
	RDebug::Print(_L("read: %S\n"), &wide_buffer);

	test.Next(_L("Reading from a file via a plugin using CFsPlugin::FileRead complete"));
	file.Close();
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1338
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to write to a file directly
//! @SYMPREQ					REQ7902
//! @SYMTestPriority			High
//! @SYMTestActions				TestWriteFileDirect() disables intercepts on any mounted plugin.
//!								Following this, it opens a test file and sends a write request directly
//!								to the fileserver.
//!
//!								1.	Disable intercepts.
//!								2.  Set up test file name and path.
//!								3.	Open file for write access.
//!								4.	Write to test file.
//!								5.  Seek to the beginning of test file.
//!								6.  Read from test file.
//!								7.  Verify that the correct data was read from the test file.
//!								8.	Close test file.
//!								9.	Enable intercepts.
//!
//!
//! @SYMTestExpectedResults		1.	Intercepts disabled without any panic
//!								2.	Test file created without any panic
//!								3.	File opened without any panic
//!								4.	Write request completes by bypassing plug-in without any panic
//!								6.  Seek to beginning of test file completes without any panic.
//!								5.  Read from test file completes without any panic.
//!								6.  Correct data read by from test file.
//!								7.	File close without any panic.
//!								8.	Intercepts enabled.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestWriteFileDirect()
	{
	//As the plugin is installed at this point, rather than removing it
	//or not mounting it until after, lets disable intercepts
	//and enabled them at the end of this function.
	MyRPlugin rplugin;
	TInt r = rplugin.Open(TheFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	TBool interceptsStatus;
	TPckg<TBool> interceptsStatusDes(interceptsStatus);
	test.Next(_L("RPlugin: toggle intercepts (Modifier)"));
	r = rplugin.DoControl(KPluginToggleIntercepts,interceptsStatusDes);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Ensure that intercepts are now disabled"));
	r = interceptsStatus == EFalse;
	if(r) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	interceptsStatus = ETrue;

	r = rplugin.Open(TheFs,KObserverPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("RPlugin: toggle intercepts (Observer)"));
	r = rplugin.DoControl(KPluginToggleIntercepts,interceptsStatusDes);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Ensure that intercepts are now disabled"));
	r = interceptsStatus == EFalse;
	if(r) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();



	RFile file;
	TBuf8<64> buffer;
	buffer.FillZ(64);
    TBuf8<64> wbuffer;
	wbuffer.FillZ(64);
    wbuffer.Copy(_L8("TestTestTest"));

	TBuf<256> filename;
	TBuf8<10> content;

	//setting up test files
	filename.Append(gDriveToTest);
	filename.Append(_L(":\\data\\test.txt"));
	content.Copy(_L8("1234567890"));
	ReplaceFiles(filename, content,__LINE__);

    test.Next(_L("Opening file test.txt"));
    // we assume that file test.txt still exists
    r = file.Open(TheFs,filename, EFileWrite);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Writing to file test.txt"));
    r = file.Write(wbuffer);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Seeking to beginning of file test.txt"));
	TInt pos = 0;
	r = file.Seek(ESeekStart, pos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Reading file test.txt"));
    r = file.Read(buffer);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Comparing buffers"));
	r = wbuffer.Compare(buffer);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Closing file test.txt"));
    file.Close();

    //Turn intercepts back on.
    interceptsStatus = EFalse;

	r = rplugin.Open(TheFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("RPlugin: toggle intercepts (Modifier)"));
	r = rplugin.DoControl(KPluginToggleIntercepts,interceptsStatusDes);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Ensure that intercepts are now enabled"));
	if(interceptsStatus) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	interceptsStatus = EFalse;

	r = rplugin.Open(TheFs,KObserverPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("RPlugin: toggle intercepts (Observer)"));
	r = rplugin.DoControl(KPluginToggleIntercepts,interceptsStatusDes);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Ensure that intercepts are now enabled"));
	if(interceptsStatus) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1339
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to write to a file via a plug-in
//! @SYMPREQ					REQ7902
//! @SYMTestPriority			High
//! @SYMTestActions				TestWriteViaPlugin() opens a file for write access.
//!								Following this, it sends a write request via a plugin to the fileserver.
//!
//!								1.	Set up test file name and path.
//!								2.  Open file for write access.
//!								3.	Write to test file.
//!								4.  Seek to the beginning of test file.
//!								5.  Read data from test file .
//!								6.	Verify that the correct data was read from the test file.
//!								7.	Close test file.
//!
//! @SYMTestExpectedResults		1.	Test file created without any panic.
//!								2.	File opened without any panic.
//!								3.	Read request completes by bypassing plug-in without any panic.
//!								4.	Seek to beginning of test file completes without any panic.
//!								5.	Read request completes by bypassing plug-in without any panic.
//!								6.	Correct data read by from test file.
//!								7.	File close without any panic.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestWriteFileViaPlugin()
	{
	RFile file;
	TBuf8<64> buffer;
	buffer.FillZ(64);
    TBuf8<64> wbuffer;
	wbuffer.FillZ(64);
    wbuffer.Copy(_L8("TestTestTest"));


	TBuf<256> filename;
	TBuf8<10> content;

	//setting up test files
	filename.Append(gDriveToTest);
	filename.Append(_L(":\\data\\test.tst"));
	content.Copy(_L8("1234567890"));
	ReplaceFiles(filename, content,__LINE__);

    test.Next(_L("TestWriteFileViaPlugin(): Opening file test.tst"));
    // we assume that file test.tst still exists
    TInt r = file.Open(TheFs, filename, EFileWrite);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Writing to file test.tst"));
    r = file.Write(0,wbuffer);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Seeking to beginning of file test.tst"));
	TInt pos = 0;
	r = file.Seek(ESeekStart, pos);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Reading file test.tst"));
	TInt length = wbuffer.Length();
	r = file.Read(buffer, length);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Comparing buffers"));
	r = wbuffer.Compare(buffer);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Closing file test.tst"));
    file.Close();
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1350
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to rename a file
//! @SYMPREQ					REQ7901
//! @SYMTestPriority			High
//! @SYMTestActions				TestRename() opens a file for write access.
//!								Following this, it sends a rename request via a plugin to the fileserver.
//!
//!								1.	Set up test file name and path.
//!								2.  Open file for write access.
//!								3.	Rename test file.
//!								4.  Write to file.
//!								5.  Rename test file again
//!								6.	Close test file.
//!
//! @SYMTestExpectedResults		1.	Test files created without any panic.
//!								2.	File opened without any panic.
//!								3.	Rename request intercepted by plugin and completes without any panic.
//!								4.	Write request comletes without any panic.
//!								5.	Rename request intercepted by plugin and completes without any panic.
//!								6.	File close without any panic.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestRename()
	{
	RFile file;

	TBuf<256> filename1;
	TBuf<256> filename2;

	//setting up test files
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\renametest.tst"));

	filename2.Append(gDriveToTest);
	filename2.Append(_L(":\\Data\\test.tst\n"));

	TInt r = TheFs.Delete(filename1);
	if(r == KErrNotFound || r == KErrPathNotFound)
		r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("TestRename(): Opening file test.tst"));
	// we assume that file test.tst exists
	r = file.Open(TheFs, filename2, EFileWrite);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	filename1.Append(_L("\n"));
	test.Next(_L("Renaming file test.tst to renametest.tst"));
	r = file.Rename(filename1);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Write to file"));
	r=file.Write(_L8("Hello World"),11);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file.Close();

	test.Next(_L("Renaming file renametest.tst to test.tst"));
	r = TheFs.Rename(filename1,filename2);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Closing renamed file renametest.tst"));
	file.Close();
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1343
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to open directory and read entries
//! @SYMPREQ					REQ7901
//! @SYMTestPriority			High
//! @SYMTestActions				TestDir() opens a directory.
//!								Following this, it sends a reads the directory entry and closes directory
//!
//!								1.	Set up path for directory and test files
//!								2.  Open a directory
//!								3.	Read directory entry via plugin.
//!								4.  Close directory.
//!								5.  Open another directory.
//!								6.  Read directory entry directly
//!								7.  Close directory.
//!								8.  Compare entry read via plugin with entry read directly.
//!								9.  Open another directory.
//!								10. Read one entry via plugin
//!								11. Compare Read One entry with Read Packed.
//!								12. Close directory.
//!
//! @SYMTestExpectedResults		1.	Test path creat ed without any panic.
//!								2.	Directory open request intercepted by plugin and directory opened without any panic.
//!								3.	Directory read request intercepted by plugin and read entry completes without any panic.
//!								4.	Directory close request comletes without any panic.
//!								5.	Directory open request intercepted by plugin and directory opened without any panic.
//!								6.	Directory read request bypasses plugin and read entry completes without any panic.
//!								7.	Directory close request comletes without any panic.
//!								8.	Enrties read are identical.
//!								9.	Open another directory.
//!								10. Read directory entry intercepted by plugin and read entry completes without any panic.
//!								11. Enrties read are identical.
//!								12. Directory close request comletes without any panic
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestDir()
	{
	test.Next(_L("TestDir()"));

	//READ MANY

	// "Via PLugin"
	RDir dir;

	TBuf<256> filename1;
	TBuf<256> filename2;

	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\"));

	filename2.Append(gDriveToTest);
	filename2.Append(_L(":\\Data\\"));

	test.Next(_L("Opening directory "));

	//Need to send which filename we want to read to the plugin
	//This is a hack until there's a proper way of getting the directory filename.
	test.Next(_L("Open RPlugin connection for KModifierPos"));
	MyRPlugin rplugin;
	TInt r = rplugin.Open(TheFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Send dir name down to plugin"));
	typedef TBuf<256> TDirName;
	TPckg<TDirName> dirname1Pckg(filename1);
	r = rplugin.DoControl(KPluginSetDirFullName,dirname1Pckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	 r = dir.Open(TheFs,filename1,KEntryAttMatchMask);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	TEntryArray eA;
	r = dir.Read(eA);
	if(r==KErrEof) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	TEntry a = eA[0];
	RDebug::Print(_L("Filename : %S"),&a.iName);

	test.Next(_L("Closing directory \\Data\\"));
	dir.Close();




	//Need to send which filename we want to read to the plugin
	//This is a hack until there's a proper way of getting the directory filename.
	test.Next(_L("Open RPlugin connection for KModifierPos"));
	r = rplugin.Open(TheFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Send dir name down to plugin"));
	TPckg<TDirName> dirname2Pckg(filename2);
	r = rplugin.DoControl(KPluginSetDirFullName,dirname2Pckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	test.Next(_L("Opening directory \\Data\\"));
	r = dir.Open(TheFs,filename2,KEntryAttMatchMask);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	// "Direct"
	TEntryArray eA2;
	r = dir.Read(eA2);
	if(r==KErrEof) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	TEntry a2 = eA2[0];
	RDebug::Print(_L("Filename : %S"),&a2.iName);

	test.Next(_L("Compare plugin and direct read on \\Data\\"));
	test(a.iName.Compare(a2.iName));

	test.Next(_L("Closing directory \\Data\\"));
	dir.Close();





	// READ ONE


	//Need to send which filename we want to read to the plugin
	//This is a hack until there's a proper way of getting the directory filename.
	test.Next(_L("Open RPlugin connection for KModifierPos"));
	r = rplugin.Open(TheFs,KModifierPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Send dir name down to plugin"));
	r = rplugin.DoControl(KPluginSetDirFullName,dirname1Pckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();


	test.Next(_L("Opening directory "));
	r = dir.Open(TheFs,filename1,KEntryAttMatchMask);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Read One entry via plugin - \\"));
	TEntry oneEntry;
	r= dir.Read(oneEntry);

	test.Next(_L("Compare Read One entry with Read Packed 1st TEntry"));
	RDebug::Print(_L("ReadOne Filename : %S"),&oneEntry.iName);

	safe_test(oneEntry.iName.Compare(a.iName),__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	dir.Close();
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1344
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to create and open a new file.
//! @SYMPREQ					REQ8113
//! @SYMTestPriority			High
//! @SYMTestActions				TestCreate() creates and opens a new file.
//!								Following this, it sends a write request to the file.
//!
//!								1.	Set up test file name and path.
//!								2.  Delete any existing test file.
//!								3.  Send create request to create and open a new file.
//!								4.	Write to file.
//!								5.	Delete file.
//!								6.  Close test file.
//!
//! @SYMTestExpectedResults		1.	Test file created without any panic.
//!								2.  Any existing test file deleted without any panic.
//!								2.	Create request intercepted by plugin and completes without any panic.
//!								3.  Write request comletes without any panic.
//!								4.	Test file deleted without any panic.
//!								6.  Test file close without any panic.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestCreate()
	{
	RFile file;

	test.Next(_L("Creating file createtest.tst"));

	TBuf<256> filename1;

	//setting up test files
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\createtest.tst"));

	TInt r = TheFs.Delete(filename1);
	if(r == KErrNotFound || r == KErrPathNotFound)
		r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = file.Create(TheFs, filename1, EFileWrite);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Write to file"));
	r=file.Write(_L8("Hello World"),11);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Closing created file createtest.tst"));
	file.Close();

	r = TheFs.Delete(filename1);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1345
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to open a file
//! @SYMPREQ					REQ8113
//! @SYMTestPriority			High
//! @SYMTestActions				TestOpen() opens a file for write access.
//!								Following this, it sends a write request via a plugin to the fileserver
//!
//!								1.	Set up test file name and path.
//!								2.  Open file for write access.
//!								3.	Write to file.
//!								4.  Close test file.
//!
//!
//! @SYMTestExpectedResults		1.	Test files created without any panic.
//!								2.	Open request intercepted by plugin and completes without any panic.
//!								3.  Write request comletes without any panic.
//!								4.	File close without any panic.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestOpen()
	{
	RFile file;

	TBuf<256> filename;
	TBuf8<10> content;

	//setting up test files
	filename.Append(gDriveToTest);
	filename.Append(_L(":\\data\\test.tst"));
	content.Copy(_L8("1234567890"));
	ReplaceFiles(filename, content,__LINE__);

	test.Next(_L("Opening file test.tst"));
	// we assume that file test.tst does not exists
	TInt r = file.Open(TheFs, filename, EFileWrite);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Write to file test.tst"));
	r=file.Write(_L8("Hello World"),11);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Closing created file test.tst"));
	file.Close();
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1348
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to create and open a temporary file.
//! @SYMPREQ					REQ8113
//! @SYMTestPriority			High
//! @SYMTestActions				TestTemp() sends a temp request via a plugin to the fileserver
//!								Following this, it writes to the file.
//!
//!								1.	Set up test path.
//!								2.  Send temp request to create and open a temporary file.
//!								3.	Write to file.
//!								4.  Close test file.
//!
//! @SYMTestExpectedResults		1.	Test path created without any panic.
//!								2.	Temp request intercepted by plugin and completes without any panic.
//!								3.  Write request comletes without any panic.
//!								4.	File close without any panic.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestTemp()
	{
	RFile file;
	TFileName fileName;

	TBuf<256> filename1;
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\"));

	TInt r = file.Temp(TheFs, filename1,fileName, EFileWrite);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r=file.Write(_L8("Temp File"),9);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file.Close();
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1346
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to replace a file
//! @SYMPREQ					REQ8113
//! @SYMTestPriority			High
//! @SYMTestActions				TestReplace() opens a file for writing by creating a new file
//!								Following this, it sends a write request via a plugin to the fileserver.
//!                             It also replaces an existing file with another.
//!
//!								1.	Set up test file name and path.
//!								2.  Delete any existing test file.
//!								3.  Send replace request to create and open a new file for write.
//!								4.  Write to file.
//!								5.	Close test file.
//!								6.	Send replace request to create and open a new file for write.
//!								7.	Delete test files.
//!
//! @SYMTestExpectedResults		1.	Test files created without any panic.
//!								2.	Any existing file deleted without any panic.
//!								3.	Replace request intercepted by plugin and completes without any panic.
//!								4.  Write request comletes without any panic.
//!								5.	Test file close without any panic.
//!								5.	Test file deleted without any panic.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestReplace()
	{
	RFile file;

	test.Next(_L("Creating file replacetest.tst"));

	TBuf<256> filename1;
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\replacetest.tst"));


	TBuf<256> filename2;
	filename2.Append(gDriveToTest);
	filename2.Append(_L(":\\data\\replacetest2.tst"));


	TInt r = TheFs.Delete(filename1);
	if(r == KErrNotFound || r == KErrPathNotFound)
		r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = file.Replace(TheFs, filename1, EFileWrite);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Write to file"));
	r=file.Write(_L8("Hello World"),11);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Closing replaced file replacetest.tst"));
	file.Close();

	r = TheFs.Replace(filename1,filename2);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.Delete(filename1);
	if(r==KErrNotFound) r = KErrNone;
	else r = KErrGeneral;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.Delete(filename2);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1347
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to read a section of a file
//! @SYMPREQ					REQ7902
//! @SYMTestPriority			High
//! @SYMTestActions				TestReadFileSection() creates and opens a file for writing.
//!								Following this, it reads a section of the file by sending a ReadFileSection
//!								request via a plugin to the fileserver.
//!
//!								1.	Set up test file name and path.
//!								2.  Delete any existing test file.
//!								3.  Send create request to create and open a test file for write.
//!								4.  Write to test file.
//!								5.	Close test file.
//!								6.	Send Readfilesection request to read a section of the testfile.
//!								7.	Verify that the correct section was read.
//!								8.	Delete test files.
//!
//! @SYMTestExpectedResults		1.	Test files created without any panic.
//!								2.	Any existing file deleted without any panic.
//!								3.	Create request completes without any panic.
//!								4.  Write request comletes without any panic.
//!								5.  close request comletes without any panic.
//!								6.  Readfilesection request intercepted by plugin and completes without any panic.
//!								7.	Correct file section read.
//!								8.	Test file deleted without any panic.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestReadFileSection()
{

	test.Next(_L("Reading a file section enter"));

	RFile file;

	TBuf<256> filename1;
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\testReadFileSection.tst"));


	TInt r = TheFs.Delete(filename1);
	if(r == KErrNotFound || r == KErrPathNotFound)
		r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = file.Create(TheFs, filename1, EFileWrite);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Write to file"));
	r=file.Write(0, _L8("Read File Section"));
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file.Close();

	TBuf8<64> temp1;
	temp1.FillZ(64);

	test.Next(_L("ReadFileSection:Enter"));
	r=TheFs.ReadFileSection(filename1,0,temp1,9);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Printf(_L("ReadFileSection - read: %s\n"),&temp1);
	test.Printf(_L("ReadFileSection - temp1.Length()=%d\n"),temp1.Length());


	test.Next(_L("ReadFileSection:Exit"));
	test.Printf(_L("ReadFileSection - read: %s"),temp1.Ptr());
	TInt compare = (temp1.Length()==9);
	if(compare) compare = KErrNone;
	safe_test(compare,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	compare = (temp1==_L8("Read File")); // This should be .Compare()?
	if(compare) compare = KErrNone;
	safe_test(compare,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Reading a file section complete"));

	test.Next(_L("ReadFileSection:Delete"));
	r = TheFs.Delete(filename1);
	if(r == KErrNotFound || r == KErrPathNotFound)
		r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1342
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to change and get the size of a file
//! @SYMPREQ					REQ7901
//! @SYMTestPriority			High
//! @SYMTestActions				TestFileSize() creates and opens a file for read.
//!								Following this, it performs operations on the test file to set and change its size
//!								request via a plugin to the fileserver.
//!
//!								1.	Set up test file name and path.
//!								2.	Open first test file for read
//!								3	Retrieve size of test file.
//!								4	Close first test file
//!								5	Open second test file for read
//!								6	Retrieve size of test file
//!								7   Verify that size of both first and second files are the same
//!								8	Close second test file
//!								9.	Open third test file for write
//!								10	Retrieve size of test file.
//!								11	Change the size of test file by reducing the size.
//!								12.	Retrieve new size of test file.
//!								13.	Verify that new size of file is the same as the size to which the file was changed to.
//!								14.	Change the size of test file by increasing it.
//!								16.	Retrieve new size of test file.
//!								17.	Close third test file
//!								18.	Verify that new size of file is the same as the size to which the file was changed to.

//!
//! @SYMTestExpectedResults		1.	Test files created without any panic.
//!								2.	Test files created without any panic.
//!								3.	First test file opened for read without any panic
//!								4.	KErrNone returned and current file size returned directly without any panic
//!								5.	First file closed with no panic
//!								6.	Second test file opened for read without any panic
//!								7.	KErrNone returned and current size of first file returned via  a plug in directly without any panic
//!								8.	Size of both files are the same.
//!								9.	Second file closed with no panic
//!								10.	Third test file opened for read without any panic
//!								11.	KErrNone returned  and current file size returned directly without any panic
//!								12.	KErrNone returned and file size changed directly without any panic
//!								13.	KErrNone returned and current file size returned directly without any panic
//!								14.	Size of both files are the same.
//!								15.	KErrNone and current file size changed directly without any panic
//!								16.	Current file size returned directly without any panic
//!								17.	Third file closed with no panic
//!								18.	Size of both files are the same.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestFileSize()
	{
	RFile file;

	// Drive thread Direct
	TBuf<256> filename1;
	TBuf<256> filename2;
	TBuf<256> filename3;
	TBuf8<10> content;

	//setting up test files
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\test.size.2"));
	content.Copy(_L8("0987654321"));
	ReplaceFiles(filename1, content,__LINE__);

	filename2.Append(gDriveToTest);
	filename2.Append(_L(":\\data\\test.size"));
	ReplaceFiles(filename2, content,__LINE__);

	filename3.Append(gDriveToTest);
	filename3.Append(_L(":\\data\\test.setsize"));
	ReplaceFiles(filename3, content,__LINE__);

	TInt r = file.Open(TheFs, filename1, EFileRead);
    TInt size=0;
    r = file.Size(size);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
    file.Close();

    //Via Plugin
	r = file.Open(TheFs, filename2, EFileRead);
    TInt psize=0;
    r = file.Size(psize);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
    file.Close();

    r = psize==size;
    if(r) r = KErrNone;
    safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.exe"));

    //Via Plugin
	r = file.Open(TheFs, filename3, EFileWrite);
    size=0;
    r = file.Size(size);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

    r = file.SetSize(size-1);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

    TInt endSize=0;
    r = file.Size(endSize);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = (endSize==size-1);
	if(r) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

    r = file.SetSize(endSize+1);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

    r = file.Size(endSize);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

    file.Close();

    r = endSize==size;
    if(r) r=KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1340
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to lock a section of a file
//! @SYMPREQ					REQ7902
//! @SYMTestPriority			High
//! @SYMTestActions				TestFileLock() creates and opens a file
//!								Following this, it performs operations on the test file to lock and unlock the file
//!								by sending request via a plugin to the fileserver.
//!
//!								1.	Set up test file name and path.
//!								2.	Open first test file for write
//!								3.	Open same test file read
//!								4.	Lock test file open for write.
//!								5.	Read data from test file open for read
//!								6.	Unlock test file opened for write.
//!								7.	Close opened files
//!								8.	Open second test file for write
//!								9.	Open same test file read
//!								10.	Lock test file open for write.
//!								11.	Read data from test file open for read
//!								12.	Unlock test file opened for write.
//!								13.	Close opened files
//!
//! @SYMTestExpectedResults		1.	Test files created without any panic.
//!								2   First test file opened for write without any panic
//!								3.	First test file opened for read without any panic
//!								4.	First test file is locked via plug-In without any panic
//!								5.	KErrLocked is returned from read request
//!								6.	Test file opened for write is unlocked without any panic
//!								7.	Second test file opened for write without any panic
//!								8.	Files closed with no panic
//!								9.	Second test file opened for read without any panic
//!								10.	Secondt test files is locked directly without any panic
//!								11.	KErrLocked is returned from read request
//!								12.	Test file opened for write is unlocked without any panic
//!								13.	Files closed with no panic
//!
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestFileLock()
	{

	//Lock Via Plugin
	RFile file;
    TBuf8<64> wbuffer;
    wbuffer.Copy(_L8("TestTestTest"));

	// Drive thread Direct
	TBuf<256> filename1;
	TBuf<256> filename2;
	TBuf8<10> content;

	//setting up test files
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\test.lock"));
	content.Copy(_L8("0987654321"));
	ReplaceFiles(filename1, content,__LINE__);


	filename2.Append(gDriveToTest);
	filename2.Append(_L(":\\data\\test.tst"));
	content.Copy(_L8("1234567890"));
	ReplaceFiles(filename2, content,__LINE__);


    test.Next(_L("Opening file test.lock"));

    TInt r = file.Open(TheFs, filename1, EFileShareReadersOrWriters | EFileWrite);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	RFile file2;
	r = file2.Open(TheFs, filename1, EFileShareReadersOrWriters | EFileRead );
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	TBuf8<64> lockbuffer;
	r = file.Lock(0,2); // This will lock via plugin call to RFilePlugin::Lock
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = file2.Read(lockbuffer);
	// Test & Ensure KErrLocked returned.
	if(r==KErrLocked) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file2.Close();

	//Unlock file (was locked in plugin)
	r = file.UnLock(0,2);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	
	file.Close();

	//Lock via RFile
    wbuffer.Copy(_L8("TestTestTest"));
    test.Next(_L("TestFileLock(): Opening file test.tst"));
    // we assume that file test.tst still exists
    r = file.Open(TheFs, filename2, EFileShareReadersOrWriters | EFileWrite);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = file2.Open(TheFs, filename2, EFileShareReadersOrWriters | EFileRead );
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = file.Lock(0,2); // This will lock via drive thread directly; RFile::Lock
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = file2.Read(lockbuffer);

	r = (r==KErrLocked);
	if(r) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	file2.Close();

	//Unlock file (was locked in plugin)
	r = file.UnLock(0,2);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	file.Close();

	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1341
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to get current file position
//! @SYMPREQ					REQ7901
//! @SYMTestPriority			High
//! @SYMTestActions				TestFileSeek() creates and opens a file for writing.
//!								Following this, it sends a request to get the current file position
//!
//!
//!								1.	Set up test file name and path.
//!								2.  Open first test file for read.
//!								3.  Send seek request to retrieve current file position of using a zero offset.
//!								4.  Close test file.
//!								5.	Open second test file for read
//!								6.	Send seek request to retrieve current file position of using a zero offset.
//!								7.  Close test file.
//!							    8.  Verify that current file position for both files are the same.
//!
//! @SYMTestExpectedResults		1.	Test files created without any panic.
//!								2.	Test file opened for read without any panic.
//!								3.	Seek request intercepted by plugin and completes without any panic.
//!								4.  Close request completes without any panic.
//!								5.  Test file opened for read without any panic.
//!								6.  Seek request intercepted by plugin and completes without any panic.
//!								7.	Close request completes without any panic.
//!								8.	Current file position for both files are the same.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestFileSeek()
	{
	RFile file;

	// Drive thread Direct
	TBuf<256> filename1;
	TBuf<256> filename2;
	TBuf8<10> content;

	//setting up test files
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\Data\\test.seek"));
	content.Copy(_L8("0987654321"));
	ReplaceFiles(filename1, content,__LINE__);


	filename2.Append(gDriveToTest);
	filename2.Append(_L(":\\data\\test.tst"));
	content.Copy(_L8("1234567890"));
	ReplaceFiles(filename2, content,__LINE__);

	// Drive thread
	TInt r = file.Open(TheFs,filename2, EFileRead);
    TInt pos=0;
    TSeek mode = ESeekCurrent;
    r = file.Seek(mode,pos);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
    file.Close();

    //Via Plugin
	r = file.Open(TheFs, filename1, EFileRead);
    TInt pos2=0;
    mode = ESeekCurrent;
    r = file.Seek(mode,pos2);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
    file.Close();

    r = (pos == pos2);
    if(r) r = KErrNone;
    safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}

//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1349
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Client request to get and set both the attributes for a file or directory
//! @SYMPREQ					REQ7901
//! @SYMTestPriority			High
//! @SYMTestActions				TestFileEntry() sets up a test file
//!								Following this, it perfoms operations on the file and direct to get and set attributes
//!
//!
//!								1.	Set up test file name and path.
//!								2.	Send get entry request for test file
//!								3.	Set new date and time
//!								4.	Send set entry request to test file using new data and time
//!								5.	Send get entry request for test file
//!
//!
//! @SYMTestExpectedResults		1.	Test files created without any panic.
//!								2.	KErrNone returned and get entry request completes via plug in without any panic
//!								3.	Date and time set without any panic
//!								4.	KErrNone returned and set entry request completes via plug in without any panic
//!								5	KErrNone returned and get entry request completes via plug in without any panic
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestEntry()
	{
	TEntry entry;

	//Drive thread Direct
	TBuf<256> filename1;
	TBuf8<10> content;

	//setting up test files
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\test.size"));
	content.Copy(_L8("0987654321"));
	ReplaceFiles(filename1, content,__LINE__);

	TInt r = TheFs.Entry(filename1, entry);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

    test.Next(_L("Test SetEntry - Set to Read Only"));
	TDateTime dateTime(2008,EMay,27,15,35,0,0);
	TTime time(dateTime);
	r=TheFs.SetEntry(filename1,time,KEntryAttReadOnly,KEntryAttArchive);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	TEntry entry1;
	r = TheFs.Entry(filename1, entry1);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = entry1.iModified == dateTime;
	if(r) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = (entry1.iAtt == ((entry.iAtt &~ KEntryAttArchive) | KEntryAttReadOnly));
	if(r) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

    test.Next(_L("Test SetEntry - Remove Read Only attribute and compare"));
	r=TheFs.SetEntry(filename1,time,0,KEntryAttReadOnly);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.Entry(filename1, entry1);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = entry1.iModified == dateTime;
	if(r) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = (entry1.iAtt == (entry.iAtt &~ KEntryAttArchive));
	if(r) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}

//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1352
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Testing various negative scenarios using lock ,read and close
//! @SYMPREQ					REQ8114
//! @SYMTestPriority			High
//! @SYMTestActions				TestComLock() sets up test files
//!								Following this, it perfoms lock, read and close operations the file via
//!								a  plugin
//!
//!
//! @SYMTestExpectedResults		Operations complete without any panic
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestComLock()
	{
	//Lock Via Plugin
	RFile file;
	RFile file2;
    TBuf8<64> wbuffer;
	TBuf<256> filename1;
	TBuf<256> filename2;
	TBuf8<10> content;

    wbuffer.Copy(_L8("TestLockRead"));

	//setting up test files
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\test.lockread"));
	content.Copy(_L8("0987654321"));
	ReplaceFiles(filename1, content,__LINE__);

	filename2.Append(gDriveToTest);
	filename2.Append(_L(":\\data\\test.lockclose"));
	content.Copy(_L8("1234567890"));
	ReplaceFiles(filename2, content,__LINE__);

   	// Lock and post intercept read
	test.Next(_L("Opening file test.lockread"));

	TInt r = file2.Open(TheFs, filename1, EFileShareReadersOrWriters | EFileWrite );
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = file.Open(TheFs, filename1, EFileShareReadersOrWriters | EFileRead );
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//This will lock via plugin call to RFilePlugin::Lock and request a Read from post-interception
	r = file.Lock(0,2);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file2.Close();

	//Unlock file (was locked in plugin)
	r = file.UnLock(0,2);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file.Close();

	// Lock and post intercept close
	test.Next(_L("Opening file test.lockclose"));

	r = file2.Open(TheFs, filename2, EFileShareReadersOrWriters | EFileWrite );
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	 r = file.Open(TheFs, filename2, EFileShareReadersOrWriters | EFileRead );
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//This will lock via plugin call to RFilePlugin::Lock and request a close from post-interception
	r = file.Lock(0,2);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file2.Close();
	//Unlock file (was locked in plugin)
	r= file.UnLock(0,2);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file.Close();
	}

//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1351
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Testing various negative scenarios using read, readfilesection and size
//! @SYMPREQ					REQ7902
//! @SYMTestPriority			High
//! @SYMTestActions				TestComReadFileSection() sets up a test file
//!								Following this, it perfoms read request which is intercepted by a plugin
//!			                    and replaced by a readfilesection request
//!
//! @SYMTestExpectedResults	    Operations complete without any panic
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestComReadFileSection()
	{
	RFile file;
	TBuf8<4> narrow_buffer;
	narrow_buffer.FillZ(4);
	TBuf16<4> wide_buffer;
	wide_buffer.FillZ(4);
	TBuf<256> filename;
	TBuf8<10> content;

	//setting up test files
	filename.Append(gDriveToTest);
	filename.Append(_L(":\\data\\test.readfile"));
	content.Copy(_L8("1234567890"));
	ReplaceFiles(filename, content,__LINE__);


	//opening test file
	TInt r = file.Open(TheFs,filename, EFileRead);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Reading from a file via a plugin using CFsPlugin::FileRead but issuing a ReadFileSection request"));
	r = file.Read(narrow_buffer); // -> returns data from pos 0 to 3
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	wide_buffer.Copy(narrow_buffer);
	RDebug::Print(_L("read: %S\n"), &wide_buffer);
	TInt size=0;
	r = file.Size(size);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	plugin_test(test,KPreModifierPos,__LINE__,(TText*)Expand("premodifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	RDebug::Print(_L("RFile::size = %d"),size);

	test.Next(_L("Reading from a file via a plugin using CFsPlugin::FileRead but issuing a ReadFileSection request complete"));
	file.Close();
	}



void DismountAndUnloadPlugins()
	{
	test.Next(_L("Un-Loading Observer plugin"));
	TInt r = TheFs.DismountPlugin(KObserverPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.RemovePlugin(KObserverPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Un-Loading Modifier plugin"));
	r = TheFs.DismountPlugin(KModifierPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.RemovePlugin(KModifierPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Un-Loading Pre-Modifier plugin"));
	r = TheFs.DismountPlugin(KPreModifierPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.RemovePlugin(KPreModifierPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1357
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Testing plugin interception on certain drives
//! @SYMPREQ					REQ8109
//! @SYMTestPriority			High
//! @SYMTestActions				LoadPluginsForSpecificDriveTests() mounts three plugins and send request down the plugins
//!
//!								1.	Add and mount plugins.
//!								2.	Send replace and  open request.
//!								3.	close test files
//!								4.	Dismount plugins.
//!
//! @SYMTestExpectedResults	    1.  Plugin added and mounted without any panic.
//!								2.  Requests intercepted by plugins.
//!								3.	Test file closed with no panic.
//!								4.	Plugin dismount without any panic.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void LoadPluginsForSpecificDriveTests()
	{
	TInt r = KErrNone;

	//Add Plugins for specific drive.
	test.Next(_L("Loading DriveC plugin"));
	r = TheFs.AddPlugin(KDriveCPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Mounting DriveC plugin on a different drive 25 (Z) : should return -5 KErrNotSupported"));
	r = TheFs.MountPlugin(KDriveCPluginName,25);
	test(r==KErrNotSupported);

	test.Next(_L("Mounting DriveC plugin on drive 2 (C)"));
	r = TheFs.MountPlugin(KDriveCPluginName,2);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));


	test.Next(_L("Loading DriveZ plugin"));
	r = TheFs.AddPlugin(KDriveZPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));


	test.Next(_L("Mounting DriveZ plugin on drive (Z)"));
	r = TheFs.MountPlugin(KDriveZPluginName,KPluginMountDriveZ);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Loading AllSupportedDrives plugin"));
	r = TheFs.AddPlugin(KAllSupportedDrivesPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));


	test.Next(_L("Mounting AllSupportedDrives plugin on all supported drives"));
	r = TheFs.MountPlugin(KAllSupportedDrivesPluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));


	#ifdef __WINS__
		RFile file;
		r = file.Replace(TheFs,_L("x:\\drivexplugin.txt"),EFileWrite);
		safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
		file.Close();

		r = file.Open(TheFs,_L("z:\\TEST\\clean.txt"),EFileRead);
		//if this fails, did you forget to do a \f32test\group\wintest ?
		plugin_test(test,KDriveZPos,__LINE__,(TText*)Expand("drivez_plugin.cpp"));
		safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp")); //if this fails, did you forget to do a \f32test\group\wintest ?
		file.Close();

		r = file.Replace(TheFs,_L("c:\\drivecplugin.txt"),EFileWrite);
		plugin_test(test,KDriveCPos,__LINE__,(TText*)Expand("drivec_plugin.cpp"));
		safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

		file.Close();

	#endif

	r = TheFs.DismountPlugin(KAllSupportedDrivesPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.RemovePlugin(KAllSupportedDrivesPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.DismountPlugin(KDriveZPluginName,25);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.RemovePlugin(KDriveZPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.DismountPlugin(KDriveCPluginName,2);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.RemovePlugin(KDriveCPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}

//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1358
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Testing unremovable plugins
//! @SYMPREQ					REQ8794
//! @SYMTestPriority			High
//! @SYMTestActions				LoadUnremovablePluginAndTest() mounts a unremovable plugin and attempts to
//!                             dismounts it
//!
//!								1.	Add and mount an unremovale plugin.
//!								2.	Attempt to dismount plugin.
//!								3.	Make plugin removable.
//!								4.	Attempt to dismount plugin.
//!
//! @SYMTestExpectedResults	    1.  Unremovale plugin added and mounted without any panic.
//!								2.  KErrAccessDenied returned and plugin does not dismount.
//!								3.	KErrNone returned
//!								4.	Plugin dismount without any panic.

//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void LoadUnremovablePluginAndTest()
	{
	TInt r = KErrNone;

	//Add Unremovable Plugin.
	test.Next(_L("Loading Unremovable plugin"));
	r = TheFs.AddPlugin(KUnremovablePluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//Mount
	test.Next(_L("Mounting Unremovable Plugin"));
	r = TheFs.MountPlugin(KUnremovablePluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//Attempt to dismount this plugin
	//It's setup such that it is intercepting EFsDismountPlugin
	//and returns KErrAccessDenied.
	//Mount
	test.Next(_L("Attempting to dismount Unremovable Plugin"));
	r = TheFs.DismountPlugin(KUnremovablePluginName);
	r = (r==KErrPermissionDenied);
	if(r)
		{
		r = KErrNone;
		}
	else
		{
		r = KErrGeneral;
		}
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	MyRPlugin rplugin;
	TPckg<TBool> removablePckg(ETrue);
	test.Next(_L("Open RPlugin connection for UnremovablePlugin"));
	r = rplugin.Open(TheFs,KUnremovablePos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Set removable to true"));
	r = rplugin.DoControl(KPluginSetRemovable,removablePckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	//Attempt to dismount this plugin
	//Should now work.
	test.Next(_L("Dismounting Unremovable Plugin"));
	r = TheFs.DismountPlugin(KUnremovablePluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Removing Unremovable Plugin"));
	r = TheFs.RemovePlugin(KUnremovablePluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}

//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1353
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Testing that plug-ins can perform file reads and writes regardless of the permissions a file is open with.
//!
//! @SYMPREQ					REQ8114
//! @SYMTestPriority			High
//! @SYMTestActions				LoadExclusiveAccessPluginAndTest() mounts a plugin and performs operations on files
//!								open in exclusive access mode.
//!
//!								1.	Add and mount plugin.
//!								2.	Replace a file and open it in exclusive access mode.
//!								3.	Send a read request down.
//!								4.	Send a write request down.
//!								5.	Verify data sent down.
//!								6.	Close test file.
//!								7.	Dismount plugin.
//!
//! @SYMTestExpectedResults	    1.  Plugin added and mounted without any panic.
//!								2.  Request intercepted by plugin test file opened in exclisive access mode
//!								3.	Read request completes and KErrNone returned.
//!								4.	Write request completes and KErrNone returned.
//!								5.  Correct data returned.
//!								6.	Test file closed without any panic.
//!								7.	Plugin dismount without any panic.
//!
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void LoadExclusiveAccessPluginAndTest()
	{
	TInt r = KErrNone;

	//Add Plugins for specific drive.
	test.Next(_L("Loading Exclusive Access plugin"));
	r = TheFs.AddPlugin(KExclusiveAccessPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//Mount
	test.Next(_L("Mounting Exclusive Access Plugin"));
	r = TheFs.MountPlugin(KExclusiveAccessPluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	TBuf<256> filename;
	filename.Append(gDriveToTest);
	filename.Append(_L(":\\exclusiveaccess.file"));

	test.Next(_L("Replacing file and opening it in ReadOnly mode"));
	test.Printf(_L("Filename : %s\n"), &filename);

	//Replace a file and open it in exclusive access mode.
	RFile file;
	r = file.Replace(TheFs,filename, EFsFileRead | EFsFileWrite | EFileShareExclusive);
	plugin_test(test,KExclusiveAccessPos,__LINE__,(TText*)Expand("exclusiveaccess_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//Try to send a read request down.
	TBuf8<256> rdata;
	r = file.Read(rdata);
	plugin_test(test,KExclusiveAccessPos,__LINE__,(TText*)Expand("exclusiveaccess_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	_LIT8(KData,"0123456789");
	TPtrC8 wdata(KData());

	//Try to send a write request down.
	r = file.Write(wdata);
	plugin_test(test,KExclusiveAccessPos,__LINE__,(TText*)Expand("exclusiveaccess_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//We need a way to determine whether the plugin did actually write anything.
	//Just read the data back and compare?
	r = file.Read(rdata);
	plugin_test(test,KExclusiveAccessPos,__LINE__,(TText*)Expand("exclusiveaccess_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp")); //read should be ok
	safe_test(rdata.Compare(wdata),__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file.Close();

	test.Next(_L("Un-Mounting ExclusiveAccess plugin"));
	r = TheFs.DismountPlugin(KExclusiveAccessPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Removing ExclusiveAccess plugin"));
	r = TheFs.RemovePlugin(KExclusiveAccessPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}
//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1355
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Testing various scenarios using write, replace, read, Entry and Dir
//!
//! @SYMPREQ					REQ8110
//! @SYMTestPriority			High
//! @SYMTestActions				LoadCombinationPluginAndTest() mounts two plugins and performs operations on files and directories
//!
//!
//!
//! @SYMTestExpectedResults	    Operations complete without any panic
//!
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void LoadCombinationPluginAndTest()
	{
	TInt r = KErrNone;
	RFile file;
	TBuf<256> filename1;
	TBuf<256> filename2;
	TBuf<256> filename3;
	TBuf<256> filename4;
	TBuf<256> filename6;
	TBuf<256> filename7;
	TBuf<256> filename8;
	TBuf<256> dirname1;
	TBuf<256> dirname2;
	TBuf<256> dirname3;
	TBuf8<10> content;
	TBuf8<64> wbuffer;

	test.Next(_L("Loading Combinational2 plugin"));
	r = TheFs.AddPlugin(KCombinational2PluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Mounting Combinational2 plugin"));
	r = TheFs.MountPlugin(KCombinational2PluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	// Load Plugins
	test.Next(_L("Loading Combinational plugin"));

	r = TheFs.AddPlugin(KCombinationalPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Mounting Combinational plugin"));
	r = TheFs.MountPlugin(KCombinationalPluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//Use RPlugin to communicate to the plugins which drive they should be
	//testing on.
	//This is needed because sometimes a plugin may open a different file etc and will need
	//to know which drive it should be testing on.

	test.Next(_L("Open RPlugin connection for CombinationalPlugin"));
	MyRPlugin rplugin;
	r = rplugin.Open(TheFs,KCombinationalPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	TPckg<TChar> drivePckg(gDriveToTest);

	test.Next(_L("Send drive letter to test down to plugin"));
	r = rplugin.DoControl(KPluginSetDrive,drivePckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();



	//setting up test files
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\combi.txt"));
	content.Copy(_L8("combi0"));
	ReplaceFiles(filename1, content,__LINE__);

	filename2.Append(gDriveToTest);
	filename2.Append(_L(":\\combi1.txt"));
	content.Copy(_L8("combi1"));
	ReplaceFiles(filename2, content,__LINE__);

	filename3.Append(gDriveToTest);
	filename3.Append(_L(":\\combi2.txt"));
	content.Copy(_L8("combi2"));
	ReplaceFiles(filename3, content,__LINE__);

	filename4.Append(gDriveToTest);
	filename4.Append(_L(":\\combiReplace.txt"));

	//File Tests

	r = file.Open(TheFs, filename1, EFileWrite);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	wbuffer.FillZ(64);
	wbuffer.Copy(_L8("CombiCombiCombi"));
	r = file.Write(2,wbuffer);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file.Close();

	RFile file2;

	r = file2.Replace(TheFs,filename4,EFileWrite);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	file2.Close(); //combiReplace

	TEntry entry;
	r = TheFs.Entry(filename1,entry);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	plugin_test(test,KCombinational2Pos,__LINE__,(TText*)Expand("combinational2_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	TEntry entry2;
	r = TheFs.Entry(filename4,entry2);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	plugin_test(test,KCombinational2Pos,__LINE__,(TText*)Expand("combinational2_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//DIRECTORY TESTS

	//Lets make some directories (allow KErrAlreadyExists)
	//Then open 1 via RDir and then open the 2 from the plugin then open 3
	//Return the contents of 3, and then read 3's contents from
	// RDir and compare that it's the same.

	dirname1.Append(gDriveToTest);
	dirname1.Append(_L(":\\dir1\\"));

	dirname2.Append(gDriveToTest);
	dirname2.Append(_L(":\\dir2\\"));

	dirname3.Append(gDriveToTest);
	dirname3.Append(_L(":\\dir3\\"));


	r = TheFs.MkDir(dirname1);
	if(r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.MkDir(dirname2);
	if(r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.MkDir(dirname3);
	if(r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));


	filename6.Append(gDriveToTest);
	filename7.Append(gDriveToTest);
	filename8.Append(gDriveToTest);

	filename6.Append(_L(":\\dir1\\dir1.file"));
	filename7.Append(_L(":\\dir2\\dir2.file"));
	filename8.Append(_L(":\\dir3\\dir3.file"));

	r = file.Replace(TheFs,filename6,EFileWrite);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	file.Close();

	r = file.Replace(TheFs,filename7,EFileWrite);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	file.Close();

	r = file.Replace(TheFs,filename8,EFileWrite);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	file.Close();

	RDir dir;
	r = dir.Open(TheFs,dirname1,KEntryAttNormal);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));


	//Need to send which filename we want to read to the plugin
	//This is a hack until there's a proper way of getting the directory filename.

	test.Next(_L("Open RPlugin connection for CombinationalPlugin"));
	r = rplugin.Open(TheFs,KCombinationalPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Send dir name down to plugin"));

	typedef TBuf<256> TDirName;
	TPckg<TDirName> dirname1Pckg(dirname1);

	r = rplugin.DoControl(KPluginSetDirFullName,dirname1Pckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	TEntryArray entryarray1;
	r = dir.Read(entryarray1); //returns read results for directory3
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	if(r == KErrEof) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	RDir dir2;
	r = dir2.Open(TheFs,dirname3,KEntryAttNormal);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Open RPlugin connection for CombinationalPlugin"));
	r = rplugin.Open(TheFs,KCombinationalPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Send dir name down to plugin"));
	TPckg<TDirName> dirname3Pckg(dirname3);
	r = rplugin.DoControl(KPluginSetDirFullName,dirname3Pckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	TEntryArray entryarray2;
	r = dir2.Read(entryarray2);
	plugin_test(test,KCombinationalPos,__LINE__,(TText*)Expand("combinational_plugin.cpp"));
	if(r == KErrEof) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//compare results
	safe_test(entryarray1[0].iName.Compare(entryarray2[0].iName),__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//cleanup
	dir.Close();
	dir2.Close();

	TheFs.DismountPlugin(KCombinational2PluginName);
	TheFs.DismountPlugin(KCombinationalPluginName);
	TheFs.RemovePlugin(KCombinational2PluginName);
	TheFs.RemovePlugin(KCombinationalPluginName);
	}

//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1356
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Testing various scenarios using write, replace, read, Entry and Dir
//!
//! @SYMPREQ					REQ8110
//! @SYMTestPriority			High
//! @SYMTestActions				LoadStackedPluginAndTest() mounts three plugins and performs read and write
//!								operations on a test file by sending the request down the stack of plugins
//!
//!
//!
//! @SYMTestExpectedResults	    Operations complete without any panic
//!
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void LoadStackedPluginAndTest()
	{
	RFile file;
	TBuf<256> filename;

	//Setting up test files
    filename.Append(gDriveToTest);
	filename.Append(_L(":\\test.stack"));

	TBuf8<20> content;
	content.Copy(_L8("HELLO SYMBIAN WORLD1"));
	ReplaceFiles(filename, content,__LINE__);


	//Load Plugins
	test.Next(_L("Loading Stacked plugin"));
	TInt r = TheFs.AddPlugin(KStackedPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Mounting Stacked plugin"));
	r = TheFs.MountPlugin(KStackedPluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//Using RPlugin to communicate to the plugins which drive they should be testing on.
	test.Next(_L("Open RPlugin connection for StackedPlugin"));
	MyRPlugin rplugin;
	r = rplugin.Open(TheFs,KStackedPos);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	TPckg<TChar> drivePckg(gDriveToTest);
	test.Next(_L("Send drive letter to test down to plugin"));
	r = rplugin.DoControl(KPluginSetDrive,drivePckg);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	rplugin.Close();

	test.Next(_L("Loading Stacked2 plugin"));
	r = TheFs.AddPlugin(KStacked2PluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Mounting Stacked2 plugin"));
	r = TheFs.MountPlugin(KStacked2PluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Loading Stacked3 plugin"));
	r = TheFs.AddPlugin(KStacked3PluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Mounting Stacked3 plugin"));
	r = TheFs.MountPlugin(KStacked3PluginName);
	if (r == KErrInUse) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));


	//Setting up test data
    TBuf8<20> wbuffer;
	wbuffer.FillZ(20);
	wbuffer.Copy(_L8("HELLO WORLD  SYMBIAN"));

    TBuf8<20> rbuffer;
	rbuffer.FillZ(20);

	//Open  test file and send a write request
	r = file.Open(TheFs, filename, EFileWrite);
	plugin_test(test,KStackedPos,__LINE__,(TText*)Expand("stacked_plugin.cpp"));
	plugin_test(test,KStacked2Pos,__LINE__,(TText*)Expand("stacked2_plugin.cpp"));
	plugin_test(test,KStacked3Pos,__LINE__,(TText*)Expand("stacked3_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//Writing to test file via plugins
	test.Next(_L("Writing to file test.stack"));
    r = file.Write(wbuffer);
    plugin_test(test,KStackedPos,__LINE__,(TText*)Expand("stacked_plugin.cpp"));
    plugin_test(test,KStacked2Pos,__LINE__,(TText*)Expand("stacked2_plugin.cpp"));
    plugin_test(test,KStacked3Pos,__LINE__,(TText*)Expand("stacked3_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	//Reading from test files
	test.Next(_L("Reading file test.stack"));
	plugin_test(test,KStackedPos,__LINE__,(TText*)Expand("stacked_plugin.cpp"));
	plugin_test(test,KStacked2Pos,__LINE__,(TText*)Expand("stacked2_plugin.cpp"));
	plugin_test(test,KStacked3Pos,__LINE__,(TText*)Expand("stacked3_plugin.cpp"));
    r = file.Read(rbuffer);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	// Check data returned
	test.Next(_L("Comparing buffers"));
	r = wbuffer.Compare(rbuffer);
	plugin_test(test,KStackedPos,__LINE__,(TText*)Expand("stacked_plugin.cpp"));
	plugin_test(test,KStacked2Pos,__LINE__,(TText*)Expand("stacked2_plugin.cpp"));
	plugin_test(test,KStacked3Pos,__LINE__,(TText*)Expand("stacked3_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file.Close();

	// Dismounting Plugins
	r = TheFs.DismountPlugin(KStacked3PluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	r = TheFs.DismountPlugin(KStacked2PluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	r = TheFs.DismountPlugin(KStackedPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	// Removing Plugins
	r = TheFs.RemovePlugin(KStacked3PluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	r = TheFs.RemovePlugin(KStacked2PluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	r = TheFs.RemovePlugin(KStackedPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}

//-------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_plugin_v2-1359
//! @SYMTestType				CT
//! @SYMTestCaseDesc			Test calling RFsPlugin::Volume()
//!
//! @SYMDEF						DEF142711: RFsPlugin needs to expose a Volume() API
//! @SYMTestPriority			High
//! @SYMTestActions				TestVolume() sets the volume label and gets the volume
//!								information via the modifier plugin, which will verify
//!								that the volume has the correct label and then change it.
//!
//!								1.	Set volume label to "MyVolume".
//!								2.	Get volume information via modifier plugin.
//!								3.	Verify correct volume label after changes by modifier plugin.
//!								4.	Reset volume label.
//!
//! @SYMTestExpectedResults		1.	Volume label set without any panic.
//!								2.	Volume information request completes without any panic.
//!								3.	Correct volume label after modification.
//!								4.	Volume label reset without any panic.
//!
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
void TestVolume()
	{
	test.Next(_L("TestVolume()"));
	
	TInt theDrive;
	TInt r = TheFs.CharToDrive(gDriveToTest,theDrive);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	
	// Set volume label to MyVolume so we can test it in the modifier plugin
	_LIT(KVolumeLabel1,"1Volume");
	r = TheFs.SetVolumeLabel(KVolumeLabel1,theDrive);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Printf(_L("TestVolume, RFs::SetVolumeLabel returned %d\n"), r);
	
	test.Printf(_L("TestVolume, Get volume information for drive %d via modifier plugin\n"), theDrive);
	TVolumeInfo volInfo;
	r = TheFs.Volume(volInfo,theDrive);
	test.Printf(_L("TestVolume, RFs::Volume returned %d\n"), r);
	plugin_test(test,KModifierPos,__LINE__,(TText*)Expand("modifier_plugin.cpp"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	
	// Check volume name after modification by modifier plugin
	_LIT(KVolumeLabel2,"2Volume");
	r = volInfo.iName.Compare(KVolumeLabel2);
	test.Printf(_L("TestVolume, Compare volume label returned %d\n"), r);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	
	test.Printf(_L("TestVolume, Reset volume label to nothing\n"));
	r = TheFs.SetVolumeLabel(_L(""),theDrive);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	}


GLDEF_C void CallTestsL()
	{
	__UHEAP_MARK;
	TInt theDrive;
	TInt r = TheFs.CharToDrive(gDriveToTest,theDrive);
	test(r == KErrNone);

	TVolumeInfo volInfo;
	r = TheFs.Volume(volInfo, theDrive);
	test (r == KErrNone);
	
	TDriveInfo drvInfo;
	r = TheFs.Drive(drvInfo,theDrive);
	test (r == KErrNone);

	if(drvInfo.iType == EMediaRam || drvInfo.iType == EMediaRom || drvInfo.iMediaAtt == KMediaAttWriteProtected || drvInfo.iMediaAtt == KMediaAttLocked)
		{
		test.Printf(_L("T_PLUGIN_V2 SKIPPED:\n"));
		test.Printf(_L("Local Buffers are not supported on RAM or ROM drives\n"));
		if(drvInfo.iMediaAtt == KMediaAttLocked)
			{
			test.Printf(_L("This media is locked. Drive %d\n"),theDrive);
			}
		return;
		}
	
	test.Next(_L("Loading Observer plugin"));
	r = TheFs.AddPlugin(KObserverPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.MountPlugin(KObserverPluginName, (TUint)gDriveToTest.GetUpperCase() - 65);
	if (r == KErrNotSupported)
		{
		test.Printf(_L("Plugins are not supported on pagable drives.\nSkipping test.\n"));

		r = TheFs.RemovePlugin(KObserverPluginName);
		safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
		
		return;
		}
	
	test.Next(_L("Un-Loading Observer plugin"));
	r = TheFs.DismountPlugin(KObserverPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = TheFs.RemovePlugin(KObserverPluginName);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	TBuf<256> theFSName;

	r = TheFs.FileSystemName(theFSName, theDrive);

	TBool Win32Filesystem = (theFSName.CompareF(_L("Win32")) == 0);

    if(!Win32Filesystem)
        {
		// Drive is not mapped to local pc filesystem so can be formatted
		test.Next(_L("Formating Drive......... "));
		Format(theDrive);
		}

	// This should be changed to an actual mechanism of determining if the drives filesystem and extensions has local buffer support
	TBool LocalBufferSupported = ETrue;

	if (!LocalBufferSupported)
		{
		if((volInfo.iDrive.iType == EMediaRam) && !Win32Filesystem)
			{
			test.Printf(_L("Local Buffers are not supported on RAM drive\n"));
			test.Printf(_L("Skipping Test\n"));
			return;
			}
		else if(IsFileSystemLFFS(TheFs, theDrive))
			{
			test.Printf(_L("Local Buffers are not supported on LFFS drives\n"));
			test.Printf(_L("Skipping Test\n"));
			return;
			}
		else
			{
			test.Printf(_L("Error: Local Buffers are not supported on the selected drive\n"));
			test(EFalse);
			}
		}

	TBuf<10> extensionName;
	for(TInt i = 0; i < 2; i++)
		{
		r = TheFs.ExtensionName(extensionName,CurrentDrive(),i);
		if(r==KErrNone && extensionName.Compare(_L("Nandftl")) == 0)
			{
			test.Printf(_L("Local Buffers are not supported on the nandftl extension\n"));
			test.Printf(_L("Skipping Test\n"));
			return;
			}
		}

	SetupTestFiles();




	test.Next(_L("////////////////////////////"));
	test.Next(_L("////////////////////////////"));
	test.Next(_L("////////////////////////////"));
	test.Next(_L("////////////////////////////"));

	test.Next(_L("Reading a file section enter"));

	RFile file;
	TBuf<256> filename1;
	filename1.Append(gDriveToTest);
	filename1.Append(_L(":\\data\\testReadFileSection.tst"));


	r = TheFs.Delete(filename1);
	if(r == KErrNotFound || r == KErrPathNotFound)
		r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	r = file.Create(TheFs, filename1, EFileWrite);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	test.Next(_L("Write to file"));
	r=file.Write(0, _L8("Read File Section"));
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	file.Close();

	TBuf8<64> temp1;
	temp1.FillZ(64);

	test.Next(_L("ReadFileSection:Enter"));
	r=TheFs.ReadFileSection(filename1,0,temp1,9);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

	TBuf16<64> temp1_wide;
	temp1_wide.Copy(temp1);
	test.Printf(_L("\nReadFileSection - read: %s\n"),&temp1_wide);
	test.Printf(_L("ReadFileSection - temp1.Length()=%d\n\n"),temp1.Length());


	test.Next(_L("ReadFileSection:Exit"));
	test.Printf(_L("ReadFileSection - read: %S"),&temp1);
	TInt compare = temp1.Length()==9;
	if(compare) compare = KErrNone;
	safe_test(compare,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	compare = temp1.Compare(_L8("Read File"));
	compare = (temp1==_L8("Read File")); // This should be .Compare()?
	if(compare) compare = KErrNone;
	safe_test(compare,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	test.Next(_L("Reading a file section complete"));

	test.Next(_L("ReadFileSection:Delete"));
	r = TheFs.Delete(filename1);
	if(r == KErrNotFound || r==KErrPathNotFound)
		r = KErrNone;
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));


	test.Next(_L("////////////////////////////"));
	test.Next(_L("////////////////////////////"));
	test.Next(_L("////////////////////////////"));
	test.Next(_L("////////////////////////////"));


	LoadAndMountPlugins();

	#if defined __WINS__ && defined __DEBUG__
	// Turn on request tracing (for debug purposes)
	r = TheFs.SetDebugRegister(KPLUGIN | KCACHE);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	#endif

	TInt drive = EDriveX;
	TBuf<25> path;
	r = TheFs.Subst(path,drive);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));

#ifndef __WINS__
	// RFs::SetVolumeLabel - Does not run on WINS
	TestVolume();
#endif
	TestReadFileDirect();
	TestReadFileViaPlugin();
	TestWriteFileDirect();
	TestWriteFileViaPlugin();
	//TestFileLock();
	TestFileSeek();
	TestFileSize();
	TestDir();
	TestCreate();
	TestOpen();
	TestReplace();
	TestReadFileSection();
	TestTemp();
	TestEntry();
	TestRename();
	TestComReadFileSection();
	//TestComLock();

	DismountAndUnloadPlugins();

	LoadExclusiveAccessPluginAndTest();
	LoadPluginsForSpecificDriveTests();
	LoadCombinationPluginAndTest();
	LoadStackedPluginAndTest();

	//Unremovable plugin is removed by setting a flag via RPlugin
	LoadUnremovablePluginAndTest();

	__UHEAP_MARKEND;

	#if defined __WINS__ && defined __DEBUG__
	// Turn off request tracing
	r = TheFs.SetDebugRegister(0);
	safe_test(r,__LINE__,(TText*)Expand("t_plugin_v2.cpp"));
	#endif

}
