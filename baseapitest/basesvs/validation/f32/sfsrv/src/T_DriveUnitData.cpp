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

This contains CT_DriveUnitData
*/

//	User includes
#include "T_DriveUnitData.h"

/*@{*/
///	Parameters
_LIT(KDriveNameType,					"driveNameType");
_LIT(KDriveName,						"driveName");
_LIT(KDriveExpValue,                    "driveExpValue");
_LIT(KDriveNameExpValue,				"driveNameExpValue");

///	Commands
_LIT(KCmdNew,							"new");
_LIT(KCmdDestructor,					"~");
_LIT(KCmdAssign,                        "=");
_LIT(KCmdConvertToInt,                  "convertToInt");
_LIT(KCmdName,					        "name");
/*@}*/

CT_DriveUnitData* CT_DriveUnitData::NewL()
/**
* Two phase constructor
*/
	{
	CT_DriveUnitData* ret = new (ELeave) CT_DriveUnitData();
	CleanupStack::PushL(ret);
	ret->ConstructL();
	CleanupStack::Pop(ret);
	return ret;
	}

CT_DriveUnitData::CT_DriveUnitData()
:	iDriveUnit(NULL)
/**
* Protected constructor. First phase construction
*/
	{
	}

void CT_DriveUnitData::ConstructL()
/**
* Protected constructor. Second phase construction
*/	
	{
	}

CT_DriveUnitData::~CT_DriveUnitData()
/**
* Destructor.
*/
	{
	DoCleanup();
	}

void CT_DriveUnitData::DoCleanup()
/**
* Contains cleanup implementation
*/
	{
	if (iDriveUnit)
		{
		INFO_PRINTF1(_L("Deleting current TDriveUnit"));
		delete iDriveUnit;
		iDriveUnit = NULL;
		}
	}

TAny* CT_DriveUnitData::GetObject()
/**
* Return a pointer to the object that the data wraps
*
* @return pointer to the object that the data wraps
*/
	{
	return iDriveUnit;
	}

TBool CT_DriveUnitData::DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/)
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
		DoCmdNewL(aSection);
		}
	else if (aCommand == KCmdDestructor)
		{
		DoCmdDestructor();
		}
	else if (aCommand == KCmdAssign)
	    {
	    DoCmdAssign(aSection);
	    }
	else if (aCommand == KCmdConvertToInt)
	    {
	    DoCmdConvertToInt(aSection);
	    }
	else if (aCommand == KCmdName)
	    {
	    DoCmdName(aSection);
	    }
	else
		{
		retVal = EFalse;
		}
	return retVal;
	}

void CT_DriveUnitData::DoCmdNewL(const TDesC& aSection)
/** Creates new TDriveUnit class instance */
	{
	//Deletes previous TDriveUnit class instance if it was already created.
	DoCleanup();

	INFO_PRINTF1(_L("Create new TDriveUnit class instance"));
    
	TPtrC driveNameType;
	if (GET_MANDATORY_STRING_PARAMETER(KDriveNameType(), aSection, driveNameType))
	    {

	    if (driveNameType == _L("number"))
	        {
	       	TInt driveNumber;
        	if (GET_MANDATORY_INT_PARAMETER(KDriveName(), aSection, driveNumber))
	            {
            	// do create
            	TRAPD(err, iDriveUnit = new (ELeave) TDriveUnit(driveNumber));
            	
            	if (err != KErrNone)
        			{
        			ERR_PRINTF2(_L("Error creating TDriveUnit(driveNumber): %d"), err);
        			SetError( err );
        			}
	            }
	        }
        else if (driveNameType == _L("letter"))
            {
            TPtrC driveLetter;
            if (GET_MANDATORY_STRING_PARAMETER(KDriveName(), aSection, driveLetter))
                {
               	// do create
            	TRAPD(err, iDriveUnit = new (ELeave) TDriveUnit(driveLetter));
                
                if (err != KErrNone)
        			{
        			ERR_PRINTF2(_L("Error creating TDriveUnit(driveLetter): %d"), err);
        			SetError( err );
        			}
                }
            }
        else
            {
            TRAPD(err, iDriveUnit = new (ELeave) TDriveUnit());

        	if (err != KErrNone)
        		{
        		ERR_PRINTF2(_L("Error creating TDriveUnit(): %d"), err);
        		SetError( err );
        		}
            }
        }
	}

void CT_DriveUnitData::DoCmdAssign(const TDesC& aSection)
/** Assigns the drive number or letter to the drive unit */
    {
	TPtrC driveNameType;
	if (GET_MANDATORY_STRING_PARAMETER(KDriveNameType(), aSection, driveNameType))
	    {
	    if (driveNameType == _L("number"))
	        {
            TInt driveNumber;
        	if (GET_MANDATORY_INT_PARAMETER(KDriveName(), aSection, driveNumber))
        	    {
        	    INFO_PRINTF1(_L("Assign a new drive number to the drive unit"));
        	    *iDriveUnit = iDriveUnit->operator=(driveNumber);
        	    
        	    }
	        }
	    else if (driveNameType == _L("letter"))
	        {
            TPtrC driveLetter;
            if (GET_MANDATORY_STRING_PARAMETER(KDriveName(), aSection, driveLetter))
                {
                INFO_PRINTF1(_L("Assign a new drive letter to the drive unit"));
                *iDriveUnit = iDriveUnit->operator=(driveLetter);
                }
	        }
        else
            {
            ERR_PRINTF1(_L("Drive name type is not specified!"));
            SetBlockResult(EFail);
            }
	    }
    }

void CT_DriveUnitData::DoCmdConvertToInt(const TDesC& aSection)
/** Converts the drive unit to integer */
    {
    INFO_PRINTF1(_L("Convert the drive unit to an integer value"));

    TInt intValue = iDriveUnit->operator TInt();
    INFO_PRINTF2(_L("Drive unit integer value is %d"), intValue);

    TInt driveExpValue;
	if (GET_OPTIONAL_INT_PARAMETER(KDriveExpValue(), aSection, driveExpValue))
	    {	    
        if ( driveExpValue != intValue )
            {
            ERR_PRINTF3(_L("Drive expected integer value does not match! Expected value: %d, actual value: %d"), driveExpValue, intValue);
            SetBlockResult(EFail);
            }
	    else
	    	{
	    	INFO_PRINTF1(_L("Drive expected integer value matches the actual value!"));
	    	}
	    }    
    }

void CT_DriveUnitData::DoCmdName(const TDesC& aSection)
/** Get the drive unit name as text */
    {
    INFO_PRINTF1(_L("Get the drive unit name as text with a colon in the end using Name()"));

    TDriveName driveName = iDriveUnit->Name();

    INFO_PRINTF2(_L("Drive name: %S"), &driveName);
    
    TPtrC driveNameExpValue;
    if (GET_OPTIONAL_STRING_PARAMETER(KDriveNameExpValue(), aSection, driveNameExpValue))
	    {	    
        if ( driveNameExpValue != driveName )
            {
            ERR_PRINTF3(_L("Drive expected name value does not match! Expected value: %S, actual value: %S"), &driveNameExpValue, &driveName);
            SetBlockResult(EFail);
            }
	    else
	    	{
	    	INFO_PRINTF1(_L("Drive expected name value matches the actual value!"));
	    	}
	    } 
    }

void CT_DriveUnitData::DoCmdDestructor()
/** Destroy TDriveUnit the object */
	{
	DoCleanup();
	}
