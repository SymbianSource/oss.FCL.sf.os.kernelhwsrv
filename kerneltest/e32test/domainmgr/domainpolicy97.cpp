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

const TDmDomainSpec DomainHierarchy[] = 
	{
		{ KDmIdRoot,	KDmIdNone,	_INIT_SECURITY_POLICY_C1(ECapabilityWriteDeviceData),		EStartupCriticalStatic,	KDomainTimeout	},

		// row 1		
		{ KDmIdTestA,	KDmIdRoot,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},
		{ KDmIdTestB,	KDmIdRoot,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},
		{ KDmIdTestC,	KDmIdRoot,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},

		// row 2
		{ KDmIdTestAA,	KDmIdTestA,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},
		{ KDmIdTestAB,	KDmIdTestA,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},
		{ KDmIdTestBA,	KDmIdTestB,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},
		{ KDmIdTestCA,	KDmIdTestC,	_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},
		
		// row 3
		{ KDmIdTestABA,	KDmIdTestAB,_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},
		{ KDmIdTestABB,	KDmIdTestAB,_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},
		{ KDmIdTestCAA,	KDmIdTestCA,_INIT_SECURITY_POLICY_PASS,									EStartupCriticalStatic,	KDomainTimeout	},

		// end of array marker
		TDM_DOMAIN_SPEC_END
	};

const TDmHierarchyPolicy HierarchyPolicy	= 
	{ETraverseParentsFirst, ETraverseChildrenFirst, ETransitionFailureStop};

const SDmStateSpecV1 StateSpecification[] =
	{
	//    iState, 					iTimeoutMs, iDeferralLimit,	iFailurePolicy
		{ EStartupCriticalStatic,	0, 	 		0, 				ETransitionFailureUsePolicyFromOrdinal3 },
		{ EStartupCriticalDynamic,	0, 	 		0, 				ETransitionFailureUsePolicyFromOrdinal3 },
		{ EStartupNonCritical,		0, 	 		0, 				ETransitionFailureUsePolicyFromOrdinal3 },
		{ ENormalRunning,			0,	 		0, 				ETransitionFailureUsePolicyFromOrdinal3 }, 
		{ EBackupMode,				0, 			0, 				ETransitionFailureUsePolicyFromOrdinal3 },
		{ ERestoreMode,				0, 	 		0, 				ETransitionFailureUsePolicyFromOrdinal3 },
		{ EShutdownCritical,		0, 			0, 				ETransitionFailureUsePolicyFromOrdinal3 },
		{ EShutdownNonCritical,		0, 			0, 				ETransitionFailureUsePolicyFromOrdinal3 }
	};


const TUint StateSpecificationSize = sizeof(StateSpecification)/sizeof(SDmStateSpecV1);
const TInt StateSpecificationVersion = KSDmStateSpecV1;

