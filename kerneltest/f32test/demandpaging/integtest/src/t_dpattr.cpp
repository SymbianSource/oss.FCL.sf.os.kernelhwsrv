// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// @SYMTestCaseID				KBASE-DP_FUNC_ATTRIBUTES-0303
// @SYMTestCaseDesc			Symbian OS Toolchain's Paging Override and the
// Paging Policy settings
// @SYMREQ						REQ6808
// @SYMPREQ					PREQ1110
// @SYMTestPriority			High
// @SYMTestActions
// 1.	Load executables with the RProcess API and run them. Each executable has a
// different set of attributes:
// "	Paging attribute: paged, unpaged or no paging attribute in MMP and/or
// OBY files
// "	Compression mode: specified or not
// "	Executable built in ROM as 'data' or 'file'
// "	Solid binaries or aliases
// Retrieve and analyse the demand paging activity trace caused by the preceding
// action, in order to determine whether this binary is paged on demand or not.
// 2.	Repeat the preceding action by loading DLLs with the RLibrary API and
// making calls to them.
// @SYMTestExpectedResults
// 1.	Process complete without error. If, according to the trace data gathered,
// the main thread newly created process causes the executable to be paged in,
// then it is a proof that the executable is paged. Depending on the Paging
// Override setting and the Loader Paging Policy specified at the time the ROM was
// built, the Loader makes a decision as to whether page the binary or not,
// according to the rules listed in the Design Sketch. This is how we determine
// this, in this order:
// a.	If ROM paging is disabled: not paged
// b.	If executable was built in ROM as 'data': not paged
// c.	If the a file compression scheme was specified at ROM build time: not paged
// d.	If the Loader Paging policy is 'NOT PAGED': not paged
// e.	If the Loader Paging policy is 'ALWAYS PAGE': paged
// f.	If the Paging Override is 'NOT PAGED': not paged
// g.	If the Paging Override is 'ALWAYS PAGE': paged
// h.	If the OBY paging keyword for this executable is 'PAGED': paged
// i.	If the OBY paging keyword for this executable is 'NOT PAGED': unpaged
// j.	If the MMP paging keyword for this executable is 'PAGED': paged
// k.	If the MMP paging keyword for this executable is 'NOT PAGED': not paged
// l.	If the Paging Override is 'DEFAULT UNPAGED': not paged
// m.	If the Paging Override is 'DEFAULT PAGED': paged
// n.	If the Paging Policy is 'DEFAULT UNPAGED': not paged
// o.	The executable must be paged
// 2.	DLL is loaded. Functions are called and complete without errors. The rules
// to determine whether the binary should be paged or not are the same as in the
// preceding action.
// 
//

#include <e32test.h>
#include <e32ldr.h>
#include <d32btrace.h>
#include "u32std.h"
#include <f32file.h>
#include <dptest.h>

#define TEST_EQ(a,b) { if (a != b) { test.Printf(_L("%d != %d\n"), a, b); test(EFalse); } }
#define TEST_CONDITION(a) { if (!a) { test.Printf(_L("TEST FAILED at line %d\n"), __LINE__); pass = EFalse; } }
#define LE4(a) ((TUint) (*((a) + 3) << 24) + (*((a) + 2) << 16) + (*((a) + 1) << 8) + *(a))

RTest test(_L("T_DPATTR"));
RBTrace btrace;
RFs fs;

// ROM paging settings
TBool gIsRomDemangPagingEnabled;
TInt gPagingOverride;
TInt gPagingPolicy;

// This process
TUint32 gNThreadId = 0;

// Test executables attributes flags
enum
	{
	ENone					= 0,
	EMMPPaged				= 1 << 0,		// "PAGED" keyword in MMP file
	EMMPUnpaged				= 1 << 1,		// "UNPAGED" keyword in MMP file
	EMMPCompressTarget		= 1 << 2,		// "COMPRESSTARGET" keyword in MMP file
	EMMPNoCompressTarget	= 1 << 3,		// "UNCOMPRESSTARGET" keyword in MMP file
	EIBYData				= 1 << 4,		// File included as "data" in IBY file
	EIBYFile				= 1 << 5,		// File included as "file" in IBY file
	EIBYFileCompress		= 1 << 6,		// File included as "file_x_" in IBY file (_x_=compression scheme)
	EIBYPaged				= 1 << 7,		// File declared "paged" in IBY file
	EIBYUnpaged				= 1 << 8,		// File declared "unpaged in IBY file
	EIBYAlias				= 1 << 9,		// File name is an alias
	EDLLWritableStaticData	= 1 << 10		// DLL contains writable static data
	};

// Test executables attributes
const TUint testExeAttr[] =
	{
	/* 000 - does not exist */ ENone,
	/* 001 */ EIBYFile,
	/* 002 */ EMMPPaged | EIBYFile,
	/* 003 */ EMMPUnpaged | EIBYFile,
	/* 004 */ EIBYFileCompress,
	/* 005 */ EMMPPaged | EIBYFileCompress,
	/* 006 */ EMMPUnpaged | EIBYFileCompress,
	/* 007 */ EIBYFileCompress,
	/* 008 */ EMMPPaged | EIBYFileCompress,
	/* 009 */ EMMPUnpaged | EIBYFileCompress,
	/* 010 */ EIBYFileCompress,
	/* 011 */ EMMPPaged | EIBYFileCompress,
	/* 012 */ EMMPUnpaged | EIBYFileCompress,
	/* 013 */ EIBYFile | EMMPCompressTarget,
	/* 014 */ EMMPPaged | EIBYFile | EMMPNoCompressTarget,
	/* 015 */ EMMPUnpaged | EIBYFile | EIBYFileCompress | EIBYPaged,
	/* 016 */ EIBYData,
	/* 017 */ EMMPPaged | EIBYData,
	/* 018 */ EMMPUnpaged | EIBYData,
	/* 019 */ EIBYFile | EIBYPaged,
	/* 020 */ EMMPPaged | EIBYFile | EIBYPaged,
	/* 021 */ EMMPUnpaged | EIBYFile | EIBYPaged,
	/* 022 */ EIBYFile | EIBYUnpaged,
	/* 023 */ EMMPPaged | EIBYFile | EIBYUnpaged,
	/* 024 */ EMMPUnpaged | EIBYFile | EIBYUnpaged,
	/* 025 */ EIBYData | EIBYPaged,
	/* 026 */ EMMPPaged | EIBYData | EIBYPaged,
	/* 027 */ EMMPUnpaged | EIBYData | EIBYPaged,
	/* 028 */ EIBYData | EIBYUnpaged,
	/* 029 */ EMMPPaged | EIBYData | EIBYUnpaged,
	/* 030 */ EMMPUnpaged | EIBYData | EIBYUnpaged,
	/* 031 */ EIBYAlias | EIBYFile,
	/* 032 */ EIBYAlias | EMMPPaged | EIBYFile,
	/* 033 */ EIBYAlias | EMMPUnpaged | EIBYFile,
	/* 034 */ EIBYAlias | EIBYFileCompress,
	/* 035 */ EIBYAlias | EMMPPaged | EIBYFileCompress,
	/* 036 */ EIBYAlias | EMMPUnpaged | EIBYFileCompress,
	/* 037 */ EIBYAlias | EIBYFileCompress,
	/* 038 */ EIBYAlias | EMMPPaged | EIBYFileCompress,
	/* 039 */ EIBYAlias | EMMPUnpaged | EIBYFileCompress,
	/* 040 */ EIBYAlias | EIBYFileCompress,
	/* 041 */ EIBYAlias | EMMPPaged | EIBYFileCompress,
	/* 042 */ EIBYAlias | EMMPUnpaged | EIBYFileCompress,
	/* 043 */ EIBYAlias | EIBYFile | EMMPCompressTarget,
	/* 044 */ EIBYAlias | EMMPPaged | EIBYFile | EMMPNoCompressTarget,
	/* 045 */ EIBYAlias | EMMPUnpaged | EIBYFile | EIBYFileCompress | EIBYPaged,
	/* 046 */ EIBYAlias | EIBYData,
	/* 047 */ EIBYAlias | EMMPPaged | EIBYData,
	/* 048 */ EIBYAlias | EMMPUnpaged | EIBYData,
	/* 049 */ EIBYAlias | EIBYFile | EIBYPaged,
	/* 050 */ EIBYAlias | EMMPPaged | EIBYFile | EIBYPaged,
	/* 051 */ EIBYAlias | EMMPUnpaged | EIBYFile | EIBYPaged,
	/* 052 */ EIBYAlias | EIBYFile | EIBYUnpaged,
	/* 053 */ EIBYAlias | EMMPPaged | EIBYFile | EIBYUnpaged,
	/* 054 */ EIBYAlias | EMMPUnpaged | EIBYFile | EIBYUnpaged,
	/* 055 */ EIBYAlias | EIBYData | EIBYPaged,
	/* 056 */ EIBYAlias | EMMPPaged | EIBYData | EIBYPaged,
	/* 057 */ EIBYAlias | EMMPUnpaged | EIBYData | EIBYPaged,
	/* 058 */ EIBYAlias | EIBYData | EIBYUnpaged,
	/* 059 */ EIBYAlias | EMMPPaged | EIBYData | EIBYUnpaged,
	/* 060 */ EIBYAlias | EMMPUnpaged | EIBYData | EIBYUnpaged
	};
const TUint testDllAttr[] =
	{
	/* 000 - does not exist */ ENone,
	/* 001 */ EIBYFile,
	/* 002 */ EMMPPaged | EIBYFile,
	/* 003 */ EMMPUnpaged | EIBYFile,
	/* 004 */ EIBYFileCompress,
	/* 005 */ EMMPPaged | EIBYFileCompress,
	/* 006 */ EMMPUnpaged | EIBYFileCompress,
	/* 007 */ EDLLWritableStaticData,
	/* 008 */ EMMPPaged | EDLLWritableStaticData,
	/* 009 */ EMMPUnpaged | EDLLWritableStaticData,
	/* 010 */ EIBYFileCompress,
	/* 011 */ EMMPPaged | EIBYFileCompress,
	/* 012 */ EMMPUnpaged | EIBYFileCompress,
	/* 013 */ EIBYFile | EMMPCompressTarget,
	/* 014 */ EMMPPaged | EIBYFile | EMMPNoCompressTarget,
	/* 015 */ EMMPUnpaged | EIBYFile | EIBYFileCompress | EIBYPaged,
	/* 016 */ EIBYData,
	/* 017 */ EMMPPaged | EIBYData,
	/* 018 */ EMMPUnpaged | EIBYData,
	/* 019 */ EIBYFile | EIBYPaged,
	/* 020 */ EMMPPaged | EIBYFile | EIBYPaged,
	/* 021 */ EMMPUnpaged | EIBYFile | EIBYPaged,
	/* 022 */ EIBYFile | EIBYUnpaged,
	/* 023 */ EMMPPaged | EIBYFile | EIBYUnpaged,
	/* 024 */ EMMPUnpaged | EIBYFile | EIBYUnpaged,
	/* 025 */ EIBYData | EIBYPaged,
	/* 026 */ EMMPPaged | EIBYData | EIBYPaged,
	/* 027 */ EMMPUnpaged | EIBYData | EIBYPaged,
	/* 028 */ EIBYData | EIBYUnpaged,
	/* 029 */ EMMPPaged | EIBYData | EIBYUnpaged,
	/* 030 */ EMMPUnpaged | EIBYData | EIBYUnpaged,
	/* 031 */ EIBYAlias | EIBYFile,
	/* 032 */ EIBYAlias | EMMPPaged | EIBYFile,
	/* 033 */ EIBYAlias | EMMPUnpaged | EIBYFile,
	/* 034 */ EIBYAlias | EIBYFileCompress,
	/* 035 */ EIBYAlias | EMMPPaged | EIBYFileCompress,
	/* 036 */ EIBYAlias | EMMPUnpaged | EIBYFileCompress,
	/* 037 */ EIBYAlias | EDLLWritableStaticData,
	/* 038 */ EIBYAlias | EMMPPaged | EDLLWritableStaticData,
	/* 039 */ EIBYAlias | EMMPUnpaged | EDLLWritableStaticData,
	/* 040 */ EIBYAlias | EIBYFileCompress,
	/* 041 */ EIBYAlias | EMMPPaged | EIBYFileCompress,
	/* 042 */ EIBYAlias | EMMPUnpaged | EIBYFileCompress,
	/* 043 */ EIBYAlias | EIBYFile | EMMPCompressTarget,
	/* 044 */ EIBYAlias | EMMPPaged | EIBYFile | EMMPNoCompressTarget,
	/* 045 */ EIBYAlias | EMMPUnpaged | EIBYFile | EIBYFileCompress | EIBYPaged,
	/* 046 */ EIBYAlias | EIBYData,
	/* 047 */ EIBYAlias | EMMPPaged | EIBYData,
	/* 048 */ EIBYAlias | EMMPUnpaged | EIBYData,
	/* 049 */ EIBYAlias | EIBYFile | EIBYPaged,
	/* 050 */ EIBYAlias | EMMPPaged | EIBYFile | EIBYPaged,
	/* 051 */ EIBYAlias | EMMPUnpaged | EIBYFile | EIBYPaged,
	/* 052 */ EIBYAlias | EIBYFile | EIBYUnpaged,
	/* 053 */ EIBYAlias | EMMPPaged | EIBYFile | EIBYUnpaged,
	/* 054 */ EIBYAlias | EMMPUnpaged | EIBYFile | EIBYUnpaged,
	/* 055 */ EIBYAlias | EIBYData | EIBYPaged,
	/* 056 */ EIBYAlias | EMMPPaged | EIBYData | EIBYPaged,
	/* 057 */ EIBYAlias | EMMPUnpaged | EIBYData | EIBYPaged,
	/* 058 */ EIBYAlias | EIBYData | EIBYUnpaged,
	/* 059 */ EIBYAlias | EMMPPaged | EIBYData | EIBYUnpaged,
	/* 060 */ EIBYAlias | EMMPUnpaged | EIBYData | EIBYUnpaged
	};

void InitNThreadID()
	{
	_LIT(KThreadName, "ARandomThreadName");
	btrace.SetFilter(BTrace::EThreadIdentification, ETrue);
	btrace.Empty();
	btrace.SetMode(RBTrace::EEnable);
	// rename the current thread to force a ThreadID trace
	User::RenameThread(KThreadName);
	btrace.SetMode(0);
	TInt size;
	TUint8* pDataStart;
	TUint8* pCurrentRecord;
	// extract the nano-kernel thread ID from the trace
	while ((size = btrace.GetData(pDataStart)) != 0)
		{
		pCurrentRecord = pDataStart;
		while (pCurrentRecord - pDataStart < size)
			{
			TInt extensionCount = 4 * (
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EHeader2Present ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::ETimestampPresent ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::ETimestamp2Present ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EContextIdPresent ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EPcPresent ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EExtraPresent ? 1 : 0));
			//
			if ((pCurrentRecord[BTrace::ECategoryIndex] == BTrace::EThreadIdentification) && (pCurrentRecord[BTrace::ESubCategoryIndex] == BTrace::EThreadName))
				{
				TBuf<255> threadName;
				threadName.Format(_L(""));
				for (TUint8* j = pCurrentRecord + 12 + extensionCount; j < pCurrentRecord + *pCurrentRecord; j++)
					{
					threadName.AppendFormat(_L("%c"), *j);
					}
				if (threadName == KThreadName)
					{
					test.Printf(_L("This thread's NThread ID: %08x\n"), LE4(pCurrentRecord + 4 + extensionCount));
					gNThreadId = LE4(pCurrentRecord + 4 + extensionCount);
					}
				}
			pCurrentRecord = BTrace::NextRecord(pCurrentRecord);
			}
		btrace.DataUsed();
		}
	}
	
void LoadExesRom()
	{
	TInt r;
	TBool pass = ETrue;
	r = btrace.ResizeBuffer(32768); // 32k should be large enough
	TEST_EQ(r, KErrNone);
	btrace.SetFilter(BTrace::EPaging, ETrue);
	btrace.SetFilter(BTrace::EThreadIdentification, ETrue);
	btrace.SetMode(0);
	
	for (TInt i = 1; i <= 60; i++)
		{
		TBuf<255> filename;
		filename.Format(_L("Z:\\SYS\\BIN\\DPEXE%03d.EXE"), i);
	
		test.Printf(_L("Loading %S... "), &filename);
		
		TBool paged = EFalse;
		TBool inRom = EFalse;
		
		TUint32 nthreadAddr = 0;
		TBuf<255> processName;
		
		if (fs.IsFileInRom(filename) != NULL)
			{
			inRom = ETrue;
			}
		else
			{
			inRom = EFalse;
			}
		
		// Ensure that the paging live list is empty
		r = DPTest::FlushCache();
		if (gIsRomDemangPagingEnabled)
			{
			TEST_EQ(r, KErrNone);
			}
		else
			{
			TEST_EQ(r, KErrNotSupported);
			}
		
		btrace.Empty(); // empty the BTrace buffer
		btrace.SetMode(RBTrace::EEnable);
		RProcess proc;
		r = proc.Create(filename, _L(""));
			
		if ((testExeAttr[i] & EIBYAlias) && (testExeAttr[i] & EIBYData) && (gIsRomDemangPagingEnabled))
		// There cannot be aliases mapping to "data" files since they are moved to ROFS if the ROM is paged
			{
			TEST_EQ(r, KErrNotFound);
			continue;
			}
		else
			{
			TEST_EQ(r, KErrNone);
			}
		
		// Resume the process and wait until it completes
		TRequestStatus ps;
		proc.Logon(ps);
		proc.Resume();
		proc.Close();
		User::WaitForRequest(ps);
		
		// Disable trace
		btrace.SetMode(0);
		
		TInt size;
		TUint8* pDataStart;
		TUint8* pCurrentRecord;
		
		// We have a while loop here, in the unlikely case that our trace is fragmented	
		while ((size = btrace.GetData(pDataStart)) != 0)
			{
			pCurrentRecord = pDataStart;
			while (pCurrentRecord - pDataStart < size)
				{
				// Number of bytes used by the BTrace extensions
				TInt extensionCount = 4 * (
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EHeader2Present ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::ETimestampPresent ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::ETimestamp2Present ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EContextIdPresent ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EPcPresent ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EExtraPresent ? 1 : 0));
				
				if ((pCurrentRecord[BTrace::ECategoryIndex] == BTrace::EThreadIdentification) && (pCurrentRecord[BTrace::ESubCategoryIndex] == BTrace::EProcessName))
				// Process renamed
					{
					processName.Format(_L(""));
					for (TUint8* j = pCurrentRecord + 12 + extensionCount; j < pCurrentRecord + *pCurrentRecord; j++)
						{
						processName.AppendFormat(_L("%c"), *j);
						}
					TBuf<255> expected;
					expected.Format(_L("dpexe%03d.exe[%08x]%04x"), i, 0, 1);
					
					if (processName == expected)
						{
						nthreadAddr = LE4(pCurrentRecord + 4 + extensionCount);
						}					
					}
				if ((pCurrentRecord[BTrace::ECategoryIndex] == BTrace::EPaging) && (LE4(pCurrentRecord + 8) == nthreadAddr))
				/* The main thread of the test process tries to page in the test executable	*/
					{
					paged = ETrue;
					}
				pCurrentRecord = BTrace::NextRecord(pCurrentRecord); // move on to the next record
				}
			btrace.DataUsed();
			}
		
		if (paged)
			test.Printf(_L("paged!\n"));
		else
			test.Printf(_L("not paged!\n"));
		
		if (!gIsRomDemangPagingEnabled)
		// ROM paging disabled. All files are in ROM and unpaged
			{
			test.Printf(_L("ROM Paging disabled: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		else if (testExeAttr[i] & EIBYData)
			// data - if ROM paged, then these executables will be moved to ROFS
			// these are always expected to be RAM loaded
			{
			test.Printf(_L("EXE is DATA in ROFS\n"));
			TEST_CONDITION(!inRom);
			}
		else if (testExeAttr[i] & EIBYFileCompress)
			// Compression format specified in the IBY file
			// These are expected to be stay in ROM, but will be RAM-loaded
			{
			test.Printf(_L("EXE has own compression method: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		// from this point onwards, all executables can potentially be paged - paging policy takes precedence
		else if (gPagingPolicy == EKernelConfigCodePagingPolicyNoPaging)
			{
			test.Printf(_L("Paging policy is No Paging: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		else if (gPagingPolicy == EKernelConfigCodePagingPolicyAlwaysPage)
			{
			test.Printf(_L("Paging policy is No Paging: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		// from this point onwards, paging policy is either Default Paged or Default Unpaged - paging override takes precedence
		else if (gPagingOverride == EKernelConfigCodePagingPolicyNoPaging)
			{
			test.Printf(_L("Paging override is No Paging: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		else if (gPagingOverride == EKernelConfigCodePagingPolicyAlwaysPage)
			{
			test.Printf(_L("Paging override is Always Page: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		// from this point onwards, paging policy and override are either Default Paged or Default Unpaged - IBY setting takes precedence
		else if (testExeAttr[i] & EIBYPaged)
			{
			test.Printf(_L("Paged keyword in OBY: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		else if (testExeAttr[i] & EIBYUnpaged)
			{
			test.Printf(_L("Unpaged keyword in OBY: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		// Next, MMP setting takes precedence
		else if (testExeAttr[i] & EMMPPaged)
			{
			test.Printf(_L("Paged keyword in MMP: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		else if (testExeAttr[i] & EMMPUnpaged)
			{
			test.Printf(_L("Unpaged keyword in MMP: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		// The test exe has no attribute. Paging overright default paging mode takes precedence
		else if (gPagingOverride == EKernelConfigCodePagingPolicyDefaultUnpaged)
			{
			test.Printf(_L("Paging override is Default Unpaged: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		else if (gPagingOverride == EKernelConfigCodePagingPolicyDefaultPaged)
			{
			test.Printf(_L("Paging override is Default Paged: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		// Paging policy default paging mode takes precedence
		else if (gPagingPolicy == EKernelConfigCodePagingPolicyDefaultUnpaged)
			{
			test.Printf(_L("Paging policy is Default Unpaged: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		else if (gPagingPolicy == EKernelConfigCodePagingPolicyDefaultPaged)
			{
			test.Printf(_L("Paging policy is Default paged: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		// ROM Paging enabled without a default paging policy - this should not happen (default policy is No Paging)
		else
			{
			test.Printf(_L("No paging policy!\n"));
			test(EFalse);
			}
		}
	test(pass);
	}
	

void LoadDllsRom()
	{
	TInt r;
	TBool pass = ETrue;
	r = btrace.ResizeBuffer(32768); // 32k should be large enough
	TEST_EQ(r, KErrNone);
	btrace.SetFilter(BTrace::EPaging, ETrue);
	btrace.SetFilter(BTrace::EThreadIdentification, ETrue);
	btrace.SetMode(0);
	
	for (TInt i = 1; i <= 60; i++)
		{
		TBuf<255> filename;
		filename.Format(_L("Z:\\SYS\\BIN\\DPDLL%03d.DLL"), i);
	
		test.Printf(_L("Loading %S... "), &filename);
		
		TBool paged = EFalse;
		TBool inRom = EFalse;
		
		TUint libLoadEnd;
		TInt filesize;
		
		TUint8* addr;
		if ((addr = fs.IsFileInRom(filename)) != NULL)
			{
			inRom = ETrue;
			}
		else
			{
			inRom = EFalse;
			}
		
		RFile file;
		r = file.Open(fs, filename, EFileRead);
		if ((testDllAttr[i] & EIBYAlias) && (testDllAttr[i] & EIBYData) && (gIsRomDemangPagingEnabled))
		// There cannot be aliases mapping to "data" files since they are moved to ROFS if the ROM is paged
			{
			TEST_EQ(r, KErrNotFound);
			continue;
			}
		else
			{
			TEST_EQ(r, KErrNone);
			}
		r = file.Size(filesize);
		TEST_EQ(r, KErrNone);
		file.Close();
		
		// Ensure that the paging live list is empty
		r = DPTest::FlushCache();
		if (gIsRomDemangPagingEnabled)
			{
			TEST_EQ(r, KErrNone);
			}
		else
			{
			TEST_EQ(r, KErrNotSupported);
			}
		
		btrace.Empty(); // empty the BTrace buffer
		btrace.SetMode(RBTrace::EEnable);
		RLibrary lib;
		r = lib.Load(filename);
		libLoadEnd = User::FastCounter();
		
		TEST_EQ(r, KErrNone);
		
		TLibraryFunction function1;
		TLibraryFunction function2;
		TLibraryFunction function3;
		TLibraryFunction function4;
		
		function1 = lib.Lookup(1);
		function2 = lib.Lookup(2);
		function3 = lib.Lookup(3);
		function4 = lib.Lookup(4);
		
		test(function1 != NULL);
		test(function2 != NULL);
		test(function3 != NULL);
		test(function4 == NULL);
		
		// Resume the process and wait until it completes
	
		function1();
		function2();
		function3();
		
		lib.Close();
		
		//processResumeStart = User::FastCounter();
		//processResumeEnd = User::FastCounter();
		
		// Disable trace
		btrace.SetMode(0);
		
		TInt size;
		TUint8* pDataStart;
		TUint8* pCurrentRecord;
		
		// We have a while loop here, in the unlikely case that our trace is fragmented	
		while ((size = btrace.GetData(pDataStart)) != 0)
			{
			pCurrentRecord = pDataStart;
			while (pCurrentRecord - pDataStart < size)
				{
				// Number of bytes used by the BTrace extensions
				TInt extensionCount = 4 * (
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EHeader2Present ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::ETimestampPresent ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::ETimestamp2Present ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EContextIdPresent ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EPcPresent ? 1 : 0) +
				(pCurrentRecord[BTrace::EFlagsIndex] & BTrace::EExtraPresent ? 1 : 0));

				if ((pCurrentRecord[BTrace::ECategoryIndex] == BTrace::EPaging)
					&& (pCurrentRecord[BTrace::ESubCategoryIndex] == BTrace::EPagingPageInBegin)
					&& (LE4(pCurrentRecord + 4) > libLoadEnd)
					&& (LE4(pCurrentRecord + extensionCount) == gNThreadId)
					&& (LE4(pCurrentRecord + 4 + extensionCount) >= (TUint32) addr)
					&& (LE4(pCurrentRecord + 4 + extensionCount) < ((TUint32) addr) + filesize))
				// If the DLL is paged in under this thread after it's been RLibrary::Load'ed, then we assume the DLL is paged
					{
					paged = ETrue;
					}
				pCurrentRecord = BTrace::NextRecord(pCurrentRecord); // move on to the next record
				}
			btrace.DataUsed();
			}

		if (paged)
			test.Printf(_L("paged!\n"));
		else
			test.Printf(_L("not paged!\n"));

		if (!gIsRomDemangPagingEnabled)
		// ROM paging disabled. All files are in ROM and unpaged
			{
			test.Printf(_L("ROM Paging disabled: shouldn't be paged\n"));
			test(inRom);
			TEST_CONDITION(!paged);
			}
		else if (testDllAttr[i] & EIBYData)
			// data - if ROM paged, then these executables will be moved to ROFS
			// these are always expected to be RAM loaded
			{
			test.Printf(_L("DLL is DATA in ROFS: shouldn't be paged\n"));
			TEST_CONDITION(!inRom);
			TEST_CONDITION(!paged);
			}
		else if (testDllAttr[i] & EIBYFileCompress)
			// Compression format specified in the IBY file
			// These are expected to be stay in ROM, but will be RAM-loaded
			{
			test.Printf(_L("DLL has own compression method: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		// from this point onwards, all executables can potentially be paged - paging policy takes precedence
		else if (gPagingPolicy == EKernelConfigCodePagingPolicyNoPaging)
			{
			test.Printf(_L("Paging policy is No Paging: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		else if (gPagingPolicy == EKernelConfigCodePagingPolicyAlwaysPage)
			{
			test.Printf(_L("Paging policy is Always Page: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		// from this point onwards, paging policy is either Default Paged or Default Unpaged - paging override takes precedence
		else if (gPagingOverride == EKernelConfigCodePagingPolicyNoPaging)
			{
			test.Printf(_L("Paging override is No Paging: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		else if (gPagingOverride == EKernelConfigCodePagingPolicyAlwaysPage)
			{
			test.Printf(_L("Paging override is Always Page: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		// from this point onwards, paging policy and override are either Default Paged or Default Unpaged - IBY setting takes precedence
		else if (testDllAttr[i] & EIBYPaged)
			{
			test.Printf(_L("Paged keyword in OBY: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		else if (testDllAttr[i] & EIBYUnpaged)
			{
			test.Printf(_L("Unpaged keyword in OBY: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		// Next, MMP setting takes precedence
		else if (testDllAttr[i] & EMMPPaged)
			{
			test.Printf(_L("Paged keyword in MMP: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		else if (testDllAttr[i] & EMMPUnpaged)
			{
			test.Printf(_L("Unpaged keyword in MMP: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		// The test exe has no attribute. Paging overright default paging mode takes precedence
		else if (gPagingOverride == EKernelConfigCodePagingPolicyDefaultUnpaged)
			{
			test.Printf(_L("Paging override is Default Unpaged: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		else if (gPagingOverride == EKernelConfigCodePagingPolicyDefaultPaged)
			{
			test.Printf(_L("Paging override is Default Paged: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		// Paging policy default paging mode takes precedence
		else if (gPagingPolicy == EKernelConfigCodePagingPolicyDefaultUnpaged)
			{
			test.Printf(_L("Paging policy is Default Unpaged: shouldn't be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(!paged);
			}
		else if (gPagingPolicy == EKernelConfigCodePagingPolicyDefaultPaged)
			{
			test.Printf(_L("Paging policy is Default paged: should be paged\n"));
			TEST_CONDITION(inRom);
			TEST_CONDITION(paged);
			}
		// ROM Paging enabled without a default paging policy - this should not happen (default policy is No Paging)
		else
			{
			test.Printf(_L("No paging policy!\n"));
			test(EFalse);
			}
		}
	test(pass);
	}

GLDEF_C TInt E32Main()
	{
	TInt r;
	test.Title();
	test.Start(_L("Check environment"));
	
	// Open the BTrace handler
	r = btrace.Open();
	TEST_EQ(r, KErrNone);
	
	// capture the NThread ID of the main thread of the current process
	InitNThreadID();
	test(gNThreadId != 0);
	
	gPagingPolicy = E32Loader::PagingPolicy();
	gPagingOverride = -1;
	
	r = fs.Connect();
	TEST_EQ(r, KErrNone);
	
	if (fs.IsFileInRom(_L("\\ovr_nopaging")) != NULL)
		{
		gPagingOverride = EKernelConfigCodePagingPolicyNoPaging;
		}
	if (fs.IsFileInRom(_L("\\ovr_alwayspage")) != NULL)
		{
		gPagingOverride = EKernelConfigCodePagingPolicyAlwaysPage;
		}
	if (fs.IsFileInRom(_L("\\ovr_defaultunpaged")) != NULL)
		{
		gPagingOverride = EKernelConfigCodePagingPolicyDefaultUnpaged;
		}
	if (fs.IsFileInRom(_L("\\ovr_defaultpaged")) != NULL)
		{
		gPagingOverride = EKernelConfigCodePagingPolicyDefaultPaged;
		}
	if (fs.IsFileInRom(_L("\\pcy_nopaging")) != NULL)
		{
		gPagingPolicy = EKernelConfigCodePagingPolicyNoPaging;
		}
	if (fs.IsFileInRom(_L("\\pcy_alwayspage")) != NULL)
		{
		gPagingPolicy = EKernelConfigCodePagingPolicyAlwaysPage;
		}
	if (fs.IsFileInRom(_L("\\pcy_defaultunpaged")) != NULL)
		{
		gPagingPolicy = EKernelConfigCodePagingPolicyDefaultUnpaged;
		}
	if (fs.IsFileInRom(_L("\\pcy_defaultpaged")) != NULL)
		{
		gPagingPolicy = EKernelConfigCodePagingPolicyDefaultPaged;
		}
		
	gIsRomDemangPagingEnabled = (fs.IsFileInRom(_L("Z:\\SYS\\BIN\\DPEXE046.EXE")) == NULL);
		
	test.Printf(_L("Demand Paging Enabled? %d\n"), gIsRomDemangPagingEnabled);
	test.Printf(_L("PagingOverride %d\n"), gPagingOverride);
	test.Printf(_L("PagingPolicy %d\n"), gPagingPolicy);
	
	test.Next(_L("Load ROM EXEs"));
	LoadExesRom();
	test.Next(_L("Load ROM DLLs"));
	LoadDllsRom();
	
	btrace.Close();
	fs.Close();
	test.End();
	test.Close();
	return KErrNone;
	}
