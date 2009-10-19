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
// e32\include\drivers\resource_category.h
// 
//

#ifndef __RESOURCE_CATEGORY_H__
#define __RESOURCE_CATEGORY_H__
/** 
@publishedPartner
@released 9.5
This file contains the definition of resource category to be used
by user side resource manager implementation. 
NOTE: Whenever resource category changes in resource.h, same needs to be
updated here aswell.
	*/
enum TResourceType {EResBinary, EResMultilevel, EResMultiProperty};
enum TResourceUsage {EResSingleUse, EResShared};
enum TResourceLatency {EResInstantaneous, EResLongLatency};
enum TResourceClass {EResPhysical, EResLogical};
enum TResourceSense {EResPositive, EResNegative, EResCustom};
#endif
