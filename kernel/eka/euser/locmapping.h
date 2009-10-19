// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __LOCMAPPING_H__
#define __LOCMAPPING_H__

#include <e32std.h>

struct TOldLocaleNameToNewLocaleName
	{
	TInt16 iOldLocaleId;
	TInt16 iNewLocaleID[3]; //Lan, Reg, Col
	};

const TInt KLocMapLength = 208;
extern const TOldLocaleNameToNewLocaleName LocaleMapping[KLocMapLength];

#endif /* LOC_MAPPING_H_ */
