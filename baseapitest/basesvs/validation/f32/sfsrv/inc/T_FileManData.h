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

This contains CT_FileManData
*/

#if (!defined __T_FILEMAN_DATA_H__)
#define __T_FILEMAN_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "FileserverUtil.h"

//	EPOC includes
#include <e32std.h>
#include <f32file.h>
#include <f32fsys.h>

struct THistoryData
	{
	TFileName 			iCurrentSource;
	TFileName 			iCurrentTarget;
	TEntry 				iCurrentEntry;
	TInt 				iBytesTransferred;
	CFileMan::TAction	iCurrentAction;
	TFileManError		iMoreInfoAboutError;
	TInt 				iLastError;
	TPtrC 				iFullPath;
	TPtrC				iAbbreviatedPath;
	};
enum TObserverNotifyType
	{
	ENotifyStarted,
	ENotifyOperation,
	ENotifyEnded,
	ENotifyUnknown
	};

class CT_FileManData: public CDataWrapperBase, public MFileManObserver
	{
public:
	static CT_FileManData*	NewL();
	~CT_FileManData();

	virtual TBool	DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TAny*	GetObject();
	
protected:
	CT_FileManData();
	void ConstructL();
	
	void RunL(CActive* aActive, TInt aIndex);
	void DoCancel(CActive* aActive, TInt aIndex);
	
private:
	inline void	DoCmdNewL(const TDesC& aSection);
	inline void DoCmdAttribsL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	inline void DoCmdCopyL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	inline void DoCmdDeleteL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	inline void DoCmdMoveL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	inline void DoCmdRenameL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	inline void DoCmdRmDirL(const TDesC& aSection, const TInt aAsyncErrorIndex);
	inline void DoCmdSetObserver();
	
	inline void DoCmdCurrentAction(const TDesC& aSection);	
	inline void DoCmdGetCurrentTarget(const TDesC& aSection);
	inline void DoCmdGetCurrentSource(const TDesC& aSection);
	inline void DoCmdBytesTransferredByCopyStep(const TDesC& aSection);
	inline void DoCmdCurrentEntryL(const TDesC& aSection);
	inline void DoCmdAbbreviatedPath(const TDesC& aSection);
	inline void DoCmdFullPath(const TDesC& aSection);
	inline void DoCmdGetLastError(const TDesC& aSection);
	inline void DoCmdGetMoreInfoAboutError(const TDesC& aSection);
	inline void	DoCleanup();
		
//  MFileManObserver events
	inline TControl NotifyFileManStarted();
	inline TControl NotifyFileManOperation();
	inline TControl NotifyFileManEnded();

// Helper functions for MFileManObserver Testing
	void 	ReadTControl(const TDesC& aSection);
	TBool	GetTControlFromConfig(const TDesC& aParameterName, const TDesC& aSection, TControl& aFlag);
		
//  Helper function
	TBool	GetActionFromConfig(const TDesC& aParameterName, const TDesC& aSection, CFileMan::TAction& aAction);
	TBool	GetFileManErrorFromConfig(const TDesC& aParameterName, const TDesC& aSection, TFileManError& aError);
	void 	ConvertFileManErrorToString(TFileManError& aError, TPtrC& aErrorStr);
	void	ConvertActionToString(CFileMan::TAction aAction, TPtrC& aActionStr);
	TBool	GetOperationFromConfig(const TDesC& aParameterName, const TDesC& aSection, TUint& aSwitch);
	void 	ClearHistory();
	void	CreateHistoryRecord(THistoryData& aRecord);
	TBool 	GetNotifyType(const TDesC& aParameterName, const TDesC& aSection, TObserverNotifyType& aType);
	RPointerArray<THistoryData>* GetHistoryDataByType(const TDesC& aSection);
	
private:
	//** CFileMan class instance that is tested */
	CFileMan*			                iFileMan;
	
	/** Sores aAsyncErrorIndex (Only For MFileObserver notifications). */
	TInt								iAsyncErrorIndex;
	
	/** Indicates if functions called asynchronouslly (Only For MFileObserver notifications). */
	TBool								iAsyncCall;
	
	/** MFileManObserver class instance */
	MFileManObserver*	                iFileManObserver;
	
    /** The request status for disk space events */
	RPointerArray<CActiveCallback>		iAttribs;
	
	/** The request status for disk space events */
	RPointerArray<CActiveCallback>		iCopy;
	
	/** The request status for disk space events */
	RPointerArray<CActiveCallback>		iDelete;
	
	/** The request status for disk space events */
	RPointerArray<CActiveCallback>		iMove;
	
	/** The request status for disk space events */
	RPointerArray<CActiveCallback>		iRename;
	
	/** The request status for disk space events */
	RPointerArray<CActiveCallback>		iRmDir;
	
	/** Stores history for NotifyFileManStarted  */
	RPointerArray<THistoryData>			iStartedHistory;
	
	/** Stores history for NotifyFileManOperation  */
	RPointerArray<THistoryData>			iOperationHistory;
	
	/** Stores history for NotifyFileManEnded  */
	RPointerArray<THistoryData>			iEndedHistory;
	
	/** Stores return value for MFileManObserver Notifications */
	TControl							iTControl;
	
	/** Stores source of filename for which return specified TControl value. */
	TFileName							iNotifyFileName;
	
	/** Stores name of Notify Function. */
	TObserverNotifyType					iObserverNotifyType;
	
	/** Idicates if we need to use specified TControl value in notifications. */
	TBool								iUseTControl;
	
	};
	
#endif /* __T_FILEMAN_DATA_H__ */
