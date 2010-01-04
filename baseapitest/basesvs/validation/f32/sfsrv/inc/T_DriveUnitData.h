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

#if (!defined __T_DRIVEUNIT_DATA_H__)
#define __T_DRIVEUNIT_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "T_FsData.h"
//	EPOC includes
#include <f32file.h>


class CT_DriveUnitData : public CDataWrapperBase
	{
public:
	static CT_DriveUnitData*	NewL();
	~CT_DriveUnitData();

	virtual TAny*	GetObject();
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);

protected:
	CT_DriveUnitData();
	void	ConstructL();

private:
	//	Commands
	void	DoCmdNewL(const TDesC& aSection);
	void	DoCmdDestructor();
	void 	DoCmdAssign(const TDesC& aSection);
	void 	DoCmdConvertToInt(const TDesC& aSection);
	void 	DoCmdName(const TDesC& aSection);

	//	Helpers
	void	DoCleanup();

private:

	/** Instance for handling to resource file */
	TDriveUnit*		iDriveUnit;

	};

#endif /* __T_DRIVEUNIT_DATA_H__ */
