// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\template_variant\replacement_utils\replacement_utils.h
// This header file controls which of the generic utility functions are replaced
// by the variant.  The replacement implmenetations are supplied in the files
// kernel.cia and common.cia in this directory.
// The replacement functions are selected by a series of macros as described
// below:
// Macro:						Functions replaced:		Location of replacement:
// USE_REPLACEMENT_MEMSET		memclr					common.cia
// memset
// USE_REPLACEMENT_MEMCPY		wordmove				common.cia
// memmove
// memcpy
// USE_REPLACEMENT_UMEMGET		kumemget32				kernel.cia
// umemget32
// kumemget
// umemget
// USE_REPLACEMENT_UMEMPUT		kumemput32				kernel.cia
// umemput32
// kumemput
// umemput
// 
//

#ifndef __REPLACEMENT_UTILS_H__

//#define USE_REPLACEMENT_MEMSET
//#define USE_REPLACEMENT_MEMCPY
//#define USE_REPLACEMENT_UMEMGET
//#define USE_REPLACEMENT_UMEMPUT

#endif
