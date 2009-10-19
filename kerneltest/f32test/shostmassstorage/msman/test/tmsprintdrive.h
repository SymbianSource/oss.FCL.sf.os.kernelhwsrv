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



/**
 @file
 @internalTechnology
*/

#ifndef TMSPRINTDRIVE_H
#define TMSPRINTDRIVE_H

class TMsPrintDrive
	{
public:
    static void VolInfoL(TInt aDriveNumber);

private:
    static void FormatDriveInfo(TDes& aBuffer, const TDriveInfo& aDriveInfo);
    static void FormatVolumeInfo(TDes& aBuffer, const TVolumeInfo& aVolumeInfo);
	};


#endif // TMSPRINTDRIVE_H
