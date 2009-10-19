// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Global configuration for the tests
// 
//

#ifndef CONFIG_H
#define CONFIG_H

//
// Define the drive used for ROFS
//

#ifdef __WINS__
const TDriveNumber KRofsDriveNumber = EDriveV;
#else
const TDriveNumber KRofsDriveNumber = EDriveJ;
#endif


#endif
