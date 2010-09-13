// Copyright (c) 1998-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_prel.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>

IMPORT_C void FurtiveD(TFileName &aInfo);
EXPORT_C void FurtiveD(TFileName &aInfo)
	{

	const char* dummy = "Link unit for Preload test";
	dummy = dummy;
	TFileName name;
	Dll::FileName(name);
	aInfo.Append(name);
	}

