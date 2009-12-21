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



#ifndef __T_PARSE_PTR_C_DATA_H__
#define __T_PARSE_PTR_C_DATA_H__

//	User Includes
#include "T_ParseBaseData.h"

//	EPOC includes
#include <f32file.h>
#include <e32std.h>
#include <f32fsys.h>

class CT_ParsePtrCData: public CT_ParseBaseData
	{
public:
	static CT_ParsePtrCData* NewL();
	~CT_ParsePtrCData();
	
	virtual TParseBase* GetParse();
	virtual TBool DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);

protected:
	CT_ParsePtrCData( );
	void ConstructL();

private:	// command processors
    inline void DoCleanup();
    inline void DoCmdNew( const TDesC& aSection );
    inline void DoCmdAddDir( const TDesC& aSection );
    inline void DoCmdPopDir();
	
private:
	
	//	TParsePtrC class instance that is tested
	TParsePtrC*				iParsePtrC;
	
	// Buffer of the filename to be parsed
	TBuf<KMaxFileName*2>	iFileName;
	};

#endif // __T_PARSE_PTR_C_DATA_H__

