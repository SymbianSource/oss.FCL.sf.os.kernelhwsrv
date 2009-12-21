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



#ifndef __T_PARSE_BASE_DATA_H__
#define __T_PARSE_BASE_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "T_FsData.h"

//	EPOC includes
#include <f32file.h>
#include <e32std.h>
#include <f32fsys.h>


class CT_ParseBaseData: public CDataWrapperBase
	{
public:
	~CT_ParseBaseData();

	virtual TBool DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/);
	virtual TAny* GetObject();
	virtual TParseBase*	GetParse() = 0;

protected:

private:
	// 	command processors
    inline void DoCmdDrive( const TDesC& aSection );
    inline void DoCmdDriveAndPath( const TDesC& aSection );
    inline void DoCmdDrivePresent( const TDesC& aSection );
    inline void DoCmdExt( const TDesC& aSection );
    inline void DoCmdExtPresent( const TDesC& aSection );
    inline void DoCmdFullName( const TDesC& aSection );
    inline void DoCmdIsKMatchAny( const TDesC& aSection );
    inline void DoCmdIsKMatchOne( const TDesC& aSection );
    inline void DoCmdIsExtWild( const TDesC& aSection );
    inline void DoCmdIsNameWild( const TDesC& aSection );
    inline void DoCmdIsRoot( const TDesC& aSection );
    inline void DoCmdIsWild( const TDesC& aSection );
    inline void DoCmdName( const TDesC& aSection );
    inline void DoCmdNameAndExt( const TDesC& aSection );
    inline void DoCmdNameOrExtPresent( const TDesC& aSection );
    inline void DoCmdNamePresent( const TDesC& aSection );
    inline void DoCmdPath( const TDesC& aSection );
    inline void DoCmdPathPresent( const TDesC& aSection );
private:

	};

#endif // __T_PARSE_BASE_DATA_H__

