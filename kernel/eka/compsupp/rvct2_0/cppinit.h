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
//

/**
 @file
 @internalComponent
*/

#ifndef _CPPINIT_H_
#define _CPPINIT_H_
extern "C" {

// Doing it like this means no space is taken by the symbols
#define NUKE_SYMBOL(sig) __asm void sig {}
typedef void (*PFV)();
typedef void (DTOR)(void *);
#define RELOCATE(loc, type) (type)((int)loc + (int)*loc)

typedef struct dtd 
{ 
//  dtd * prev;
  void * obj;
  DTOR * dtor; 
} dtd;

//extern void run_static_ctors (void);
extern void run_static_dtors (void);

}

#endif
