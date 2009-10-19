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
// e32\common\secure.cpp
// 
//

#define __INCLUDE_ALL_SUPPORTED_CAPABILITIES__
#include "common.h"
#ifdef __KERNEL_MODE__
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#endif

// Check that the layout of TSecurityInfo and SSecurityInfo are the same
// because we use this assumption in the TSecurityInfo::Set methods
__ASSERT_COMPILE(_FOFF(TSecurityInfo,iSecureId)==_FOFF(SSecurityInfo,iSecureId));
__ASSERT_COMPILE(_FOFF(TSecurityInfo,iVendorId)==_FOFF(SSecurityInfo,iVendorId));
__ASSERT_COMPILE(_FOFF(TSecurityInfo,iCaps)==_FOFF(SSecurityInfo,iCaps));


#ifdef __KERNEL_MODE__


/**
Construct a TSecurityInfo setting it to the security attributes of aProcess.
@param aProcess A process.
*/
EXPORT_C TSecurityInfo::TSecurityInfo(DProcess* aProcess)
	{
	memcpy(this, &aProcess->iS, sizeof(SSecurityInfo));
	}

/**
Construct a TSecurityInfo setting it to the security attributes to those of the process
owning the specified thread.
@param aThread A thread.
*/
EXPORT_C TSecurityInfo::TSecurityInfo(DThread* aThread)
	{
	memcpy(this, &aThread->iOwningProcess->iS, sizeof(SSecurityInfo));
	}

#else

/**
Construct a TSecurityInfo setting it to the security attributes of aProcess.
@param aProcess A process.
*/
EXPORT_C TSecurityInfo::TSecurityInfo(RProcess aProcess)
	{
	Exec::ProcessSecurityInfo(aProcess.Handle(),*(SSecurityInfo*)this);
	}

/**
Construct a TSecurityInfo setting it to the security attributes to those of the process
owning the specified thread.
@param aThread A thread.
*/
EXPORT_C TSecurityInfo::TSecurityInfo(RThread aThread)
	{
	Exec::ThreadSecurityInfo(aThread.Handle(),*(SSecurityInfo*)this);
	}

/**
Construct a TSecurityInfo setting it to the security attributes of the process
which sent the message aMsgPtr
@param aMsgPtr a message
*/
EXPORT_C TSecurityInfo::TSecurityInfo(RMessagePtr2 aMsgPtr)
	{
	Exec::MessageSecurityInfo(aMsgPtr.Handle(),*(SSecurityInfo*)this);
	}

TInt TSecurityInfo::Set(RSessionBase aSession)
	{
	return Exec::SessionSecurityInfo(aSession.Handle(),*(SSecurityInfo*)this);
	}

/**
Sets this TSecurityInfo to the security attributes of this process' creator.
*/
EXPORT_C void TSecurityInfo::SetToCreatorInfo()
	{
	Exec::CreatorSecurityInfo(*(SSecurityInfo*)this);
	}

#endif //__KERNEL_MODE__

/**
Construct a set consisting of two capabilities.
@param aCapability1 The first capability.
@param aCapability2 The second capability.
*/
EXPORT_C TCapabilitySet::TCapabilitySet(TCapability aCapability1, TCapability aCapability2)
	{
	SetEmpty();
	AddCapability(aCapability1);
	AddCapability(aCapability2);
	}

/**
Make this set empty. I.e. Containing no capabilities.
*/
EXPORT_C void TCapabilitySet::SetEmpty()
	{
	memset(iCaps,0,sizeof(iCaps));
	}


/**
Make this set consist of all capabilities supported by this OS version.
*/
EXPORT_C void TCapabilitySet::SetAllSupported()
	{
	*(SCapabilitySet*)&iCaps=AllSupportedCapabilities;
	}

#ifndef __KERNEL_MODE__
// Documented in header file
EXPORT_C void TCapabilitySet::SetDisabled()
	{
	Exec::DisabledCapabilities(*(SCapabilitySet*)this);
	}
#endif // __KERNEL_MODE__

/**
Add a single capability to the set.
If the capability is not supported by this OS version then it is not added and
the set is left unchanged.
@see TCapabilitySet::SetAllSupported()
@param aCapability Capability to add.
*/
EXPORT_C void TCapabilitySet::AddCapability(TCapability aCapability)
	{
	if((TUint32)aCapability<(TUint32)ECapability_Limit)
		{
		TInt index = aCapability>>3;
		TUint8 mask = (TUint8)(1<<(aCapability&7));
		mask &= ((TUint8*)&AllSupportedCapabilities)[index];
		((TUint8*)iCaps)[index] |= mask;
		}
	}

/**
Remove a single capability from the set, if it is present.
@param aCapability Capability to remove.
*/
EXPORT_C void TCapabilitySet::RemoveCapability(TCapability aCapability)
	{
	if((TUint32)aCapability<(TUint32)ECapability_Limit)
		{
		TInt index = aCapability>>3;
		TUint8 mask = (TUint8)(1<<(aCapability&7));
		((TUint8*)iCaps)[index] &= ~mask;
		}
	}

/**
Perform a union of this capability set with another.
The result replaces the content of 'this'.
@param aCapabilities A cpability set
*/
EXPORT_C void TCapabilitySet::Union(const TCapabilitySet& aCapabilities)
	{
	for(TInt n = (ECapability_Limit-1)>>5; n>=0; n--)
		iCaps[n] |= aCapabilities.iCaps[n];
	}

/**
Perform an intersection of this capability set with another.
The result replaces the content of 'this'.
@param aCapabilities A capability set
*/
EXPORT_C void TCapabilitySet::Intersection(const TCapabilitySet& aCapabilities)
	{
	for(TInt n = (ECapability_Limit-1)>>5; n>=0; n--)
		iCaps[n] &= aCapabilities.iCaps[n];
	}

/**
Remove a set of capabilities from this set.
@param aCapabilities The set of capabilities to remove
*/
EXPORT_C void TCapabilitySet::Remove(const TCapabilitySet& aCapabilities)
	{
	for(TInt n = (ECapability_Limit-1)>>5; n>=0; n--)
		iCaps[n] &= ~aCapabilities.iCaps[n];
	}

/**
Test if a single capability is present in the set.
The capability ECapability_None is always treated as being present.
@param aCapability The capability to test
@return 1 if the capability is present, 0 if it is not.
*/
EXPORT_C TBool TCapabilitySet::HasCapability(TCapability aCapability) const
	{
	if((TUint32)aCapability<(TUint32)ECapability_Limit)
		return (((TUint8*)iCaps)[aCapability>>3]>>(aCapability&7))&1;
	// coverity[dead_error_condition]
	if(aCapability==ECapability_None)
		return ETrue;
	return EFalse;  // Handles illegal argument and ECapability_Denied
	}

/**
Test if all the capabilities in a given set are present in this set
@param aCapabilities The capability set to test
@return A non-zero value if all the capabilities are present, zero otherwise.
*/
EXPORT_C TBool TCapabilitySet::HasCapabilities(const TCapabilitySet& aCapabilities) const
	{
	TUint32 checkFail=0;
	for(TInt n = (ECapability_Limit-1)>>5; n>=0; n--)
		checkFail |= aCapabilities.iCaps[n]&~iCaps[n];
	return checkFail?0:1;
	}

// Documented in header file
TBool TCapabilitySet::NotEmpty() const
	{
	TUint32 notEmpty=0;
	for(TInt n = (ECapability_Limit-1)>>5; n>=0; n--)
		notEmpty |= iCaps[n];
	return notEmpty;
	}

//ECapability_None is assumed to be -1 in the internals of TSecurityPolicy
__ASSERT_COMPILE(ECapability_None == -1);

/** Constructs a TSecurityPolicy to either always pass or always fail checks made
against it, depending on the value of aType.
@param aType Must be one of EAlwaysPass or EAlwaysFail
@panic USER 191 if aType is not a valid value
*/
EXPORT_C TSecurityPolicy::TSecurityPolicy(TSecPolicyType aType)
	: iType((TUint8)aType), iSecureId(TUint32(ECapability_None))
	{
	//This constructor uses TSecPolicyType as public alias for the internal
	//TType.  Thus EAlwaysFail must have the same value as ETypeFail (same with the
	//pass case too).
	__ASSERT_COMPILE(EAlwaysFail == (TSecPolicyType)ETypeFail);
	__ASSERT_COMPILE(EAlwaysPass == (TSecPolicyType)ETypePass);

	__ASSERT_ALWAYS(aType == EAlwaysFail || aType == EAlwaysPass, Panic(ETSecPolicyTypeInvalid));
	iCaps[0] = (TUint8)ECapability_None;
	iCaps[1] = (TUint8)ECapability_None;
	iCaps[2] = (TUint8)ECapability_None;
	}

/** Construct a TSecurityPolicy object to check up to 3 capabilties.
@param aCap1 The first capability to add to this policy
@param aCap2 An optional second capability to add to this policy
@param aCap3 An optional third capability to add to this policy
@panic USER 189 If any of the supplied capabilities are not valid.
*/
EXPORT_C TSecurityPolicy::TSecurityPolicy(TCapability aCap1, TCapability aCap2, TCapability aCap3)
	//iSecureId=0xFFFFFFFF sets iExtraCaps[0-3] each to ECapability_None (==0xFF)
	: iType(ETypeC3), iSecureId(TUint32(ECapability_None))
	{
	ConstructAndCheck3(aCap1, aCap2, aCap3);
	}

/** Construct a TSecurityPolicy object to check up to 7 capabilties.
@param aCap1 The first capability to add to this policy
@param aCap2 The second capability to add to this policy
@param aCap3 The third capability to add to this policy
@param aCap4 The fourth capability to add to this policy
@param aCap5 An optional fifth capability to add to this policy
@param aCap6 An optional sixth capability to add to this policy
@param aCap7 An optional seventh capability to add to this policy
@panic USER 189 If any of the supplied capabilities are not valid.
*/
EXPORT_C TSecurityPolicy::TSecurityPolicy(TCapability aCap1, TCapability aCap2, 
	TCapability aCap3, TCapability aCap4, TCapability aCap5, TCapability aCap6, TCapability aCap7)
	: iType(ETypeC7)  
	{
	ConstructAndCheck3(aCap1, aCap2, aCap3);
	__ASSERT_COMPILE(ECapability_None==-1); // Our argument check below assumes this
	__ASSERT_ALWAYS(  (TUint)(aCap4+1)<=(TUint)ECapability_Limit
					&&(TUint)(aCap5+1)<=(TUint)ECapability_Limit
					&&(TUint)(aCap6+1)<=(TUint)ECapability_Limit
					&&(TUint)(aCap7+1)<=(TUint)ECapability_Limit
					,Panic(ECapabilityInvalid));
	iExtraCaps[0] = (TUint8)aCap4;
	iExtraCaps[1] = (TUint8)aCap5;
	iExtraCaps[2] = (TUint8)aCap6;
	iExtraCaps[3] = (TUint8)aCap7;
	}

/** Construct a TSecurityPolicy object to check a secure id and up to 3 capabilties.
@param aSecureId The secure id to add to this policy
@param aCap1 The first capability to add to this policy
@param aCap2 The second capability to add to this policy
@param aCap3 The third capability to add to this policy
@panic USER 189 If any of the supplied capabilities are not valid.
*/
EXPORT_C TSecurityPolicy::TSecurityPolicy(TSecureId aSecureId, 
	TCapability aCap1, TCapability aCap2, TCapability aCap3)
	: iType(ETypeS3), iSecureId(aSecureId)
	{
	ConstructAndCheck3(aCap1, aCap2, aCap3);
	}

/** Construct a TSecurityPolicy object to check a vendor id and up to 3 capabilties.
@param aVendorId The vendor id to add to this policy
@param aCap1 The first capability to add to this policy
@param aCap2 The second capability to add to this policy
@param aCap3 The third capability to add to this policy
@panic USER 189 If any of the supplied capabilities are not valid.
*/
EXPORT_C TSecurityPolicy::TSecurityPolicy(TVendorId aVendorId, 
	TCapability aCap1, TCapability aCap2, TCapability aCap3)
	: iType(ETypeV3), iVendorId(aVendorId)
	{
	ConstructAndCheck3(aCap1, aCap2, aCap3);
	}

/** Sets up iCaps[0-2] with supplied values and checks for their validity.
@panic USER 189 If any of the supplied capabilities are invalid.
*/
void TSecurityPolicy::ConstructAndCheck3(TCapability aCap1, TCapability aCap2, TCapability aCap3)
	{
	__ASSERT_COMPILE(ECapability_None==-1); // Our argument check below assumes this
	__ASSERT_ALWAYS(  (TUint)(aCap1+1)<=(TUint)ECapability_Limit
					&&(TUint)(aCap2+1)<=(TUint)ECapability_Limit
					&&(TUint)(aCap3+1)<=(TUint)ECapability_Limit
					,Panic(ECapabilityInvalid));
	iCaps[0] = (TUint8)aCap1;
	iCaps[1] = (TUint8)aCap2;
	iCaps[2] = (TUint8)aCap3;
	}

/**
Checks that this object is in a valid state
@return A non-zero value if this object is valid, zero otherwise.
@internalComponent
*/
TBool TSecurityPolicy::Validate() const
	{
	switch(iType)
		{
		case ETypeFail:
		case ETypePass:
			if(iSecureId!=TUint32(ECapability_None))
				return EFalse;
			__ASSERT_COMPILE(TUint8(ECapability_None)==0xffu); // Test below assumes this...
			if((iCaps[0]&iCaps[1]&iCaps[2])!=TUint8(ECapability_None)) // check caps 0 to 2 are each == ECapability_None
				return EFalse;
			return ETrue;

		case ETypeC7:
			return ETrue;

		case ETypeC3:
			if(iSecureId!=TUint32(ECapability_None))
				return EFalse;
			return ETrue;

		case ETypeS3:
		case ETypeV3:
			return ETrue;

		default:
			return EFalse;
		}
	}

/** Sets this TSecurityPolicy to a copy of the policy described by the
supplied descriptor. Such a descriptor can be obtained from
TSecurityPolicy::Package().
@see TSecurityPolicy::Package()
@param aDes A descriptor representing the state of another TSecurityPolicy.
@return KErrNone, if successful, otherwise one of the other system-wide error
codes.
*/
EXPORT_C TInt TSecurityPolicy::Set(const TDesC8& aDes)
	{
	if(aDes.Size() == sizeof(TSecurityPolicy))
		{
		*this = *(TSecurityPolicy*)aDes.Ptr();
		if(Validate())
			return KErrNone;
		}
	// Set failed so set up the policy as an EAlwaysFail case.
	iType = EAlwaysFail;
	iCaps[0] = TUint8(ECapability_None);
	iCaps[1] = TUint8(ECapability_None);
	iCaps[2] = TUint8(ECapability_None);
	iSecureId = TUint32(ECapability_None);
	return KErrArgument;
	}

/** 
Constructs a TPtrC8 wrapping the platform security attributes of this
TSecurityPolicy.  Such a descriptor is suitable for passing across the
client server boundary.

The format of the descriptor is determined by the first byte which specifies 
the type of this TSecurityPolicy.  The first byte is one of the constants
specified in the enum TSecurityPolicy::TType.

For TSecurityPolicy objects of types ETypeC3, ETypeS3, ETypePass or ETypeFail
the descriptor will contain the following data in the order listed:
@code
	TUint8 iType; 		// set to ETypeC3, ETypeS3, ETypePass or ETypeFail
	TUint8 iCaps[3];
	TUint32 iSecureId;
@endcode
ETypeC3 descriptors will contain capabilities in iCaps but have iSecureId set
to ECapability_None.  ETypeS3 are similar to ETypeC3 descriptors but will have
iSecureId set to the secure ID value of the TSecurityPolicy object.
ETypePass and ETypeFail objects will have values of all of the elements of iCaps
and iSecureId set to ECapability_None.

For TSecurityPolicy objects of type ETypeV3 the descriptor will contain the
following data in the order listed:
@code
	TUint8 iType;		// set to ETypeV3
	TUint8 iCaps[3];	// set to the values of 3 capabilities
	TUint32 iVendorId;	// set to the value of the vendor ID of the TSecurityPolicy
@endcode

For TSecurityPolicy objects of type ETypeC7 the descriptor will contain the
following data in the order listed:
@code
	TUint8 iType;			// set to ETypeC7
	TUint8 iCaps[3];		// set to the values of 3 of the objects capabilities
	TUint8 iExtraCaps[4];	// set to the values of 4 of the objects capabilities
@endcode
@see TSecurityPolicy::TType
@see TSecurityPolicy::Set()
@return A TPtrC8 wrapping the platform security attributes of this TSecurityPolicy.
*/
EXPORT_C TPtrC8 TSecurityPolicy::Package() const
	{
	return TPtrC8((TUint8*)(this), sizeof(TSecurityPolicy));
	}

/** Checks this policy against the supplied SSecurityInfo.
@param aSecInfo The SSecurityInfo object to check against this TSecurityPolicy.
@param aMissing A SSecurityInfo object which this method fills with any capabilities or IDs
				it finds to be missing. This is designed to help generating diagnostic messages.
@return ETrue if all the requirements of this TSecurityPolicy are met, EFalse
@panic USER 190 if aSecInfo is an invalid SSecurityInfo object
otherwise.
*/
TBool TSecurityPolicy::CheckPolicy(const SSecurityInfo& aSecInfo, SSecurityInfo& aMissing) const
	{
	TBool result = EFalse;
	//It is thought to be by far the most common case to have 3 or less
	//capabilities in a policy.  Hence we'll set this for all of them even
	//though ETypePass doesn't need it.
	aMissing.iSecureId = 0;
	aMissing.iVendorId = 0;
	__ASSERT_COMPILE(SCapabilitySet::ENCapW == 2);
	aMissing.iCaps[0] = 0;
	aMissing.iCaps[1] = 0;
	aMissing.iCaps.AddCapability((TCapability)(iCaps[0]));
	aMissing.iCaps.AddCapability((TCapability)(iCaps[1]));
	aMissing.iCaps.AddCapability((TCapability)(iCaps[2]));
	aMissing.iCaps.Remove(aSecInfo.iCaps);
	switch(iType)
		{
		case ETypeFail:
			//result already False;
			break;
		case ETypePass:
			result = ETrue;	
			break;
		case ETypeC7:
			aMissing.iCaps.AddCapability((TCapability)(iExtraCaps[0]));
			aMissing.iCaps.AddCapability((TCapability)(iExtraCaps[1]));
			aMissing.iCaps.AddCapability((TCapability)(iExtraCaps[2]));
			aMissing.iCaps.AddCapability((TCapability)(iExtraCaps[3]));
			aMissing.iCaps.Remove(aSecInfo.iCaps);
			//It is intentional that there is no break statement here
		case ETypeC3:
			if(!aMissing.iCaps.NotEmpty())
				{
				result = ETrue;
				}
			break;
		case ETypeS3:
			if(!aMissing.iCaps.NotEmpty() && iSecureId == aSecInfo.iSecureId)
				{
				result = ETrue;
				}
			//This else if required to set the aMissing.iCaps secure id for diagnostics.
			//Doesn't affect pass case.
			else if(iSecureId != aSecInfo.iSecureId) 
				{
				aMissing.iSecureId = iSecureId;
				}
			break;
		case ETypeV3:
			if(!aMissing.iCaps.NotEmpty() && iVendorId == aSecInfo.iVendorId)
				{
				result = ETrue;
				}
			else if(iVendorId != aSecInfo.iVendorId)
				{
				aMissing.iVendorId = iVendorId;
				}
			break;
		default:
			Panic(ESecurityPolicyCorrupt);
			break;
		}
	return result;
	}

#ifndef __KERNEL_MODE__

/** Checks this policy against the platform security attributes of aProcess.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The RProcess object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(RProcess aProcess, const char* aDiagnostic) const
	{
	SSecurityInfo missing;
	TSecurityInfo secInfo(aProcess);
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), missing);
	if(!pass)
		pass = PlatSec::PolicyCheckFail(aProcess.Handle(),missing,aDiagnostic)==KErrNone;
	return pass;
	}
#else // !__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(RProcess aProcess, const char* /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aProcess);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/** Checks this policy against the platform security attributes of aProcess.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The RProcess object to check against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(RProcess aProcess) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoCheckPolicy(aProcess, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo missing;
	TSecurityInfo secInfo(aProcess);
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), missing);
	if(!pass)
		pass = (PlatSec::EmitDiagnostic() == KErrNone);
	return pass;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	}

/** Checks this policy against the platform security attributes of the process
owning aThread.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(RThread aThread, const char* aDiagnostic) const
	{
	SSecurityInfo missing;
	TSecurityInfo secInfo(aThread);
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), missing);
	if(!pass)
		pass = PlatSec::PolicyCheckFail(aThread.Handle(),missing,aDiagnostic)==KErrNone;
	return pass;
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(RThread aThread, const char* /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aThread);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/** Checks this policy against the platform security attributes of the process
owning aThread.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(RThread aThread) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoCheckPolicy(aThread, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo missing;
	TSecurityInfo secInfo(aThread);
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), missing);
	if(!pass)
		pass = (PlatSec::EmitDiagnostic() == KErrNone);
	return pass;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	}

TInt TSecurityPolicy::CheckPolicy(RSessionBase aSession) const
	{
	SSecurityInfo missing;
	TSecurityInfo secInfo;
	TInt r = secInfo.Set(aSession);
	if (r!=KErrNone)
		return r;
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), missing);
	if(!pass)
		{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
		r = PlatSec::PolicyCheckFail(aSession.Handle(),missing,NULL);
#else
		r = PlatSec::EmitDiagnostic();
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
		}
	return r;
	}

/** Checks this policy against the platform security attributes of the process which sent
the given message.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aMsgPtr The RMessagePtr2 object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aMsg, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(RMessagePtr2 aMsgPtr, const char* aDiagnostic) const
	{
	SSecurityInfo missing;
	TSecurityInfo secInfo(aMsgPtr);
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), missing);
	if(!pass)
		pass = PlatSec::PolicyCheckFail(aMsgPtr,missing,aDiagnostic)==KErrNone;
	return pass;
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(RMessagePtr2 aMsgPtr, const char* /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aMsgPtr);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/** Checks this policy against the platform security attributes of the process which sent
the given message.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aMsgPtr The RMessagePtr2 object to check against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aMsg, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(RMessagePtr2 aMsgPtr) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoCheckPolicy(aMsgPtr, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo missing;
	TSecurityInfo secInfo(aMsgPtr);
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), missing);
	if(!pass)
		pass = (PlatSec::EmitDiagnostic() == KErrNone);
	return pass;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	}

/** Checks this policy against the platform security attributes of the process which sent
the given message.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aMsgPtr The RMessagePtr2 object to check against this TSecurityPolicy.
@param aMissing A TSecurityInfo object which this method fills with any capabilities or IDs
				it finds to be missing. 
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aMsg, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
TBool TSecurityPolicy::DoCheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing, const char* aDiagnostic) const
	{
	TSecurityInfo secInfo(aMsgPtr);
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), *((SSecurityInfo*)&aMissing));
	if(!pass)
		pass = PlatSec::PolicyCheckFail(aMsgPtr,*((SSecurityInfo*)&aMissing),aDiagnostic)==KErrNone;
	return pass;
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/** Checks this policy against the platform security attributes of the process which sent
the given message.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aMsgPtr The RMessagePtr2 object to check against this TSecurityPolicy.
@param aMissing A TSecurityInfo object which this method fills with any capabilities or IDs
				it finds to be missing. 
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aMsg, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
TBool TSecurityPolicy::DoCheckPolicy(RMessagePtr2 aMsgPtr, TSecurityInfo& aMissing) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoCheckPolicy(aMsgPtr, aMissing, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	TSecurityInfo secInfo(aMsgPtr);
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), *((SSecurityInfo*)&aMissing));
	if(!pass)
		pass = (PlatSec::EmitDiagnostic() == KErrNone);
	return pass;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	}

/** Checks this policy against the platform security attributes of this process' creator.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The RProcess object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of this process' creator, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicyCreator(const char* aDiagnostic) const
	{
	SSecurityInfo missing;
	TSecurityInfo secInfo;
	secInfo.SetToCreatorInfo();
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), missing);
	if(!pass)
		pass = PlatSec::CreatorPolicyCheckFail(missing,aDiagnostic)==KErrNone;
	return pass;
	}
#else // !__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicyCreator(const char* /*aDiagnostic*/) const
	{
	return DoCheckPolicyCreator();
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/** Checks this policy against the platform security attributes of this process' creator.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The RProcess object to check against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of this process' creator, EFalse otherwise.
@panic USER 190 if 'this' is an invalid SSecurityInfo object
*/
EXPORT_C TBool TSecurityPolicy::DoCheckPolicyCreator() const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoCheckPolicyCreator(NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo missing;
	TSecurityInfo secInfo;
	secInfo.SetToCreatorInfo();
	TBool pass = CheckPolicy(*((SSecurityInfo*)&secInfo), missing);
	if(!pass)
		pass = (PlatSec::EmitDiagnostic() == KErrNone);
	return pass;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	}

#else //__KERNEL_MODE__

/** Checks this policy against the platform security attributes of aProcess.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The DProcess object to check against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
@panic KERN-COMMON 190 if 'this' is an invalid SSecurityInfo object
*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(DProcess* aProcess, const char* aDiagnostic) const
	{
	SSecurityInfo missing;
	TBool pass = CheckPolicy(aProcess->iS, missing);
	if(!pass)
		pass = PlatSec::PolicyCheckFail(aProcess,missing,aDiagnostic)==KErrNone;
	return pass;
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(DProcess* aProcess, const char* /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aProcess);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/** Checks this policy against the platform security attributes of aProcess.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aProcess The DProcess object to check against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
@panic KERN-COMMON 190 if 'this' is an invalid SSecurityInfo object
*/
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(DProcess* aProcess) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoCheckPolicy(aProcess, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo missing;
	TBool pass = CheckPolicy(aProcess->iS, missing);
	if(!pass)
		pass = (PlatSec::EmitDiagnostic() == KErrNone);
	return pass;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	}

/** Checks this policy against the platform security attributes of the process
owning aThread.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@param aDiagnostic A string that will be emitted along with any diagnostic message
							that may be issued if the policy check fails.
							This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
							which enables it to be easily removed from the system.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
@panic KERN-COMMON 190 if 'this' is an invalid SSecurityInfo object
*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(DThread* aThread, const char* aDiagnostic) const
	{
	SSecurityInfo missing;
	TBool pass = CheckPolicy(aThread->iOwningProcess->iS, missing);
	if(!pass)
		pass = PlatSec::PolicyCheckFail(aThread,missing,aDiagnostic)==KErrNone;
	return pass;
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(DThread* aThread, const char* /*aDiagnostic*/) const
	{
	return DoCheckPolicy(aThread);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/** Checks this policy against the platform security attributes of the process
owning aThread.

	When a check fails the action taken is determined by the system wide Platform Security
	configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
	If PlatSecEnforcement is OFF, then this function will return ETrue even though the
	check failed.

@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
@panic KERN-COMMON 190 if 'this' is an invalid SSecurityInfo object
*/
EXPORT_C TBool TSecurityPolicy::DoCheckPolicy(DThread* aThread) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoCheckPolicy(aThread, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo missing;
	TBool pass = CheckPolicy(aThread->iOwningProcess->iS, missing);
	if(!pass)
		pass = (PlatSec::EmitDiagnostic() == KErrNone);
	return pass;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	}

#endif // !__KERNEL_MODE__


#ifndef __KERNEL_MODE__

EXPORT_C TInt PlatSec::ConfigSetting(TConfigSetting aSetting)
	{
	TUint32 flags = Exec::KernelConfigFlags();
	switch(aSetting)
		{
		case EPlatSecEnforcement:
			flags &= EKernelConfigPlatSecEnforcement;
			break;
		case EPlatSecDiagnotics:
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
			flags &= EKernelConfigPlatSecDiagnostics;
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
			flags=0;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
			break;
		case EPlatSecProcessIsolation:
			flags &= EKernelConfigPlatSecProcessIsolation;
			break;
		case EPlatSecEnforceSysBin:
			flags &= EKernelConfigPlatSecEnforceSysBin;
			break;
		case EPlatSecLocked:
			flags &= EKernelConfigPlatSecLocked;
			break;
		default:
			flags = 0;
			break;
		}
	if(flags)
		flags = 1;
	return flags;
	}

EXPORT_C TBool PlatSec::IsCapabilityEnforced(TCapability aCapability)
	{
	if(!((TCapabilitySet&)AllSupportedCapabilities).HasCapability(aCapability))
		return EFalse;

	SCapabilitySet disabled;
	Exec::DisabledCapabilities(disabled);
	if(((TCapabilitySet&)disabled).HasCapability(aCapability))
		return EFalse;

	return PlatSec::ConfigSetting(EPlatSecEnforcement);
	}

#endif // Not __KERNEL_MODE__
