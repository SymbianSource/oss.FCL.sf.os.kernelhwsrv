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



#ifndef __T_FIND_FILE_DATA_H__
#define __T_FIND_FILE_DATA_H__

//	User Includes
#include "DataWrapperBase.h"
#include "T_FsData.h"

//	EPOC includes
#include <f32file.h>
#include <e32std.h>
#include <f32fsys.h>


class CT_FindFileData: public CDataWrapperBase
	{
public:
	static CT_FindFileData* NewL();
	~CT_FindFileData();

	virtual TBool DoCommandL( const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt /*aAsyncErrorIndex*/  );
	virtual TAny* GetObject();

protected:
	CT_FindFileData();
	void ConstructL();

private:
	inline void DoCleanup();
	inline void DoCmdNew( const TDesC& aEntry );
    inline void DoCmdFile( const TDesC& aEntry );
    inline void DoCmdFind( const TDesC& aEntry );
    inline void DoCmdFindByDir( const TDesC& aEntry );
    inline void DoCmdFindByPath( const TDesC& aEntry );
    inline void DoCmdFindWildL( const TDesC& aEntry );
    inline void DoCmdFindWildByDirL( const TDesC& aEntry );
    inline void DoCmdFindWildByPathL( const TDesC& aEntry );
    inline void DoCmdSetFindMask( const TDesC& aEntry );
    //Utility functions
    TBool		ConvertToDriveAtts(const TDesC& aMediaAttStr, TUint& aMediaAtt);
	TBool		GetDriveAttsFromConfig(const TDesC& aSection, TUint& aDriveAtt);


private:
	//	TFindFile class instance that is tested
	TFindFile*			iFindFile;
	
	//	Pointer to the path list
	TPtrC				iPathList;
	};

#endif // __T_FIND_FILE_ARRAY_DATA_H__

