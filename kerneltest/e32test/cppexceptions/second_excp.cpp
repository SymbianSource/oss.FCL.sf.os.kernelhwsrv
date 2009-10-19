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
#include "second_excp.h"

EXPORT_C int thrower2 (int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  if (x != 0) throw MySecondException(x);
#else //!__SUPPORT_CPP_EXCEPTIONS__
  if (x != 0)
	  return x;
  else
#endif //__SUPPORT_CPP_EXCEPTIONS__
  return -1;
}

EXPORT_C int thrower3 (int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  if (x != 0) throw MyThirdException(x);
#else //!__SUPPORT_CPP_EXCEPTIONS__
  if (x != 0)
	  return x;
  else
#endif //__SUPPORT_CPP_EXCEPTIONS__
  return -1;
}

#pragma warning( disable : 4673 )	/* throwing 'class MyFourthException' the following types will not be considered at the catch site */

EXPORT_C int thrower4 (int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  if (x != 0) throw MyFourthException(x);
#else //!__SUPPORT_CPP_EXCEPTIONS__
  if (x != 0)
	  return x;
  else
#endif //__SUPPORT_CPP_EXCEPTIONS__
  return -1;
}

EXPORT_C int thrower5 (int x) {
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  if (x != 0) throw MyFifthException(x);
#else //!__SUPPORT_CPP_EXCEPTIONS__
  if (x != 0)
	  return x;
  else
#endif //__SUPPORT_CPP_EXCEPTIONS__
  return -1;
}


EXPORT_C UncaughtTester::UncaughtTester(TInt & x) : aInt(x){}

EXPORT_C UncaughtTester::~UncaughtTester(){
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  if (std::uncaught_exception()) {
    aInt = 2;
  } else {
#endif //__SUPPORT_CPP_EXCEPTIONS__
    aInt = 1;
#ifdef __SUPPORT_CPP_EXCEPTIONS__
  }
#endif //__SUPPORT_CPP_EXCEPTIONS__
}

