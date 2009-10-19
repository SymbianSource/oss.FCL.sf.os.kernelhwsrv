// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// toplevel destruction routines for 'user side' code compiled 
// with the ARMEDG compiler.
// 
//

#include "cppinit.h"

extern "C" {

// Need to decide what this should do
//void CppInitializationPanic(){};

#define MAX_DTOR_RECORDS 256
static dtd dtor_rec[MAX_DTOR_RECORDS];

typedef dtd **dso_handle;
dtd * __dso_handle = &dtor_rec[MAX_DTOR_RECORDS];

int __cxa_atexit ( void (*f)(void *), void *p, dso_handle d )
    {
    dtd * drec = *d;
    drec--;
    // This is what the spec says to do!!
    if (drec < &dtor_rec[0]) return -1;

    drec->dtor = f;
    drec->obj = p;
    *d = drec;
    return 0;
    }

void __cxa_finalize ( dso_handle d )
    {
    dtd * drec = * d;
    dtd * lim = &dtor_rec[MAX_DTOR_RECORDS];
    while (drec < lim) 
        {
	drec->dtor(drec->obj);
	drec++;
        }
    *d = drec;
    }

void run_static_dtors (void)
    {
    __cxa_finalize(&__dso_handle);
    }
}

