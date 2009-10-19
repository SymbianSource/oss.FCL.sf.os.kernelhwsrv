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

#ifndef RFSYDEBUG_H
#define RFSYDEBUG_H

class RFsyDebug
    {
public:
    static const TUint32 KMntProp_DisableALL           = 0xFFFFFFFF; //-- disable all operations

    static const TUint32 KMntProp_EnableALL            = 0x00000000; //-- enable all operations
    static const TUint32 KMntProp_Disable_FsInfo       = 0x00000001; //-- mask for disabling/enabling FSInfo information
    static const TUint32 KMntProp_Disable_FatBkGndScan = 0x00000002; //-- mask for disabling/enabling FAT background scanner

    RFsyDebug(TInt iDriveNo);
    ~RFsyDebug();
    void Set(TUint32 aFlags);
    void DisableAll();

private:
    TInt iDriveNo;
    };

#endif
