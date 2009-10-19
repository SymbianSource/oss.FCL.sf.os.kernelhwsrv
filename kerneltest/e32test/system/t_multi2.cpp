// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32test\system\t_multi2.cpp
// 
//

#include "t_multin.h"

MProducer* TVirProducer::Producer()
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirProducer::Producer\n"));
	test.Getch();
#endif
	return(this);
	}

