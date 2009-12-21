/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


/**
@test
@internalComponent

This contains CT_RawDiskData
*/

//	User includes
#include "T_RawDiskData.h"

/*@{*/
///	Parameters
_LIT(KDrive,	        						"drive");
_LIT(KObjectName,                               "object_name");
_LIT(KData,										"data");
_LIT(KPosition,									"position");

///Commands
_LIT(KCmdOpen,                                  "Open");
_LIT(KCmdClose,                                 "Close");
_LIT(KCmdNew,                                   "new" );
_LIT(KCmdDestructor,                            "~" );
_LIT(KCmdRead,                                  "Read");
_LIT(KCmdWrite,                                 "Write");

_LIT(KDriveA,							        "EDriveA");
_LIT(KDriveB,				        			"EDriveB");
_LIT(KDriveC,	        						"EDriveC");
_LIT(KDriveD,						        	"EDriveD");
_LIT(KDriveE,		        					"EDriveE");
_LIT(KDriveF,		        					"EDriveF");
_LIT(KDriveG,				        			"EDriveG");
_LIT(KDriveH,		        					"EDriveH");
_LIT(KDriveI,		        					"EDriveI");
_LIT(KDriveJ,       							"EDriveJ");
_LIT(KDriveK,							        "EDriveK");
_LIT(KDriveL,					        		"EDriveL");
_LIT(KDriveM,				    	    		"EDriveM");
_LIT(KDriveN,		        					"EDriveN");
_LIT(KDriveO,       							"EDriveO");
_LIT(KDriveP,					    	    	"EDriveP");
_LIT(KDriveQ,					        		"EDriveQ");
_LIT(KDriveR,		        					"EDriveR");
_LIT(KDriveS,	    		    				"EDriveS");
_LIT(KDriveT,   				    			"EDriveT");
_LIT(KDriveU,							        "EDriveU");
_LIT(KDriveV,					        		"EDriveV");
_LIT(KDriveW,				        			"EDriveW");
_LIT(KDriveX,				           			"EDriveX");
_LIT(KDriveY,				    	    		"EDriveY");
_LIT(KDriveZ,			    	    			"EDriveZ");
/*@}*/

CT_RawDiskData* CT_RawDiskData::NewL()
/**
* Two phase constructor
*/
	{
	CT_RawDiskData* ret = new (ELeave) CT_RawDiskData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

CT_RawDiskData::CT_RawDiskData()
:	iRawDisk(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}

void CT_RawDiskData::ConstructL()
/**
* Protected constructor. Second phase construction
*/
	{
	}
	
CT_RawDiskData::~CT_RawDiskData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}

void CT_RawDiskData::DoCleanup()
/**
* Contains cleanup implementation
*/
	{
	//Deleting RRawDisk.
	if (iRawDisk)
		{
		DoCmdClose();
		INFO_PRINTF1(_L("Deleting current RRawDisk"));
		delete iRawDisk;
		iRawDisk = NULL;
		}
	}

TAny* CT_RawDiskData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return iRawDisk;
	}

TBool CT_RawDiskData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
/**
 * Process a command read from the ini file
 *
 * @param	aCommand requiring command to be processed
 * @param	aSection the section in the ini file requiring the command to be processed
 * @param	aAsyncErrorIndex the index of asynchronous command error code belongs to.
 *
 * @leave	system wide error
 *
 * @return	ETrue if the command is processed
 */
	{
	TBool retVal = ETrue;
	
	if (aCommand == KCmdNew)
		{
		DoCmdNewL();
		}
	else if (aCommand == KCmdDestructor)
		{
		DoCmdDestructor();
		}
    else if (aCommand == KCmdOpen)
	    {
	    DoCmdOpen(aSection);
	    }
	else if (aCommand == KCmdClose)
	    {
	    DoCmdClose();
	    }
    else if (aCommand == KCmdRead)
        {
        DoCmdReadL(aSection);
        }
    else if (aCommand == KCmdWrite)
        {
        DoCmdWriteL(aSection);
        }
	else
		{
		retVal = EFalse;
		}
	return retVal;
	}


void CT_RawDiskData::DoCmdNewL()
/** Creates new RRawDisk class instance */
	{
	//Deletes previous RRawDisk class instance if it was already created.
	DoCleanup();

	INFO_PRINTF1(_L("Create new RRawDisk class instance"));

	// do create
	TRAPD(err, iRawDisk = new (ELeave) RRawDisk());
	if (err != KErrNone)
		{
		ERR_PRINTF2(_L("new error %d"), err);
		SetError(err);
		}
	else
		{
		INFO_PRINTF1(_L("Create new RRawDisk class instance completed successfully!"));
		}
	}
	
	
void CT_RawDiskData::DoCmdDestructor()
/** Destroy RRawDisk the object */
	{
	DoCleanup();
	}


void CT_RawDiskData::DoCmdOpen(const TDesC& aSection)
/** RRawDisk::Open */
	{
	INFO_PRINTF1(_L("Opening a direct access channel to the disk!"));

	RFs* rfsObject = NULL;
	TPtrC rfsObjectName;
	if (GET_MANDATORY_STRING_PARAMETER(KObjectName, aSection, rfsObjectName))
		{
		TRAPD(err, rfsObject = (RFs*)GetDataObjectL(rfsObjectName));
		
		if (err == KErrNone)
			{
    		// get drive number from parameters
    		TDriveNumber driveNumber = EDriveA;
    		if (!GetDriveNumber(aSection, KDrive(), driveNumber))
    			{
    			ERR_PRINTF2(_L("No %S"), &KDrive());
    			SetBlockResult(EFail);
    			}
    		else
    		    {
		   		TInt err = KErrNone;
            	err = iRawDisk->Open(*rfsObject, driveNumber);

	    		if (err != KErrNone)
	    			{
	    			ERR_PRINTF2(_L("Opening iRawDisk failed with error : %d"), err);
	    			SetError( err );
	    			}
	    		else
					{
					INFO_PRINTF1(_L("Opening iRawDisk completed successfully!"));
					}
	    	    }
			}
		else
			{
			ERR_PRINTF1(_L("Error with fileserver"));
    		SetBlockResult(EFail);	
			}
		}
	}


void CT_RawDiskData::DoCmdClose()
/** RRawDisk::Close */
	{
	INFO_PRINTF1(_L("Closing current RRawDisk"));
	iRawDisk->Close();
	}


void CT_RawDiskData::DoCmdReadL( const TDesC& aSection )
    {
	INFO_PRINTF1(_L("Reading directly from disc!"));
    TInt err = KErrNone;
    
    TPtrC	expectedData;
	if(GET_MANDATORY_STRING_PARAMETER(KData(), aSection, expectedData))
		{		
		HBufC* expectedResultBuf = HBufC::NewL(expectedData.Length());
		
		TPtr expectedResult = expectedResultBuf->Des();
		expectedResult.Copy(expectedData);
		
		INFO_PRINTF2(_L("Expecting data: %S"), &expectedResult);
		
		HBufC8* readBuf = HBufC8::NewL( expectedData.Length());
		TPtr8 readBufPtr = readBuf->Des();
			
		TInt64 pos = 0;
		GET_OPTIONAL_INT64_PARAMETER(KPosition(), aSection, pos);
			
		err = iRawDisk->Read(pos, readBufPtr);

		if(err == KErrNone)
			{
			HBufC* readResultBuf = HBufC::NewL(readBufPtr.Length());
			
			TPtr readResult = readResultBuf->Des();
			readResult.Copy(readBufPtr);
			
			INFO_PRINTF2(_L("Reading data: %S"), &readResult);
			
			if (readResult != expectedResult)
				{
				ERR_PRINTF3(_L("Read data does not match expected data! Read: %S expected: %S"), &readResult, &expectedResult);
				SetBlockResult( EFail );	
				}
			else
				{
				INFO_PRINTF1(_L("Reading data directly from disc was successful!"));	
				}
			delete readResultBuf;
			readResultBuf = NULL;
			}
		else
			{
			ERR_PRINTF2(_L("Reading data directly from disc failed with error %d"), err);
			SetError(err);
			}
		delete expectedResultBuf;
		expectedResultBuf = NULL;
		delete readBuf;
		readBuf = NULL;
		}
    else
    	{
    	ERR_PRINTF2(_L("No %S"), &KData());
    	SetBlockResult(EFail);
    	}
    }


void CT_RawDiskData::DoCmdWriteL( const TDesC& aSection )
    {
    INFO_PRINTF1(_L("Writing directly to disc!"));
    
    TPtrC	writeData;
	if(GET_MANDATORY_STRING_PARAMETER(KData(), aSection, writeData))
		{
		
		INFO_PRINTF2(_L("Writing following data: %S"), &writeData);
		
		HBufC8*	writeBuf = NULL;
		TRAPD (err, writeBuf = HBufC8::NewL( writeData.Length() ));
		if (err == KErrNone)
			{
			TPtr8 writeBufPtr = writeBuf->Des();
			writeBufPtr.Copy(writeData);
			
			TInt64 pos = 0;
			GET_OPTIONAL_INT64_PARAMETER(KPosition(), aSection, pos);
			
			err = iRawDisk->Write(pos, writeBufPtr);
			
			if(err == KErrNone)
				{
				INFO_PRINTF1(_L("Writing data directly to disc was successful!"));
				}
			else
				{
				ERR_PRINTF2(_L("Writing data directly to disc failed with error %d"), err);
				SetError(err);
				}
			delete writeBuf;
			writeBuf = NULL;
			}
		}
    else
    	{
    	ERR_PRINTF2(_L("No %S"), &KData());
    	SetBlockResult(EFail);
    	}
    }


TBool CT_RawDiskData::GetDriveNumber(const TDesC& aSection, const TDesC& aParameterName, TDriveNumber& aDriveNumber)
/** Reads drive number from INI-file */
    {
	// Read drive number from INI file
	TPtrC driveNumberStr;
	TBool ret = GET_OPTIONAL_STRING_PARAMETER(aParameterName, aSection, driveNumberStr);
	if (ret)
		{
		if (driveNumberStr == KDriveA)
			{
			aDriveNumber = EDriveA;
			}
		else if (driveNumberStr == KDriveB)
			{
			aDriveNumber = EDriveB;
			}
		else if (driveNumberStr == KDriveC)
			{
			aDriveNumber = EDriveC;
			}
		else if (driveNumberStr == KDriveD)
			{
			aDriveNumber = EDriveD;
			}
		else if (driveNumberStr == KDriveE)
			{
			aDriveNumber = EDriveE;
			}
		else if (driveNumberStr == KDriveF)
			{
			aDriveNumber = EDriveF;
			}
		else if (driveNumberStr == KDriveG)
			{
			aDriveNumber = EDriveG;
			}
		else if (driveNumberStr == KDriveH)
			{
			aDriveNumber = EDriveH;
			}
		else if (driveNumberStr == KDriveI)
			{
			aDriveNumber = EDriveI;
			}
		else if (driveNumberStr == KDriveJ)
			{
			aDriveNumber = EDriveJ;
			}
		else if (driveNumberStr == KDriveK)
			{
			aDriveNumber = EDriveK;
			}
		else if (driveNumberStr == KDriveL)
			{
			aDriveNumber = EDriveL;
			}
		else if (driveNumberStr == KDriveM)
			{
			aDriveNumber = EDriveM;
			}
		else if (driveNumberStr == KDriveN)
			{
			aDriveNumber = EDriveN;
			}
		else if (driveNumberStr == KDriveO)
			{
			aDriveNumber = EDriveO;
			}
		else if (driveNumberStr == KDriveP)
			{
			aDriveNumber = EDriveP;
			}
		else if (driveNumberStr == KDriveQ)
			{
			aDriveNumber = EDriveQ;
			}
		else if (driveNumberStr == KDriveR)
			{
			aDriveNumber = EDriveR;
			}
		else if (driveNumberStr == KDriveS)
			{
			aDriveNumber = EDriveS;
			}
		else if (driveNumberStr == KDriveT)
			{
			aDriveNumber = EDriveT;
			}
		else if (driveNumberStr == KDriveU)
			{
			aDriveNumber = EDriveU;
			}
		else if (driveNumberStr == KDriveV)
			{
			aDriveNumber = EDriveV;
			}
		else if (driveNumberStr == KDriveW)
			{
			aDriveNumber = EDriveW;
			}
		else if (driveNumberStr == KDriveX)
			{
			aDriveNumber = EDriveX;
			}
		else if (driveNumberStr == KDriveY)
			{
			aDriveNumber = EDriveY;
			}
		else if (driveNumberStr == KDriveZ)
			{
			aDriveNumber = EDriveZ;
			}
		else
			{
			TInt driveNumber = 0;
			ret = GET_OPTIONAL_INT_PARAMETER(aParameterName, aSection, driveNumber);
			if (ret)
				{
				aDriveNumber = (TDriveNumber) driveNumber ;
				}
			}
		}

	return ret;
	}
