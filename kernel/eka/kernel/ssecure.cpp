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
// \e32\kernel\ssecure.cpp
// 
//

#define __INCLUDE_CAPABILITY_NAMES__
#define __INCLUDE_ALL_SUPPORTED_CAPABILITIES__
#include <kernel/kern_priv.h>
#include "execs.h"
#include <e32cmn.h>
#include <e32cmn_private.h>

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__

class DiagnosticMessage : private TBuf8<1024>
	{
public:
	void Append(char aChar)
		{
		if((TUint)Length()<(TUint)iMaxLength)
			TDes8::Append((TChar)aChar);
		}

	void Append(const TUint8* aString,TInt aLength)
		{
		TInt len=iMaxLength-Length();
		if(len>aLength)
			len=aLength;
		TDes8::Append(aString, len);
		}

	void Append(const TDesC8& aString)
		{
		Append(aString.Ptr(), aString.Length());
		}

	void Append(const char* aString)
		{
		const char* end = aString;
		while(*end) ++end;
		Append((const TUint8*)aString, end-aString);
		}

	void AppendKU(const TUint8* aString,TInt aLength,TBool aCalledFromKernel)
		{
		if(aCalledFromKernel)
			{
			Append(aString,aLength);
			return;
			}
		TUint8* dPtr=iBuf;
		TInt dLen=Length();
		dPtr += dLen;
		TInt len = iMaxLength-dLen;
		if(len>aLength)
			len = aLength;
		SetLength(dLen+len);
		kumemget(dPtr,aString,len);
		}

	void AppendKU(const TDesC8& aSrc,TBool aCalledFromKernel)
		{
		if(aCalledFromKernel)
			{
			Append(aSrc);
			return;
			}
		TInt sLen,sMax;
		const TUint8* sPtr=Kern::KUDesInfo(aSrc, sLen, sMax);
		AppendKU(sPtr,sLen,aCalledFromKernel);
		}

	void AppendCapability(TCapability aCapability)
		{
		if (aCapability >= 0 && aCapability < ECapability_Limit)
			Append(CapabilityNames[aCapability]);
		else if (aCapability == ECapability_Denied)
			Append("(Denied)");
		else
			Append("(Unknown)");
		}

	void AppendCapabilities(const SCapabilitySet& aCaps)
		{
		for(TInt i=0; i<ECapability_Limit; i++)
			{
			if(aCaps[i>>5] & (1<<(i&31)))
				{
				Append(CapabilityNames[i]);
				Append(" ");
				}
			}
		}

	void AppendObject(DObject* aObject)
		{
		DServer* server = NULL;
		TObjectType type=(TObjectType)(aObject->iContainerID-1);
		if (type==ESession)
			{
			Append("A session on ");
			server = ((DSession*)aObject)->iServer;
			if (server!=NULL)
				{
				if (server->Open()==KErrNone)
					{
					type=EServer;
					aObject = server;
					}
				else
					server = NULL;
				}
			if (server==NULL)
				{
				Append("unknown server");
				return;
				}
			}
		TFullName name;
		if(!NKern::HeldFastMutex())
			aObject->AppendFullName(name);
		else
			aObject->TraceAppendFullName(name,ETrue);
		const char* typeName;
		switch(type)
			{
			case EThread: typeName="Thread "; break;
			case EProcess: typeName="Process "; break;
			case EServer: typeName="Server "; break;
			default: typeName="";
			}
		Append(typeName);
		Append(name);
		if (server)
			server->Close(NULL);
		}

	void AppendObject(TInt aHandle)
		{
		DObject* pO=NULL;
		TInt r=K::OpenObjectFromHandle(aHandle,pO);		// Puts us in critical section if successful
		if (r!=KErrNone)
			K::PanicKernExec(EBadHandle);
		AppendObject(pO);
		pO->Close(NULL);
		NKern::ThreadLeaveCS();
		}

	void AppendHex(TUint aVal)
		{
		for(TInt e=8; e>0; --e)
			{
			TUint d=(aVal>>28);
			aVal <<= 4;
			if(d>9)
				d += 'a'-'9'-1;
			d += '0';
			Append((char)d);
			}
		}

	void AppendMessage(TInt aHandle)
		{
		NKern::LockSystem();
		RMessageK* pM = RMessageK::MessageK(aHandle);
		DThread* pT = pM->iClient;
		pT->Open();
		DServer* pS = pM->iSession->iServer;
		pS->Open();
		TInt fn=pM->iFunction;
		NKern::ThreadEnterCS();
		NKern::UnlockSystem();
		Append("A Message (function number=0x");
		AppendHex(fn);
		Append(") from ");
		AppendObject(pT);
		Append(", sent to ");
		AppendObject(pS);
		Append(",");
		pS->Close(NULL);
		pT->Close(NULL);
		NKern::ThreadLeaveCS();
		}

	void AppendMessageK(RMessageK* aMsg)
		{
		// Enter and leave with System Lock held
		DThread* pT = aMsg->iClient;
		DServer* pS = aMsg->iSession->iServer;
		TInt fn=aMsg->iFunction;
		Append("A Message (function number=0x");
		AppendHex(fn);
		Append(") from ");
		AppendObject(pT);
		Append(", sent to ");
		AppendObject(pS);
		Append(",");
		}

	void AppendFailed()
		{
		if(TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecEnforcement)
			Append("failed - ");
		else
			Append("would have failed - ");
		}

	void AppendCapabilityCheck()
		{
		Append("Capability check ");
		AppendFailed();
		}

	void AppendProcessIsolationCheck()
		{
		Append("Process Isolation check ");
		AppendFailed();
		}

	void AppendPolicyCheck(const TPlatSecDiagnostic& aDiagnostic)
		{
		if(aDiagnostic.iSecurityInfo.iSecureId|aDiagnostic.iSecurityInfo.iVendorId)
			{
			Append("Security Policy Check ");
			AppendFailed();
			}
		else
			AppendCapabilityCheck();
		}
	
	void AppendSecureIdCheck()
		{
		Append("Secure Id Check ");
		AppendFailed();
		}

	void Emit()
		{
		K::TextTrace(*this,EPlatSecTrace);
		Zero();
		}
	};

void DoEmitDiagnostic(TPlatSecDiagnostic& aDiagnostic,TBool aCalledFromKernel)
	{
	DiagnosticMessage message;
	message.Append("*PlatSec* ");
	TUint32 flags = TheSuperPage().KernelConfigFlags();
	if(flags&EKernelConfigPlatSecEnforcement)
		message.Append("ERROR - ");
	else
		message.Append("WARNING - ");

	switch(aDiagnostic.iType)
		{
	case TPlatSecDiagnostic::ELoaderCapabilityViolation1:
		message.AppendCapabilityCheck();
		if(aDiagnostic.iArg1)
			message.AppendObject(aDiagnostic.iArg1);
		else
			message.Append("<global>, EKern or EFile");
		message.Append(" can't load ");
		message.AppendKU(*aDiagnostic.String2(),aCalledFromKernel);
		message.Append(" because the following capabilities are missing: ");
		message.AppendCapabilities(aDiagnostic.iSecurityInfo.iCaps);
		break;

	case TPlatSecDiagnostic::ELoaderCapabilityViolation2:
		message.AppendCapabilityCheck();
		message.Append("Can't load ");
		message.AppendKU(*aDiagnostic.String1(),aCalledFromKernel);
		message.Append(" because it links to ");
		message.AppendKU(*aDiagnostic.String2(),aCalledFromKernel);
		message.Append(" which has the following capabilities missing: ");
		message.AppendCapabilities(aDiagnostic.iSecurityInfo.iCaps);
		break;

	case TPlatSecDiagnostic::EThreadCapabilityCheckFail:
		message.AppendCapabilityCheck();
		message.AppendObject((DObject*)aDiagnostic.iArg1);
		message.Append(" attempted an operation");
		if((DObject*)aDiagnostic.iArg1!=TheCurrentThread)
			{
			message.Append(" which was checked by ");
			message.AppendObject(TheCurrentThread);
			}
		message.Append(" and was found to be missing the capability ");
		message.AppendCapability((TCapability)aDiagnostic.iArg2);
		break;

	case TPlatSecDiagnostic::EProcessCapabilityCheckFail:
		message.AppendCapabilityCheck();
		message.AppendObject((DObject*)aDiagnostic.iArg1);
		message.Append(" attempted an operation");
		if((DObject*)aDiagnostic.iArg1!=TheCurrentThread->iOwningProcess)
			{
			message.Append(" which was checked by ");
			message.AppendObject(TheCurrentThread->iOwningProcess);
			}
		message.Append(" and was found to be missing the capability ");
		message.AppendCapability((TCapability)aDiagnostic.iArg2);
		break;

	case TPlatSecDiagnostic::EKernelSecureIdCheckFail:
		message.AppendSecureIdCheck();
		message.AppendObject((DObject*)aDiagnostic.iArg1);
		message.Append(" attempted an operation requiring the secure id: ");
		message.AppendHex((TUint32)aDiagnostic.iArg2);
		message.Append(".");
		break;

	case TPlatSecDiagnostic::EKernelObjectPolicyCheckFail:
		message.AppendPolicyCheck(aDiagnostic);
		message.AppendObject((DObject*)aDiagnostic.iArg1);
		goto capabilityCheckFail;

	case TPlatSecDiagnostic::ECreatorCapabilityCheckFail:
		message.AppendPolicyCheck(aDiagnostic);
		message.Append("The creator of the current process");
		goto capabilityCheckFail;

	case TPlatSecDiagnostic::EHandleCapabilityCheckFail:
		message.AppendPolicyCheck(aDiagnostic);
		message.AppendObject(aDiagnostic.iArg1);
		goto capabilityCheckFail;

	case TPlatSecDiagnostic::EMessageCapabilityCheckFail:
		message.AppendPolicyCheck(aDiagnostic);
		message.AppendMessage(aDiagnostic.iArg1);
		goto capabilityCheckFail;

	case TPlatSecDiagnostic::ECreatorPolicyCheckFail:
		message.AppendPolicyCheck(aDiagnostic);
		message.Append("The creator of the current process");
		// fall through
		
capabilityCheckFail:
		message.Append(" was checked by ");
		message.AppendObject(TheCurrentThread);
		message.Append(" and was found to be missing ");
		if(aDiagnostic.iSecurityInfo.iSecureId != 0)
			{
			message.Append("the SecureId: ");
			message.AppendHex(aDiagnostic.iSecurityInfo.iSecureId);
			if(aDiagnostic.iSecurityInfo.iCaps.NotEmpty())
				{
				message.Append(" and the capabilities: ");
				message.AppendCapabilities(aDiagnostic.iSecurityInfo.iCaps);
				}
			}
		else if(aDiagnostic.iSecurityInfo.iVendorId != 0)
			{
			message.Append("the VendorId: ");
			message.AppendHex(aDiagnostic.iSecurityInfo.iVendorId);
			if(aDiagnostic.iSecurityInfo.iCaps.NotEmpty())
				{
				message.Append(" and the capabilities: ");
				message.AppendCapabilities(aDiagnostic.iSecurityInfo.iCaps);
				}
			}
		else
			{
			if(aDiagnostic.iArg2 == ECapability_None)
				{
				if(aDiagnostic.iSecurityInfo.iCaps.NotEmpty())
					{
					message.Append("the capabilities: ");
					message.AppendCapabilities(aDiagnostic.iSecurityInfo.iCaps);
					}
				else
					message.Append("nothing. (An 'always fail' policy check)");
				}
			else
				{
				message.Append("the capability: ");
				message.AppendCapability((TCapability)aDiagnostic.iArg2);
				}
			}
		message.Append(".");
		break;
		
	case TPlatSecDiagnostic::EKernelProcessIsolationFail:
		message.AppendProcessIsolationCheck();
		message.AppendObject(&Kern::CurrentThread());
		message.Append(" attempted an prohibited operation.");
		break;

	case TPlatSecDiagnostic::EKernelProcessIsolationIPCFail:
		message.AppendProcessIsolationCheck();
		message.AppendMessageK((RMessageK*)aDiagnostic.iArg1);
		message.Append(" was used by ");
		message.AppendObject(TheCurrentThread);
		message.Append(" and found to have an invalid argument.");
		break;

	default:
		break;
		}

	if(aDiagnostic.iContextText)
		{
		message.Append("  Additional diagnostic message: ");
		message.AppendKU((const TUint8*)aDiagnostic.iContextText,aDiagnostic.iContextTextLength,aCalledFromKernel);
		}
	message.Emit();
	}

TInt PlatSec::EmitDiagnostic(TPlatSecDiagnostic& aDiagnostic, const char* aContextText)
	{
	TUint32 flags = TheSuperPage().KernelConfigFlags();
	TUint32 diag = flags & EKernelConfigPlatSecDiagnostics;
	TUint32 latency_test = TEST_DEBUG_MASK_BIT(KTESTLATENCY);
	if (diag && !latency_test && aContextText!=KSuppressPlatSecDiagnosticMagicValue)
		{
		aDiagnostic.iContextText = aContextText;
		if(aContextText)
			aDiagnostic.iContextTextLength=TPtrC8((const TUint8*)aContextText).Length();
		DoEmitDiagnostic(aDiagnostic,ETrue);
		}
	return flags&EKernelConfigPlatSecEnforcement ? KErrPermissionDenied : KErrNone;
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

TInt PlatSec::EmitDiagnostic()
	{
	return (TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecEnforcement) ? KErrPermissionDenied : KErrNone;
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
TInt ExecHandler::PlatSecDiagnostic(TPlatSecDiagnostic* aDiagnostic)
	{
	TUint32 flags = TheSuperPage().KernelConfigFlags();
	TUint32 diag = flags & EKernelConfigPlatSecDiagnostics;
	TUint32 latency_test = TEST_DEBUG_MASK_BIT(KTESTLATENCY);
	if (diag && !latency_test && aDiagnostic)
		{
		TPlatSecDiagnostic d;
		kumemget(&d,aDiagnostic,sizeof(d));
		DoEmitDiagnostic(d,EFalse);
		}
	return flags&EKernelConfigPlatSecEnforcement ? KErrPermissionDenied : KErrNone;
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
TInt ExecHandler::PlatSecDiagnostic(TPlatSecDiagnostic* /*aDiagnostic*/)
	{
	return (TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecEnforcement) ? KErrPermissionDenied : KErrNone;
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool DProcess::DoHasCapability(TCapability aCapability, const char* aContextText)
	{
	if (HasCapabilityNoDiagnostic(aCapability))
		return ETrue;
	return PlatSec::CapabilityCheckFail(this,aCapability,aContextText)==KErrNone;
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool DProcess::DoHasCapability(TCapability aCapability, const char* /*aContextText*/)
	{
	return DoHasCapability(aCapability);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool DProcess::DoHasCapability(TCapability aCapability)
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoHasCapability(aCapability, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	if (HasCapabilityNoDiagnostic(aCapability))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool DThread::DoHasCapability(TCapability aCapability, const char* aContextText)
	{
	if(iOwningProcess->HasCapabilityNoDiagnostic(aCapability))
		return ETrue;
	return PlatSec::CapabilityCheckFail(this,aCapability,aContextText)==KErrNone;
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool DThread::DoHasCapability(TCapability aCapability, const char* /*aContextText*/)
	{
	return DoHasCapability(aCapability);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool DThread::DoHasCapability(TCapability aCapability)
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoHasCapability(aCapability, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	if (iOwningProcess->HasCapabilityNoDiagnostic(aCapability))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	}


#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
/**
Checks whether the process that owns the current thread has the specified capability.

When a check fails the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then this function will return True even though the
check failed.

@param aCapability The capability to be tested.
@param aContextText A string that will be emitted along with any diagnostic message
                   that may be issued if the test finds the capability is not present.
				   This string must be enclosed in the __PLATSEC_DIAGNOSTIC_STRING macro
				   which enables it to be easily removed from the system.

@return True if the current thread's process has the capability, False otherwise.
*/
EXPORT_C TBool Kern::DoCurrentThreadHasCapability(TCapability aCapability, const char* aContextText)
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	return TheCurrentThread->HasCapability(aCapability, aContextText);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	return TheCurrentThread->DoHasCapability(aCapability, aContextText);
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
/**
Checks whether the process that owns the current thread has the specified capability.

When a check fails the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then this function will return True even though the
check failed.

@param aCapability The capability to be tested.

@return True if the current thread's process has the capability, False otherwise.
*/
EXPORT_C TBool Kern::DoCurrentThreadHasCapability(TCapability aCapability, const char* /*aContextText*/)
	{
	return TheCurrentThread->HasCapability(aCapability);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/**
Checks whether the process that owns the current thread has the specified capability.

When a check fails the action taken is determined by the system wide Platform Security
configuration. If PlatSecDiagnostics is ON, then a diagnostic message is emitted.
If PlatSecEnforcement is OFF, then this function will return True even though the
check failed.

@param aCapability The capability to be tested.
@return True if the current thread's process has the capability, False otherwise.
*/
EXPORT_C TBool Kern::DoCurrentThreadHasCapability(TCapability aCapability)
	{
	return TheCurrentThread->HasCapability(aCapability);
	}

void TCompiledSecurityPolicy::AddCapability(TCapability aCap)
	{
    if((TUint32)aCap<(TUint32)KCSPBitsFree)
        { 
		TUint32 mask = 1<<(aCap);              
		iCaps |= mask&AllSupportedCapabilities[0]; 
		} 
	} 
	
//The inards of TCompiledSecurityPolicy assume this.
__ASSERT_COMPILE(ECapability_None == -1);

TInt TCompiledSecurityPolicy::Set(const TSecurityPolicy& aPolicy)
	{
	iSecureId = 0;
	iCaps = 0;
	__ASSERT_COMPILE(ETypeFail == 0);
	//iCaps = 0 sets the type of the object to ETypeFail as well.
	//This ensures that if the set fails, the object is in a ETypeFail case.

	if(!aPolicy.Validate())
		return KErrArgument;

	switch (aPolicy.iType)
		{
		case TSecurityPolicy::ETypeFail:
			SetETypeFail();
			break;

		case TSecurityPolicy::ETypePass:
			SetETypePass();
			break;

		case TSecurityPolicy::ETypeC7:
			AddCapability(TCapability(aPolicy.iExtraCaps[0]));
			AddCapability(TCapability(aPolicy.iExtraCaps[1]));
			AddCapability(TCapability(aPolicy.iExtraCaps[2]));
			AddCapability(TCapability(aPolicy.iExtraCaps[3]));
			// Fall through to 3 caps case...
		case TSecurityPolicy::ETypeC3:
			SetETypeCapsOnly();
			AddCapability(TCapability(aPolicy.iCaps[0]));
			AddCapability(TCapability(aPolicy.iCaps[1]));
			AddCapability(TCapability(aPolicy.iCaps[2]));
			break;

		case TSecurityPolicy::ETypeS3:
			SetETypeSecureId();
			iSecureId =	aPolicy.iSecureId;
			AddCapability(TCapability(aPolicy.iCaps[0]));
			AddCapability(TCapability(aPolicy.iCaps[1]));
			AddCapability(TCapability(aPolicy.iCaps[2]));
			break;

		case TSecurityPolicy::ETypeV3:
			SetETypeVendorId();
			iVendorId =	aPolicy.iVendorId;
			AddCapability(TCapability(aPolicy.iCaps[0]));
			AddCapability(TCapability(aPolicy.iCaps[1]));
			AddCapability(TCapability(aPolicy.iCaps[2]));
			break;

		default:
			// Can't get here because aPolicy.Validate() call would detect this
			break;
		}
	return KErrNone;
	}

/** Checks this policy against the supplied SSecurityInfo.
@param aSecInfo The SSecurityInfo object to check against this TCompiledSecurityPolicy.
@param aMissing Contains a list of the missing security attributes if the
policy check fails.  This parameter is untouched if the policy check passes.
@return ETrue if all the requirements of this TSecurityPolicy are met, EFalse
otherwise.
*/
TBool TCompiledSecurityPolicy::CheckPolicy(const SSecurityInfo& aSecInfo, SSecurityInfo& aMissing) const
	{
//In the pass case, checking only against capabilities.  This function should
//be quite fast.  Most of the code here is for the failure case when aMissing
//has to be filled in.
	__ASSERT_COMPILE(SCapabilitySet::ENCapW == 2);
	__ASSERT_COMPILE(ECapability_Limit <= KCSPBitsFree);
	TUint32 requiredCaps = Caps() & ~aSecInfo.iCaps[0];
	switch(Type())
		{
		case ETypeFail:
			aMissing.iSecureId = 0;
			aMissing.iVendorId = 0;
			aMissing.iCaps[0] = 0;
			aMissing.iCaps[1] = 0;
			break;

		case ETypeCapsOnly: //this case handles the pass always case
			if(!requiredCaps)
				return  ETrue;
			aMissing.iSecureId = 0;
			aMissing.iVendorId = 0;
			aMissing.iCaps[0] = requiredCaps;
			aMissing.iCaps[1] = 0;
			break;

		case ETypeSecureId:
			if(!requiredCaps && iSecureId == aSecInfo.iSecureId)
				return ETrue;
			aMissing.iSecureId = aSecInfo.iSecureId;
			aMissing.iVendorId = 0;
			aMissing.iCaps[0] = requiredCaps;
			aMissing.iCaps[1] = 0;
			break;

		case ETypeVendorId:
			if(!requiredCaps && iVendorId == aSecInfo.iVendorId)
				return ETrue;
			aMissing.iVendorId = aSecInfo.iVendorId;
			aMissing.iSecureId = 0;
			aMissing.iCaps[0] = requiredCaps;
			aMissing.iCaps[1] = 0;
			break;

		default:
			__ASSERT_COMPILE(sizeof(iCaps)*8 - KCSPBitsFree == 2);
			//not possible.  Type() only returns 2 bits
			break;
		}
	return EFalse;
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
/** Checks this policy against the platform security attributes of aProcess.
@param aProcess The RProcess object to check against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
*/
TBool TCompiledSecurityPolicy::DoCheckPolicy(DProcess* aProcess, const char* aDiagnostic) const
	{
	SSecurityInfo missing;
	TBool pass = CheckPolicy(aProcess->iS, missing);
	if(!pass)
		pass = PlatSec::PolicyCheckFail(aProcess,missing,aDiagnostic)==KErrNone;
	return pass;
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/** Checks this policy against the platform security attributes of aProcess.
@param aProcess The RProcess object to check against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security attributes of aProcess, EFalse otherwise.
*/
TBool TCompiledSecurityPolicy::DoCheckPolicy(DProcess* aProcess) const
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

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
/** Checks this policy against the platform security attributes of the process
owning aThread.
@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
*/
TBool TCompiledSecurityPolicy::DoCheckPolicy(DThread* aThread, const char* aDiagnostic) const
	{
	SSecurityInfo missing;
	TBool pass = CheckPolicy(aThread->iOwningProcess->iS, missing);
	if(!pass)
		pass = PlatSec::PolicyCheckFail(aThread,missing,aDiagnostic)==KErrNone;
	return pass;
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

/** Checks this policy against the platform security attributes of the process
owning aThread.
@param aThread The thread whose owning process' platform security attributes
are to be checked against this TSecurityPolicy.
@return ETrue if all the requirements of this TSecurityPolicy are met by the
platform security parameters of the owning process of aThread, EFalse otherwise.
*/
TBool TCompiledSecurityPolicy::DoCheckPolicy(DThread* aThread) const
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


/**
Get the Vendor ID for a thread's process.

If an intended use of this method is to check that the Vendor ID is
a given value, then the use of a TSecurityPolicy object should be
considered. This has the benefit that the TSecurityPolicy::CheckPolicy methods are
configured by the system wide Platform Security configuration. I.e. are
capable of emitting diagnostic messages when a check fails and/or the
check can be forced to always pass.

@param aThread Pointer to thread object.

@return The Vendor ID of the process to which \a aThread belongs.
*/
EXPORT_C TVendorId Kern::ThreadVendorId(DThread* aThread)
	{
	return aThread->iOwningProcess->iS.iVendorId;
	}

/**
Get the Vendor ID for a process.

If an intended use of this method is to check that the Vendor ID is
a given value, then the use of a TSecurityPolicy object should be
considered. This has the benefit that the TSecurityPolicy::CheckPolicy methods are
configured by the system wide Platform Security configuration. I.e. are
capable of emitting diagnostic messages when a check fails and/or the
check can be forced to always pass.

@param aProcess Pointer to process object.

@return The Vendor ID of \a aProcess.
*/
EXPORT_C TVendorId Kern::ProcessVendorId(DProcess* aProcess)
	{
	return aProcess->iS.iVendorId;
	}

/**
Get the Secure ID for a thread's process.

If an intended use of this method is to check that the Secure ID is
a given value, then the use of a TSecurityPolicy object should be
considered. This has the benefit that the TSecurityPolicy::CheckPolicy methods are
configured by the system wide Platform Security configuration. I.e. are
capable of emitting diagnostic messages when a check fails and/or the
check can be forced to always pass.

@param aThread Pointer to thread object.

@return The Secure ID of the process to which \a aThread belongs.
*/
EXPORT_C TSecureId Kern::ThreadSecureId(DThread* aThread)
	{
	return aThread->iOwningProcess->iS.iSecureId;
	}

/**
Get the Secure ID for a process.

If an intended use of this method is to check that the Secure ID is
a given value, then the use of a TSecurityPolicy object should be
considered. This has the benefit that the TSecurityPolicy::CheckPolicy methods are
configured by the system wide Platform Security configuration. I.e. are
capable of emitting diagnostic messages when a check fails and/or the
check can be forced to always pass.

@param aProcess Pointer to process object.

@return The Secure ID of \a aProcess.
*/
EXPORT_C TSecureId Kern::ProcessSecureId(DProcess* aProcess)
	{
	return aProcess->iS.iSecureId;
	}
