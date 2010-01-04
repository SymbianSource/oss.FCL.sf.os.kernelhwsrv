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
* Default messages for the French language
*
*/


#include "ls_std.h"

const TText * const LMessages::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("Recommencer"),					                       // Button 1
	_S("Arr\352ter"),						        	       // Button 2
	_S("R\351ins\351rez le disque"),					       // Put the card back - line1
	_S("Sinon des donn\351es vont \352tre perdues"),		   // Put the card back - line2
	_S("Piles trop faibles"),			                       // Low power - line1
	_S("Impossible de terminer l'\351criture sur disque"),	   // Low power - line2
	_S("Erreur de disque : impossible de terminer l'\351criture"),	  // Disk error - line1
	_S("Recommencez pour \351viter de perdre des donn\351es"),		  // Disk error - line2
// SoundDriver
	_S("Carillon"),								// Chimes
	_S("Sonnerie"),								// Rings
	_S("Signal"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S("Interne"),								// Internal
	_S("Externe(01)"),							// External(01)
	_S("Externe(02)"),							// External(02)
	_S("Externe(03)"),							// External(03)
	_S("Externe(04)"),							// External(04)
	_S("Externe(05)"),							// External(05)
	_S("Externe(06)"),							// External(06)
	_S("Externe(07)"),							// External(07)
	_S("Externe(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S("Connexion(01)"),							// Socket(01)
	_S("Connexion(02)"),							// Socket(02)
	_S("Connexion(03)"),							// Socket(03)
	_S("Connexion(04)")							// Socket(04)
	};
