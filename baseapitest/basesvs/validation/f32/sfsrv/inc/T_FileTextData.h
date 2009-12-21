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



#ifndef __T_FILE_TEXT_DATA_H__
#define __T_FILE_TEXT_DATA_H__

//	User Includes
#include "T_FsData.h"
#include "DataWrapperBase.h"

//	EPOC includes
#include <f32file.h>
#include <e32std.h>
#include <f32fsys.h>


class CT_FileTextData: public CDataWrapperBase
	{
public:
	static CT_FileTextData* NewL();
	~CT_FileTextData();

	virtual TBool DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/ );
	virtual TAny* GetObject();

protected:
	CT_FileTextData( );
	void ConstructL();

private:
    // command processors
    inline void DoCleanup();
    inline void DoCmdNew();
    inline void DoCmdRead( const TDesC& aSection );
    inline void DoCmdSeek( const TDesC& aSection );
    inline void DoCmdSet( const TDesC& aSection );
    inline void DoCmdWrite( const TDesC& aSection );
    
    // helper functions
	TBool GetSeekMode( const TDesC& aSection, TSeek& aMode, TBool aOptional );
		
private:
		
	// TFileText class instance
	TFileText*			iFileText;
	};

#endif // __T_FILE_TEXT_DATA_H__

