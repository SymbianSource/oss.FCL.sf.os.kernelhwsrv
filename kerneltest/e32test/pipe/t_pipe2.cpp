// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pipe\t_pipe2.cpp
// This is supporting program for testing pipes.
// This is used by main test file ( t_pipe.cpp) for testing
// pipes in multiprocess environment.
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <rpipe.h>


LOCAL_D RTest test(_L("t_pipe2"));
_LIT(KPipeName5, "InterProcessPipe1");


_LIT8(KTestDataIP, "ABCDEFGHIJ");
_LIT8(KTestData,"Pipe Data To Be Passed");

LOCAL_C void RunTests(void)
	{
	test.Start(_L("Testing In Pipe Process 2"));

	TInt 				ret;									// Return value variable

	RPipe 				aReader;
	RPipe 				aReaderUN;
	const	TBufC<50>	cPipeName1(KPipeName5);
	
	TBuf8<150>			cPipeReadData1;
	TInt				aSize;
	TRequestStatus		stat1;
	
	test.Next(_L("PIPE TEST: PipeProcess 2-1: Open Read Handle to unamed Pipe\n"));
	ret=aReaderUN.Open(3,EOwnerProcess);
	test_KErrNone(ret);

	test.Next(_L("PIPE TEST: PipeProcess 2-1.1: Get the UnNamed pipe read handle from another process \n"));
	ret = aReaderUN.Size();
	test (ret = 22);
	
	ret = aReaderUN.MaxSize();
	test (ret = 512);
	
	ret = aReaderUN.Read(cPipeReadData1,22);
	test (ret == 22);
	
	ret = 	cPipeReadData1.Compare(KTestData);
	test (ret == KErrNone);

	aReaderUN.Close();
	

	test.Next(_L("Open handle to named pipe\n"));

	aSize =10;
	ret = aReader.Open(cPipeName1, RPipe::EOpenToRead);
	test_KErrNone(ret);	

	test.Next(_L("PIPE TEST: PipeProcess 2-2: Wait till NotifyDataAvailable.\n"));

	aReader.NotifyDataAvailable(stat1);

	User::WaitForRequest(stat1);

	test.Next(_L("PIPE TEST: PipeProcess 2-3: Call ReadBlocking and Read the data.\n"));

	aReader.Flush(); 
	ret = aReader.ReadBlocking(cPipeReadData1,aSize);
	test_Equal(aSize, ret);
	ret = cPipeReadData1.Compare(KTestDataIP);
	test( ret== KErrNone);
		
	test.Next(_L("PIPE TEST: PipeProcess 2-4: Exit Process t_pipe2.\n"));
	aReader.Close();
	test.End();
	test.Close();
}

GLDEF_C TInt E32Main()
//
//
    {

	test.Title();

	RunTests();

	return KErrNone;
    }

