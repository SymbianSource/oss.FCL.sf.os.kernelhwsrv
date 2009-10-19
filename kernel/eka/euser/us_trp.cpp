// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_trp.cpp
// 
//

#include "us_std.h"
#include "us_data.h"




EXPORT_C TTrapHandler::TTrapHandler()
/**
Default constructor.
*/
	{}


#ifndef __LEAVE_EQUALS_THROW__

EXPORT_C void TTrap::UnTrap()
//
// Pop the current trap frame
//
	{

	TTrapHandler *pH=Exec::PopTrapFrame()->iHandler;
	if (pH!=NULL)
		pH->UnTrap();
	}


#else //__LEAVE_EQUALS_THROW__

EXPORT_C void User::Leave(TInt aReason)
/**
Leaves the currently executing function, unwinds the call stack, and returns
from the most recently entered trap harness.

@param aReason The value returned from the most recent call to TRAP or TRAPD.
               This is known as the reason code and, typically, it gives the
               reason for the environment or user error causing this leave
               to occur.
              
@see TRAP
@see TRAPD              
*/
	{
#ifdef __USERSIDE_THREAD_DATA__
	Exec::LeaveStart();
	TTrapHandler* pH = GetTrapHandler();
#else
	TTrapHandler* pH = Exec::LeaveStart();
#endif
	if (pH)
		pH->Leave(aReason);	// causes things on the cleanup stack to be cleaned up
	throw XLeaveException(aReason);
	}

#endif // !__LEAVE_EQUALS_THROW__


// Private declaration to prevent def file branching
#ifndef __SUPPORT_CPP_EXCEPTIONS__
class XLeaveException
	{
public:
	IMPORT_C TInt GetReason() const;
	};
#endif //__SUPPORT_CPP_EXCEPTIONS__

#if !defined(__LEAVE_EQUALS_THROW__) || !defined(__WINS__)
EXPORT_C TInt XLeaveException::GetReason() const
	{
#ifdef __SUPPORT_CPP_EXCEPTIONS__
	Exec::LeaveEnd();
	return iR;
#else // !__SUPPORT_CPP_EXCEPTIONS__
	return KErrNone;
#endif //__SUPPORT_CPP_EXCEPTIONS__
	}
#endif	// !defined(__LEAVE_EQUALS_THROW__) || !defined(__WINS__)

EXPORT_C void User::LeaveNoMemory()
/**
Leaves with the specific reason code KErrNoMemory.

@see KErrNoMemory
*/
	{

	User::Leave(KErrNoMemory);
	}




EXPORT_C TInt User::LeaveIfError(TInt aReason)
/**
Leaves or returns with a specified reason code.

If the reason code is negative the function leaves, and the reason code is 
returned through the trap harness.

If the reason code is zero or positive, the function simply returns with the 
reason value.

@param aReason The reason code.

@return If the function returns, the reason code which is either zero or positive.
*/
	{

	if (aReason<0)
		User::Leave(aReason);
	return(aReason);
	}




EXPORT_C TAny * User::LeaveIfNull(TAny *aPtr)
/**
Leaves with the reason code KErrNoMemory, if the specified pointer is NULL. 

If the pointer is not NULL, the function simply returns with the value of 
the pointer.

@param aPtr The pointer to be tested.

@return If the function returns, the value of aPtr.
*/
	{

	if (aPtr==NULL)
		User::LeaveNoMemory();
	return(aPtr);
	}

