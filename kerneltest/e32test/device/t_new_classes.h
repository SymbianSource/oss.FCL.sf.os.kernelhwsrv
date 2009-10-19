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

#ifndef T_NEW_CLASSES
#define T_NEW_CLASSES

#include <e32cmn.h>

/**
A flag to record what part of lifecylce an object is in
*/
enum TObjState
	{
	ENull = 0,
	EConstructed = 1,
	EDeconstructed = 2
	};

class XCtorAndDtor
	{
public:
	XCtorAndDtor();
	~XCtorAndDtor();
	TObjState iState;
	};

class XCtorOnly
	{
public:
	XCtorOnly();
	TObjState iState;
	};

class XDtorOnly
	{
public:
	~XDtorOnly();
	TObjState iState;
	};

class XNoTors
	{
public:
	TObjState iState;
	};

//A buffer length for an object so large that we always
//expect allocation to fail.
const TInt KOOMBufferLength=10000000;

class XVeryLargeClassCtorAndDtor
	{
public:
	XVeryLargeClassCtorAndDtor();
	~XVeryLargeClassCtorAndDtor();
	TInt iBust[KOOMBufferLength];
	};

class XVeryLargeClassCtorOnly
	{
public:
	XVeryLargeClassCtorOnly();
	TInt iBust[KOOMBufferLength];
	};

class XVeryLargeClassDtorOnly
	{
public:
	~XVeryLargeClassDtorOnly();
	TInt iBust[KOOMBufferLength];
	};

class XVeryLargeClassNoTors
	{
public:
	TInt iBust[KOOMBufferLength];
	};

#endif //T_NEW_CLASSES
