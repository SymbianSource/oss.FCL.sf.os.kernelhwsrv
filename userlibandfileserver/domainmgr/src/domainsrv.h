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
// domain\src\domainsrv.h
// 
//

#ifndef __DOMAIN_SRV_H__
#define __DOMAIN_SRV_H__

#include <e32def.h>

#include <domaindefs.h>

_LIT(KDmDomainServerNameLit,"!DmDomainServer");
#define KDmDomainServerVersion	TVersion(1, 0, 0)
_LIT(KDmManagerServerNameLit,"!DmManagerServer");
#define KDmManagerServerVersion	TVersion(1, 0, 0)

enum 
	{
	EDmDomainJoin,
	EDmStateAcknowledge,
	EDmStateRequestTransitionNotification,
	EDmStateCancelTransitionNotification
	};

enum 
	{
	EDmRequestSystemTransition,
	EDmRequestDomainTransition,
	EDmCancelTransition,
	EDmHierarchyJoin,
	EDmHierarchyAdd,
	EDmGetTransitionFailureCount,
	EDmGetTransitionFailures,
	EDmObserverJoin,
	EDmObserverStart,
	EDmObserverStop,
	EDmObserverNotify,
	EDmObserverEventCount,
	EDmObserverGetEvent,
	EDmObserverCancel,
	EDmObserveredCount
	};

inline TInt DmStatePropertyKey(TDmHierarchyId aHierarchyId, TDmDomainId aDomainId)
	{ return (TInt) ((aHierarchyId << 8) | ((aDomainId << 8) & 0xff0000) | (aDomainId & 0xff) ); }

inline TInt DmStatePropertyValue(TUint aId, TUint32 aState)
	{ return (TInt) ((aId << 24) | (aState & 0xffffff)); }

inline TDmDomainState DmStateFromPropertyValue(TInt aValue)
	{ return (TDmDomainState) (aValue & 0xffffff); }

#endif
