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
// e32test\pccd\t_med_writebm.cpp
// 
//

/**
 @file
*/

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <f32fsys.h>
#include <e32math.h>

/*
    SD/MMC/other media benchmark test
    Writes data to the media and prints out time taken (microseconds).
    Works on TBusLocalDrive level. May require a script to filter the results.

    Principle of operation:
    In the simple case scenario the user specifies media start and end position in bytes,
    write buffer size (window size), the number of write repetitions (optional) and write position increment.
    
    The test fills buffer with random data, writes it to the media (probably several times); prints out the time taken;
    increments or decrements window position; goto begin.  Test finishes when the window slides outside specified beginning or end media position.

    It more complex case we have 2 windows. It is useful for FAT emulation.
    
    -------------------------------------------
    Command line shall look like this:
    drv=[0..25] pos=xxx:yyy [wrep=<number>] wn=[1,2] w1sz=<number> w1pi=<number> [w2sz=<number> w2pi=<number>]

    where:
    drv=N           local drive physical number for 1st MMC slot on H2 & H4 it will be 1, see variantmediadef.h
    pos=xxx:yyy     xxx start media position, yyy end media position in bytes. The test will be writing data in the range xxx-yyy
    wrep=N          optional. Specifies how many times the window will be written to the media, just for more precise time calculation. Default is 1.
    wn=N            Number of data windows being written on the media. can be 1 or 2
    
    w1sz=N          Size of the data window #1 in bytes. Must be > 0
    w1pi=N          Media position increment for the window #1. if positive, the window will be moving from xxx to yyy (see media pos parameter); 
                    if 0, the window won't change its position; if <0, the window will be moving from yyy to xxx

    w2sz            the same for the window #2 if it is specified
    w2pi            the same for the window #2 if it is specified

    The test will finish when one of the windows slides across the media boundaries xxx of yyy. If you specify w1pi=0 or both w1pi=0 w2pi=0
    it may run forever :)

    Be careful, all information on the medium will be lost !!!
*/

RTest test(_L("MMC/SD write performance test"));

TBusLocalDrive BusLocalDrv;
RFs            Fs;

RBuf8   gWriteBuffer1;
RBuf8   gWriteBuffer2;

TInt    gLocalDrvNum = -1;       //-- LOCAL physical drive number (see Estart.txt)
TBool   gChangeFlag;

TUint   gWindowsNum = 0;         //-- number of windows being written

TUint32 gWriteBufSize1     = 0;  //-- write buffer 1 size, bytes
TInt32  gWriteGranularity1 = 0;  //-- write granularity 1 (write buffer position increment) 

TUint32 gWriteBufSize2     = 0;  //-- write buffer 2 size, bytes
TInt32  gWriteGranularity2 = 0;  //-- write granularity 2 (write buffer position increment) 

TInt64  gMediaStartPos = 0;      //-- media start position
TInt64  gMediaEndPos = 0;        //-- media end position

TUint   gNumWrites = 1;          //-- number of buffer writes to the media


//---------------------------------------------------------------------------------

void RndFillBuf(TDes8& aBuf);

//---------------------------------------------------------------------------------

/**
    The main part of the test, actually. Writes 1 or 2 buffers to the media (possibly several times) and 
    prints out the time taken.
*/
void DoWriteBMTest(void)
{
    test.Next(_L("Performing write benchmark test.\n"));

    TInt    nRes;
    TTime   timeStart;
    TTime   timeEnd;
    
    //-- if window pos increment is <0, it will move from end to the beginning position, backwards   
    TInt64 currMediaPos1 =  (gWriteGranularity1 >=0) ? gMediaStartPos : gMediaEndPos-gWriteBufSize1;
    TInt64 currMediaPos2 =  (gWriteGranularity2 >=0) ? gMediaStartPos : gMediaEndPos-gWriteBufSize2;

    if(gWindowsNum == 1) //-- we have only 1 window 
    {
        currMediaPos2 = 0;
        gWriteGranularity2 = 0;
    }

	RndFillBuf(gWriteBuffer1); 
	if(gWindowsNum == 2)
		RndFillBuf(gWriteBuffer2); 

    for(;;)
    {
        if(currMediaPos1 <0 || (currMediaPos1 + gWriteBufSize1) > gMediaEndPos)
            break;

        if(currMediaPos2 <0 || (currMediaPos2 + gWriteBufSize2) > gMediaEndPos)
            break;

        timeStart.UniversalTime(); //-- take start time

        for(TUint i=0; i<gNumWrites; ++i)
        {
            nRes = BusLocalDrv.Write(currMediaPos1, gWriteBuffer1); //-- write window 1
            test_KErrNone(nRes);

            if(gWindowsNum == 2)
            {
                nRes = BusLocalDrv.Write(currMediaPos2, gWriteBuffer2); //-- write window 2
                test_KErrNone(nRes);
            }
        }//for(TUint i=0; i<gNumWrites; ++i)

        timeEnd.UniversalTime(); //-- take end time
        
        TTimeIntervalMicroSeconds  usElapsed=timeEnd.MicroSecondsFrom(timeStart);
        TInt64 usTaken = usElapsed.Int64()/gNumWrites;

        //-- print out the result

        test.Printf(_L("~#pos:%lu:%lu, time:%d us\n"), currMediaPos1, currMediaPos2, (TInt32)usTaken);

        //-- move windows
        currMediaPos1 += gWriteGranularity1;
        currMediaPos2 += gWriteGranularity2;
    }

}


//---------------------------------------------------------------------------------

/** fill a given buffer with random bytes */
void RndFillBuf(TDes8& aBuf)
{
    static TInt64 rndSeed = Math::Random();

    //-- ?? optimise here ??
    for(TInt i=0; i<aBuf.Size(); ++i)
    {
        aBuf[i] = (TUint8)Math::Rand(rndSeed);
    }
}


//---------------------------------------------------------------------------------

/** Initialise environment */
TBool Initialise()
{
    //-- print out some parameters:
    test.Printf(_L("~#Local Drive:%d\n"), gLocalDrvNum);
    test.Printf(_L("~#MediaPos:%lu:%lu\n"), gMediaStartPos, gMediaEndPos);
    test.Printf(_L("~#WinNum:%d\n"), gWindowsNum);
    test.Printf(_L("~#NumWrites:%d\n"), gNumWrites);
    test.Printf(_L("~#Window1 sz:%d, posInc:%d \n"), gWriteBufSize1, gWriteGranularity1);

    if(gWindowsNum == 2)
    {
        test.Printf(_L("~#Window2 sz:%d, posInc:%d \n"), gWriteBufSize2, gWriteGranularity2);
    }

    
    test((gLocalDrvNum >= EDriveA) && (gLocalDrvNum <= EDriveZ));
    test(gMediaStartPos >=0 && gMediaEndPos >gMediaStartPos);
    test(gWindowsNum == 1 || gWindowsNum == 2);
    test(gWriteBufSize1 > 0);
    if(gWindowsNum == 2)
    {
        test(gWriteBufSize2 > 0);
    }
    test(gNumWrites > 0);

    
    TInt nRes;
    nRes = Fs.Connect();
    test_KErrNone(nRes);

    //-- connect to the TBusLocalDrive
    test.Printf(_L("Connecting to the PHYSICAL drive #%d\n"), gLocalDrvNum);

    nRes = BusLocalDrv.Connect(gLocalDrvNum, gChangeFlag);
    test_KErrNone(nRes);

    TLocalDriveCapsV2 info;
    TPckg<TLocalDriveCapsV2> infoPckg(info);
    nRes = BusLocalDrv.Caps(infoPckg);
    test_KErrNone(nRes);

    //-- create write buffer 1
    nRes=gWriteBuffer1.CreateMax(gWriteBufSize1);
    test_KErrNone(nRes);


    //-- create write buffer 2
    if(gWindowsNum == 2)
    {
        nRes=gWriteBuffer2.CreateMax(gWriteBufSize2);
        test_KErrNone(nRes);
    }

    return ETrue;
}

//---------------------------------------------------------------------------------

/** Finalise environment */
void Finalise(void)
{
    BusLocalDrv.Disconnect();
    BusLocalDrv.Close();
    
    gWriteBuffer1.Close();
    gWriteBuffer2.Close();

    Fs.Close();
}


/**
    Just a helper method. Looks for a given pattern in the given string and returns the rest of the found token.
    @return KErrNotFound if the aPattern wasn't found in aSrc
            KErrNone otherwise and the rest of the token in aToken
*/
TInt DoFindToken(const TDesC& aSrc, const TDesC& aPattern,TPtrC& aToken)
{
    TLex    lex(aSrc);
    TPtrC   token;

    for(;;)
    {
        lex.SkipSpace();
        token.Set(lex.NextToken());
    
        if(token.Length() == 0)
        {
            test.Printf(_L("Parameter %S not found!\n"), &aPattern);   
            return KErrNotFound;            
        }

        if(token.FindF(aPattern) == 0)
        {//-- found a requires patern, extract substring next to it
            aToken.Set(token.Right(token.Length() - aPattern.Length()));
            break;
        }


    }

    return KErrNone;
}


/**
    Parse the command line, which shall look like:
    drv=[0..25] pos=xxx:yyy [wrep=<number>] wn=[1,2] w1sz=<number> w1pi=<number> [w2sz=<number> w2pi=<number>]
*/
TBool ParseCommandLine(void)
{
    TBuf<0x100> cmdLine;
    User::CommandLine(cmdLine);

    cmdLine.LowerCase();

    test.Printf(_L("Command line:\n"));   
    test.Printf(cmdLine);   
    test.Printf(_L("\n"));   

    TLex lexParam;

    TInt    nVal;
    TUint   uVal;
    TInt    nRes;
    
    TPtrC   token;
    
    //-- process "drv" parameter. It shall look like: "drv=1"
    //-- this is a physical number of a local drive
    if(DoFindToken(cmdLine, _L("drv="), token) != KErrNone)
        return  EFalse;

    lexParam.Assign(token);
    lexParam.SkipSpace();
    nRes = lexParam.Val(nVal);
    if(nRes!= KErrNone || nVal < EDriveA || nVal > EDriveZ)
    {
            test.Printf(_L("Invalid 'drv' parameter value!\n"));   
            return EFalse;
    }

    gLocalDrvNum = nVal;


    //-- process "pos" parameter It shall look like: "pos=xxx:yyy" where "xxx" is a start media position, "yyy" end media position
    //-- It specifies start and end media position
    if(DoFindToken(cmdLine, _L("pos="), token) != KErrNone)
        return  EFalse;

    lexParam.Assign(token);
    lexParam.SkipSpace();

    TInt64 startPos;
    TInt64 endPos;
                
    //-- start media position
    nRes = lexParam.Val(startPos);
    if(nRes!= KErrNone || startPos< 0)
    {
        test.Printf(_L("invalid start 'pos' value!\n"));   
        return EFalse;
    }

    //-- delimiter
    lexParam.SkipSpace();
    if(lexParam.Get() != ':')
    {
        test.Printf(_L("invalid 'pos' parameter!\n"));   
        return EFalse;
    }

    //-- end media position
    lexParam.SkipSpace();
    nRes = lexParam.Val(endPos);
    if(nRes!= KErrNone || endPos < 0)
    {
        test.Printf(_L("invalid end 'pos' value!\n"));   
        return EFalse;
    }

    gMediaStartPos = startPos;
    gMediaEndPos = endPos;


    //-- process "wn" parameter It shall look like: "wn=1" or "wn=2"
    //-- It specifies number of sliding windows.
    lexParam.SkipSpace();
    if(DoFindToken(cmdLine, _L("wn="), token) != KErrNone)
        return  EFalse;
    
    lexParam.Assign(token);
    lexParam.SkipSpace();

    nRes = lexParam.Val(uVal);
    if(nRes!= KErrNone || uVal > 2)
    {
        test.Printf(_L("wrong 'wn' parameter value, it must be 1 or 2 !\n"));   
        return EFalse;
    }

    gWindowsNum = uVal;


    //-- process "w1sz" & "w1pi" parameters. They shall look like: "w1sz=16384" & "w1pi=512"
    //-- these parameters specify size and position increment for the window 1
    //-- if w1pi <0 the window will slide from the media end position to the beginning
    lexParam.SkipSpace();
    if(DoFindToken(cmdLine, _L("w1sz="), token) != KErrNone)
        return  EFalse;

    lexParam.Assign(token);
    lexParam.SkipSpace();

    nRes = lexParam.Val(uVal);
    if(nRes!= KErrNone || uVal ==0)
    {
        test.Printf(_L("wrong 'w1sz' parameter value, it must be > 0 !\n"));   
        return EFalse;
    }

    gWriteBufSize1 = uVal;
    

    lexParam.SkipSpace();
    if(DoFindToken(cmdLine, _L("w1pi="), token) != KErrNone)
        return  EFalse;

    lexParam.Assign(token);
    lexParam.SkipSpace();

    nRes = lexParam.Val(nVal);
    if(nRes!= KErrNone)
    {
        return EFalse;
    }

    gWriteGranularity1 = nVal;

    //-- process "w2sz" & "w2pi" parameters. They shall look like: "w2sz=16384" & "w2pi=512"
    //-- these parameters specify size and position increment for the window 1
    //-- if w1pi <0 the window will slide from the media end position to the beginning
    if(gWindowsNum == 2)
    {
        lexParam.SkipSpace();
        if(DoFindToken(cmdLine, _L("w2sz="), token) != KErrNone)
            return  EFalse;

        lexParam.Assign(token);
        lexParam.SkipSpace();

        nRes = lexParam.Val(uVal);
        if(nRes!= KErrNone || uVal ==0)
        {
            test.Printf(_L("wrong 'w2sz' parameter value, it must be > 0 !\n"));   
            return EFalse;
        }

        gWriteBufSize2 = uVal;
    

        lexParam.SkipSpace();
        if(DoFindToken(cmdLine, _L("w2pi="), token) != KErrNone)
            return  EFalse;

        lexParam.Assign(token);
        lexParam.SkipSpace();

        nRes = lexParam.Val(nVal);
        if(nRes!= KErrNone)
        {
            return EFalse;
        }

        gWriteGranularity2 = nVal;
     }
    
    //-- extract wrep=<number> parameter.
    //-- it specifies how many times buffers will be written to the media
    lexParam.SkipSpace();
    if(DoFindToken(cmdLine, _L("wrep="), token) == KErrNone)
    {
    
    
    lexParam.Assign(token);
    lexParam.SkipSpace();

    nRes = lexParam.Val(uVal);
    if(nRes!= KErrNone || uVal ==0 )
    {
        test.Printf(_L("wrong 'wrep' parameter value, it must be >0 !\n"));   
        return EFalse;
    }
     
        gNumWrites = uVal;
    }
    else
    {
        gNumWrites = 1;
    }


    return ETrue;
}

void PrintInfo()
{
    test.Printf(_L("Media write benchmark test. For use mostly with mmc/sd cards.\n"));
    test.Printf(_L("Usage: See source code for command line parameters.\n"));
}


//---------------------------------------------------------------------------------

void MainL(void)
{
    test.Title();
    test.Start(_L("Start testing...\n"));

    
    //-- it will initialise global test parameters
    if(!ParseCommandLine())
    {
        PrintInfo();
        return;
    }

    if(!Initialise())
    {
        return; //-- something went wrong
    }


    DoWriteBMTest();
    
    Finalise();
    
    test.End();
}


TInt E32Main()
{

    CTrapCleanup* cleanup=CTrapCleanup::New() ; // get clean-up stack
    
    TRAPD(r,MainL());
    
    delete cleanup ; // destroy clean-up stack
    
    return r;
}



