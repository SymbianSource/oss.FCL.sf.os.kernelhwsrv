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
// e32test\pipe\t_pipe5.cpp
// This process has ReadDeviceData capability
// This is supporting program for testing pipes.
// This is used by main test file ( t_pipe.cpp) for testing
// pipes in multiprocess environment.
// 
//

#include <e32test.h>
#include <e32svr.h>
#include "rpipe.h"


LOCAL_D RTest test(_L("t_pipe5"));

_LIT(KPipeName2, "PipeWithNoCap");
_LIT(KPipeName4, "PipeWithRWCap");
_LIT(KPipeName5, "PipeWithComDDCap");
_LIT(KPipeName6, "PipeWithRWComDDCap");
_LIT(KPipeName7, "PipeWithRWComDDCapVID");
_LIT(KPipeName8, "PipeWithRWRUCap");




LOCAL_C void RunTests(void)
	{
	
	test.Start(_L("Testing In Pipe Process 5 : Process with ReadDeviceData Capabilities"));
	RPipe					aReader, aWriter;
		
	TInt		ret,aSize;
	
/////Part 1		
	test.Next(_L("PIPE TEST: PipeProcess 5-1\n"));
	ret = aReader.Open(KPipeName2,RPipe::EOpenToRead);
	test ( ret == KErrNone);
	
	test.Next(_L("PIPE TEST: PipeProcess 5-2\n"));				 
	ret = aWriter.Open(KPipeName2,RPipe::EOpenToWrite);
	test ( ret == KErrNone);
	
	aReader.Close();
	aWriter.Close();
	
	test.Next(_L("PIPE TEST: PipeProcess 5-3\n"));				 
	ret = aReader.Open(KPipeName2,RPipe::EOpenToRead);
	test ( ret == KErrNone);
	ret = aWriter.Open(KPipeName2,RPipe::EOpenToWriteNamedPipeButFailOnNoReaders);
	test ( ret == KErrNone);
	
	aReader.Close();
	aWriter.Close();
	
	test.Next(_L("PIPE TEST: PipeProcess 5-4\n"));				 
	ret = RPipe::Destroy (KPipeName2);
	test ( ret == KErrNone);


/////Part 2
	test.Next(_L("PIPE TEST: PipeProcess 5-5\n"));
	ret = aReader.Open(KPipeName4,RPipe::EOpenToRead);
	test ( ret == KErrNone);
	
	test.Next(_L("PIPE TEST: PipeProcess 5-6\n"));				 
	ret = aWriter.Open(KPipeName4,RPipe::EOpenToWrite);
	test ( ret == KErrNone);
	
	aReader.Close();
	aWriter.Close();
		
	test.Next(_L("PIPE TEST: PipeProcess 5-7\n"));				 
	ret = RPipe::Destroy (KPipeName4);
	test ( ret == KErrNone);
	
/////Part 3

	test.Next(_L("PIPE TEST: PipeProcess 5-8\n"));
	ret = aReader.Open(KPipeName5,RPipe::EOpenToRead);
	test ( ret == KErrNone);
	
	test.Next(_L("PIPE TEST: PipeProcess 5-9\n"));				 
	ret = aWriter.Open(KPipeName5,RPipe::EOpenToWrite);
	test ( ret == KErrNone);
	
	aReader.Close();
	aWriter.Close();
		
	test.Next(_L("PIPE TEST: PipeProcess 5-10\n"));				 
	ret = RPipe::Destroy (KPipeName5);
	test ( ret == KErrNone);
	
/////Part 4

	test.Next(_L("PIPE TEST: PipeProcess 5-11\n"));
	ret = aReader.Open(KPipeName6,RPipe::EOpenToRead);
	test ( ret == KErrNone);
	
	test.Next(_L("PIPE TEST: PipeProcess 5-12\n"));				 
	ret = aWriter.Open(KPipeName6,RPipe::EOpenToWrite);
	test ( ret == KErrNone);
	
	aReader.Close();
	aWriter.Close();
		
	test.Next(_L("PIPE TEST: PipeProcess 5-13\n"));				 
	ret = RPipe::Destroy (KPipeName6);
	test ( ret == KErrNone);
	
/////Part 5

	test.Next(_L("PIPE TEST: PipeProcess 5-14\n"));
	ret = aReader.Open(KPipeName7,RPipe::EOpenToRead);
	test ( ret == KErrPermissionDenied);
	
	test.Next(_L("PIPE TEST: PipeProcess 5-15\n"));				 
	ret = aWriter.Open(KPipeName7,RPipe::EOpenToWrite);
	test ( ret == KErrPermissionDenied);
	
	aReader.Close();
	aWriter.Close();
		
	test.Next(_L("PIPE TEST: PipeProcess 5-16\n"));				 
	ret = RPipe::Destroy (KPipeName7);
	test ( ret == KErrPermissionDenied);

/////Part 6

	test.Next(_L("PIPE TEST: PipeProcess 5-17\n"));
	ret = aReader.Open(KPipeName8,RPipe::EOpenToRead);
	test ( ret == KErrPermissionDenied);
	
	test.Next(_L("PIPE TEST: PipeProcess 5-18\n"));				 
	ret = aWriter.Open(KPipeName8,RPipe::EOpenToWrite);
	test ( ret == KErrPermissionDenied);
	
	aReader.Close();
	aWriter.Close();
		
	test.Next(_L("PIPE TEST: PipeProcess 5-19\n"));				 
	ret = RPipe::Destroy (KPipeName8);
	test ( ret == KErrPermissionDenied);
	
/////Part 7	
	aSize = 10;
	ret = RPipe::Create(	aSize,
							aReader, 
							aWriter,
							EOwnerProcess,//TOwnerType aTypeW
							EOwnerProcess//TOwnerType aTypeW
						);
	
	test.Next(_L("PIPE TEST: PipeProcess 5-20\n"));				 
	test ( ret == KErrNone);
	aReader.Close();
	aWriter.Close();
	
	test.End();		
	test.Close();
	
	return;
}

GLDEF_C TInt E32Main()
{

	test.Title();

	RunTests();

	return KErrNone;
}

