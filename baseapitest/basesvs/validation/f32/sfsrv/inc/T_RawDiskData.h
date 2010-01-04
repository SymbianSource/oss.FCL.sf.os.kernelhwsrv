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

#if (!defined __T_RAWDISK_DATA_H__)
#define __T_RAWDISK_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "T_FsData.h"

//	EPOC includes
#include <f32file.h>

class CT_RawDiskData : public CDataWrapperBase
	{
public:
	static CT_RawDiskData*	NewL();
	~CT_RawDiskData();

	virtual TAny*	GetObject();
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);


protected:
	CT_RawDiskData();
	void ConstructL();

private:
	//	Commands
	void	DoCmdNewL();
	void	DoCmdOpen(const TDesC& aSection);
	void	DoCmdClose();
	void 	DoCmdReadL(const TDesC& aSection);
	void 	DoCmdWriteL(const TDesC& aSection);
	void	DoCmdDestructor();


	//	Helpers
	inline void	DoCleanup();
    TBool GetDriveNumber(const TDesC& aSection, const TDesC& aParameterName, TDriveNumber& aDriveNumber);

private:

	/** Instance for handling to resource file */
    RRawDisk*       iRawDisk;

	};

#endif /* __T_RAWDISK_DATA_H__ */
