/*
* Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Default messages for the English language (UK & US)
*
*/


#include "ls_std.h"

const TText * const LMessages::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("Retry"),								// Button 1
	_S("Stop"),									// Button 2
	_S("Put the disk back"),					// Put the card back - line1
	_S("or data will be lost"),					// Put the card back - line2
	_S("Batteries too low"),					// Low power - line1
	_S("Cannot complete write to disk"),		// Low power - line2
	_S("Disk error - cannot complete write"),	// Disk error - line1
	_S("Retry or data will be lost"),			// Disk error - line2
// SoundDriver
	_S("Chimes"),								// Chimes
	_S("Rings"),								// Rings
	_S("Signal"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S("Internal"),								// Internal
	_S("External(01)"),							// External(01)
	_S("External(02)"),							// External(02)
	_S("External(03)"),							// External(03)
	_S("External(04)"),							// External(04)
	_S("External(05)"),							// External(05)
	_S("External(06)"),							// External(06)
	_S("External(07)"),							// External(07)
	_S("External(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S("Socket(01)"),							// Socket(01)
	_S("Socket(02)"),							// Socket(02)
	_S("Socket(03)"),							// Socket(03)
	_S("Socket(04)")							// Socket(04)
	};
