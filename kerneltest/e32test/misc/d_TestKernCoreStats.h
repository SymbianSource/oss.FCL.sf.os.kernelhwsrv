// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\d_kerncorestas.h
// 


#ifndef D_TESTKERNCORESTATS
#define D_TESTKERNCORESTATS


#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif



_LIT(KTestKernCoreStatsName,"TestKernCoreStats");



class RTestKernCoreStats : public RBusLogicalChannel
        {
public:

        enum TControl
                {

        ERequestGetStats,
        ERequestConfigure,
        ERequestDumpInfo
                };
public:
        inline TInt Open();
        inline TInt GetStats( TAny* a0);
        inline TInt Configure( TInt a0);
        inline TInt DumpInfo();

        };

#ifndef __KERNEL_MODE__

inline TInt RTestKernCoreStats::Open()
		{ return DoCreate(KTestKernCoreStatsName,TVersion(0,1,1),KNullUnit,NULL,NULL); }


inline TInt RTestKernCoreStats::GetStats( TAny* a0)
        { return DoControl(ERequestGetStats, (TAny*) a0); }

inline TInt RTestKernCoreStats::Configure( TInt a0)
        { return DoControl(ERequestConfigure, (TAny*) a0); }

inline TInt RTestKernCoreStats::DumpInfo()
        { return DoControl(ERequestDumpInfo,NULL); }




#else

#define DRIVER_NAME(x)
#define DRIVER_REQUEST TInt

#endif // __KERNEL_MODE__

#endif

