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



#ifndef __T_ENTRY_DATA_H__
#define __T_ENTRY_DATA_H__

//	User Includes
#include "T_FsData.h"
#include "DataWrapperBase.h"

//	EPOC includes
#include <f32file.h>
#include <e32std.h>
#include <f32fsys.h>


class CT_EntryData: public CDataWrapperBase
	{
public:
	static CT_EntryData* NewL();
	~CT_EntryData();

	virtual TBool 	DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex );
	virtual TAny* 	GetObject();
	virtual void 	SetObjectL( TAny* aAny );
	virtual void	DisownObjectL();	
	inline virtual 	TCleanupOperation CleanupOperation();
	
protected:
	CT_EntryData( );
	void ConstructL();

private:
    inline void DoCmdAssignmentOperatorL( const TDesC& aSection );
    inline void DoCmdIndexOperator( const TDesC& aSection );
    inline void DoCleanup();
    inline void DoCmdIsArchive( const TDesC& aSection );
    inline void DoCmdIsDir( const TDesC& aSection );
    inline void DoCmdIsHidden( const TDesC& aSection );
    inline void DoCmdIsReadOnly( const TDesC& aSection );
    inline void DoCmdIsSystem( const TDesC& aSection );
    inline void DoCmdIsTypeValid( const TDesC& aSection );
    inline void DoCmdIsUidPresent( const TDesC& aSection );
    inline void DoCmdMostDerived( const TDesC& aSection );
    inline void DoCmdNew( const TDesC& aSection );
    inline void DoCmdSetAttribute( const TDesC& aSection );
        
private:  
    // Helper function
    inline TBool CompareBool( TBool aResult, TBool aExpected );
	static void CleanupOperation( TAny* aAny );
 
private:
	// TEntry class instance
	TEntry*				iEntry;
	
	// RFs class instance 
	RFs*				iFs;
	};

#endif // __T_ENTRY_DATA_H__

