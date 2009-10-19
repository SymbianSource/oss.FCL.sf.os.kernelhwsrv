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
// f32\inc\F32PluginUtils.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/
#include <f32plugin.h>


IMPORT_C TInt GetName(TFsPluginRequest* aRequest, TDes& aName);
IMPORT_C TInt GetNewName(TFsPluginRequest* aRequest, TDes& aNewName);
IMPORT_C TInt GetPath(TFsPluginRequest* aRequest, TDes& aPath);
IMPORT_C TInt GetAtt(TFsPluginRequest* aRequest, TInt& aAtt);
IMPORT_C TInt GetModifiedTime(TFsPluginRequest* aRequest, TTime*& aModified, TBool aCurrent=ETrue);
IMPORT_C TInt GetFileAccessInfo(TFsPluginRequest* aRequest, TInt& aLength, TInt& aPos);
