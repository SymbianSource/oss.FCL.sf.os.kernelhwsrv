// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Description: Common header for version 2 plugins
//

#include <f32plugin.h>
#include <e32test.h>

const TInt KPluginSetDrive			= -112233;
const TInt KPluginGetError			= -112234;
const TInt KPluginSetRemovable		= -112235;
const TInt KPluginToggleIntercepts	= -112236;
const TInt KPluginSetDirFullName	= -112237;

//This is some stupid thing for making strings wide.
//We're using this for printing out the filename.
#define ExpandMe(X)	 L ## X
#define Expand(X)	 ExpandMe(X)



