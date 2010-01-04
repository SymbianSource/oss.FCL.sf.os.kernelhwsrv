/*
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* LS_MSENG.CPP
* Default messages for the English language (UK & US)
*
*/


#include "ls_std.h"

const TText * const LMessages::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("NROM Retry"),								// Button 1
	_S("NROM Stop"),									// Button 2
	_S("NROM Put the disk back"),					// Put the card back - line1
	_S("or NROM data will be lost"),					// Put the card back - line2
	_S("NROM Batteries too low"),					// Low power - line1
	_S("NROM Cannot complete write to disk"),		// Low power - line2
	_S("NROM Disk error - cannot complete write"),	// Disk error - line1
	_S("NROM Retry or data will be lost"),			// Disk error - line2
// SoundDriver
	_S("NROM Chimes"),								// Chimes
	_S("NROM Rings"),								// Rings
	_S("NROM Signal"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S("NRInternal"),								// Internal
	_S("NRExternal(01)"),							// External(01)
	_S("NRExternal(02)"),							// External(02)
	_S("NRExternal(03)"),							// External(03)
	_S("NRExternal(04)"),							// External(04)
	_S("NRExternal(05)"),							// External(05)
	_S("NRExternal(06)"),							// External(06)
	_S("NRExternal(07)"),							// External(07)
	_S("NRExternal(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S("NRSocket(01)"),							// Socket(01)
	_S("NRSocket(02)"),							// Socket(02)
	_S("NRSocket(03)"),							// Socket(03)
	_S("NRSocket(04)")							// Socket(04)
	};
