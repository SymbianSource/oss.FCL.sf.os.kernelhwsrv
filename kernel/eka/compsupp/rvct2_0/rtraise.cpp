// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// function the runtime can call to 'raise an exception'
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <signal.h> // get from %ARMINC%

extern "C" {

EXPORT_C TInt __rt_raise(TInt signal, TInt type)
    {
    TExcType aExc = EExcGeneral;
    // translate signal into EPOC exception
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
    // yuk. Introduces dependendcy on EUSER!!
    User::RaiseException(aExc);
    return signal;
    }
}

