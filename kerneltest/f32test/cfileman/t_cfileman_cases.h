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
// f32test\cfileman\t_cfileman_cases.h
// 
//

// Define Test Cases
#ifndef T_CFILEMAN_CASES_H
#define T_CFILEMAN_CASES_H

#include "t_cfileman_aux.h"

extern CFileMan* gFileMan;
extern RPointerArray<RFile>* gFileHandles;
extern TBool gAsynch;
extern TRequestStatus gStat;

TChar gFixedDriveValid 		= 'C';
TChar gFixedDriveInvalid 	= '?';
TChar gFixedDriveReadOnly 	= 'Z';
TChar gFixedDriveNotReady 	= 'A';

////////////////////////////////////////////////////////////
//	Test case definitions
////////////////////////////////////////////////////////////
static const TTestCaseUnitaryBasic gBasicUnitaryTestCases[] =
	{
//*****************************Delete API**************************
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0816
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Non-Recursive option 
//! The condition is deleting a file 'FILE01.TXT'
//! @SYMTestActions Deletes the specified file
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   
	
		{
		{816, ECFMDelete, 0, KErrNone, KErrNone, KErrNone}, 
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL,{2,EOB}}},
		},
	
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0817
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Non-Recursive option 
//! The condition is deleting files with wildcard combination '*.*'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------  
		{
		{817, ECFMDelete, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK24}},
		},
		
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0818
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Non-Recursive option 
//! The condition is deleting files with wildcard combination 'FILE*.TXT'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 		
		{
		{818, ECFMDelete, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK02}},
		},
		
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0819
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Non-Recursive option 
//! The condition is deleting files with wildcard combination 'FILE?.TXT'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------  
		{
		{819, ECFMDelete, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {0,1,EOB}}},
		},
		
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0820
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Non-Recursive option 
//! The condition is deleting files with wildcard combination '*.TXT'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
		{
		{820, ECFMDelete, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK20}},
		},
		
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0821
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Non-Recursive option 
//! The condition is deleting files with wildcard combination '*.TXT'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
		{
		{821, ECFMDelete, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {4,6,55,EOB}}},
		},
		
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0822
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Recursive option 
//! The condition is deleting files with combination 'FILE01.TXT'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------	
		{
		{822, ECFMDelete, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,11,20,29,38,EOB}}},
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0823
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Recursive option 
//! The condition is deleting files with wildcard combination 'FILE.*'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
		{
		{823, ECFMDelete, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {{8,17,26,35,EOB},EMPTY}},
		},
		
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0824
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Recursive option 
//! The condition is deleting files with wildcard combination 'FILE*.TXT'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
		{
		{824, ECFMDelete, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK05}},
		},
	
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0825
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Recursive option 
//! The condition is deleting files with wildcard combination 'FILE?.TXT'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------	
		{
		{825, ECFMDelete, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK06}},
		},
		
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0826
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Recursive option 
//! The condition is deleting files with wildcard combination '*.TXT'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
		{
		{826, ECFMDelete, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK36}},
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0827
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Delete()
//! @SYMTestCaseDesc 1.Tests API with Recursive option 
//! The condition is deleting files with wildcard combination 'FILE.*'
//! @SYMTestActions Deletes the specified files
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
		{
		{827, ECFMDelete, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK37}},
		},


//*****************************RmDir API**************************
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0828
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with Recursive option 
//! Removes directory and all files directory structure.
//! @SYMTestActions Removes the specified directory and its contents.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
		{
		{828, ECFMRmDir, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL,BLOCK39}},
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0829
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with Recursive option 
//! Deletes directories with wildcard combination 'DIR*' and directory structure.
//! @SYMTestActions Removes the specified directory and its contents.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------		
		//case 101 :Wild card case. This case shows,directory is considered only if '\\' is given  
		{
		{829, ECFMRmDir, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\DIR*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK39}},
		},
	
//Basic Negative test case for RmDir:
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0955
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Remove a directory with longpath name from Src.
//! @SYMTestActions Remove directory does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   	
	  		{
	   		{955, ECFMRmDir, 0, KErrBadName, KErrBadName, KErrBadName},
			{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26\\fdsa21asdffds\\NAME\\FGHIJ\\\\", 
			(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
			(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
			},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0956
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Remove a File with long name from Src.
//! @SYMTestActions Remove directory does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
   		{
   		{956, ECFMRmDir, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asfdsa21asfdsa22asfdsa23asfdsa24asfdsa25asfdsa26", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0957
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Remove a directory that doesnot exist.
//! @SYMTestActions Remove directory does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrNotReady in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------     	

   		{
   		{957, ECFMRmDir, 0, KErrNotReady, KErrNone, KErrNotReady},
		{&gFixedDriveNotReady, (TText*)L"?:\\Src\\", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},
   	
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0958
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Remove 'Readonly' directory.
//! @SYMTestActions Remove directory does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrInUse in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    	

   		{
   		{958, ECFMRmDir, 0, KErrInUse, KErrNone, KErrInUse},
		{&gFixedDriveReadOnly, (TText*)L"?:\\", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0959
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Remove 'non-available' directory.
//! @SYMTestActions Remove directory does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrPathNotFound in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   	

   		{
   		{959, ECFMRmDir, 0, KErrPathNotFound, KErrNone, KErrPathNotFound},
		{&gDriveToTest, (TText*)L"?:\\Src\\NODIR\\", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0960
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Remove 'Invalid' directory pathname.
//! @SYMTestActions Remove directory does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    		

   		{
   		{960, ECFMRmDir, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gDriveToTest, (TText*)L"C\\Src\\Dir?", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0961
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Remove 'Invalid' directory pathname.
//! @SYMTestActions Remove directory does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------  

			{
			{961, ECFMRmDir, 0, KErrBadName, KErrBadName, KErrBadName},
			{&gFixedDriveInvalid, (TText*)L"::C:", 
			(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
			(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
			},		

//Basic Negative test case for Delete:
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0962
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Delete a file with longpath name from Src.
//! @SYMTestActions Delete files does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    		
   		
		{
		{962, ECFMDelete, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26\\fdsa21asdffds\\NAME\\FGHIJ\\\\", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0963
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Delete a file with longname from Src.
//! @SYMTestActions Delete files does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    

			{
		   	{963, ECFMDelete, 0, KErrBadName, KErrBadName, KErrBadName},
			{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26", 
			(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
			(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
			},


  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0964
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Delete a drives not ready.
//! @SYMTestActions Delete files does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrNotReady in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    	

   		{
   		{964, ECFMDelete, 0, KErrNotReady, KErrNone, KErrNotReady},
		{&gFixedDriveNotReady, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0965
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Delete a file from directory from source that is 'ReadOnly'.
//! @SYMTestActions Delete files does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAccessDenied in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------     

   		{
   		{965, ECFMDelete, 0, KErrAccessDenied, KErrNone, KErrAccessDenied},
		{&gFixedDriveReadOnly, (TText*)L"?:\\test\\", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0966
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Delete a directory from source doesnot exist.
//! @SYMTestActions Delete files does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrPathNotFound in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------     

   		{
   		{966, ECFMDelete, 0, KErrPathNotFound, KErrNone, KErrPathNotFound},
		{&gDriveToTest, (TText*)L"?:\\Src\\NODIR\\", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0967
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Delete a directory with invalid pathname.
//! @SYMTestActions Delete files does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    	

   		{
   		{967, ECFMDelete, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gDriveToTest, (TText*)L"C\\Src\\Dir", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},
  
   //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0968
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::RmDir()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive' option 
//! Delete a directory with invalid pathname.
//! @SYMTestActions Delete files does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 

   		{
   		{968, ECFMDelete, 0, KErrBadName, KErrNone, KErrBadName},
		{&gDriveToTest, (TText*)L"::C:", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		},
	 
//End unary API test cases 		
		{{0}}
	 	
	};

static TTestCaseBinaryBasic gBasicBinaryTestCases[] =
	{
//*****************************Copy API**************************
				
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0830
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Copy a file FILE2.TXT from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
			
		{ 
		{830, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE2.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{1,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0831
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Copy files with wildcard combination '*.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{831, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0832
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Copy files with wildcard combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{832, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0833
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Copy files with wildcard combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{833, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0834
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Copy files with wildcard combination '*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{834, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK20, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0835
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Copy files with wildcard combination 'FILE.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{835, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0836
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy file with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{836, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0837
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination '*.*' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{837, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK24, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0838
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{838, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK02, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0839
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copies files with wildcard combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{839, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{0,1,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0840
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//!	Copies files with wildcard combination '*.TXT' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{840, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK29, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK29, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0841
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copies files with wildcard combination 'FILE.*' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{841, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{4,6,55,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0842
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy file 'FILE01.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{842, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0843
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy files with wildcard combination '*.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{843, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0844
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy files with wildcard combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{844, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0845
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy files with wildcard combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{845, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0846
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy files with wildcard combination '*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{846, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK20, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0847
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy files with wildcard combination 'FILE.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{847, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0848
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Copy file with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{848, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0849
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination '*.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{849, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK01, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0850
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{850, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK02, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0851
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{851, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{0,1,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0852
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination '*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{852, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK03, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK20, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0853
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{853, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{4,6,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0854
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Copy file with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{854, ECFMCopy, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,11,20,29,38,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0855
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Copy files with wildcard combination '*.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{855, ECFMCopy, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0856
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Copy files with wildcard combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 	

		{	
		{856, ECFMCopy, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK05, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0857
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Copy files with wildcard combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{857, ECFMCopy, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK06, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0858
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Copy files with wildcard combination '*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{858, ECFMCopy, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK36, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0859
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Copy files with wildcard combination 'FILE.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{859, ECFMCopy, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK37, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0860
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Copy does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{860, ECFMCopy, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,11,20,29,38,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,11,20,29,38,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0861
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination '*.*' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{861, ECFMCopy, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0862
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
	
		{	
		{862, ECFMCopy, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK05, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK05, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0863
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------		

		{	
		{863, ECFMCopy, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK06, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK06, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0864
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination '*.TXT' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------			

		{	
		{864, ECFMCopy, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK34, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK34, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0865
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE.*' from source to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{865, ECFMCopy, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK37, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK37, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0866
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{866, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,11,20,29,38,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,11,20,29,38,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0867
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination '*.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{867, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK04, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0868
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{868, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK05, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK05, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0869
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{869, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK06, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK06, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0870
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination '*.TXT' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{870, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK36, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK36, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0871
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files with wildcard combination 'FILE.*' from source to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{871, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK37, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK37, EMPTY}}
		},

//*******************Special Cases for Copy*************************
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0872
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy files from Src directory without backward slash to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   

		{	
		{872, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0873
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{873, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0874
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Copies the specified files from source to the target (default session path is taken as target path).
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{874, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0875
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Copies the specified files from source(default session path is taken as source path) to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{875, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0876
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy Src to the target while renaming with wildcard condition 'FILE*.TXT'.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------		

		{	
		{876, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE?.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE*.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK14, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0877
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy Src to the target while renaming without wildcard condition.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{877, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{5,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0878
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy from Src to same Src location .
//! @SYMTestActions Copy does not happen, returns the error code..
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{
		{878, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0879
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy a file 'FILE01.TXT' from Src  and rename file to 'RENAMED.TXT' on the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------		

		{	
		{879, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0882
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy files '*.TXT' from Src  and rename file to '*.REN' on the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------	

		{	
		{882, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK19, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0883
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy files 'FILE.*' from Src  and rename file to 'RENAMED.*' on the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------	

		{	
		{883, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,49,57,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0885
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files from Src directory without backward slash to the target
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------		

		{	
		{885, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE02.TXT", 
		(TText*)L"?:\\Trg\\", {{3,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{3,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0886
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------	

		{	
		{886, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg", 
		(TText*)L"?:\\Trg\\", {BLOCK24, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0887
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Copies the specified files from source to the target (default session path is taken as target path).
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{887, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {BLOCK24, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0888
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Copies the specified files from source(default session path is taken as source path) to the target.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{888, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg", 
		(TText*)L"?:\\Trg\\", {BLOCK24, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0889
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy Src file?.TXT to the target while renaming with wildcard condition 'FILE*.TXT'.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{889, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE?.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE*.TXT", 
		(TText*)L"?:\\Trg\\", {BLOCK14, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK14, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0890
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy Src to the target while renaming without wildcard condition.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{890, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {{5,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{5,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0891
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy from Src to same Src location.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{891, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Src\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0892
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy a file 'FILE01.TXT' from Src  and rename file to 'RENAMED.TXT' on the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{892, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0893
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files 'FILE*.TXT' from Src  and rename file to 'RENAMED*.TXT' on the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{893, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED*.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0894
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files 'FILE?.TXT' from Src  and rename file to 'RENAMED?.TXT' on the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{894, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED?.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0895
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy files '*.TXT' from Src  and rename file to '*.REN' on the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------		
 	
		{	
		{895, ECFMCopy, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {BLOCK19, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK19, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0896
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Copy files from Src directory without backward slash to the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{896, ECFMCopy, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE02.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{3,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0939
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy Src to the target while renaming without wildcard condition.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    

		{	
		{939, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK16, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0940
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy from Src to same Src location .
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    

   		{	
   		{940, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},
   
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0941
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy a file 'FILE01.TXT' from Src  and rename file to 'RENAMED.TXT' on the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   

   		{	
   		{941, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK30, EMPTY}}
		},
		
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0942
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files 'FILE*.TXT' from Src  and rename file to 'RENAMED*.TXT' on the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------     

   		{	
   		{942, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED*.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK30, EMPTY}}
		},	
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0943
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files 'FILE?.TXT' from Src  and rename file to 'RENAMED?.TXT' on the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    

   		{	
   		{943, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED?.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK30, EMPTY}}
		},
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0944
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files '*.TXT' from Src  and rename file to '*.REN' on the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    

   		{	
   		{944, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {BLOCK19, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK32, EMPTY}}
		},
		
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0945
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Copy files 'FILE.*' from Src  and rename file to 'RENAMED.*' on the target.
//! @SYMTestActions Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    

   		{	
   		{945, ECFMCopy, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {BLOCK46, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK46, EMPTY}}
		},
	
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0947
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy files Cyclically.
//! @SYMTestActions Cyclically Copies the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------     

			{ 
			{947, ECFMCopy, 0, KErrNone, KErrNone, KErrNone},
			{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1\\*.*", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
			{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\DIR11\\", 
			(TText*)L"?:\\Src\\DIR1\\DIR11\\", {EMPTY, EMPTY},
			(TText*)L"?:\\TrgCom\\", {BLOCK42, EMPTY}}
			}, 
				
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0948
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy() (overloaded Copy)
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Copy file 'FILE01.TXT'from source to target .
//! @SYMTestActions Copies the specified file from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 

		{	
		{948, ECFMCopyHandle, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0949
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy() (overloaded Copy)
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Copy file 'FILE01.TXT'from source to target .
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    		

			{ 
			{949, ECFMCopyHandle, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
			{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
			{&gDriveToTest, (TText*)L"?:\\Trg\\", 
			(TText*)L"?:\\Trg\\", {{2,EOB}, EMPTY},
			(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
			}, 
		
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0950
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy() (overloaded Copy)
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy file 'FILE01.TXT'from source to target .
//! @SYMTestActions Copies the specified file from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    

		{	
		{950, ECFMCopyHandle, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},
		
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0951
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy() (overloaded Copy)
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy file 'FILE01.TXT'from source to target .
//! @SYMTestActions Copies the specified file from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    		

		{	
		{951, ECFMCopyHandle, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},
			
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0952
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy() (overloaded Copy)
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy file 'FILE01.TXT'from source to target .
//! @SYMTestActions Copies the specified file from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   

			{ 
			{952, ECFMCopyHandle, 0, KErrNone, KErrNone, KErrNone},
			{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
			{&gDriveToTest, (TText*)L"", 
			(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {EMPTY, EMPTY},
			(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
			}, 


  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0953
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy() (overloaded Copy)
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy files 'FILE01.TXT' from Src  and rename file to 'OTHER.TXT' on the target.
//! @SYMTestActions Copies the specified file from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   			

		{ 
		{953, ECFMCopyHandle, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{5,EOB}, EMPTY}}
		}, 

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0954
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy() (overloaded Copy)
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,EmpTar' option 
//! Copy from Src to same Src location .
//! @SYMTestActions Copies the specified file from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    	   	

   		{	
		{954, ECFMCopyHandle, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

//Basic Negative test case for Copy:
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0970
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy a directory with longpath name from Src to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   	

   		{	
		{970, ECFMCopy, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26\\fdsa21asdffds\\NAME\\FGHIJ\\\\", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		 },


  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0971
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy a file with longname from Src to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    	

   		{	
		{971, ECFMCopy, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asfdsa21asfdsa22asfdsa23asfdsa24asfdsa25asfdsa26", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0972
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy a directory from non available Src drive to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrNotReady in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   		

   		{	
		{972, ECFMCopy, 0, KErrNotReady, KErrNone, KErrNotReady},
		{&gFixedDriveNotReady, (TText*)L"?:\\Src\\", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0973
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy a directory from Src drive to the non available target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrNotReady in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   
  
   		{	
		{973, ECFMCopy, 0, KErrNotReady, KErrNone, KErrNotReady},
		{&gDriveToTest, (TText*)L"?:\\Src\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gFixedDriveNotReady, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0974
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy a directory from Src drive to the Readonly target drive.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAccessDenied in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------     		
  
   		{	
		{974, ECFMCopy, 0, KErrAccessDenied, KErrNone, KErrAccessDenied},
		{&gDriveToTest, (TText*)L"?:\\Src\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gFixedDriveReadOnly, (TText*)L"?:\\test\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0975
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy a directory from non available Src directory to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrPathNotFound in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 

   		{	
		{975, ECFMCopy, 0, KErrPathNotFound, KErrNone, KErrPathNotFound},
		{&gFixedDriveValid, (TText*)L"?:\\src\\NODIR\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\test\\", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0976
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy a directory from non available Src directory to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrPathNotFound in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------    		

   		{	
		{976, ECFMCopy, 0, KErrPathNotFound, KErrNone, KErrPathNotFound},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\Dir1\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\test\\", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0977
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy a invaid directory path from Src directory to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   		
 
   		{	
		{977, ECFMCopy, 0, KErrBadName, KErrNone, KErrBadName},
		{&gFixedDriveValid, (TText*)L"::C:", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\test\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0979
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy a invaid directory path with single from Src directory to the target.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrPathNotFound in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------     

   		{	
		{979, ECFMCopy, 0, KErrPathNotFound, KErrNone, KErrPathNotFound},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01zz.txt\\", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0980
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy files from Src directory to the non available target directory.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrPathNotFound in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------     		

   		{	
		{980, ECFMCopy, 0, KErrPathNotFound, KErrNone, KErrPathNotFound},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1\\*.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\Dummy\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0981
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Copy()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,emp-Tar' option 
//! Copy files from Src directory to the target directory with a long paths.
//! @SYMTestActions Copy does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrBadName in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------   

   		{	
		{981, ECFMCopy, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0982
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move a file 'FILE01.TXT' from source to the target.
//! @SYMTestActions Moves specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   		

   		{	
		{982, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0983
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move files with wildcard combination '*.*' from source to the target.
//! @SYMTestActions Moves specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    		

   		{	
		{983, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK24}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0984
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move files with wildcard combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Moves specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

   		{	
		{984, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK02}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0985
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move files with wildcard combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Moves specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   	

   		{	
		{985, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {0,1,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0986
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move files with wildcard combination '*.TXT' from source to the target.
//! @SYMTestActions Moves specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    	
	
   		{	
		{986, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK29}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK29, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0987
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move files with wildcard combination 'FILE.*' from source to the target.
//! @SYMTestActions Moves specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

   		{	
		{987, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {4,6,55,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},

 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0988
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
 			
   		{	
		{988, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0989
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination '*.*' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

   		{	
		{989, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK24, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0990
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

   		{	
		{990, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK02, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
		},

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0991
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

   		{	
		{991, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{0,1,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0992
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination '*.TXT' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

   		{	
		{992, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK29, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK29, EMPTY}}
		},

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0993
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE.*' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

   		{	
		{993, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{4,6,55,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0994
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

   		{	
		{994, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0995
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination '*.*' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{995, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK24}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK24, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0996
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

   		{	
		{996, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK02}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK02, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0997
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

   		{	
		{997, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {0,1,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{0,1,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0998
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination '*.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    		
  		
		{	
		{998, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK29}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK29, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK29, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-0999
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE.*' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
		
		{	
		{999, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {4,6,55,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{4,6,55,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1000
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
	
		{	
		{1000, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,11,20,29,38,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,11,20,29,38,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1001
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1001, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1002
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files with combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1002, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK05}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK05, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1003
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files with combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1003, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK06}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK06, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1004
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files with combination '*.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1004, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK34}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK34, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1005
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files with combination 'FILE.*' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1005, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK35}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK35, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1006
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------  

		{	
		{1006, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,11,20,29,38,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,11,20,29,38,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1007
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination '*.*' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1007, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1008
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1008, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK05, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK05, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1009
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1009, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK06, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK06, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1010
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination '*.TXT' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1010, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK34, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK34, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1011
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE.*' from source to the target.
//! @SYMTestActions Move does not happen just returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1011, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK35, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK35, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1012
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    	

		{	
		{1012, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,11,20,29,38,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,11,20,29,38,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,11,20,29,38,EOB}, EMPTY}}
		},
		
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1013
//! @SYMTestType CIT 
		//! @SYMREQ NA
		//! @SYMTestPurpose CFileMan::Move()
		//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
		//! Move files with combination '*' from source to the target.
		//! @SYMTestActions Moves the specified files from source to the target.
		//! @SYMTestExpectedResults 1.KErrNone in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------  
		
			{ 
			{1013, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
			{&gDriveToTest, (TText*)L"?:\\Src\\*", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
			{&gDriveToTest, (TText*)L"?:\\Trg\\", 
			(TText*)L"?:\\Trg\\", {ALL, EMPTY},
			(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
			}, 
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1014
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{1014, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK05}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK05, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK05, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1015
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1015, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK06}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK06, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK06, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1016
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination '*.TXT' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1016, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK34}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK34, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK34, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1017
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files with combination 'FILE.*' from source to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1017, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK35}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK35, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK35, EMPTY}}
		},
   	
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1018
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move files from Src directory without backward slash to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    	

		{	
		{1018, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1019
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1019, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"?:\\Trg", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1020
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1020, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1021
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1021, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK24}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1022
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move Src 'FILE?.TXT' to the target while renaming with wildcard condition 'FILE*.TXT'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1022, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE?.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {0,1,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE*.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1023
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move files 'FILE01.TXT' from Src  and rename file to 'RENAMED.TXT' on the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1023, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1026
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move files '*.TXT' from Src  and rename file to '*.REN' on the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    						

		{	
		{1026, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK20}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK19, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1027
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move files 'FILE.*' from Src  and rename file to 'RENAMED.*' on the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1027, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {4,6,55,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,49,57,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1029
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move files 'FILE01.TXT' from Src  and rename file to 'OTHER.TXT' on the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------  

		{	
		{1029, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{5,EOB}, EMPTY}}
		},

   			
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1030
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Move files from Src to same Src location. 
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1030, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1031
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from Src directory without backward slash to the target.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1031, ECFMMove, 0, KErrAlreadyExists, KErrAlreadyExists, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK41, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1032
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			
	
		{	
		{1032, ECFMMove, 0, KErrAlreadyExists, KErrAlreadyExists, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg", 
		(TText*)L"?:\\Trg\\", {BLOCK41, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1033
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{1033, ECFMMove, 0, KErrAlreadyExists, KErrAlreadyExists, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\SRC\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {BLOCK41, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1034
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1034, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK24, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1035
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move Src 'FILE?.TXT' to the target while renaming with wildcard condition 'FILE*.TXT'.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1035, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE?.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE*.TXT", 
		(TText*)L"?:\\Trg\\", {{0,1,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1036
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files 'FILE01.TXT' from Src  and rename file to 'RENAMED.TXT' on the target.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------     			
	
		{	
		{1036, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1039
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files '*.TXT' from Src  and rename file to '*.REN' on the target.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    						

		{	
		{1039, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {BLOCK19, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK19, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1040
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files 'FILE.*' from Src  and rename file to 'RENAMED.*' on the target.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1040, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {{44,49,57,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,49,57,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1042
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from Src  and rename file on the target without wildcard.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1042, ECFMMove, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {{5,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{5,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1043
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from Src to same Src location.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
	
		{	
		{1043, ECFMMove, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1044
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files from Src directory without backward slash to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------	
   			
			{	
			{1044, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
			{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
			{&gDriveToTest, (TText*)L"?:\\Trg\\", 
			(TText*)L"?:\\Trg\\", {BLOCK41, EMPTY},
			(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
			},
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1045
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------
				
				{	
				{1045, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
				{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
				(TText*)L"?:\\Src\\", {ALL, EMPTY},
				(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
				{&gDriveToTest, (TText*)L"?:\\Trg", 
				(TText*)L"?:\\Trg\\", {BLOCK41, EMPTY},
				(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
				},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1046
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			
				
			{	
			{1046, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
			{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
			{&gDriveToTest, (TText*)L"", 
			(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {BLOCK41, EMPTY},
			(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
			},		
		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1047
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------     			

		{	
		{1047, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK24}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK24, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK24, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1048
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move Src 'FILE?.TXT' to the target while renaming with wildcard condition 'FILE*.TXT'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1048, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE?.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {0,1,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE*.TXT", 
		(TText*)L"?:\\Trg\\", {{0,1,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1049
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move Src 'FILE01.TXT' to the target while renaming with wildcard condition 'RENAMED.TXT'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1049, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\SRC\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1052
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move Src '*.TXT' to the target while renaming with wildcard condition '*.REN'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    						

		{	
		{1052, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK20}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {BLOCK19, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK19, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1053
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move Src 'FILE.*' to the target while renaming with wildcard condition 'RENAMED.*'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1053, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {4,6,55,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {{44,49,57,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,49,57,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1055
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files 'FILE01.TXT' from Src  and rename file to 'OTHER.TXT' on the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1055, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {{5,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{5,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1056
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Move files from Src to same Src location.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

			{	
			{1056, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
			{&gDriveToTest, (TText*)L"?:\\F32-TST\\CFILEMAN\\FILE01.TXT", 
			(TText*)L"?:\\F32-TST\\CFILEMAN\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
			{&gDriveToTest, (TText*)L"?:\\F32-TST\\CFILEMAN\\FILE01.TXT", 
			(TText*)L"?:\\Trg\\", {ALL, EMPTY},
			(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
			},
			
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1057
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files from Src directory without backward slash to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			
	
		{	
		{1057, ECFMMove, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\SRC\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1058
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{1058, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"?:\\Trg", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1059
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1059, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1060
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1060, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1061
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move Src 'FILE?.TXT' to the target while renaming with wildcard condition 'FILE*.TXT'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1061, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE?.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK06}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE*.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK06, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1062
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move Src 'FILE01.TXT' to the target while renaming with wildcard condition 'RENAMED.TXT'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1062, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,11,20,29,38,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK30, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1065
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move Src '*.TXT' to the target while renaming with wildcard condition '*.REN'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

		{	
		{1065, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK34}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK32, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1066
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move Src 'FILE.*' to the target while renaming with wildcard condition 'RENAMED.*'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1066, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK35}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK40, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1068
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files 'FILE01.TXT' from Src  and rename file to 'OTHER.TXT' on the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1068, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,11,20,29,38,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{5,14,23,32,41,EOB}, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1069
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,EmpTar' option 
//! Move files from Src to same Src location. 
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
  
		{	
		{1069, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1070
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from Src directory without backward slash to the target.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   	

		{	
		{1070, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrAlreadyExists, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK09, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK09, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1071
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1071, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrAlreadyExists, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg", 
		(TText*)L"?:\\Trg\\", {BLOCK09, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK09, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1072
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1072, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrAlreadyExists, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {BLOCK09, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK09, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1073
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1073, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1074
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move Src 'FILE?.TXT' to the target while renaming with wildcard condition 'FILE*.TXT'.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1074, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE?.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE*.TXT", 
		(TText*)L"?:\\Trg\\", {BLOCK06, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK06, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1075
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move Src 'FILE01.TXT' to the target while renaming with wildcard condition 'RENAMED.TXT'.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1075, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {BLOCK30, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK30, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1078
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move Src '*.TXT' to the target while renaming with wildcard condition '*.REN'.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   						

		{	
		{1078, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {BLOCK32, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK32, EMPTY}}
		},
 
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1079
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move Src 'FILE.*' to the target while renaming with wildcard condition 'RENAMED.*'.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------   			

		{	
		{1079, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {BLOCK40, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK40, EMPTY}}
		},
 
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1081
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files 'FILE01.TXT' from Src  and rename file to 'OTHER.TXT' on the target.
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------    		
			  
		{	
		{1081, ECFMMove, CFileMan::ERecurse, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {{5,14,23,32,41,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{5,14,23,32,41,EOB}, EMPTY}}
		},
 
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1082
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Non-Overwrite,Non-EmpTar' option 
//! Move files from Src to same Src location. 
//! @SYMTestActions Move does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------    			
		  
		{	
		{1082, ECFMMove, CFileMan::ERecurse, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},
 
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1083
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files from Src directory without backward slash to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------    			
 			  
		{	
		{1083, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK41, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},
   			
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1084
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------    			

		{	
		{1084, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"?:\\Trg", 
		(TText*)L"?:\\Trg\\", {BLOCK41, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},
 		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1085
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------   			

		{	
		{1085, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {BLOCK41, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1086
//! @SYMTestType CIT 
//! @SYMREQ NA
//! @SYMTestPurpose CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files from unspecified(NULL) Src to the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------  
	
		{ 
		{1086, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},		
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1087
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move Src 'FILE?.TXT' to the target while renaming with wildcard condition 'FILE*.TXT'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------     			

		{	
		{1087, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE?.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK06}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\FILE*.TXT", 
		(TText*)L"?:\\Trg\\", {BLOCK06, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK06, EMPTY}}
		},
   			
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1088
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move Src 'FILE01.TXT' to the target while renaming with wildcard condition 'RENAMED.TXT'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------   					

		{	
		{1088, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,11,20,29,38,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {BLOCK30, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK30, EMPTY}}
		},

  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1091
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move Src '*.TXT' to the target while renaming with wildcard condition '*.REN'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------   						

		{	
		{1091, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK34}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {BLOCK32, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK32, EMPTY}}
		},
  
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1092
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move Src 'FILE.*' to the target while renaming with wildcard condition 'RENAMED.*'.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------     			
 
		{	
		{1092, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK35}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {BLOCK33, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK40, EMPTY}}
		},
   					
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1094
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files 'FILE01.TXT' from Src  and rename file to 'OTHER.TXT' on the target.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------    			
   
		{	
		{1094, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gFixedDriveValid, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,11,20,29,38,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\OTHER.TXT", 
		(TText*)L"?:\\Trg\\", {{5,14,23,32,41,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{5,14,23,32,41,EOB}, EMPTY}}
		},
    //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1095
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Recursive,Overwrite,Non-EmpTar' option 
//! Move files from Src to same Src location.
//! @SYMTestActions Moves the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------    			

		{	
		{1095, ECFMMove, CFileMan::ERecurse|CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1096
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Rename a file 'FILE01.TXT' from source to the target.
//! @SYMTestActions Renames specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    	
 
		{	
		{1096, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1097
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Rename files with wildcard combination '*.*' from source to the target.
//! @SYMTestActions Renames specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------     			
		
			{	
			{1097, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
			{&gDriveToTest, (TText*)L"?:\\Src\\*.*", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
			{&gDriveToTest, (TText*)L"?:\\Trg\\", 
			(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
			(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
			},

//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1098
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Rename files with wildcard combination 'FILE*.TXT' from source to the target.
//! @SYMTestActions Renames specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1098, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK02}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
		},
   			
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1099
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Rename files with wildcard combination 'FILE?.TXT' from source to the target.
//! @SYMTestActions Renames specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1099, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {0,1,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},
 
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1100
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Rename files with wildcard combination '*.TXT' from source to the target.
//! @SYMTestActions Renames specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1100, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK20}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK20, EMPTY}}
		},
   			
//--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1101
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Rename files with wildcard combination 'FILE.*' from source to the target.
//! @SYMTestActions Renames specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1101, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {4,6,55,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1102
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Rename files with wildcard combination 'FILE01.TXT' from source to the target.
//! @SYMTestActions Renames specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

		{	
		{1102, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1105
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Rename files with wildcard combination '*.TXT' from source to the '*.REN' in target.
//! @SYMTestActions Renames specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

		{	
		{1105, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK20}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK19, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1106
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Rename files with wildcard combination 'FILE.*' from source to the '*.REN' in target.
//! @SYMTestActions Renames specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			
		
		{	
		{1106, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {4,6,55,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,49,57,EOB}, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1108
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE01.TXT' from source to target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
			
		{	
		{1108, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1109
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination '*.*' from source to target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
   			
			{	
			{1109, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
			{&gDriveToTest, (TText*)L"?:\\Src\\*", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
			{&gDriveToTest, (TText*)L"?:\\Trg\\", 
			(TText*)L"?:\\Trg\\", {ALL, EMPTY},
			(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
			},
   			
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1110
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE*.TXT' from source to target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
		
				{	
				{1110, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
				{&gDriveToTest, (TText*)L"?:\\Src\\FILE*.TXT", 
				(TText*)L"?:\\Src\\", {ALL, EMPTY},
				(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {BLOCK02, EMPTY},
				(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
				},		
							
	

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1111
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE?.TXT' from source to target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			
   			
		{	
		{1111, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{0,1,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},
    			
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1112
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination '*.TXT' from source to target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
   			
		{	
		{1112, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK20, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK20, EMPTY}}
		},
   
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1113
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE.*' from source to target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
   			
		{	
		{1113, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{4,6,55,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},
   			
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1114
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE01.TXT' from source to 'RENAMED.TXT' in target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
   			
		{	
		{1114, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1117
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination '*.TXT' from source to '*.REN' in target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
			
		{	
		{1117, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {BLOCK19, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK19, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1118
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE.*' from source to 'RENAMED.*' in target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
			
		{	
		{1118, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {{44,49,57,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,49,57,EOB}, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1120
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE01.TXT' from source to target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 
			
		{	
		{1120, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{2,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{2,EOB}, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1121
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination '*.*' from source to target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
		
			{	
			{1121, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
			{&gDriveToTest, (TText*)L"?:\\Src\\*", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, BLOCK24}},
			{&gDriveToTest, (TText*)L"?:\\Trg\\", 
			(TText*)L"?:\\Trg\\", {ALL, EMPTY},
			(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
			},	

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1122
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE*.TXT' from source to target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1122, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK02}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK02, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK02, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1123
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE?.TXT' from source to target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1123, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE?.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {0,1,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{0,1,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{0,1,EOB}, EMPTY}}
		},
  //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1124
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination '*.TXT' from source to target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1124, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK20}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK20, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK20, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1125
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE.*' from source to target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1125, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {4,6,55,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {{4,6,55,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{4,6,55,EOB}, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1126
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE01.TXT' from source to 'RENAMED.TXT' in target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1126, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {2,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.TXT", 
		(TText*)L"?:\\Trg\\", {{44,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,EOB}, EMPTY}}
		},
 


 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1129
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination '*.TXT' from source to '*.REN' in target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

		{	
		{1129, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\*.TXT", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK20}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\*.REN", 
		(TText*)L"?:\\Trg\\", {BLOCK19, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK19, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1130
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmpTar' option 
//! Rename files with wildcard combination 'FILE.*' from source to 'RENAMED.*' in target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
			
		{	
		{1130, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\FILE.*", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, {4,6,55,EOB}}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\RENAMED.*", 
		(TText*)L"?:\\Trg\\", {{44,49,57,EOB}, EMPTY},
		(TText*)L"?:\\TrgCom\\", {{44,49,57,EOB}, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1132
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Rename files from Src directory without backward slash to the target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   	
  			
		{	
		{1132, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1133
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Rename files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			

			{	
			{1133, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
			{&gDriveToTest, (TText*)L"?:\\Src\\DIR1", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
			{&gDriveToTest, (TText*)L"?:\\Trg", 
			(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
			(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
			},
		
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1134
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Rename files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
 			
		{	
		{1134, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, BLOCK41}},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},
  			
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1135
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Rename files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 
   			
			{	
			{1135, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
			{&gDriveToTest, (TText*)L"", 
			(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
			{&gDriveToTest, (TText*)L"?:\\Trg\\", 
			(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
			(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
			},	
  			
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1136
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmpTar' option 
//! Rename files from Src to same Src location.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   
   			
		{	
		{1136, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},
   			
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1137
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmptyTar' option 
//! Rename files from Src directory without backward slash to the target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
  			
		{	
		{1137, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK09, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK09, EMPTY}}
		},
  
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1138
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmptyTar' option 
//! Rename files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------     			
   			
		{	
		{1138, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {BLOCK41, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},
  
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1139
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmptyTar' option 
//! Rename files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
    			
		{	
		{1139, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"?:\\Src\\DIR1", 
		(TText*)L"?:\\Src\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {BLOCK41, EMPTY},
		(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
		},
    			
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1140
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmptyTar' option 
//! Rename files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Rename does not happen, returns the error code.
//! @SYMTestExpectedResults 1.KErrAlreadyExists in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
			
		{	
		{1140, ECFMRename, 0, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
		{&gDriveToTest, (TText*)L"", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {ALL, EMPTY},
		(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1141
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,Non-EmptyTar' option 
//! Rename files from Src to same Src location.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------     			
 			
		{	
		{1141, ECFMRename, 0, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},
  			
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1142
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmptyTar' option 
//! Rename files from Src directory without backward slash to the target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 
     
			{	
			{1142, ECFMRename, CFileMan::EOverWrite, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
			{&gDriveToTest, (TText*)L"?:\\Src\\DIR1", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
			{&gDriveToTest, (TText*)L"?:\\Trg\\", 
			(TText*)L"?:\\Trg\\", {BLOCK41, EMPTY},
			(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
			},
	
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1143
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmptyTar' option 
//! Rename files from Src directory without backward slash to the target directory without backward slash.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------

				{	
				{1143, ECFMRename, CFileMan::EOverWrite, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
				{&gDriveToTest, (TText*)L"?:\\Src\\DIR1", 
				(TText*)L"?:\\Src\\", {ALL, EMPTY},
				(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg", 
				(TText*)L"?:\\Trg\\", {ALL, EMPTY},
				(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
				},
			
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1144
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmptyTar' option 
//! Rename files from Src to the unspecified(NULL) target path.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			

			{	
			{1144, ECFMRename, CFileMan::EOverWrite, KErrAlreadyExists, KErrNone, KErrAlreadyExists},
			{&gDriveToTest, (TText*)L"?:\\Src\\DIR1", 
			(TText*)L"?:\\Src\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
			{&gDriveToTest, (TText*)L"", 
			(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {BLOCK41, EMPTY},
			(TText*)L"?:\\TrgCom\\", {BLOCK41, EMPTY}}
			},
				
				
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1145
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmptyTar' option 
//! Rename files from unspecified(NULL) Src  to the target.
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//--------------------------------------------- 

			{	
			{1145, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
			{&gDriveToTest, (TText*)L"", 
			(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
			(TText*)L"?:\\SrcCom\\", {ALL, BLOCK24}},
			{&gDriveToTest, (TText*)L"?:\\Trg\\", 
			(TText*)L"?:\\Trg\\", {ALL, EMPTY},
			(TText*)L"?:\\TrgCom\\", {ALL, EMPTY}}
			},
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1146
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Rename()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Overwrite,Non-EmptyTar' option 
//! Rename files from Src to same Src location. 
//! @SYMTestActions Renames the specified files from source to the target.
//! @SYMTestExpectedResults 1.KErrNone in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			
      			
		{	
		{1146, ECFMRename, CFileMan::EOverWrite, KErrNone, KErrNone, KErrNone},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {ALL, EMPTY},
		(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\F32-TST\\T_CFILEMAN\\FILE01.TXT", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},
   	
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1147
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move files with invalid path 'C\\Src\\FILE01.TXT' from Src to target. 
//! @SYMTestActions Move does not happen, returns the error code..
//! @SYMTestExpectedResults 1.KErrBadName in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------   			
  			
		{	
		{1147, ECFMMove, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gFixedDriveValid, (TText*)L"C\\Src\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1148
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move files with invalid path 'C:Src\\FILE01.TXT'from Src to target. 
//! @SYMTestActions Move does not happen, returns the error code..
//! @SYMTestExpectedResults 1.KErrBadName in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
 			
		{	
		{1148, ECFMMove, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gFixedDriveValid, (TText*)L"C:Src\\FILE01.TXT", 
		(TText*)L"?:\\F32-TST\\T_CFILEMAN\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},

		
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1150
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move file with long filename from Src to target. 
//! @SYMTestActions Move does not happen, returns the error code..
//! @SYMTestExpectedResults 1.KErrBadName in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
     			
		{	
		{1150, ECFMMove, 0, KErrBadName, KErrBadName, KErrBadName},
		{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26", 
		(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
		(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
		{&gDriveToTest, (TText*)L"?:\\Trg\\", 
		(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
		(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},
 
 //--------------------------------------------- 
//! @SYMTestCaseID PBASE-T_CFILEMAN-1151
//! @SYMTestType CIT 
//!
//! @SYMAPI CFileMan::Move()
//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
//! Move file with long pathname from Src to target. 
//! @SYMTestActions Move does not happen, returns the error code..
//! @SYMTestExpectedResults 1.KErrBadName in success case.
//! @SYMTestPriority High
//! @SYMTestStatus Implemented 
//---------------------------------------------    			
   			
		{	
				{1151, ECFMMove, 0, KErrBadName, KErrBadName, KErrBadName},
				{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26\\fdsa21asdffds\\NAME\\FGHIJ\\TEST\\", 
				(TText*)L"?:\\Src\\", {ALL, EMPTY},
				(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
		},
		 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1152
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Move()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Move file from path that doesnot exist in Src to target. 
		//! @SYMTestActions Move does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrPathNotFound in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------   			
	
				/*{	
				{1152, ECFMMove, 0, KErrPathNotFound, KErrNone, KErrPathNotFound},
				{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1\\NODIR\\*.*", 
				(TText*)L"?:\\Src\\", {ALL, EMPTY},
				(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},*/
		 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1153
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Move()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Move file from Src to path that doesnot exist in target. 
		//! @SYMTestActions Move does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrPathNotFound in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------    			
		   			
					{	
					{1153, ECFMMove, 0, KErrPathNotFound, KErrNone, KErrPathNotFound},
					{&gFixedDriveValid, (TText*)L"?:\\Src\\DIR1\\*.*", 
					(TText*)L"?:\\Src\\", {ALL, EMPTY},
					(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
					{&gDriveToTest, (TText*)L"?:\\NOTARGET\\", 
					(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
					(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
					},
		   	
		   			
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1154
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Move()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Move file from Src to target with longpath. 
		//! @SYMTestActions Move does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrBadName in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------
  			
				{	
				{1154, ECFMMove, 0, KErrBadName, KErrBadName, KErrBadName},
				{&gFixedDriveValid, (TText*)L"?:\\Src\\FILE01.TXT", 
				(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
				(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\TRG\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26\\",
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 			
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1155
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Move()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Move file from ReadOnly Drive Src to target. 
		//! @SYMTestActions Move does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrAccessDenied in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------   			
    			
				{	
				{1155, ECFMMove, 0, KErrAccessDenied, KErrNone, KErrAccessDenied},
				{&gFixedDriveReadOnly, (TText*)L"?:\\TEST\\", 
				(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
				(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1156
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Move()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Move file Src to target with ReadOnly Drive . 
		//! @SYMTestActions Move does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrAccessDenied in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------     			
  			
				{	
				{1156, ECFMMove, 0, KErrAccessDenied, KErrNone, KErrAccessDenied},
				{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
				(TText*)L"?:\\Src\\", {ALL, EMPTY},
				(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
				{&gFixedDriveReadOnly, (TText*)L"?:\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1157
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Move()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Move file Src to target with Drives not ready . 
		//! @SYMTestActions Move does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrNotReady in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------     			
		   			         			
				{	
				{1157, ECFMMove, 0, KErrNotReady, KErrNone, KErrNotReady},
				{&gFixedDriveNotReady, (TText*)L"?:\\", 
				(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
				(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 		   			
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1158
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Rename()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Rename files with invalid path 'C\\Src\\FILE01.TXT' from Src to target.
		//! @SYMTestActions Rename does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrBadName in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------    			
		         			
				{	
				{1158, ECFMRename, 0, KErrBadName, KErrBadName, KErrBadName},
				{&gDriveToTest, (TText*)L"C\\Src\\FILE01.TXT", 
				(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
				(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1159
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Rename()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Rename files with invalid path 'C:Src\\FILE01.TXT'from Src to target. 
		//! @SYMTestActions Rename does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrBadName in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------    			
		         			
				{	
				{1159, ECFMRename, 0, KErrBadName, KErrBadName, KErrBadName},
				{&gDriveToTest, (TText*)L"C:Src\\FILE01.TXT", 
				(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
				(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1161
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Rename()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Rename file with long filename from Src to target. 
		//! @SYMTestActions Rename does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrBadName in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------    			
		         			
				{	
				{1161, ECFMRename, 0, KErrBadName, KErrBadName, KErrBadName},
				{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26", 
				(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
				(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1162
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Rename()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Rename file with long pathname from Src to target. 
		//! @SYMTestActions Rename does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrBadName in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------    			
		         			
				{	
				{1162, ECFMRename, 0, KErrBadName, KErrBadName, KErrBadName},
				{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26\\fdsa21asdffds\\NAME\\FGHIJ\\TEST\\", 
				(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
				(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1163
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Rename()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Renaming files from the path that does not exist 
		//! @SYMTestActions Rename does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrPathNotFound in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------    			
		         			
				{	
				{1163, ECFMRename, 0, KErrPathNotFound, KErrNone, KErrPathNotFound},
				{&gDriveToTest, (TText*)L"?:\\Src\\DIR1\\NODIR\\*.*", 
				(TText*)L"?:\\Src\\", {ALL, EMPTY},
				(TText*)L"?:\\SrcCom\\", {ALL, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1164
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Rename()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Rename file from Src to path that doesnot exist in target
		//! @SYMTestActions Rename does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrPathNotFound in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------   	
		         			
					{	
					{1164, ECFMRename, 0, KErrBadName, KErrBadName, KErrBadName},
					{&gDriveToTest, (TText*)L"?:\\Src\\FILE01.TXT", 
					(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
					(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
					{&gDriveToTest, (TText*)L"?:\\TRG\\asdffdsa01asdffdsa02asdffdsa03asdffdsa04asdffdsa05asdffdsa06asdffdsa07asdffdsa08asdffdsa09asdffdsa10asdffdsa11asdffdsa12asdffdsa13asdffdsa14asdffdsa15asdffdsa16asdffdsa17asdffdsa18asdffdsa19asdffdsa20asdffdsa21asdffdsa22asdffdsa23asdffdsa24asdffdsa25asdffdsa26\\", 
					(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
					(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
					},
					 
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1165
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Rename()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Rename file from ReadOnly Drive Src to target. 
		//! @SYMTestActions Rename does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrArgument in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//---------------------------------------------    			
		         			
				{	
				{1165, ECFMRename, 0, KErrArgument, KErrNone, KErrArgument},
				{&gFixedDriveReadOnly, (TText*)L"?:\\TEST\\", 
				(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
				(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},
		 	  								
		 //--------------------------------------------- 
		//! @SYMTestCaseID PBASE-T_CFILEMAN-1166
		//! @SYMTestType CIT 
		//!
		//! @SYMAPI CFileMan::Rename()
		//! @SYMTestCaseDesc 1.Tests API with 'Non-Recursive,Non-Overwrite,EmptyTar' option 
		//! Rename file Src to target with Drives not ready . 
		//! @SYMTestActions Rename does not happen, returns the error code..
		//! @SYMTestExpectedResults 1.KErrNotReady in success case.
		//! @SYMTestPriority High
		//! @SYMTestStatus Implemented 
		//--------------------------------------------- 
			         			
				{	
				{1166, ECFMRename, 0, KErrNotReady, KErrNone, KErrNotReady},
				{&gFixedDriveNotReady, (TText*)L"?:\\", 
				(TText*)L"?:\\Src\\", {EMPTY, EMPTY},
				(TText*)L"?:\\SrcCom\\", {EMPTY, EMPTY}},
				{&gDriveToTest, (TText*)L"?:\\Trg\\", 
				(TText*)L"?:\\Trg\\", {EMPTY, EMPTY},
				(TText*)L"?:\\TrgCom\\", {EMPTY, EMPTY}}
				},

//End biary API test cases 	
				{{0}}
				
		};
#endif /*T_CFILEMAN_CASES_H*/
