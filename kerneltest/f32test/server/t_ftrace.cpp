// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_ftrace.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <f32tracedef.h>
#include <e32test.h>
#include "t_server.h"

#include "../../../kernel/eka/include/d32btrace.h"
#include "../../../kernel/eka/include/e32btrace.h"
#include <utraceefsrv.h>

RTest test(_L("T_FTRACE"));

RBTrace Trace;

void SetBTraceFilter(const TUint32* aNew,TUint32* aOld)
	{
	TUint category = 0;
	do
		{
		TUint32 newBits = *aNew++;
		TUint32 oldBits = 0;
		do
			{
			oldBits >>= 1;
			if(Trace.SetFilter(category,newBits&1))
				oldBits |= 0x80000000u;
			newBits >>= 1;
			++category;
			}
		while(category&31);
		if(aOld)
			*aOld++ = oldBits;
		}
	while(category<256);
	}



//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-T_FTRACE-0001
//! @SYMTestCaseDesc			Test File Server Tracing of RFile::Replace()
//! @SYMTestType				UT
//! @SYMPREQ					PREQ1617
//! @SYMTestPriority			Medium
//! @SYMTestActions				
//! 	1.	Call RFile::Replace() to create a file
//! 	2.	Get trace data from BTrace and verify that the expected trace data is present
//! 
//! @SYMTestExpectedResults
//! 	1.	Trace data payload should be as expected, i.e. it should contain the file name, mode etc.
//---------------------------------------------------------------------------------------------------------------------
void TestRFileReplace()
	{
	test.Start(_L("Test trace output from creating a file"));
	RFile file;
	TFileName testFileName = _L("File.txt");

	TheFs.Delete(testFileName);

	Trace.Empty();

	TInt r = file.Replace(TheFs,testFileName,EFileStreamText);
	test_KErrNone(r);


	TBool funcInFound = EFalse;
	TBool funcOutFound = EFalse;

	TBuf8<1024> buf;
	for(;;)
		{
		TUint8* record;
		TInt dataSize = Trace.GetData(record);
		if(!dataSize)
			break;
		TUint8* end = record+dataSize;

		while(record<end)
			{
			TUint size = record[BTrace::ESizeIndex];
			TUint flags = record[BTrace::EFlagsIndex];
			TUint category = record[BTrace::ECategoryIndex];
			TUint subCategory = record[BTrace::ESubCategoryIndex];
			TUint8* data = record+4;
			size -= 4;

			buf.Zero();
			if(flags&(BTrace::EHeader2Present))
				{
				data += 4;
				size -= 4;
				}

			if((flags&(BTrace::ETimestampPresent|BTrace::ETimestamp2Present))==(BTrace::ETimestampPresent|BTrace::ETimestamp2Present))
				{
				buf.AppendFormat(_L8("time:%08x:%08x "),((TUint32*)data)[1],*(TUint32*)data);
				data += 8;
				size -= 8;
				}
			else if(flags&(BTrace::ETimestampPresent|BTrace::ETimestamp2Present))
				{
				buf.AppendFormat(_L8("time:%08x "),*(TUint32*)data);
				data += 4;
				size -= 4;
				}

			if(flags&(BTrace::EContextIdPresent))
				{
				buf.AppendFormat(_L8("context:%08x "),*(TUint32*)data);
				data += 4;
				size -= 4;
				}
			else
				{
				buf.AppendFormat(_L8("                 "));
				}

			if(flags&(BTrace::EPcPresent))
				{
				buf.AppendFormat(_L8("pc:%08x "),*(TUint32*)data);
				data += 4;
				size -= 4;
				}

			if(flags&(BTrace::EExtraPresent))
				{
				data += 4;
				size -= 4;
				}

			TUint32 data0 = (size>0) ? *(TUint32*)(data) : 0;
			TUint32 data1 = (size>4) ? *(TUint32*)(data+4) : 0;
			TPtrC8 des(0,0);
			if(size>=8)
				des.Set(data+8,size-8);

			buf.AppendFormat(_L8("size:%d flags:%02x cat:%d,%d data: "),size,flags,category,subCategory);
			for(TUint i=0; i<size; i+=4)
				buf.AppendFormat(_L8("%08x "),*(TUint32*)(data+i));
			buf.Append('\r');
			buf.Append('\n');
			test(buf.MaxLength() >= (buf.Length()*2));
			RDebug::RawPrint(buf.Expand());


			if (category == UTF::EBorder && subCategory == 0 && data0 == EF32TraceUidEfsrv)
				{
				if (data1 == UTraceModuleEfsrv::EFileReplace)
					{
					TInt sessionHandle = (size>8) ? *(TUint32*)(data+8) : 0;
					TUint32 fileMode = (size>12) ? *(TUint32*)(data+12) : 0;
					TInt fileNameLen = (size>16) ? *(TUint32*)(data+16) : 0;
					fileNameLen/= 2;	// convert to unicode length
					TText16* fileName = (TText16*) ((size>20) ? (data+20) : NULL);

					test(sessionHandle == TheFs.Handle());
					test(fileMode == EFileStreamText);
					test(fileNameLen == testFileName.Length());
					TPtrC16 fileNamePtr (fileName, fileNameLen);
					test(fileName != NULL);
					test(testFileName.Compare(fileNamePtr) == 0);
					funcInFound = ETrue;
					}
				else if (data1 == UTraceModuleEfsrv::EFileReplaceReturn)
					{
					TInt retCode = (size>8) ? *(TUint32*)(data+8) : 0;
					TInt subsessionHandle = (size>12) ? *(TUint32*)(data+12) : 0;

					test(retCode == KErrNone);
					test(subsessionHandle == file.SubSessionHandle());
					funcOutFound = ETrue;
					}
				}

			record = BTrace::NextRecord(record);
			}
		Trace.DataUsed();
		}

	file.Close();
	TheFs.Delete(testFileName);

	test (funcInFound);
	test (funcOutFound);
	}

//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-T_FTRACE-0002
//! @SYMTestCaseDesc			Test File Server Tracing of RFs::Rename()
//! @SYMTestType				UT
//! @SYMPREQ					PREQ1617
//! @SYMTestPriority			Medium
//! @SYMTestActions				
//! 	1.	Call RFile::Replace() to create a file
//! 	2.	Close the file
//! 	3.	Call RFs::Rename to rename the file
//! 	4.	Get trace data from BTrace and verify that the expected trace data is present
//! 
//! @SYMTestExpectedResults
//! 	1.	Trace data payload should be as expected, i.e. it should contain both file names, etc.
//---------------------------------------------------------------------------------------------------------------------
void TestRFsRename()
	{
	test.Start(_L("Test trace output from renaming a file"));
	RFile file;
	TFileName testFileName1 = _L("File1.txt");
	TFileName testFileName2 = _L("File2.txt");

	TheFs.Delete(testFileName1);
	TheFs.Delete(testFileName2);

	TInt r = file.Replace(TheFs,testFileName1,EFileStreamText);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	file.Close();

	Trace.Empty();

	r = TheFs.Rename(testFileName1, testFileName2);
	test_KErrNone(r);


	TBool funcInFound = EFalse;
	TBool funcOutFound = EFalse;

	TBuf8<1024> buf;
	for(;;)
		{
		TUint8* record;
		TInt dataSize = Trace.GetData(record);
		if(!dataSize)
			break;
		TUint8* end = record+dataSize;

		while(record<end)
			{
			TUint size = record[BTrace::ESizeIndex];
			TUint flags = record[BTrace::EFlagsIndex];
			TUint category = record[BTrace::ECategoryIndex];
			TUint subCategory = record[BTrace::ESubCategoryIndex];
			TUint8* data = record+4;
			size -= 4;

			buf.Zero();
			if(flags&(BTrace::EHeader2Present))
				{
				data += 4;
				size -= 4;
				}

			if((flags&(BTrace::ETimestampPresent|BTrace::ETimestamp2Present))==(BTrace::ETimestampPresent|BTrace::ETimestamp2Present))
				{
				buf.AppendFormat(_L8("time:%08x:%08x "),((TUint32*)data)[1],*(TUint32*)data);
				data += 8;
				size -= 8;
				}
			else if(flags&(BTrace::ETimestampPresent|BTrace::ETimestamp2Present))
				{
				buf.AppendFormat(_L8("time:%08x "),*(TUint32*)data);
				data += 4;
				size -= 4;
				}

			if(flags&(BTrace::EContextIdPresent))
				{
				buf.AppendFormat(_L8("context:%08x "),*(TUint32*)data);
				data += 4;
				size -= 4;
				}
			else
				{
				buf.AppendFormat(_L8("                 "));
				}

			if(flags&(BTrace::EPcPresent))
				{
				buf.AppendFormat(_L8("pc:%08x "),*(TUint32*)data);
				data += 4;
				size -= 4;
				}

			if(flags&(BTrace::EExtraPresent))
				{
				data += 4;
				size -= 4;
				}

			TUint32 data0 = (size>0) ? *(TUint32*)(data) : 0;
			TUint32 data1 = (size>4) ? *(TUint32*)(data+4) : 0;
			TPtrC8 des(0,0);
			if(size>=8)
				des.Set(data+8,size-8);

			buf.AppendFormat(_L8("size:%d flags:%02x cat:%d,%d data: "),size,flags,category,subCategory);
			for(TUint i=0; i<size; i+=4)
				buf.AppendFormat(_L8("%08x "),*(TUint32*)(data+i));
			buf.Append('\r');
			buf.Append('\n');
			test(buf.MaxLength() >= (buf.Length()*2));
			RDebug::RawPrint(buf.Expand());


			if (category == UTF::EBorder && subCategory == 0 && data0 == EF32TraceUidEfsrv)
				{
				TUint8* recData = data+8;
				if (data1 == UTraceModuleEfsrv::EFsRename)
					{
					TInt sessionHandle = *(TUint32*) recData; recData+= 4;

					TInt fileNameLen1 = *(TUint32*) recData; recData+= 4;
					TText16* fileName1 = (TText16*) recData; recData+= ((fileNameLen1 +4) & ~3);

					TInt fileNameLen2 = *(TUint32*) recData; recData+= 4;
					TText16* fileName2 = (TText16*) recData; recData+= fileNameLen2;

					fileNameLen1/= 2;	// convert to unicode length
					fileNameLen2/= 2;	// convert to unicode length


					test(sessionHandle == TheFs.Handle());
					
					test(fileNameLen1 == testFileName1.Length());
					TPtrC16 fileNamePtr1 (fileName1, fileNameLen1);
					test(fileName1 != NULL);
					test(testFileName1.Compare(fileNamePtr1) == 0);

					test(fileNameLen2 == testFileName2.Length());
					TPtrC16 fileNamePtr2 (fileName2, fileNameLen2);
					test(fileName2 != NULL);
					test(testFileName2.Compare(fileNamePtr2) == 0);

					funcInFound = ETrue;
					}
				else if (data1 == UTraceModuleEfsrv::EFsRenameReturn)
					{
					TInt retCode = (size>8) ? *(TUint32*)(data+8) : 0;

					test(retCode == KErrNone);

					funcOutFound = ETrue;
					}
				}

			record = BTrace::NextRecord(record);
			}
		Trace.DataUsed();
		}


	test (funcInFound);
	test (funcOutFound);

	TheFs.Delete(testFileName1);
	TheFs.Delete(testFileName2);
	}

void CallTestsL()
	{

// By default, file server trace-points are only compiled in in debug mode
#if defined(_DEBUG)
	test.Title();
	TInt r;

	test.Start(_L("Open LDD"));
	r = Trace.Open();
	test_KErrNone(r);


	TUint32 OldTraceFilter[8] = {0};

	TUint savedMode = Trace.Mode();
	SetBTraceFilter(OldTraceFilter,OldTraceFilter);

	Trace.ResizeBuffer(0x100000);
	Trace.Empty();

	Trace.SetMode(RBTrace::EEnable | RBTrace::EFreeRunning);

	TBool b;
//	b = Trace.SetFilter(BTrace::EThreadIdentification, ETrue);
//	test(b >= 0);
	b = Trace.SetFilter(UTF::EPanic, ETrue);
	test(b >= 0);
	b = Trace.SetFilter(UTF::EError, ETrue);
	test(b >= 0);
	b = Trace.SetFilter(UTF::EBorder, ETrue);
	test(b >= 0);

	b = Trace.SetFilter2(EF32TraceUidEfsrv, ETrue);
	test(b >= 0);

	TestRFileReplace();
	TestRFsRename();

	// restore trace settings...
	Trace.SetMode(0);
	SetBTraceFilter(OldTraceFilter,OldTraceFilter);
	Trace.SetMode(savedMode);


	test.Next(_L("Close LDD"));
	Trace.Close();

	test.End();
#endif
	}

