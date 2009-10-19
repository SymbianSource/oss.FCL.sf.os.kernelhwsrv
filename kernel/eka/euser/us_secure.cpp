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
// e32\euser\us_secure.cpp
// 
//

#include <u32exec.h>

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TInt PlatSec::EmitDiagnostic(TPlatSecDiagnostic& aDiagnostic, const char* aContextText)
	{
	if(aContextText == KSuppressPlatSecDiagnosticMagicValue)
		return Exec::PlatSecDiagnostic(NULL);
	aDiagnostic.iContextText = aContextText;
	if(aContextText)
		aDiagnostic.iContextTextLength=User::StringLength((const TUint8*)aContextText);
	return Exec::PlatSecDiagnostic(&aDiagnostic);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TInt PlatSec::EmitDiagnostic(TPlatSecDiagnostic& /*aDiagnostic*/, const char* /*aContextText*/)
	{
	return Exec::PlatSecDiagnostic(NULL);
	}
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TInt PlatSec::EmitDiagnostic()
	{
	return Exec::PlatSecDiagnostic(NULL);
	}

EXPORT_C TSecureId RProcess::SecureId() const
	{
	SSecurityInfo info;
	Exec::ProcessSecurityInfo(iHandle,info);
	return info.iSecureId;
	}

EXPORT_C TVendorId RProcess::VendorId() const
	{
	SSecurityInfo info;
	Exec::ProcessSecurityInfo(iHandle,info);
	return info.iVendorId;
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RProcess::DoHasCapability(TCapability aCapability, const char* aDiagnostic) const
	{
	SSecurityInfo info;
	Exec::ProcessSecurityInfo(iHandle,info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability))
		return ETrue;
	return KErrNone==PlatSec::CapabilityCheckFail(iHandle,aCapability,aDiagnostic);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RProcess::DoHasCapability(TCapability aCapability, const char* /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability);
	}
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool RProcess::DoHasCapability(TCapability aCapability) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoHasCapability(aCapability, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo info;
	Exec::ProcessSecurityInfo(iHandle,info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RProcess::DoHasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic) const
	{
	SSecurityInfo info;
	Exec::ProcessSecurityInfo(iHandle,info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability1) && caps.HasCapability(aCapability2))
		return ETrue;
	TCapabilitySet missing(aCapability1,aCapability2);
	missing.Remove(caps);
	return KErrNone==PlatSec::CapabilityCheckFail(iHandle,missing,aDiagnostic);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RProcess::DoHasCapability(TCapability aCapability1, TCapability aCapability2, const char* /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability1, aCapability2);
	}
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool RProcess::DoHasCapability(TCapability aCapability1, TCapability aCapability2) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoHasCapability(aCapability1, aCapability2, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo info;
	Exec::ProcessSecurityInfo(iHandle, info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability1) && caps.HasCapability(aCapability2))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
	}

EXPORT_C TSecureId RThread::SecureId() const
	{
	SSecurityInfo info;
	Exec::ThreadSecurityInfo(iHandle,info);
	return info.iSecureId;
	}

EXPORT_C TVendorId RThread::VendorId() const
	{
	SSecurityInfo info;
	Exec::ThreadSecurityInfo(iHandle,info);
	return info.iVendorId;
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RThread::DoHasCapability(TCapability aCapability, const char* aDiagnostic) const
	{
	SSecurityInfo info;
	Exec::ThreadSecurityInfo(iHandle,info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability))
		return ETrue;
	return KErrNone==PlatSec::CapabilityCheckFail(iHandle,aCapability,aDiagnostic);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RThread::DoHasCapability(TCapability aCapability, const char* /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability);
	}
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool RThread::DoHasCapability(TCapability aCapability) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoHasCapability(aCapability, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo info;
	Exec::ThreadSecurityInfo(iHandle, info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RThread::DoHasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic) const
	{
	SSecurityInfo info;
	Exec::ThreadSecurityInfo(iHandle,info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability1) && caps.HasCapability(aCapability2))
		return ETrue;
	TCapabilitySet missing(aCapability1,aCapability2);
	missing.Remove(caps);
	return KErrNone==PlatSec::CapabilityCheckFail(iHandle,missing,aDiagnostic);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RThread::DoHasCapability(TCapability aCapability1, TCapability aCapability2, const char* /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability1, aCapability2);
	}
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool RThread::DoHasCapability(TCapability aCapability1, TCapability aCapability2) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoHasCapability(aCapability1, aCapability2, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo info;
	Exec::ThreadSecurityInfo(iHandle, info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability1) && caps.HasCapability(aCapability2))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
	}


EXPORT_C TSecureId RMessagePtr2::SecureId() const
	{
	SSecurityInfo info;
	Exec::MessageSecurityInfo(iHandle,info);
	return info.iSecureId;
	}

EXPORT_C TVendorId RMessagePtr2::VendorId() const
	{
	SSecurityInfo info;
	Exec::MessageSecurityInfo(iHandle,info);
	return info.iVendorId;
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RMessagePtr2::DoHasCapability(TCapability aCapability, const char* aDiagnostic) const
	{
	SSecurityInfo info;
	Exec::MessageSecurityInfo(iHandle,info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability))
		return ETrue;
	return KErrNone==PlatSec::CapabilityCheckFail(*this,aCapability,aDiagnostic);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RMessagePtr2::DoHasCapability(TCapability aCapability, const char* /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability);
	}
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool RMessagePtr2::DoHasCapability(TCapability aCapability) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoHasCapability(aCapability, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo info;
	Exec::MessageSecurityInfo(iHandle,info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if (caps.HasCapability(aCapability))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RMessagePtr2::DoHasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic) const
	{
	SSecurityInfo info;
	Exec::MessageSecurityInfo(iHandle,info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability1) && caps.HasCapability(aCapability2))
		return ETrue;
	TCapabilitySet missing(aCapability1,aCapability2);
	missing.Remove(caps);
	return KErrNone==PlatSec::CapabilityCheckFail(*this,missing,aDiagnostic);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool RMessagePtr2::DoHasCapability(TCapability aCapability1, TCapability aCapability2, const char* /*aDiagnostic*/) const
	{
	return DoHasCapability(aCapability1, aCapability2);
	}
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool RMessagePtr2::DoHasCapability(TCapability aCapability1, TCapability aCapability2) const
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoHasCapability(aCapability1, aCapability2, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo info;
	Exec::MessageSecurityInfo(iHandle,info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability1) && caps.HasCapability(aCapability2))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
	}

EXPORT_C TSecureId User::CreatorSecureId()
	{
	SSecurityInfo info;
	Exec::CreatorSecurityInfo(info);
	return info.iSecureId;
	}

EXPORT_C TVendorId User::CreatorVendorId()
	{
	SSecurityInfo info;
	Exec::CreatorSecurityInfo(info);
	return info.iVendorId;
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool User::DoCreatorHasCapability(TCapability aCapability, const char* aDiagnostic)
	{
	SSecurityInfo info;
	Exec::CreatorSecurityInfo(info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability))
		return ETrue;
	return KErrNone==PlatSec::CreatorCapabilityCheckFail(aCapability,aDiagnostic);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool User::DoCreatorHasCapability(TCapability aCapability, const char* /*aDiagnostic*/)
	{
	return DoCreatorHasCapability(aCapability);
	}
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool User::DoCreatorHasCapability(TCapability aCapability)
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoCreatorHasCapability(aCapability, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo info;
	Exec::CreatorSecurityInfo(info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool User::DoCreatorHasCapability(TCapability aCapability1, TCapability aCapability2, const char* aDiagnostic)
	{
	SSecurityInfo info;
	Exec::CreatorSecurityInfo(info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability1) && caps.HasCapability(aCapability2))
		return ETrue;
	TCapabilitySet missing(aCapability1,aCapability2);
	missing.Remove(caps);
	return KErrNone==PlatSec::CreatorCapabilityCheckFail(missing,aDiagnostic);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
EXPORT_C TBool User::DoCreatorHasCapability(TCapability aCapability1, TCapability aCapability2, const char* /*aDiagnostic*/)
	{
	return DoCreatorHasCapability(aCapability1, aCapability2);
	}
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__

EXPORT_C TBool User::DoCreatorHasCapability(TCapability aCapability1, TCapability aCapability2)
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	return DoCreatorHasCapability(aCapability1, aCapability2, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTICS__
	SSecurityInfo info;
	Exec::CreatorSecurityInfo(info);
	TCapabilitySet& caps = (TCapabilitySet&)info.iCaps;
	if(caps.HasCapability(aCapability1) && caps.HasCapability(aCapability2))
		return ETrue;
	return (PlatSec::EmitDiagnostic() == KErrNone);
#endif //!__REMOVE_PLATSEC_DIAGNOSTICS__
	}
