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
* Default messages for the Spanish language
*
*/


#include "ls_std.h"

const TText * const LMessages::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("Reintentar"),					                       // Button 1
	_S("Detener"),					        	       // Button 2
	_S("Vuelva a introducir el disco"),			                       // Put the card back - line1
	_S("Inserte el disco o se perder\341n los datos"),		                       // Put the card back - line2
	_S("Nivel de pilas principales demasiado bajo"),			                       // Low power - line1
	_S("Imposible terminar escritura en el disco"),		                           // Low power - line2
	_S("Error de disco - imposible terminar escritura"),	                       // Disk error - line1
	_S("Reint\351ntelo o se perder\341n los datos"),	      	                   // Disk error - line2
// SoundDriver
	_S("Carill\363n"),							// Chimes
	_S("Rings"),								// Rings
	_S("Se\361al"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S("Interno"),								// Internal
	_S("Externo(01)"),							// External(01)
	_S("Externo(02)"),							// External(02)
	_S("Externo(03)"),							// External(03)
	_S("Externo(04)"),							// External(04)
	_S("Externo(05)"),							// External(05)
	_S("Externo(06)"),							// External(06)
	_S("Externo(07)"),							// External(07)
	_S("Externo(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S("Z\363calo(01)"),						// Socket(01)
	_S("Z\363calo(02)"),						// Socket(02)
	_S("Z\363calo(03)"),						// Socket(03)
	_S("Z\363calo(04)")							// Socket(04)
	};
