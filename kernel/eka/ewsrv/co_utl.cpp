// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\ewsrv\co_utl.cpp
// 
//

#include "ws_std.h"

EXPORT_C void Panic(TConsolePanic aPanic)
//
// Panic the client side console code
//
	{

	User::Panic(_L("Console-TWIN"),aPanic);
	}

EXPORT_C TKeyCode TConsoleKey::Code() const
//
// Return the keycode
//
	{

	return((*((TConsoleKey *)this))().iCode);
	}

EXPORT_C TInt TConsoleKey::Modifiers() const
//
// Return the key modifiers
//
	{

	return((*((TConsoleKey *)this))().iModifiers);
	}

EXPORT_C TUint8 TConsoleKey::PointerNumber() const
//
// Return the pointer number
//
	{
	return((*((TConsoleKey *)this))().iPointerNumber);
	}

EXPORT_C TInt TConsoleKey::Type() const
//
// Return the key type
//
	{

	return((*((TConsoleKey *)this))().iType);
	}

EXPORT_C TPoint TConsoleKey::MousePos() const
//
// Return the mouse position
//
	{

	return((*((TConsoleKey *)this))().iMousePos);
	}

