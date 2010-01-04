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



#ifndef __T_OPEN_FILE_SCAN_DATA_H__
#define __T_OPEN_FILE_SCAN_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "T_FsData.h"

//	EPOC includes
#include <f32file.h>
#include <e32std.h>
#include <f32fsys.h>

class CT_OpenFileScanData: public CDataWrapperBase
	{
public:
	static CT_OpenFileScanData* NewL();
	~CT_OpenFileScanData();

	virtual TBool DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/);
	virtual TAny* GetObject();

protected:
	CT_OpenFileScanData();
	void ConstructL();

private:	// command processors
    inline void DoCleanup();
    inline void DoCmdNew( const TDesC& aSection );
    inline void DoCmdNextL( const TDesC& aSection );
    inline void DoCmdThreadId( const TDesC& aSection );
    
private:	// helper functions
	TBool GetRfsParam( const TDesC& aSection, RFs*& aRfs, TPtrC& aRfsName );
	
private:
	//	TOpenFileScan class instance that is tested
	TOpenFileScan*		iOpenFileScan;
	};

#endif // __T_OPEN_FILE_SCAN_DATA_H__

