// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\cppexceptions\t_romtable.cpp
// Overview:
// Check accessibility and integrity of ROM exception search table.
// API Information:
// TRomHeader, TRomExceptionSearchTable, TExceptionDescriptor
// Details:	
// - Get the ROM exception search table, verify results are as expected.
// - Check that the correct exception descriptor is returned by
// GetExceptionDescriptor().
// - Check that the correct index table entry can be found.
// - Initialize the unwinder cache of a UCB with an exception descriptor
// and check we get the right result.
// - Throw and catch a variety of exceptions and verify results are as expected.
// Platforms/Drives/Compatibility:
// Hardware (Automatic).
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

/* Environment: */
#include "unwind_env.h"
/* Language-independent unwinder declarations: */
#include "unwinder.h"

#include "symbian_support.h"

#include <f32file.h>
#include <e32test.h>
#include <e32rom.h>
#include <cpudefs.h>

GLDEF_D RTest test(_L("T_ROMTABLE"));

TRomHeader * pTheRomHeader;
char * GetExceptionDescriptor(void);
extern "C" {
  IMPORT_C TRomExceptionSearchTable * GetROMExceptionSearchTable(void);
  IMPORT_C TExceptionDescriptor * SearchEST(uint32_t addr, TRomExceptionSearchTable * aESTp);
  TExceptionDescriptor * GetRAMLoadedExceptionDescriptor(uint32_t addr);
  IMPORT_C const __EIT_entry *SearchEITV1(uint32_t return_address_offset, const __EIT_entry *base, unsigned int nelems);
  IMPORT_C __EIT_entry* SearchEITV2(uint32_t return_address, const __EIT_entry* base, unsigned int nelems);
  TExceptionDescriptor * ReLoadExceptionDescriptor(uint32_t addr, _Unwind_Control_Block * ucbp);
  IMPORT_C void InitialiseSymbianSpecificUnwinderCache(uint32_t addr, _Unwind_Control_Block * ucbp);

  void *__cxa_allocate_exception(size_t size);
  IMPORT_C __cxa_eh_globals *  __cxa_get_globals();

#ifdef _DEBUG
  IMPORT_C void DebugPrintf(const char * aFmt, ...);
#endif
}

int catcher(int x);
int catcher2(int x);
int catcher3(int x);
int catcher4(int x);
int catcher5(int x);
void TestUncaught(void);
const __EIT_entry * DumbGetEITV1(TUint32 offset, const __EIT_entry * base, TUint32 n);
__EIT_entry * DumbGetEITV2(TUint32 addr, __EIT_entry * base, TUint32 n);

GLDEF_C TInt E32Main()
    {
    test.Start(_L("Check accessibility and integrity of ROM exception search table"));

    TRomExceptionSearchTable * pEST = GetROMExceptionSearchTable();
    test.Printf(_L("ROM EST @ %08x\n"), pEST);
    test(pEST != 0);

    test.Printf(_L("ROM EST contains %d entries\n"), pEST->iNumEntries);

    for (int i=0; i<pEST->iNumEntries; i++)
        {
	TRomImageHeader * pE = (TRomImageHeader *)pEST->iEntries[i];
	TRomImageHeader * pH = pE -1;
	TUint xd = pH->iExceptionDescriptor;
	if ((xd&1)==0 || xd==0xffffffffu)
		continue;
	xd &= ~1;
	TExceptionDescriptor * pED = (TExceptionDescriptor *) ((char *)pE + xd);
	char * aExIdxBase = (char *)pED->iExIdxBase;
	char * aExIdxLimit = (char *)pED->iExIdxLimit;
	char * aROBase = (char *) pED->iROSegmentBase;
	char * aROLimit = (char *)pED->iROSegmentLimit;

	test.Printf(_L("%d\n"),i);

	test(aExIdxBase <= aExIdxLimit);

	test(aROBase <= aROLimit);

	test(aROBase <= aExIdxBase);
	test(aExIdxLimit <= aROLimit);
        }
    test.Printf(_L("\n"));

#ifdef __SUPPORT_CPP_EXCEPTIONS__
    // Check we get the right Exception Descriptor
    char * myExcpDesc = GetExceptionDescriptor();
    uint32_t addr = (uint32_t)GetExceptionDescriptor;

    test.Printf(_L("Checking &GetExceptionDescriptor [%08x] in EST range: %08x - %08x\n"), 
		addr, pEST->iEntries[0], GET_EST_FENCEPOST(pEST));
    test(addr >= pEST->iEntries[0] && addr < GET_EST_FENCEPOST(pEST));
    TExceptionDescriptor * aExcpDescP = SearchEST(addr, pEST);
    test.Printf(_L("Found entry %08x in EST\n"), aExcpDescP);
    test(aExcpDescP != NULL);
    test.Printf(_L("Check myExcpDesc[%08x] == aExcpDescP[%08x]\n"), myExcpDesc, aExcpDescP);
    test(myExcpDesc==(char*)aExcpDescP);

    // Now check we can find the right entry in the index table.
    __EIT_entry * aExIdxBase1 = (__EIT_entry *)aExcpDescP->iExIdxBase;
    __EIT_entry * aExIdxLimit1 = (__EIT_entry *)aExcpDescP->iExIdxLimit;
    unsigned int nelems = aExIdxLimit1 - aExIdxBase1;
    uint32_t aROBase1 = (uint32_t)aExcpDescP->iROSegmentBase;
    int ehabiv2 = aROBase1 & 1;
    aROBase1 = aROBase1 & EHABI_MASK;
    uint32_t aRetOffest = addr - aROBase1;
    test.Printf(_L("EHABI_V2= %d: Looking up %08x with base %08x in ExIdx @ %08x with %d entries\n"),
		ehabiv2, addr, aROBase1, aExIdxBase1, nelems);
    if (ehabiv2) {
      __EIT_entry * aResult = SearchEITV2(addr, aExIdxBase1, nelems);
      __EIT_entry * aResult1 = DumbGetEITV2(addr, aExIdxBase1, nelems);
      test.Printf(_L("Check result %08x == %08x\n"), aResult, aResult1);
      test(aResult == aResult1);
    } else {
      const __EIT_entry * aResult = SearchEITV1(aRetOffest, aExIdxBase1, nelems);
      const __EIT_entry * aResult1 = DumbGetEITV1(aRetOffest, aExIdxBase1, nelems);
      test.Printf(_L("Check result %08x == %08x\n"), aResult, aResult1);
      test(aResult == aResult1);
    }
#ifdef _DEBUG
    DebugPrintf("Exception diagnostic print support is working if you can see this!!\n\r");
#else
    test.Printf(_L("Exception diagnostic print support only available in UDEB builds - not tested\n"));
#endif

    //check we've got some eh_globals
    __cxa_eh_globals *g = __cxa_get_globals();
    test.Printf(_L("Exception Handling globals for this thread allocated @ %08x\n"), g);
    test(g != NULL);
    test.Printf(_L("Emergency Buffer @ %08x\n"), g->emergency_buffer);

    // now initialize a the unwinder cache of a ucbp with an exception descriptor
    // and check we get the right result.
    test.Printf(_L("Allocate an empty exception object\n"));
    __cxa_exception *ep = ((__cxa_exception *)__cxa_allocate_exception(0)) - 1;
    test.Printf(_L("Empty Exception Object @ %08x UCB @ %08x\n"), ep, &ep->ucb);
    test.Printf(_L("Initialize the UCB with the EST and the current exception descriptor\n"));
    InitialiseSymbianSpecificUnwinderCache(addr, &ep->ucb);
    test.Printf(_L("Check the EST in the UCB [%08x] == %08x and the Exception Desc [%08x] == %08x\n"),
		GET_ROM_EST(&ep->ucb), pEST, GET_EXCEPTION_DESCRIPTOR(&ep->ucb), myExcpDesc);
    test(GET_ROM_EST(&ep->ucb)==pEST);
    test((char*)GET_EXCEPTION_DESCRIPTOR(&ep->ucb)== myExcpDesc);
    
    test.Printf(_L("Throwing first exception.\n"));
    int r = catcher(2);
    test.Printf(_L("Returned %d expected 2\n"), r);
    test(r==2);

    test.Printf(_L("Not throwing first exception.\n"));
    r = catcher(0);
    test.Printf(_L("Returned %d expected -1\n"), r);
    test(r==-1);

    test.Printf(_L("Throwing second exception.\n"));
    r = catcher2(3);
    test.Printf(_L("Returned %d expected 3\n"), r);
    test(r==3);

    test.Printf(_L("Not throwing second exception.\n"));
    r = catcher2(0);
    test.Printf(_L("Returned %d expected -1\n"), r);
    test(r==-1);

    test.Printf(_L("Throwing third exception.\n"));
    r = catcher3(4);
    test.Printf(_L("Returned %d expected 4\n"), r);
    test(r==4);

    test.Printf(_L("Not throwing third exception.\n"));
    r = catcher3(0);
    test.Printf(_L("Returned %d expected -1\n"), r);
    test(r==-1);

    test.Printf(_L("Throwing fourth exception.\n"));
    r = catcher4(5);
    test.Printf(_L("Returned %d expected 5\n"), r);
    test(r==5);

    test.Printf(_L("Not throwing fourth exception.\n"));
    r = catcher4(0);
    test.Printf(_L("Returned %d expected -1\n"), r);
    test(r==-1);

    test.Printf(_L("Throwing fifth exception.\n"));
    r = catcher5(6);
    test.Printf(_L("Returned %d expected 6\n"), r);
    test(r==6);

    test.Printf(_L("Not throwing fifth exception.\n"));
    r = catcher5(0);
    test.Printf(_L("Returned %d expected -1\n"), r);
    test(r==-1);

    test.Printf(_L("Testing std::uncaught_exception.\n"));
    TestUncaught();
#endif

	test.End();
	test.Close();
    return 0;
    }

#ifdef __ARMCC__
// We rely on this immediately following E32main. DONT MOVE IT
__asm char * GetExceptionDescriptor(void) {
  extern |Symbian$$CPP$$Exception$$Descriptor|
  ldr r0, theExceptionDesc

#ifdef __SUPPORT_THUMB_INTERWORKING
  bx lr
#else
  mov pc, lr
#endif

theExceptionDesc dcd |Symbian$$CPP$$Exception$$Descriptor|
}
#endif

const __EIT_entry * DumbGetEITV1(TUint32 offset, const __EIT_entry * base, TUint32 n) {
  if (n && offset < base[0].fnoffset) return 0;
  for (int i=0; i<n; i++) {
    // check for last entry 
    if (i == n-1 && base[i].fnoffset <= offset) return &base[i];
    if (base[i].fnoffset <= offset && offset < base[i+1].fnoffset) return &base[i];
  }
  return 0;
}

static uint32_t __ARM_resolve_prel31(void *p)
{
  return (uint32_t)((((*(int32_t *)p) << 1) >> 1) + (int32_t)p);
}

__EIT_entry * DumbGetEITV2(TUint32 addr, __EIT_entry * base, TUint32 n) {
  if (n && (addr < __ARM_resolve_prel31(&base[0].fnoffset)))
    return 0;
  for (int i=0; i<n; i++) {
    // check for last entry 
    if (i == n-1 && __ARM_resolve_prel31(&base[i].fnoffset) <= addr) 
      return &base[i];
    if ((__ARM_resolve_prel31(&base[i].fnoffset) <= addr) && 
	(addr < __ARM_resolve_prel31(&base[i+1].fnoffset)))
      return &base[i];
  }
  return 0;
}
    
#ifdef __SUPPORT_CPP_EXCEPTIONS__

class MyFirstException {
public:
  MyFirstException(int x) { iVal = x; };
  virtual ~MyFirstException();
  int iVal;
};

MyFirstException::~MyFirstException(){}

int thrower (int x) {
  if (x != 0) throw MyFirstException(x);
  return -1;
}

int catcher(int x) {
  try {
    return thrower(x);
  }
  catch(MyFirstException& e) 
    {
      return e.iVal;
    }
}


#include "second_excp.h"


int catcher2(int x) {
  try {
    return thrower2(x);
  }
  catch(MySecondException& e) 
    {
      return e.iVal;
    }
}

int catcher3(int x) {
  try {
    return thrower3(x);
  }
  catch(MyThirdException& e) 
    {
      return e.iVal;
    }
}

int catcher4(int x) {
  try {
    return thrower4(x);
  }
  catch(MyFourthException& e) 
    {
      return e.iVal;
    }
}

int catcher5(int x) {
  try {
    return thrower5(x);
  }
  catch(MyFifthException& e) 
    {
      return e.iVal;
    }
}

void TestUncaught(void) {
  TInt x = 0;
  try {
    UncaughtTester aTester(x);
    test.Printf(_L("Check throw case\n"));
    thrower(0);
  }
  catch(MyFirstException& e) 
    {
      test.Printf(_L("~Check x == 0\n"));
      test(x == 0);
    }
  try {
    UncaughtTester aTester(x);
    test.Printf(_L("Check no throw case\n"));
  }
  catch(MyFirstException& e) 
    {
      test.Printf(_L("Whoops!!!\n"));
    }
  test(x==1);
}


#endif







