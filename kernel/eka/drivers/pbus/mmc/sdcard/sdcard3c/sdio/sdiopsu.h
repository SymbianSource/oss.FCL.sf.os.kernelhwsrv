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
// Class definitions for SDIO PSU object
// 
//

/**
 @file sdiopsu.h
 @internalTechnology
*/

#ifndef __SDIOPSU_H__
#define __SDIOPSU_H__

#include <drivers/sdio/sdio.h>

class DSDIOPsu : public DMMCPsu
/**
 * DPBusPsuBase derived abstract class to control the SDIO socket's power supply.
 *
 * Provides special support for SDIO Cards to enter low-power sleep mode when
 * not in use, or fully powering down the stack.
 *
 * This class is intended for derivation at the variant layer, which handles the
 * variant specific functionality of the power supply.
 */
    {
public:
	IMPORT_C DSDIOPsu(TInt aPsuNum, TInt aMediaChangedNum);

	IMPORT_C TInt DoCreate();
	IMPORT_C void DoTickService();
	IMPORT_C TBool IsLocked();

	inline void Lock();
	inline void Unlock();

	/**
	 Re-declaring pure-virtual functions from DMMCPsu for clarity
	 */

	/**
	 @publishedPartner
	 @released

	 Controls the power supply.
	 Implemented by the variant, directly controls the power to the MMC stack.
	 @param aState A TPBusPsuState enumeration specifying the required state
	 				 (EPsuOnFull, EPsuOff, EPsuOnCurLimit)

	 */
	virtual void DoSetState(TPBusPsuState aState) = 0;

	/**
	 @publishedPartner
	 @released

	 Checks the PSU's voltage.
	 Implemented by the variant, uses a mechanism such as a comparator to check
	 the PSU's voltage level.  Upon reciept of the voltage level (the process may
	 be asynchronous), the variant calls ReceiveVoltageCheckResult() with KErrNone
	 if the voltage is OK, KErrGeneral if there is a problem, or KErrNotReady if the
	 hardware has not yet powered up.
	 */
	virtual void DoCheckVoltage() = 0;

	/**
	 @publishedPartner
	 @released

	 Fills in the supplied TPBusPsuInfo object with the characteristics of the platform.
	 Provided at the variant layer.
	 @param anInfo A reference to a TPBusPsuInfo to be filled in with the PSU characteristics.
	 */
    virtual void PsuInfo(TPBusPsuInfo &anInfo) = 0;

private:
	TBool iIsLocked;	// Prevents the PSU from powering down when ETrue
	
    //
    // Dummy functions to maintain binary compatibility
    IMPORT_C virtual void Dummy1();
    IMPORT_C virtual void Dummy2();
    IMPORT_C virtual void Dummy3();
    IMPORT_C virtual void Dummy4();
    //
    // Reserved members to maintain binary compatibility
    TInt iReserved[4];
    };
    
#include <drivers/sdio/sdiopsu.inl>

#endif	// #ifndef __SDIOPSU_H__
