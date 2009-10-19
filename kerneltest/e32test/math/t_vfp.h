// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\math\t_vfp.h
// 
//

#ifndef __T_VFP_H__
#define __T_VFP_H__
#include <e32std.h>
#include <arm_vfp.h>

#define ARCH_VERSION_VFPV2				1
#define ARCH_VERSION_VFPV3_SUBARCH_V2	2
#define ARCH_VERSION_VFPV3_SUBARCH_NULL	3
#define ARCH_VERSION_VFPV3_SUBARCH_V3	4

class Vfp
	{
public:
	static TUint32 Fpscr();
	static void SetFpscr(TUint32 aVal);

	static TReal32 SReg(TInt aReg);
	static void SetSReg(TReal32 aVal, TInt aReg);
	static TInt32 SRegInt(TInt aReg);
	static void SetSReg(TInt32 aVal, TInt aReg);

	static void AbsS();				// S0 = ABS(S1)
	static void AddS();				// S0 = S1 + S2
	static void CmpS();				// compare S0, S1
	static void CmpES();			// compare S0, S1
	static void CmpEZS();			// compare S0, 0
	static void CmpZS();			// compare S0, 0
	static void Cpy0S(TInt aReg);	// S0 = Sn
	static void CpyS0(TInt aReg);	// Sn = S0
	static void DivS();				// S0 = S1 / S2
	static void MacS();				// S0 += S1 * S2
	static void MscS();				// S0 = S1 * S2 - S0
	static void MulS();				// S0 = S1 * S2
	static void NegS();				// S0 = -S1
	static void NMacS();			// S0 -= S1 * S2
	static void NMscS();			// S0 = -S0 - S1 * S2
	static void NMulS();			// S0 = -S1 * S2
	static void SqrtS();			// S0 = sqrt(S1)
	static void SubS();				// S0 = S1 - S2

	static TReal64 DReg(TInt aReg);
	static void SetDReg(TReal64 aVal, TInt aReg);
	static TInt64 DRegInt(TInt aReg);
	static void SetDReg(TInt64 aVal, TInt aReg);
	static void AbsD();				// D0 = ABS(D1)
	static void AddD();				// D0 = D1 + D2
	static void CmpD();				// compare D0, D1
	static void CmpED();			// compare D0, D1
	static void CmpEZD();			// compare D0, 0
	static void CmpZD();			// compare D0, 0
	static void Cpy0D(TInt aReg);	// D0 = Dn
	static void CpyD0(TInt aReg);	// Dn = D0
	static void DivD();				// D0 = D1 / D2
	static void MacD();				// D0 += D1 * D2
	static void MscD();				// D0 = D1 * D2 - D0
	static void MulD();				// D0 = D1 * D2
	static void NegD();				// D0 = -D1
	static void NMacD();			// D0 -= D1 * D2
	static void NMscD();			// D0 = -D0 - D1 * D2
	static void NMulD();			// D0 = -D1 * D2
	static void SqrtD();			// D0 = sqrt(D1)
	static void SubD();				// D0 = D1 - D2

	static void CvtDS();			// D0 = S2
	static void CvtSD();			// S0 = D1
	static void SitoD();			// D0 = IS2
	static void SitoS();			// S0 = IS2
	static void TosiD();			// IS0 = D1
	static void TosiZD();			// IS0 = D1 round towards 0
	static void TosiS();			// IS0 = S2
	static void TosiZS();			// IS0 = S2 round towards 0
	static void UitoD();			// D0 = US2
	static void UitoS();			// S0 = US2
	static void TouiD();			// US0 = D1
	static void TouiZD();			// US0 = D1 round towards 0
	static void TouiS();			// US0 = S2
	static void TouiZS();			// US0 = S2 round towards 0

	// VFPv3 support
	static void ToFixedS(TInt aBits);	// Convert to fixed (aBits) precision 
	static void FromFixedS(TInt aBits); // Convert from fixed (aBits) precision 
	static void TconstS2();			// S0=2
	static void TconstS2_8();		// S0=2.875
	static void TconstD2();			// D0=2
	static void TconstD2_8();		// D0=2.875
	
	};

GLREF_C TInt NeonWithF2(TAny*);
GLREF_C TInt NeonWithF3(TAny*);
GLREF_C TInt NeonWithF4x(TAny*);
GLREF_C TInt ThumbMode(TAny*);

#endif

