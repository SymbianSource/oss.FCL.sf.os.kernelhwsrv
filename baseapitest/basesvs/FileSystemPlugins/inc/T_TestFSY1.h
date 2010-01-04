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

This contains CTestFileSystem1
*/

#if (!defined __T_TEST_FSY1_H__)
#define __T_TEST_FSY1_H__

//	User includes
#include "T_TestFSY.h"

class CTestFileSystem1 : public CTestFileSystem
	{
public:
	static CFileSystem* NewL();

	TInt	Install();
	TBool	IsExtensionSupported() const;

protected:
	CTestFileSystem1();
	};

#endif /* __T_TEST_FSY2_H__ */
