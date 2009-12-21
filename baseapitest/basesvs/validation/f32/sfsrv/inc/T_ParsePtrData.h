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



#ifndef __T_PARSE_PTR_DATA_H__
#define __T_PARSE_PTR_DATA_H__

//	User Includes
#include "T_ParseBaseData.h"

//	EPOC includes
#include <f32file.h>
#include <e32std.h>
#include <f32fsys.h>

class CT_ParsePtrData: public CT_ParseBaseData
	{
public:
	static CT_ParsePtrData* NewL();
	~CT_ParsePtrData();

	virtual TParseBase* GetParse();
	virtual TBool DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);
	virtual TAny* GetObject();

protected:
	CT_ParsePtrData();
	void ConstructL();

private:	// command processors
    inline void DoCleanup();
    inline void DoCmdNew( const TDesC& aSection );
    inline void DoCmdAddDir( const TDesC& aSection );
    inline void DoCmdPopDir();
    
	
private:
	
	//	TParsePtr class instance that is tested
	TParsePtr*			iParsePtr;

	// Buffer of the filename to be parsed
	TBuf<KMaxFileName>	iFileName;
	};

#endif // __T_PARSE_PTR_DATA_H__

