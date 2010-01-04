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

This contains CT_FormatData
*/

#if (!defined __T_FORMAT_DATA_H__)
#define __T_FORMAT_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "T_FsData.h"
//	EPOC includes
#include <f32file.h>


class CT_FormatData : public CDataWrapperBase
	{
public:
	static CT_FormatData*	NewL();
	~CT_FormatData();

	virtual TAny*	GetObject();
	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);

protected:
	CT_FormatData();
	void	ConstructL();

private:
	//	Commands
	void	DoCmdNewL();
	void	DoCmdDestructor();
	void	DoCmdOpenL(const TDesC& aSection);
	void	DoCmdClose();
	void	DoCmdNext(const TDesC& aSection, const TInt aAsyncErrorIndex);

	//	Helpers
	void	DoCleanup();
	TBool	GetFormatMode(const TDesC& aParameterName, const TDesC& aSection, TUint& aFormatMode);
	TBool	ConvertToFormatMode(const TDesC& aFormatModeStr, TUint& aFormatMode);

	//	MActiveCallback implementation
	virtual void	RunL(CActive* aActive, TInt aIndex);
	virtual void	DoCancel(CActive* aActive, TInt aIndex);

private:
	/** Instance for handling to resource file */
	RFormat*			iFormat;
	/** The request status for files/dir events */
	CActiveCallback*	iNext;
	/** Outstanding async call counts */

	/** count for async format */
	TPckgBuf<TInt>		iCountPckg;

	/** count for sync format */
	TInt				iCount;

	/** count for sync format */
	TInt				iCountNextEnd;
	};

#endif /* __T_FORMAT_DATA_H__ */
