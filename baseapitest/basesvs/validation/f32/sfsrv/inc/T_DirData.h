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

This contains CT_DirData
*/

#if (!defined __T_DIR_DATA_H__)
#define __T_DIR_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "T_FsData.h"

//	EPOC includes
#include <f32file.h>

class CT_DirData: public CDataWrapperBase
	{
public:
	static CT_DirData*	NewL();
	~CT_DirData();

	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TAny*	GetObject();
	virtual void	SetObjectL(TAny* aAny);
	virtual void	DisownObjectL();	
	inline virtual 	TCleanupOperation CleanupOperation();
	
protected:
	CT_DirData();
	void ConstructL();
	
private:
	void	DoCleanup();
	void	DoCmdCount(const TDesC& aSection);
	void	DoCmdOperatorBracketsL(const TDesC& aSection);
	void	DoCmdSort(const TDesC& aSection);
	
//Helper function
	static void CleanupOperation(TAny* aAny);
	TBool ConvertSortKeys(TDesC &aConstantName, TUint& aSortKey);
private:
	//** Directory entry list */
	CDir*				iDir;
	};
		
#endif /* __T_DIR_DATA_H__ */
