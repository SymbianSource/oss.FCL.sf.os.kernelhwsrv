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
// e32test\pccd\t_nandbm.cpp
// Test read/write performance for NAND drives accessed directly
// through the Local Media Sub-System
// When no command parameter is supplied search for first 
// writeable NAND drive that can be found
// If a drive number parameter in the range 0 - F is provided,
// interpret this as a local drive number in the range 0 - 15
// and test the drive if it is a writeable NAND drive
// Please note that local drive numbers at the Local Media Sub-System 
// level are not the same as drive numbers / drive letters at the 
// File Server level. File Server drive numbers / drive letters fall
// in the range 0 - 25 / a - z. The mapping between local drive numbers 
// and File Server drive numbers / drive letters is defined in 
// estart.txt or its equivalent 
// Use TBusLocalDrive directly and bypass the File Server
// Fill NAND user data drive with data prior to executing actual 
// performance tests. TBusLocalDrive should read from areas that have 
// been written to and are therefore assigned. Where areas are 
// unassigned, XSR FTL returns FFs without accessing the NAND flash 
// hardware. This type of behaviour generates misleadingly quick 
// performance figures
// Test various block sizes in the range 16 - 65536 bytes
// 
//


#include <e32test.h>
#include <f32fsys.h>
#include <e32hal.h>
#include <e32uid.h>
#include <f32dbg.h>


LOCAL_D TBuf<1048576> DataBuf;
LOCAL_D	TBusLocalDrive TheDrive;
LOCAL_D TBool ChangedFlag;
LOCAL_D RFs TheFs;

RTest test(_L("Local NAND Drive BenchMark Test"));

LOCAL_C void DoRead(TInt aReadBlockSize)
//
// Do Read benchmark
//
	{
    TInt msgHandle = KLocalMessageHandle;
	TLocalDriveCapsV2 info;
	TPckg<TLocalDriveCapsV2> infoPckg(info);
	TInt maxSize;
	TheDrive.Caps(infoPckg);
	maxSize=I64LOW(info.iSize);
	TInt count,pos,err;
	count=pos=err=0;

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	timer.After(reqStat,10000000); // After 10 secs
	while(reqStat==KRequestPending)
		{
		if (TheDrive.Read(pos,aReadBlockSize,&DataBuf,msgHandle,0)==KErrNone)
			count++;
		else
			err++;
		pos+=aReadBlockSize;
		if (pos>=(maxSize-aReadBlockSize))
			pos=0;
		}
#if defined (__WINS__)
	test.Printf(_L("Read %d %d byte blocks in 10 secs\n"),count,aReadBlockSize);
#else
	TBuf<60> buf;
	TReal32 rate=((TReal32)(count*aReadBlockSize))/10240.0F;
	TRealFormat rf(10,2);
	buf.Format(_L("Read %d %d byte blocks in 10 secs ("),count,aReadBlockSize);
	buf.AppendNum(rate,rf);
	buf.Append(_L("Kb/s)\n"));
	test.Printf(buf);
#endif
	test.Printf(_L("Errors:%d\n"),err);
	}

LOCAL_C void DoWrite(TInt aWriteBlockSize)
//
// Do write benchmark
//
	{
    TInt msgHandle = KLocalMessageHandle;
	TLocalDriveCapsV2 info;
	TPckg<TLocalDriveCapsV2> infoPckg(info);
	TInt maxSize;
	TheDrive.Caps(infoPckg);
	maxSize=I64LOW(info.iSize);
	TInt count,pos,err;
	count=pos=err=0;

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	timer.After(reqStat,10000000); // After 10 secs
	while(reqStat==KRequestPending)
		{
		if (TheDrive.Write(pos,aWriteBlockSize,&DataBuf,msgHandle,0)==KErrNone)
			count++;
		else
			err++;
		pos+=aWriteBlockSize;
		if (pos>=(maxSize-aWriteBlockSize))
			pos=0;
		}
#if defined (__WINS__)
	test.Printf(_L("Write %d %d byte blocks in 10 secs\n"),count,aWriteBlockSize);
#else
	TBuf<60> buf;
	TReal32 rate=((TReal32)(count*aWriteBlockSize))/10240.0F;
	TRealFormat rf(10,2);
	buf.Format(_L("Write %d %d byte blocks in 10 secs ("),count,aWriteBlockSize);
	buf.AppendNum(rate,rf);
	buf.Append(_L("Kb/s)\n"));
	test.Printf(buf);
#endif
	test.Printf(_L("Errors:%d\n"),err);
	}

GLDEF_C TInt E32Main()
    {
	test.Title();

	TBuf<0x100> cmd;
	User::CommandLine(cmd);						// put command line into decriptor
	TLex lex(cmd);
	TPtrC param=lex.NextToken();				// point token at local drive number if any
	test.Printf(_L("Local Drive = %S\r\n"),&param);

	TChar localDrv;
	TInt localDrvNum;
	TBusLocalDrive drive;
	TBool changeFlag;
	TLocalDriveCapsV4 driveCaps;
	TPckg<TLocalDriveCapsV4> capsPckg(driveCaps);
	TInt r;
	
	if (param.Length()==0)
		{
		// locate writeable NAND drive
		for (localDrvNum=0; localDrvNum<KMaxLocalDrives; localDrvNum++)
			{

			r = drive.Connect(localDrvNum,changeFlag);

			if (r!=KErrNone)
				continue;

			r = drive.Caps(capsPckg);
  			drive.Disconnect();
			if ((r==KErrNone)
				&&(driveCaps.iType==EMediaNANDFlash)
				&&!(driveCaps.iMediaAtt&KMediaAttWriteProtected)
				&&(driveCaps.iPartitionType!=KPartitionTypeSymbianCrashLog))
			break;
			}
		if (localDrvNum==16)
			{
			test.Printf(_L("Suitable drive could not be found\r\n"));
			test.Printf(_L("Writeable NAND drive required for test\r\n"));
			return(KErrGeneral);
			}
		}
	else
		{
		// is selected local drive number in the range 0-15
		localDrv=param[0];
		localDrv.UpperCase();
		if (localDrv>='0'&&localDrv<='9')
			{
			localDrvNum=((TInt)localDrv-'0');
			}
		else if (localDrv>='A'&&localDrv<='F')
			{
			localDrvNum=((TInt)localDrv-'A'+10);
			}
		else
			{
			test.Printf(_L("Commandline %S invalid\r\n"), &cmd);
			test.Printf(_L("Usage:\r\n"));
			test.Printf(_L("t_nandbm with no arguments, test first suitable NAND drive \r\n"));
			test.Printf(_L("t_nandbm x, where x = 0-F test local drive number 0-15\r\n"));
			return(KErrGeneral);
			}
		// is selected drive suitable for test
		r = drive.Connect(localDrvNum,changeFlag);
		if(r!=KErrNone)
			{
			test.Printf(_L("Can't connect to drive %d\r\n"), localDrvNum);
			return(KErrGeneral);
			}
		r = drive.Caps(capsPckg);
		if(r!=KErrNone)
			{
			test.Printf(_L("Drive %d caps method error\r\n"), localDrvNum);
			return(KErrGeneral);
			}
 		drive.Disconnect();
		if ((driveCaps.iType!=EMediaNANDFlash)
			||(driveCaps.iMediaAtt&KMediaAttWriteProtected)
			||(driveCaps.iPartitionType==KPartitionTypeSymbianCrashLog))
			{
			test.Printf(_L("Drive %d has unsuitable capabilities\r\n"), localDrvNum);
			test.Printf(_L("Writeable NAND drive required for test\r\n"));
			return(KErrGeneral);
			}

		}

	// Fill the nand user data drive with data so TBusLocalDrive reads 
	// from areas that have been written to and are therefore assigned.
	// Where areas are unassigned, XSR FTL returns FFs without accessing 
	// the NAND flash hardware. This type of behaviour generates misleadingly 
	// quick performance figures.
	TInt writesize=1048576;
	TInt pos = 0;
	TInt msgHandle = KLocalMessageHandle;
	
	TheDrive.Connect(localDrvNum,ChangedFlag);
	test.Printf( _L("Fill up NAND drive %d\r\n"),localDrvNum);
	while(writesize>=1)
		{
		test.Printf( _L("%d byte write to pos %d of NAND drive\r\n"),writesize,pos);
		if (TheDrive.Write(pos,writesize,&DataBuf,msgHandle,0)!=KErrNone)
			{
			writesize/=16;
			}
		else
			{
			pos += writesize;
			}
		}

	test.Start(_L("Start Benchmarking ..."));

	DoRead(16);
	DoRead(256);
	DoRead(512);
	DoRead(513);
	DoRead(2048);
	DoRead(4096);
	DoRead(16384);
	DoRead(32768);
	DoRead(65536);

	DoWrite(16);
	DoWrite(256);
	DoWrite(512);
	DoWrite(513); 
	DoWrite(2048);
	DoWrite(4096);
	DoWrite(16384);
	DoWrite(32768);
	DoWrite(65536);
    
	test.End();

	TheFs.Close();

	return(KErrNone);
	}
  
