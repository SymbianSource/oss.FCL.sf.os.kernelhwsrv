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

This contains CT_DirScanData
*/

#if (!defined __T_DIRSCAN_DATA_H__)
#define __T_DIRSCAN_DATA_H__

//	User Includes
#include "DataWrapperBase.h"

//	EPOC includes
#include <f32file.h>

class CT_DirScanData: public CDataWrapperBase
	{
public:
	static CT_DirScanData*	NewL();
	~CT_DirScanData();

	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TAny*	GetObject();
	
protected:
	CT_DirScanData();
	void ConstructL();
	
private:
	inline void	DoCmdNewL(const TDesC& aSection);
	inline void	DoCmdNewLC(const TDesC& aSection);
	inline void	DoCleanup();
	inline void	DoCmdSetScanDataL(const TDesC& aSection);
	inline void	DoCmdNextL(const TDesC& aSection);
	inline void	DoCmdAbbreviatedPath(const TDesC& aSection);	
	inline void	DoCmdFullPath(const TDesC& aSection);

//Helpers
	TBool		ConvertToSortKey(const TDesC& aSortKeyStr, TUint& aSortKey);
	TBool 		ConvertToScanDirection(const TDesC& aScanDirectionStr, CDirScan::TScanDirection& aScanDirection);

private:
	//** CDirScan class instance that is tested */
	CDirScan*				iDirScan;

	};
	
#endif /* __T_DIRSCAN_DATA_H__ */
