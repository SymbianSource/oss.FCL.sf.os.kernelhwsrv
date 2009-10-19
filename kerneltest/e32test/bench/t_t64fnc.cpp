// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\t_t64fnc.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>

GLREF_D volatile TInt64 count;
GLREF_D volatile TInt64 a, b, c;

TInt TInt64Addition(TAny*)
    {

      TInt64  al = a;
      const TInt64 bl = b;
      const TInt64 x = 1;
      for(;;)
        {
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;
        al +=bl;

		a = al;
        count += x;
		if (!count)
			break;
      }
      return (TInt)count;
    }

TInt TInt64Subtraction(TAny*)
    {

    a = MAKE_TINT64(0x31514531,0x93471672u);
	b = MAKE_TINT64(0x51514531,0x33847162);

      TInt64  al = a;
      const TInt64 bl = b;
      const TInt64 x = 1;
      for(;;)
        {
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;
        al -=bl;

		a = al;
        count += x;
		if (!count)
			break;
      }
      return (TInt)count;
    }


TInt TInt64Multiplication(TAny*)
    {
    a = MAKE_TINT64(0xffffffff,0xfa0a1f00u);
	b = MAKE_TINT64(0x0,0xfa0a1f00u);
	
      TInt64  al = a;
      const TInt64 bl = b;
      const TInt64 x = 1;

      for(;;)
        {
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;
        al *=bl;

		a = al;
        count += x;
		if (!count)
			break;
      }
      return (TInt)count;
    }

TInt TInt64Division(TAny*)
    {

    a = MAKE_TINT64(0x31514531,0x93471672u);
	b = MAKE_TINT64(0x0,0x05F5E100);
	
      TInt64  al = a;
      const TInt64 bl = b;
      const TInt64 x = 1;

      for(;;)
        {
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;
        al /=bl;

		a = al;
        count += x;
		if (!count)
			break;
      }
      return (TInt)count;    }

