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

This contains CT_FileData
*/

#if (!defined __T_FILE_DATA_H__)
#define __T_FILE_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "T_FsData.h"
#include "T_FileActiveCallback.h"

//	EPOC includes
#include <f32file.h>

class CT_FileData: public CDataWrapperBase
	{
public:
	static CT_FileData*	NewL();
	~CT_FileData();

	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TAny*	GetObject();

protected:
	CT_FileData();
	void ConstructL();

private:
	//	Commands
	void	DoCmdNewL();
	void	DoCmdDestructor();
	void	DoCmdCreateL(const TDesC& aSection);
	void	DoCmdOpenL(const TDesC& aSection);
	void	DoCmdWriteL(const TDesC& aSection, const TInt aSyncErrorIndex);
	void	DoCmdClose();
	void 	DoCmdReplaceL(const TDesC& aSection);
	void	DoCmdReadL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	void 	DoCmdFlushL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	void 	DoCmdTempL(const TDesC& aSection);
	void 	DoCmdRenameL(const TDesC& aSection);
	void	DoCmdSeekL(const TDesC& aSection);
	void	DoCmdReadCancelL(const TDesC& aSection);
	void 	DoCmdLockL(const TDesC& aSection);
	void 	DoCmdUnLockL(const TDesC& aSection);
	void	DoCmdSizeL(const TDesC& aSection);
	void	DoCmdSetSizeL(const TDesC& aSection);
	void	DoCmdAttL(const TDesC& aSection);
	void	DoCmdSetAttL(const TDesC& aSection);
	void 	DoCmdModifiedL(const TDesC& aSection);
	void 	DoCmdSetModifiedL(const TDesC& aSection);
	void 	DoCmdSetL(const TDesC& aSection);
	void 	DoCmdChangeModeL(const TDesC& aSection);
	void	DoCmdDriveL(const TDesC& aSection);
	void	DoCmdBlockMap(const TDesC& aSection);
	void 	DoCmdDuplicateL(const TDesC& aSection);
	void	DoCmdFullName(const TDesC& aSection);
	void 	DoCmdNameL(const TDesC& aSection);
		
	

	//	Helpers
	inline void	DoCleanup();
	
	TBool GetFileModeL(const TDesC& aParameterName, const TDesC& aSection, TUint& aFileMode);
	TBool ConvertToSeek(const TDesC& aParameterName, const TDesC& aSection, TSeek& aSeek);
	TBool ConvertToOwnerType(const TDesC& aParameterName, const TDesC& aSection, TOwnerType& aResult);
	TBool ConvertToAttributeL(const TDesC& aParameterName, const TDesC& aSection, TUint& aAttribute);
    TBool ConvertToBlockMapUsage(const TDesC& aParameterName, const TDesC& aSection, TBlockMapUsage& aResult);
	
	void RunL(CActive* aActive, const TInt aIndex); 
	void DoCancel(CActive* aActive, TInt aIndex);
	
	void ReadCancelAll();	
	void ReadCancel();
	void DoAsynchronousWritesL(const TDesC& aSection, const TInt aSyncErrorIndex);
	void DoSynchronousWritesL(const TDesC& aSection);
	void HandleExpectedString(const TDesC8& aReadedData, const TDesC& aSection);
	void HandleExpectedString(const TDesC& aReadedData, const TDesC& aSection);	
	void DoSynchronousReadsL(const TDesC& aSection);
	void DoAsynchronousReadsL(const TDesC& aSection, const TInt aAsyncErrorIndex);	
	
	void PrintFileAttributes(TUint aAttValue);
	
	void ToArrayL(RPointerArray<HBufC>& aArray, HBufC*& aBuffer);
	RPointerArray<HBufC> SplitL(const TDesC& aInput, const char* aToken);


private:
	/** Instance for handling to resource file */
	RPointerArray<CT_FileActiveCallback> iReadCallbackArray;
	RPointerArray<CActiveCallback> iWriteCallbackArray;
	RPointerArray<CActiveCallback> iFlushCallbackArray;
	RFile*		iFile;
	TBool 		iFileOpened;
	};

#endif /* __T_FILE_DATA_H__ */
