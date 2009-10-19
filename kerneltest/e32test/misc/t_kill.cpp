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
// e32test\misc\t_kill.cpp
// 
//

#include <e32test.h>
#include <f32file.h>
#include "u32std.h"
#include "../misc/prbs.h"

RTest test(_L("T_KILL"));

_LIT(KFileName1,"D:\\File1");
_LIT(KFileName2,"D:\\File2");

LOCAL_D TBuf8<1024> Cluster;
LOCAL_D TBuf8<2048> WriteBuf;

GLDEF_C TInt E32Main()
	{
	test.Title();
	test.Start(_L("Connect to file server"));
	RFs fs;
	TInt r=fs.Connect();
	test(r==KErrNone);

	test.Next(_L("Delete old files"));
	r=fs.Delete(KFileName1);
	test(r==KErrNone || r==KErrNotFound);
	r=fs.Delete(KFileName2);
	test(r==KErrNone || r==KErrNotFound);

	test.Next(_L("Create file 1"));
	RFile file1;
	r=file1.Create(fs,KFileName1,EFileWrite);
	test(r==KErrNone);

	test.Next(_L("Create file 2"));
	RFile file2;
	r=file2.Create(fs,KFileName2,EFileWrite);
	test(r==KErrNone);

	Cluster.SetLength(512);
	test.Next(_L("Write 1024 bytes to file 1"));
	r=file1.Write(Cluster);
	test(r==KErrNone);

	test.Next(_L("Write 1024 bytes to file 2"));
	r=file2.Write(Cluster);
	test(r==KErrNone);

	test.Next(_L("Write 1024 bytes to file 1"));
	r=file1.Write(Cluster);
	test(r==KErrNone);

	TUint seed[2];
	seed[0]=0xb504f334;
	seed[1]=0;
	TInt p=0;
	test.Next(_L("Rewind file 1"));
	r=file1.Seek(ESeekStart,p);
	test(r==KErrNone);
	TInt size=WriteBuf.MaxLength();
	WriteBuf.SetLength(size);
	test.Next(_L("Get 2K of random data"));
	TInt i;
	for (i=0; i<size; i++)
		WriteBuf[i]=static_cast<TUint8>(Random(seed));
	test.Next(_L("Press a key to write to file 1"));
	test.Getch();
	TRequestStatus stat;
	file1.Write(WriteBuf,stat);

	return 0;
	}

