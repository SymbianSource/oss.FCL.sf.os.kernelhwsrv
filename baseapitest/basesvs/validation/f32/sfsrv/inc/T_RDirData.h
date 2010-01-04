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

This contains CT_RDirData
*/

#if (!defined __T_RDIR_DATA_H__)
#define __T_RDIR_DATA_H__

//	User Includes
#include "DataWrapperBase.h"

//	EPOC includes
#include <f32file.h>

class CT_RDirData: public CDataWrapperBase
	{
public:
	static CT_RDirData*	NewL();
	~CT_RDirData();

	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TAny*	GetObject();
	
protected:
	CT_RDirData();
	void ConstructL();
	
	void RunL(CActive* aActive, TInt aIndex);
	void DoCancel(CActive* aActive, TInt aIndex);
	
private:
	inline void	DoCmdNew();
	inline void	DoCmdClose();
	inline void	DoCmdOpenL(const TDesC& aSection);
	inline void	DoCmdReadL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	inline void DoCmdDestructor();
	void ReadExpectedNamesL( const TDesC& aSection );
	
//Helper function
	void 	DoCleanup();
	void 	CompareEntryArray(TEntryArray* aEntryArray);
	void	CompareEntryData(TEntry* aEntry);

private:
	//** Instance for handling to resource directory */
	RDir*						iRDir;
	
	/** The request status for read event */
	CActiveCallback*			iRead;	
	
	/** The request status for read array event */
	CActiveCallback*			iReadBlock;
	
	/** entry for async format */
	TPckg<TEntry>*				iEntry;
	
	/** Array of expected file names*/
	RPointerArray<TPath>*		iExpectedNames;
	
		
	TBuf<KMaxPath>				iObjName;
	
	TBool						iCompare;
	
	};
	
#endif /* __T_RDIR_DATA_H__ */
