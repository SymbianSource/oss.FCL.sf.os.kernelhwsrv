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

This contains CT_FileNamesIdenticalData
*/

#if (!defined __T_FILENAMESIDENTICAL_DATA_H__)
#define __T_FILENAMESIDENTICAL_DATA_H__

//	User Includes
#include "DataWrapperBase.h"

//	EPOC includes
#include <f32file.h>

class CT_FileNamesIdenticalData: public CDataWrapperBase
	{
public:
	static CT_FileNamesIdenticalData*	NewL();
	~CT_FileNamesIdenticalData();

	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TAny*	GetObject();
	
protected:
	CT_FileNamesIdenticalData();
	void ConstructL();
	
private:
	inline void	DoCmdFileNamesIdentical(const TDesC& aSection);
	
private:
	};
	
#endif /* __T_FILENAMESIDENTICAL_DATA_H__ */
