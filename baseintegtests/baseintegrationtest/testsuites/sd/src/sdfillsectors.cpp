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
// Obliterate first x sectors of a memory card (starts from sector 0 - includes MBR)
// 
//

#include "sdfillsectors.h"

/*
Class constructor

@param None
@return None
*/
CBaseTestSDFillSectors::CBaseTestSDFillSectors()
	{
	SetTestStepName(KTestStepFillSectors);
	}

/*
Test Step Preamble
 - Load device driver for direct disk access

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDFillSectors::doTestStepPreambleL()
	{
	SetTestStepResult(EFail);
	
	if (!InitDeviceDriver())
		return TestStepResult();
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

/*
Test step

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDFillSectors::doTestStepL()
	{
	if (TestStepResult() == EPass)
		{
		TInt start;
		TInt end;
		TInt value;
		TInt r;
		
		_LIT(KFillSectorsStart, "FillSectorsStart");
		_LIT(KFillSectorsEnd, "FillSectorsEnd");
		_LIT(KFillSectorsHexValue, "FillSectorsHexValue");
		if (!GetIntFromConfig(ConfigSection(), KFillSectorsStart, start))
			{
			ERR_PRINTF1(_L("INI file read error"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		
		if (!GetIntFromConfig(ConfigSection(), KFillSectorsEnd, end))
			{
			ERR_PRINTF1(_L("INI file read error"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		if (!GetHexFromConfig(ConfigSection(), KFillSectorsHexValue, value))
			{
			ERR_PRINTF1(_L("INI file read error"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		
		TBuf8<255> sector;
		sector.Fill((TUint8) value);		
		INFO_PRINTF6(_L("Filling sectors %08xh (%d) to %08xh (%d) with %02xh"), start, start, end, end, (TUint8) value);
		for (TInt i = start; i <= end; i++)
			{
			r = WriteSector(i, sector);
			if (r != KErrNone)
				{
				SetTestStepResult(EFail);
				return TestStepResult();
				}
			}
		}
	else
		{
		INFO_PRINTF1(_L("Test preamble did not complete succesfully - Test Step skipped"));
		}
	return TestStepResult();
	}
