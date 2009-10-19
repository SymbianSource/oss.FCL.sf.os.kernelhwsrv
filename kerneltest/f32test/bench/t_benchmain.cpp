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
// f32test\bench\t_benchmain.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#include <f32dbg.h>
#include "t_benchmain.h"
#include "t_chlffs.h"

RFs         TheFs;
TFileName   gSessionPath;
TFileName   gExeFileName(RProcess().FileName());
TInt        gAllocFailOff=KAllocFailureOff;
TInt        gAllocFailOn=KAllocFailureOff;
TInt64      gSeed = 51703;
TInt        gFilesLimit; 	
TInt        gTypes;  		
TInt        gMode; 
TInt        gFormat = EFalse; 	
TInt        gMinutes; 
TInt        gFileSize = 0; 			// In Kbytes	

TInt 		gTestHarness = 0;
TInt 		gTestCase = 1;
TInt 		gTimeUnit = 1000; // values: 1 - us, 1000 - ms, 1000000 - s


TChar       gDriveToTest = ' ';

	
////////////////////////////////////////////////////////////
// Template functions encapsulating ControlIo magic
//
template <class C>

TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c)
	{
    TPtr8 ptrC((TUint8 *)&c, sizeof(C), sizeof(C));

    TInt r = fs.ControlIo(drv, fkn, ptrC);

    return r;
	}

/** Prints headers of FAT32 and File Cache benchmarking

	@param aType type of test (FAT32 = 1/2 or File Cache = 3/4/5)
	@param
*/
void PrintHeaders(TInt aType, TPtrC16 aTitle ) 
	{
	TBuf16<250> title = _L("#~TestTitle_%d: ");
	
	title.Append(aTitle);
	title.Append(_L("\n"));
	
	test.Printf(_L("#~TestId=%d\n"), gTestHarness);	
	test.Printf(title, gTestHarness);
	test.Printf(_L("#~Report Variant: \n"));	
	test.Printf(_L("#~Report Description: \n"));	
	
	_LIT(KSeconds,"seconds");
	_LIT(KMilliSecs,"milliseconds");
	_LIT(KMicroSecs,"microseconds");

	TPtrC16 timeUnit;
	if (gTimeUnit == 1) 
		timeUnit.Set(KMicroSecs);
	else if (gTimeUnit == 1000) 
		timeUnit.Set(KMilliSecs);
	else if (gTimeUnit == 1000000)
		timeUnit.Set(KSeconds);
	else
		{
		test.Printf(_L("Please, check gTimeUnit value\n"));
		test(EFalse);	
		}


	if(aType == 1) 
		{ // All FAT32 tests but t_fsrrepeat
		test.Printf(_L("#~TestParam_%d: MaxFiles=%d, type=%d, mode=%d, timeUnit=%S\n"), gTestHarness, gFilesLimit, gTypes, gMode, &timeUnit);
		
		test.Printf(_L("#~TestRows_%d: 4\n"), gTestHarness);
		test.Printf(_L("#~TestColumns_%d: 4, NFiles, 8_3, 20_chars, 50_50\n"), gTestHarness);	
		}
	else if(aType == 2) 
		{ // t_fsrrepeat
		test.Printf(_L("#~TestParam_%d: MaxFiles=%d, type=%d, mode=%d, timeUnit=%S\n"), gTestHarness, gFilesLimit, gTypes, gMode, &timeUnit);
		
		test.Printf(_L("#~TestRows_%d: 12\n"), gTestHarness);
		test.Printf(_L("#~TestColumns_%d: 4, DirName, 1st, 2nd, 3rd\n"), gTestHarness);			
		} 
	else if(aType == 3)
		{ // Large sequential reads/writes
		test.Printf(_L("#~TestParam_%d: mode=%d, timeUnit=%S\n"), gTestHarness, gMode, &timeUnit);

		test.Printf(_L("#~TestRows_%d: 8\n"), gTestHarness);
		test.Printf(_L("#~TestColumns_%d: 4, bsize, 100KB, 1MB, 10MB\n"), gTestHarness);
		}
	else if(aType == 4)
		{ // Small random reads/writes
		test.Printf(_L("#~TestParam_%d: mode=%d, timeUnit=%S\n"), gTestHarness, gMode, &timeUnit);

		test.Printf(_L("#~TestRows_%d: 11\n"), gTestHarness);
		test.Printf(_L("#~TestColumns_%d: 8, bsize, 1KB, 2KB, 4KB, 8KB, 16KB, 32KB, 64KB\n"), gTestHarness);
		}
	else if(aType == 5)
		{ // Streaming
		test.Printf(_L("#~TestParam_%d: mode=%d, timeUnit=%S\n"), gTestHarness, gMode, &timeUnit);

		test.Printf(_L("#~TestRows_%d: 6\n"), gTestHarness);
		test.Printf(_L("#~TestColumns_%d: 4, bsize, 5m_w, 15s_w_r, 15s_w_r \n"), gTestHarness);
		}
	else 
		{
		test.Printf(_L("Check the function PrintHeaders() in t_benchmain.cpp, which wasn't called with the right parameters\n"));
		test(EFalse);	
		}
	
	}

/** Prints a line of results 

	@param aPosX  Row of the table
	@param aPosY  Column of the table
	@param aValue Value figure for that position 
*/
void PrintResultTime( TInt aPosX, TInt aPosY, TInt aValue) 
	{
	test.Printf(_L("#~TS_Res_%d,%d,[%d,%d]=%d\n "), gTestHarness, gTestCase, aPosX, aPosY, aValue );
	}


/** Prints any other type of data 

	@param aPosX  Row of the table
	@param aPosY  Column of the table
	@param aValue Value figure for that position 
*/
void PrintResult( TInt aPosX, TInt aPosY, TInt aValue) 
	{
	test.Printf(_L("#~TS_Res_%d,%d,[%d,%d]=%d\n "), gTestHarness, gTestCase, aPosX, aPosY, aValue);
	}

/** Prints string

	@param aPosX  Row of the table
	@param aPosY  Column of the table
	@param aValue Value figure for that position 
*/
void PrintResultS( TInt aPosX, TInt aPosY, TDes16& aValue) 
	{
	TBuf16<250> buffer = _L("#~TS_Res_%d,%d,[%d,%d]=");
	
	buffer.Append(aValue);
	buffer.Append(_L("\n"));
	test.Printf(buffer, gTestHarness, gTestCase, aPosX, aPosY);
	}

/** Call all RFormat methods

	@param aDrive Drive to be formatted
*/
void FormatFat(TDriveUnit aDrive)
	{
	RFormat format;
	TPckgBuf<TInt> count;
	TInt r = format.Open(TheFs, aDrive.Name(), EQuickFormat, count());
	FailIfError(r);
	
	test(count() == 100);
	TRequestStatus status;
	while (count())
		{
		format.Next(count, status);
		User::WaitForRequest(status);
		test(status == KErrNone || status == KErrNotSupported);
		}
		
	format.Close();
	test.Printf(_L("Drive formatted\n"));
	}

/** Validates the drive selection

	@param aDrive Drive to be validated
	@param aTest  Type of test 
*/
TInt ValidateDriveSelection(TDriveUnit aDrive,TSelectedTest aTest)
	{
	if ((aDrive == EDriveZ) || ((aDrive == EDriveC) && (aTest == ELocalDriveTest)))
		{
		test.Printf(_L("Test not available for this drive\n"));
		test.Printf(_L("Press any key to continue...\n"));
		return (KErrNotSupported);
		}
	else
		return (KErrNone);
	}

/** Validates the selection for this tests and update appropriate variables

	@param aSelector This object is meant to give information about what needs 
					 to be tests in manual mode
*/
TInt Validate(TAny* aSelector) 
	{
	if(gMode == 0) 
		{
		if (((CSelectionBox*)aSelector)->CurrentKeyPress() != EKeyEnter)
				return(KErrNone);
		
		TInt r = ValidateDriveSelection(((CSelectionBox*)aSelector)->CurrentDrive(),EFileSeekTest);
		if (r == KErrNotSupported)
			return (r);
		
		TDriveUnit drive = ((CSelectionBox*)aSelector)->CurrentDrive();
		gSessionPath[0] = TUint8('A' + drive);
		r = TheFs.SetSessionPath(gSessionPath);
		FailIfError(r);
		}
	
	return KErrNone;
	}

/** Prints current volume information

*/
void PrintVolInfo()
	{
    TVolumeInfo volInfo;
    TInt r;

    r = TheFs.Volume(volInfo);
    FailIfError(r);

    test.Printf(_L("DriveAtt:0x%X, MediaAtt:0x%X, Free:%d KBytes\n"), volInfo.iDrive.iDriveAtt, volInfo.iDrive.iMediaAtt, (TUint32)(volInfo.iFree / 1024));
	}

/** Create directory with a number of files, with specified file name type

    @param  aN      number of files to create
    @param  aType   file names type : 1 - 8.3, 2 - 20 characters, 3 - 50/50
*/
TInt CreateDirWithNFiles(TInt aN, TInt aType)
	{
	TInt i,r=0;

	RFile file;
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	
	TBuf16<50> path;
	TBuf16<50> buffer(50); 	
	
	dirtemp.Format(KDirMultipleName,aType, aN);
	directory=gSessionPath;
	directory.Append(dirtemp);
	
	r = TheFs.MkDir(directory);
	test(r == KErrNone || r == KErrAlreadyExists);

    const TUint KNumFilesPrintTreshold = 100;
    const TInt  KFileSize = gFileSize * 1024;
    
    PrintVolInfo();
	
	i = 0; 
	while( i < aN )
		{        
        // generate file name depending on type required
        switch(aType)    
        	{
            case 1: // 8.3 filemnames 
                FileNamesGeneration(buffer, 8, i, i%3+1);
            break;

            case 2: // 20 characrer filenames
                FileNamesGeneration(buffer, 20, i, i%3+1);
            break;

            case 3: // 50/50 mix
                if(i%2) 
                	FileNamesGeneration(buffer, 8, i, i%3+1) ;
			    else  	
			    	FileNamesGeneration(buffer, 20, i, i%3+1) ;
            break;
            default:
                test(0);
            break;
        	};
        path = directory;
		path.Append(buffer);
        
        // create or replace a file
        r = file.Replace(TheFs, path, EFileShareAny|EFileWrite);
        if(r != KErrNone)
        	{
            PrintVolInfo();
            test.Printf(_L("Error creating file: %S, %d\n"), &path, r);
            test(0);
        	}

        // set file size if required, file contents doesn't matter
        if(gFileSize > 0)
        	{
            r = file.SetSize(KFileSize); 
            if(r != KErrNone)
            	{
                PrintVolInfo();
                test.Printf(_L("Error setting file size: %S, %d, err:%d\n"), &path, KFileSize, r);
                test(0);
            	}
        	}

		file.Close();
		
        if(i > 0 && ((i+1)%KNumFilesPrintTreshold) == 0)
        	{
            test.Printf(_L("created %d files, type:%d\n"), i + 1, aType);
        	}
        
		i++;
    	}//while


    // write "last.txt" file to the end of directory
	path = directory;
	path.Append(KCommonFile);
	r = file.Replace(TheFs,path,EFileShareAny|EFileWrite);
    if(r != KErrNone)
    	{
        PrintVolInfo();
        test.Printf(_L("Error creating file: %S, %d\n"), &path, r);
        test(0);
    	}
	
	// put random content to the "last.txt" file if specified length of files > 0
    if(gFileSize > 0)
    	{
        r = file.SetSize(gFileSize * 1024); // gFileSize is in KBytes
        if(r != KErrNone)
        	{
            PrintVolInfo();
            test.Printf(_L("Error setting file size: %S, %d\n"), &path, r);
            test(0);
        	}
    	}

	file.Close();
	
	return(KErrNone);
	}


/**
 Creates 12 directories with different sort of files and namefiles
 100 files with 8.3, 20 chars and 50/50
 1000 files with 8.3, 20 chars and 50/50
 5000 files with 8.3, 20 chars and 50/50
 10000 files with 8.3, 20 chars and 50/50 
*/
TInt TestFileCreate(TAny* aSelector)
	{
	TInt i = 100, j = 1;
	
	Validate(aSelector);
	
	while(i <= gFilesLimit) 
		{
		if(i == 100) 
			{	
			j = 1;
			while(j <= gTypes) 
				CreateDirWithNFiles(100, j++); 
			}
		if(i == 1000) 
			{	
			j=1;
			while(j <= gTypes) 
				CreateDirWithNFiles(1000, j++); 
			}
		if(i == 5000) 
			{	
			j = 1;
			while(j <= gTypes) 
				CreateDirWithNFiles(5000, j++); 
			}
		if(i == 10000) 
			{	
			j = 1;
			while(j <= gTypes) 
				CreateDirWithNFiles(10000, j++); 
			}
		i += 100;
		}
	
	return(KErrNone);
	}

/** Generate a filename according to the parameters configuration

    @param  aBuffer Buffer where the name of the file will be returned
    @param  aLong 	Length of the name of the file
    @param  aPos 	Number to be attached to the name of the file
    @param  ext 	Type of extension (1/2/3) 
*/
void FileNamesGeneration(TDes16& aBuffer, TInt aLong, TInt aPos,TInt ext) 
	{
	TInt padding;
	TInt i=0;
	TBuf16<10> tempbuf;
	
	_LIT(KNumber,"%d");
	tempbuf.Format(KNumber, aPos);
	padding = aLong - tempbuf.Size() / 2;
	aBuffer = _L("");
	while(i < padding)
		{
		aBuffer.Append('F');
		i++;
		}
	
	_LIT(KExtension1, ".TXT");
	_LIT(KExtension2, ".HTM");
	_LIT(KExtension3, ".LOG");

	aBuffer.Append(tempbuf);
	switch(ext)
		{
		case 1: aBuffer.Append(KExtension1);break;
		case 2: aBuffer.Append(KExtension2);break;
		case 3: aBuffer.Append(KExtension3);break;
		default: aBuffer.Append(KExtension1);break;
		}	
	}

/** Do a checkdisk and report failure

*/
void CheckDisk()
	{
	test.Next(_L("Check Disk"));
	TInt r = TheFs.CheckDisk(gSessionPath);
	if (r != KErrNone && r != KErrNotSupported && r != KErrPermissionDenied)
		ReportCheckDiskFailure(r);
	}

/** Report a disk failure
	
	@param aRet The error will be returned in this variable
*/
void ReportCheckDiskFailure(TInt aRet)
	{
	test.Printf(_L("CHECKDISK FAILED: "));
	switch(aRet)
		{
		case 1:	test.Printf(_L("File cluster chain contains a bad value (<2 or >maxCluster)\n")); break;
		case 2:	test.Printf(_L("Two files are linked to the same cluster\n")); break;
		case 3:	test.Printf(_L("Unallocated cluster contains a value != 0\n"));	break;
		case 4:	test.Printf(_L("Size of file != number of clusters in chain\n")); break;
		default: test.Printf(_L("Undefined Error value %d\n"),aRet);
		}
	}


/** Expand the cleanup stack

*/
void PushLotsL()
	{
	TInt i;
	
	for(i=0;i<1000;i++)
		CleanupStack::PushL((CBase*)NULL);
	
	CleanupStack::Pop(1000);
	}

/** Do testing on aDrive

	@param aDrive Drive for the testing
*/
void DoTests(TInt aDrive)
	{

	gSessionPath=_L("?:\\");
	TChar driveLetter;
	TInt r = TheFs.DriveToChar(aDrive, driveLetter);
	FailIfError(r);
	gSessionPath[0] = (TText)driveLetter;
	r = TheFs.SetSessionPath(gSessionPath);
	FailIfError(r);

    test.Printf(_L("DoTests() Session Path: %S\n"),&gSessionPath);

	CheckMountLFFS(TheFs,driveLetter);
	
	User::After(1000000);

	r = TheFs.MkDirAll(gSessionPath);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		test.Printf(_L("MkDirAll() r %d\n"),r);
		test(EFalse);
		}
	TheFs.ResourceCountMarkStart();
	TRAP(r,CallTestsL());
	if (r == KErrNone)
		TheFs.ResourceCountMarkEnd();
	else
		{
		test.Printf(_L("Error: Leave %d\n"),r);
		test(EFalse);
		}
		
	CheckDisk();
	}

/** Syntax of the test 

	@param aOption option 1 is related to FAT32 testing/option 2 relates to the file caching
*/
void syntax (TInt aOption) 
	{
	_LIT(KBad, "Wrong argument"); 

	if(aOption == 1) 
		{
		test.Printf(_L("Usage: \n testname <drive> <100/1000/5000/10000 number of files> <1/2/3 type of files> <mode 0 manual, 1 automatic>\n "));
		}
	else if (aOption == 2) 
		{
		test.Printf(_L("Usage: \n t_fcachebm <drive> <mode 0 manual, 1 automatic>\n "));
		}
	
	User::Panic(KBad,KErrArgument);
	}

/**
 Parse commands :
	 
	 1) t_name drive maxfiles filetypes manual/automatic
	    maxfiles can be: 100, 1000, 5000 or 10000
	    filetypes can be 1 for 8.3, 2 for 20 chars as well and 3 for all
	    manual 0 and automatic 1
	 2) t_fcachebm drive manua/automatic
	 3) t_fsrcreatefiles
 */
void ParseCommandArguments()
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	
    test.Printf(_L("Command line:\n"));
    test.Printf(cmd);
    test.Printf(_L("\n"));

    TLex lex(cmd);
	lex.SkipSpace();

    TPtrC token;
	TFileName thisfile=RProcess().FileName();
	
	TInt i = 0;
	TInt testcase = 0;
	TBool finish = EFalse;
	_LIT(KCacheBM, "Z:\\sys\\bin\\t_fcachebm.exe");
	_LIT(KCreate, "Z:\\sys\\bin\\t_fsrcreatefiles.exe");
	
	test.Printf(KCacheBM);
	test.Printf(thisfile);
	while((!finish) && (i <= 5))
		{
		switch(i)
			{
			case 0:
				if((thisfile != KCacheBM) && (thisfile != KCreate)) // FAT32 tests
					testcase = 1; 
				else if(thisfile == KCreate) // FAT32 test files creation
					testcase = 3;
				else 					 // File Cache (PREQ914) performance tests
					testcase = 2;			
				break;
			case 1:
				test.Printf(_L("\nCLP=%S\n"), &token);
				if(token.Length() != 0)		
					{
					gDriveToTest = token[0];
					gDriveToTest.UpperCase();
					if(gMode==1) 
						gSessionPath[0] = (TText)gDriveToTest;
					}
					else						
						{
						gDriveToTest = 'C';
						}
			break;
			case 2:
				if((testcase == 1) || (testcase == 3)) // testcase == 1 || testcase == 3
					{
					if(token.Length() != 0) 
						{
						test.Printf(_L("Number of files=%S\n"),&token);
						if(token[0]=='5') 
						{
							gFilesLimit = 5000;
						}
						else 
							if(token[0] == '1' && token.Length() == 3) gFilesLimit = 100;
							else if(token[0] == '1' && token.Length() == 4) gFilesLimit = 1000; 
								 else if(token[0] == '1' && token.Length() == 5) gFilesLimit = 10000;
								 	  else syntax(testcase);
						}
					else
						gFilesLimit = 10000 ;
					
					}
				else // (testcase == 2)
					{
					if(token.Length() == 1) 
						{
						TChar c = token[0];
						test.Printf(_L("0 manual/1 automatic ? %S\n"),&token);
						if(c.IsDigit() && ((c == '0') || (c == '1'))) 
							gMode = c.GetNumericValue();
						 	  else syntax(testcase);
						}
					else 
						syntax(testcase);		
						}
			break;
			case 3:
				if((testcase == 1) || (testcase == 3)) 
					{
					if(token.Length() == 1) 
						{	
						TChar c = token[0];
						test.Printf(_L("File type=%S\n"), &token);
						if(c.IsDigit() &&((c == '1') || (c == '2') || (c == '3'))) 
							gTypes = c.GetNumericValue();
						else syntax(testcase);
						}
					else // Default value
						gTypes = 3;
					}
			break;
			case 4: 
				if((testcase == 1) || (testcase == 3)) 
					{	
					TChar c = token[0];
					if(token.Length() == 1) 
						{	
						test.Printf(_L("0 manual/1 automatic ? %S\n"),&token);
						if(c.IsDigit() && ((c == '0') || (c == '1'))) 
							gMode = c.GetNumericValue();
				 	    else syntax(1);
						}
					else 
						gMode = 0;
				}
			break;
			case 5:
				if(testcase == 3)
					{
					TChar c = token[0];
					if(c.IsDigit()) 
						gFileSize = c.GetNumericValue();
					else syntax(1);
					}
			break;
			
			default: 
			syntax(testcase); 
			}
		finish = lex.Eos();
		token.Set(lex.NextToken());	
		i++;
		}
	}

/** Main function

	@return KErrNone if everything was ok, panics otherwise
*/
TInt E32Main()
    {
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();
	TRAPD(r,PushLotsL());
	__UHEAP_MARK;	

	test.Title();
	test.Start(_L("Starting benchmarking tests..."));

	ParseCommandArguments(); //need this for drive letter to test and all the parameters

	r = TheFs.Connect();
	FailIfError(r);
	TheFs.SetAllocFailure(gAllocFailOn);
	TTime timerC;
	timerC.HomeTime();
	TFileName sessionp;
	TheFs.SessionPath(sessionp);

	TInt theDrive;
		
	r = TheFs.CharToDrive(gDriveToTest,theDrive);
	FailIfError(r);
	
	PrintDrvInfo(TheFs, theDrive); 
		
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TPckgBuf<TIOCacheValues> pkgOrgValues;
	TIOCacheValues& orgValues=pkgOrgValues();
	r = controlIo(TheFs,theDrive, KControlIoCacheCount, orgValues);
	FailIfError(r);
	
	test.Printf(_L("\nNumber of items on close list at beginning=%d\n"), orgValues.iCloseCount);
	test.Printf(_L("Number of items on free list at beginning=%d\n"), orgValues.iFreeCount);
	test.Printf(_L("Number of items allocated at beginning=%d\n"), orgValues.iAllocated);
#endif
	
	DoTests(theDrive);
	
	TTime endTimeC;
	endTimeC.HomeTime();
	TTimeIntervalSeconds timeTakenC;
	r = endTimeC.SecondsFrom(timerC,timeTakenC);
	FailIfError(r);
	
	if(gFormat) 
		{
		FormatFat(gSessionPath[0]-'A');
		}
	
	test.Printf(_L("#~T_Timing_%d: %d S\n"), gTestHarness, timeTakenC.Int());
	TheFs.SetAllocFailure(gAllocFailOff);
	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TPckgBuf<TIOCacheValues> pkgValues;
	TIOCacheValues& values=pkgValues();
	r = controlIo(TheFs,theDrive, KControlIoCacheCount, values);
	test(r==KErrNone);
	
	test.Printf(_L("\nNumber of items on close list at end=%d\n"),values.iCloseCount);
	test.Printf(_L("Number of items on free list at end=%d\n"),values.iFreeCount);
	test.Printf(_L("Number of items allocated at the end=%d\n"),values.iAllocated);
	
	test(orgValues.iCloseCount==values.iCloseCount);
	test(orgValues.iAllocated == values.iAllocated);
#endif

	TheFs.Close();
	test.End();
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }
