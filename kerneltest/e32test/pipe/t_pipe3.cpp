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
// e32test\pipe\t_pipe3.cpp
// This is supporting program for testing pipes.
// This is used by main test file ( t_pipe.cpp) for testing
// pipes in multiprocess environment.
// 
//

#include <e32test.h>
#include <e32svr.h>
#include "rpipe.h"


LOCAL_D RTest test(_L("t_pipe3"));

_LIT(KPipeName2, "PipeWithNoCap");
_LIT(KPipeName3, "PipeWithNoCapVID");
_LIT(KPipeName4, "PipeWithRWCap");




LOCAL_C void RunTests(void)
	{
	
	test.Start(_L("Testing In Pipe Process 3 : Process with No Capabilities"));
	RPipe					aReader, aWriter;
		
	TInt		ret,aSize;
	
/////// Part 1		
	test.Next(_L("PIPE TEST: PipeProcess 3-1: Open Read handle to Pipe with No capability.\n"));
	ret = aReader.Open(KPipeName2,RPipe::EOpenToRead);
	test ( ret == KErrNone);
	
	test.Next(_L("PIPE TEST: PipeProcess 3-2: Open Write handle to Pipe with No capability.\n"));				 
	ret = aWriter.Open(KPipeName2,RPipe::EOpenToWrite);
	test ( ret == KErrNone);
	
	aReader.Close();
	aWriter.Close();
	
	test.Next(_L("PIPE TEST: PipeProcess 3-3: Open Write handle to Pipe with No capability.\n"));				 
	ret = aReader.Open(KPipeName2,RPipe::EOpenToRead);
	test ( ret == KErrNone);
	ret = aWriter.Open(KPipeName2,RPipe::EOpenToWriteNamedPipeButFailOnNoReaders);
	test ( ret == KErrNone);
	
	aReader.Close();
	aWriter.Close();
	
	test.Next(_L("PIPE TEST: PipeProcess 3-4: Destroy Pipe with No capability.\n"));				 
	ret = RPipe::Destroy (KPipeName2);
	test ( ret == KErrNone);
	
/////// Part 2
	test.Next(_L("PIPE TEST: PipeProcess 3-5: Open Read handle to Pipe with Read-Write capability.\n"));
	ret = aReader.Open(KPipeName4,RPipe::EOpenToRead);
	test ( ret == KErrPermissionDenied);
	
	test.Next(_L("PIPE TEST: PipeProcess 3-6: Open Write handle to Pipe with Read-Write capability.\n"));				 
	ret = aWriter.Open(KPipeName4,RPipe::EOpenToWrite);
	test ( ret == KErrPermissionDenied);
	
	aReader.Close();
	aWriter.Close();
	
/////// Part 3
	test.Next(_L("PIPE TEST: PipeProcess 3-7: Open Read handle to Pipe with No capability.\n"));
	ret = aReader.Open(KPipeName3,RPipe::EOpenToRead);
	test ( ret == KErrNone);
	
	test.Next(_L("PIPE TEST: PipeProcess 3-8: Open Write handle to Pipe with No capability.\n"));				 
	ret = aWriter.Open(KPipeName3,RPipe::EOpenToWrite);
	test ( ret == KErrNone);
	
	aReader.Close();
	aWriter.Close();
	
	test.Next(_L("PIPE TEST: PipeProcess 3-9: Destroy Pipe with No capability.\n"));				 
	ret = RPipe::Destroy (KPipeName3);
	test ( ret == KErrNone);
	
/////// Part 4			
	aSize = 10;
	ret = RPipe::Create(	aSize,
							aReader, 
							aWriter,
							EOwnerProcess,//TOwnerType aTypeW
							EOwnerProcess//TOwnerType aTypeW
						);
	
	test.Next(_L("PIPE TEST: PipeProcess 3-10: Create UnNamed Pipe. It shall be allow.\n"));				 
	test ( ret == KErrNone);
	aReader.Close();
	aWriter.Close();

	test.End();		
	test.Close();

/////// Test End
	
	return;
}

GLDEF_C TInt E32Main()
{

	test.Title();

	RunTests();

	return KErrNone;
}

