/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

#include <e32std.h>

class MySecondException {
public:
  MySecondException(){};
  MySecondException(int x) { iVal = x; };
  int iVal;
};

IMPORT_C int thrower2 (int x);


class MyThirdException : public MySecondException {
public:
  MyThirdException(){};
  MyThirdException(int x) { iVal = x; iVal1 = x+1;};
  int iVal1;
};

IMPORT_C int thrower3 (int x);

NONSHARABLE_CLASS(VB1) : virtual public MySecondException {};
NONSHARABLE_CLASS(VB2) : virtual public MySecondException {};

class MyFourthException : public VB1 , public VB2 {
public:
#if defined(__ARMCC__) && __ARMCC_VERSION >= 400000
  EXPORT_C MyFourthException(int x) { iVal = x; iVal2=x+2;};
#else
  MyFourthException(int x) { iVal = x; iVal2=x+2;};
#endif
  int iVal2;
};

IMPORT_C int thrower4 (int x);

class B1 {
 public:
  B1(int x): iX(x){}
  int iX;
};

class MyFifthException : public MySecondException, public B1 {
 public:
  MyFifthException(int x): MySecondException(x), B1(x){}
};

IMPORT_C int thrower5 (int x);

class UncaughtTester {
public:
  IMPORT_C UncaughtTester(TInt & x);
  IMPORT_C ~UncaughtTester();
  TInt & aInt;
};
