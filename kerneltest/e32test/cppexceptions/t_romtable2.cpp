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
// e32test\cppexceptions\t_romtable2.cpp
// Overview:
// Check exceptions in RAM loaded image.
// API Information:
// N/A
// Details:	
// - Throw and catch a variety of exceptions and verify 
// results are as expected.
// Platforms/Drives/Compatibility:
// Hardware (Automatic).
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//


#include <e32test.h>

GLDEF_D RTest test(_L("T_ROMTABLE"));

int catcher(int x);
int catcher2(int x);
int catcher3(int x);
int catcher4(int x);
int catcher5(int x);
void TestUncaught(void);

GLDEF_C TInt E32Main()
    {
    test.Start(_L("Check exceptions in RAM loaded image."));

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

	test.End();
	test.Close();

    return 0;
    }


class MyFirstException {
public:
  MyFirstException(int x) { iVal = x; };
  virtual ~MyFirstException();
  int iVal;
};

MyFirstException::~MyFirstException(){}

int thrower (int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  if (x != 0) throw MyFirstException(x);
#else //!__SUPPORT_CPP_EXCEPTIONS__
  if (x != 0)
	  return x;
  else
#endif //__SUPPORT_CPP_EXCEPTIONS__
  return -1;
}

int catcher(int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  try {
#endif //__SUPPORT_CPP_EXCEPTIONS__
    return thrower(x);
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  }
  catch(MyFirstException& e) 
    {
      return e.iVal;
    }
#endif //__SUPPORT_CPP_EXCEPTIONS__
}


#include "second_excp.h"


int catcher2(int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  try {
#endif //__SUPPORT_CPP_EXCEPTIONS__
    return thrower2(x);
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  }
  catch(MySecondException& e) 
    {
      return e.iVal;
    }
#endif //__SUPPORT_CPP_EXCEPTIONS__
}

int catcher3(int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  try {
#endif //__SUPPORT_CPP_EXCEPTIONS__
    return thrower3(x);
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  }
  catch(MyThirdException& e) 
    {
      return e.iVal;
    }
#endif //__SUPPORT_CPP_EXCEPTIONS__
}

int catcher4(int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  try {
#endif //__SUPPORT_CPP_EXCEPTIONS__
    return thrower4(x);
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  }
  catch(MyFourthException& e) 
    {
      return e.iVal;
    }
#endif //__SUPPORT_CPP_EXCEPTIONS__
}

int catcher5(int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  try {
#endif //__SUPPORT_CPP_EXCEPTIONS__
    return thrower5(x);
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  }
  catch(MyFifthException& e) 
    {
      return e.iVal;
    }
#endif //__SUPPORT_CPP_EXCEPTIONS__
}

void TestUncaught(void) {
  TInt x = 0;
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  try {
    UncaughtTester aTester(x);
    test.Printf(_L("Check throw case\n"));
    thrower(5);
	test(x == 5);
  }
  catch(MyFirstException& /*e*/) 
    {
#if defined(__MSVCDOTNET__) || (defined(__CW32__) && (__MWERKS__ > 0x3200)) || defined(__EABI__)
      test.Printf(_L("~Check x == 2\n"));
	  test(x == 2);
#else
	  test.Printf(_L("Checking x == 1, as std::uncaught_exception() broken\n"));
	  test(x == 1);
#endif
    }
  try
#endif //__SUPPORT_CPP_EXCEPTIONS__
	{
    UncaughtTester aTester(x);
    test.Printf(_L("Check no throw case\n"));
    }
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  catch(MyFirstException& /*e*/) 
    {
      test.Printf(_L("Whoops!!!\n"));
    }
#endif //__SUPPORT_CPP_EXCEPTIONS__
  test(x==1);
}
