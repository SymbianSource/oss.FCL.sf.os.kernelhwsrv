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
* Default messages for the Italian language
*
*/


#include "ls_std.h"

const TText * const LMessages::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("Riprova"),					                       // Button 1
	_S("Stop"),					        	       // Button 2
	_S("Reinserisci il disco"),			                       // Put the card back - line1
	_S("Oppure perderai le informazioni"),			               // Put the card back - line2
	_S("Batterie troppo basse"),			                       // Low power - line1
	_S("Impossibile completare scrittura sul disco"),		       // Low power - line2
	_S("Errore disco: impossibile completare la scrittura"),	       // Disk error - line1
	_S("Riprova o perderai le informazioni"),		               // Disk error - line2
// SoundDriver
	_S("Carillon"),								// Chimes
	_S("Squilli"),								// Rings
	_S("Segnale"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S("Interno"),								// Internal
	_S("Esterno(01)"),							// External(01)
	_S("Esterno(02)"),							// External(02)
	_S("Esterno(03)"),							// External(03)
	_S("Esterno(04)"),							// External(04)
	_S("Esterno(05)"),							// External(05)
	_S("Esterno(06)"),							// External(06)
	_S("Esterno(07)"),							// External(07)
	_S("Esterno(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S("Presa(01)"),							// Socket(01)
	_S("Presa(02)"),							// Socket(02)
	_S("Presa(03)"),							// Socket(03)
	_S("Presa(04)")							// Socket(04)
	};
