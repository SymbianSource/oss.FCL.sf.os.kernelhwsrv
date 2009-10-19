// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include "t_new_classes.h"

XCtorAndDtor::XCtorAndDtor()
	{
	iState = EConstructed;
	}
XCtorAndDtor::~XCtorAndDtor()
	{
	iState = EDeconstructed;
	}


XCtorOnly::XCtorOnly()
	{
	iState = EConstructed;
	}


XDtorOnly::~XDtorOnly()
	{
	iState = EDeconstructed;
	}


XVeryLargeClassCtorAndDtor::XVeryLargeClassCtorAndDtor()
	{
	iBust[0] = EConstructed;
	}
XVeryLargeClassCtorAndDtor::~XVeryLargeClassCtorAndDtor()
	{
	iBust[0] = EDeconstructed;
	}


XVeryLargeClassCtorOnly::XVeryLargeClassCtorOnly()
	{
	iBust[0] = EConstructed;
	}


XVeryLargeClassDtorOnly::~XVeryLargeClassDtorOnly()
	{
	iBust[0] = EDeconstructed;
	}

