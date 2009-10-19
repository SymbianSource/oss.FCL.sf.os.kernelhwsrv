// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_sysbin_dll.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>

// Dummy export because toolchain gives warning for DLLs without exports
EXPORT_C void dummyExport() {}
