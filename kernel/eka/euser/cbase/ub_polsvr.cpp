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
// e32\euser\cbase\ub_polsvr.cpp
// 
//

#include "ub_std.h"

_LIT(KPolicyServer, "CPolicyServer");

#include <e32debug.h>
#define __PSD(x) __PLATSEC_DIAGNOSTIC_STRING(x)

EXPORT_C CPolicyServer::CPolicyServer(TInt aPriority, const TPolicy& aPolicy, TServerType aType)
	: CServer2(aPriority, aType), iPolicy(aPolicy)
	{
#ifdef _DEBUG
	TUint i;
	TInt prev = iPolicy.iRanges[0];
	//iPolicy.iRangeCount must be greater than 0. (ie you must have at least
	//one policy
	__ASSERT_DEBUG(iPolicy.iRangeCount > 0, Panic(EPolSvrIRangeCountInvalid));
	//iRanges must start from request number 0.
	__ASSERT_DEBUG(prev == 0, Panic(EPolSvr1stRangeNotZero));
	__ASSERT_DEBUG((iPolicy.iElementsIndex[0] < ESpecialCaseHardLimit
		|| iPolicy.iElementsIndex[0] > ESpecialCaseLimit), 
		Panic(EPolSvrElementsIndexValueInvalid) );
	for(i=1; i<iPolicy.iRangeCount; i++)
		{
		TInt next = iPolicy.iRanges[i];
		//iRanges must be in increasing order.
		__ASSERT_DEBUG(next > prev, Panic(EPolSvrRangesNotIncreasing));
		//iElementsIndex must not contain invalid values.
		__ASSERT_DEBUG((iPolicy.iElementsIndex[i] < ESpecialCaseHardLimit
			|| iPolicy.iElementsIndex[i] > ESpecialCaseLimit), 
			Panic(EPolSvrElementsIndexValueInvalid) );
		prev = next;
		}
	//iOnConnect must not be an invalid value.
	__ASSERT_DEBUG((iPolicy.iOnConnect < ESpecialCaseHardLimit
		|| iPolicy.iOnConnect > ESpecialCaseLimit), 
		Panic(EPolSvrIOnConnectValueInvalid) );
#endif
	}

EXPORT_C void CPolicyServer::RunL()
	{
	const RMessage2& msg = Message();
	msg.ClearAuthorised();
	TInt fn = msg.Function();

	__ASSERT_COMPILE(-1 == RMessage2::EConnect);
	if(fn >= RMessage2::EConnect) 
		//So this implies any "normal" message or Connect
		//Now we have two steps to follow each having two mutually exculsive
		//parts.
		//Step 1: Find policy.
		//Step 2: Apply policy.
		{
		const TPolicyElement* element = 0;
		TUint specialCase = 0;
		//1a: If its a normal message.  Find the associate policy or special
		//case action.
		if(fn >= 0)
			{
			element = FindPolicyElement(fn, specialCase);
			}
		//1b: If its a connect message, there's a shortcut to the policy.
		else 
			{
			TUint8 i = iPolicy.iOnConnect;
			if(i >= ESpecialCaseHardLimit)
				specialCase = i;
			else
				element = &(iPolicy.iElements[i]);
			}
		//2a: We found a policy that we can automatically apply... Apply it!
		if(element)
			{
			TSecurityInfo missing;
			//If policy check succeeds, allow it through
			if(element->iPolicy.CheckPolicy(msg, missing, __PSD("Checked by CPolicyServer::RunL")))
				{
				ProcessL(msg);
				}
			//Else see what failure action is required (return error code,
			//panic client, ask user, etc...)
			else
				{
				CheckFailedL(msg, element->iAction, missing);
				}
			}
		//2b: The policy is a special case
		else 
			{
			switch(specialCase)
				{
				//If you change this you'll have to add to the switch statement
				__ASSERT_COMPILE(ESpecialCaseLimit == 252u);
				case ECustomCheck:
					{
					TInt action = EFailClient; 
					//The default action after failing a CustomSecurityCheck is
					//to complete the message with KErrPermissionDenied.  If
					//you want a different action, then change the action
					//parameter prior to returning from your derived
					//implementation of CustomSecurityCheckL
					TSecurityInfo missing;
					__ASSERT_COMPILE(SCapabilitySet::ENCapW == 2);
					memset(&missing, 0, sizeof(SSecurityInfo));
					TCustomResult result = CustomSecurityCheckL(msg, action, missing);
					if(result == EPass)
						{
						ProcessL(msg);
						}
					else if(result == EFail)
						{
						CheckFailedL(msg, action, missing); 
						}
					else if(result == EAsync)
						{
						//Do Nothing.  Derived CustomSecurityCheck is
						//responsible for calling ProcessL/CheckFailedL
						}
					else
						Panic(EPolSvrInvalidCustomResult);
					}
					break;	
				case ENotSupported:
					msg.Complete(KErrNotSupported);	
					break;
				case EAlwaysPass:
					ProcessL(msg);
					break;
				default:
					Panic(EPolSvrPolicyInvalid);
					break;
				}
			}
		}
	//else it must be either Disconnect or bad message.  Both are handled by
	//ProcessL
	else 
		{
		ProcessL(msg);
		}

	// Queue reception of next message if it hasn't already been done
	if(!IsActive())
		ReStart();
	}

EXPORT_C TInt CPolicyServer::RunError(TInt aError)
	{
	ProcessError(Message(), aError);
	if (!IsActive())
		ReStart();
	return KErrNone;
	}

EXPORT_C void CPolicyServer::ProcessL(const RMessage2& aMsg)
	{
	aMsg.SetAuthorised();
	TInt fn = aMsg.Function();

	if(fn >= 0)
		{
		CSession2* session=aMsg.Session();
		if(session)
			{
			session->ServiceL(aMsg);
			}
		else
			{
			NotConnected(aMsg);
			}
		}
	else if(fn==RMessage2::EConnect)
		{
		Connect(aMsg);
		}
	else if(fn==RMessage2::EDisConnect)
		{
		Disconnect(aMsg);
		}
	else
		{
		BadMessage(aMsg);
		}
	}

EXPORT_C void CPolicyServer::ProcessError(const RMessage2& aMsg, TInt aError)
	{
	__ASSERT_COMPILE(-1 == RMessage2::EConnect);
	__ASSERT_ALWAYS(aMsg.Function() >= RMessage2::EConnect, User::Panic(KPolicyServer, 2));
	if(aMsg.Authorised() && aMsg.Function() >= 0)
		{
		aMsg.Session()->ServiceError(aMsg, aError);
		}
	else //Either ServiceL hadn't been called yet (not (yet) authorised) or
		//it's a Connect message
		{
		aMsg.Complete(aError);
		}
	}

EXPORT_C CPolicyServer::TCustomResult CPolicyServer::CustomSecurityCheckL(const RMessage2& /*aMsg*/, TInt& /*aAction*/, TSecurityInfo& /*aMissing*/)
	{
	Panic(EPolSvrCallingBaseImplementation);
	return EFail;
	}

EXPORT_C void CPolicyServer::CheckFailedL(const RMessage2& aMsg, TInt aAction, const TSecurityInfo& aMissing)
	{
	if(aAction < 0)
		{
		TCustomResult result = CustomFailureActionL(aMsg, aAction, aMissing);
		if(result == EPass)
			ProcessL(aMsg);
		else if(result == EFail)
			aMsg.Complete(KErrPermissionDenied);
		else if(result == EAsync)
			{}
			//Do Nothing.  Derived CustomFailureActionL is responsible for
			//calling ProcessL/completing message with KErrPermissionDenied
		else
			Panic(EPolSvrInvalidCustomResult);
		}
	else if(aAction == EFailClient)
		{
		aMsg.Complete(KErrPermissionDenied);
		}
	else //if (aAction == EPanic) and all other +ve values
		{
		_LIT(KE32UserCBase, "E32USER-CBase");
		aMsg.Panic(KE32UserCBase, EPolSvrActionPanicClient);
		}
	}

EXPORT_C CPolicyServer::TCustomResult CPolicyServer::CustomFailureActionL(const RMessage2& /*aMsg*/, TInt /*aAction*/, const TSecurityInfo& /*aMissing*/)
	{
	Panic(EPolSvrCallingBaseImplementation);
	return EFail;
	}

const CPolicyServer::TPolicyElement* CPolicyServer::FindPolicyElement(TInt aFn, TUint& aSpecialCase) const
	{
	//Connect (aFn == -1) is handled through iPolicy.iOnConnect.  So aFn should
	//always be greater than -1.
	__ASSERT_DEBUG(aFn >= 0, User::Panic(KPolicyServer, 1));

	TUint l = 0;
	TUint u = iPolicy.iRangeCount;
	TUint m = 0;
	while(u > l)
		{
		m = (l+u) >> 1;
		if(iPolicy.iRanges[m] > aFn)
			u = m;
		else
			l = m + 1;
		}
	--l;
	//the mth element of iElementsIndex tells us the index in iElements
	//we want
	TUint8 i = iPolicy.iElementsIndex[l];
	//if the mth element of iElementsIndex is >= 250 -> Special Case
	if(i >= ESpecialCaseHardLimit)
		{
		aSpecialCase = i;
		return 0;
		}
	return &(iPolicy.iElements[i]);
	}




/**
Extension function


*/
EXPORT_C TInt CPolicyServer::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return CServer2::Extension_(aExtensionId, a0, a1);
	}
