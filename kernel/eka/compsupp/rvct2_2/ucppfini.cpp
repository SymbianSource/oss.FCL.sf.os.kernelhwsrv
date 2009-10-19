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

#define MAX_DTOR_RECORDS 256
static dtd dtor_rec[MAX_DTOR_RECORDS];

typedef dtd **dso_handle;
dtd * __dso_handle = &dtor_rec[MAX_DTOR_RECORDS];

void __cxa_finalize ( dso_handle d );

// Need to decide what this should do we run out of dtor space.
// Probably need to run __cxa_finalize then do some kind of panic.
void CppBSSInitializationError()
    {
//  __cxa_finalize(&dtor_rec[0]);
    };

// This is called by compiler generated code to record needed destructions of
// dynamically initialized (ctor) top level (BSS) data. 
// I guess this is more efficient for the compiler than __cxa_atexit, since 
// it takes the object that needs dtoring as its first arg, which means its in
// the right register when the ctor returns.
void __aeabi_atexit(void *aObject, void (*aDtor)(void *), dso_handle aHandle)
    {
    dtd * drec = *aHandle;
    drec--;

    if (drec < &dtor_rec[0])
      // Need to decide what to do here
      return CppBSSInitializationError();

    drec->dtor = aDtor;
    drec->obj = aObject;
    *aHandle = drec;
    }

int __cxa_atexit ( void (*aDtor)(void *), void *aObject, dso_handle aHandle )
    {
      __aeabi_atexit(aObject, aDtor, aHandle);

    // This is what the C++ GABI spec says to do!!
    if (*aHandle < &dtor_rec[0]) 
      return -1;
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

