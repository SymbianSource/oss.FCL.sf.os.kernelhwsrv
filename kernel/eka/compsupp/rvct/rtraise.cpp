// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// This file is part of drtrvct.dll and drtrvct_vfpv2.dll.
// 
//

#include <e32std.h>
#include <signal.h> // Will be found among RVCT's header files.


extern "C" EXPORT_C TInt __rt_raise(TInt signal, TInt type)
    {
    TExcType aExc = EExcGeneral;

    // Translate the signal into an EPOC exception.
    switch (signal)
        {
	case SIGABRT : 
	    aExc = EExcAbort;
	    break;

	case SIGFPE :
	    switch (type)
	        {
		case DIVBYZERO :
		    aExc = EExcAbort;
		    break;
		default:
		    aExc = EExcFloatInvalidOperation;
		}
	    break;

	case SIGILL :
	    aExc = EExcCodeAbort;
	    break;

	case SIGINT :
	    aExc = EExcUserInterrupt;
	    break;

	case SIGSEGV :
	    aExc = EExcDataAbort;
	    break;

	case SIGTERM :
	    aExc = EExcKill;
	    break;
	}

    User::RaiseException(aExc);

    return signal;
    }
