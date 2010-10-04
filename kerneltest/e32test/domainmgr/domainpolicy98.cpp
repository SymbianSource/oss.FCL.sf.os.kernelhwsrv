// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
//


#include "domainpolicytest.h"

const TInt KDomainTimeoutShort = 1500000; /* 1500ms */

const TDmDomainSpec DomainHierarchy[] =
	{
		{ KDmIdRoot,	KDmIdNone,	_INIT_SECURITY_POLICY_C1(ECapabilityWriteDeviceData),		EStartupCriticalStatic,	KDomainTimeoutShort	},

		// row 1
		{ KDmIdTestA,	KDmIdRoot,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},
		{ KDmIdTestB,	KDmIdRoot,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},
		{ KDmIdTestC,	KDmIdRoot,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},

		// row 2
		{ KDmIdTestAA,	KDmIdTestA,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},
		{ KDmIdTestAB,	KDmIdTestA,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},
		{ KDmIdTestBA,	KDmIdTestB,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},
		{ KDmIdTestCA,	KDmIdTestC,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},

		// row 3
		{ KDmIdTestABA,	KDmIdTestAB,_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},
		{ KDmIdTestABB,	KDmIdTestAB,_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},
		{ KDmIdTestCAA,	KDmIdTestCA,_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeoutShort	},

		// end of array marker
		TDM_DOMAIN_SPEC_END
	};

const TDmHierarchyPolicy HierarchyPolicy	=
	{ETraverseParentsFirst, ETraverseChildrenFirst, ETransitionFailureStop};


const SDmStateSpecV1 StateSpecification[] =
	{
	//    iState, 					iTimeoutMs, iDeferralLimit,	iFailurePolicy
		{ EStartupCriticalStatic,	0, 	 		0, 				ETransitionFailureUsePolicyFromOrdinal3 },
		{ EStartupCriticalDynamic,	0, 	 		0, 				ETransitionFailureStop },
		{ EStartupNonCritical,		0, 	 		0, 				ETransitionFailureUsePolicyFromOrdinal3 },
		{ ENormalRunning,			0,	 		0, 				ETransitionFailureContinue },
		{ EBackupMode,				3000, 		1, 				ETransitionFailureStop },
		{ ERestoreMode,				150, 		3, 				ETransitionFailureStop },
		{ EShutdownCritical,		3000, 		5, 				ETransitionFailureStop },
		{ EShutdownNonCritical,		3000, 		1, 				ETransitionFailureContinue }
	};

const TUint StateSpecificationSize = sizeof(StateSpecification)/sizeof(SDmStateSpecV1);
const TInt StateSpecificationVersion = KSDmStateSpecV1;

