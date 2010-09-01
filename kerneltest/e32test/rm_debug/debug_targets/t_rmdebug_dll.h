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
// Description:
//

#include <e32std.h>

/**
  This is a simple function that uses an element from rm_debug_api.h.
  If the e32tests can be built and run then this is 'proof' that the
  rm_debug_api.h header file can be #include'd into a dll
  */
IMPORT_C TUid GetDSSUid();
