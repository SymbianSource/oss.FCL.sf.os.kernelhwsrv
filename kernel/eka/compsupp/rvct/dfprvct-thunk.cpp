// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

// This file is part of dfprvct-thunk.dll.
//


extern "C"
{
IMPORT_C double __aeabi_l2d(long long);
IMPORT_C double __aeabi_ul2d(unsigned long long);
IMPORT_C long long __aeabi_d2lz(double);
IMPORT_C unsigned long long __aeabi_d2ulz(double);
IMPORT_C float __aeabi_l2f(long long);
IMPORT_C float __aeabi_ul2f(unsigned long long);
IMPORT_C long long __aeabi_f2lz(float);
IMPORT_C unsigned long long __aeabi_f2ulz(float);

EXPORT_C double _ll_sto_d(long long val)
	{
	return __aeabi_l2d(val);
	}

EXPORT_C double _ll_usto_d(unsigned long long val)
	{
	return __aeabi_ul2d(val);
	}

EXPORT_C long long _ll_sfrom_d(double val)
	{
	return __aeabi_d2lz(val);
	}

EXPORT_C unsigned long long _ll_usfrom_d(double val)
	{
	return __aeabi_d2ulz(val);
	}

EXPORT_C float _ll_sto_f(long long val)
	{
	return __aeabi_l2d(val);
	}

EXPORT_C float _ll_usto_f(unsigned long long val)
	{
	return __aeabi_ul2d(val);
	}

EXPORT_C long long _ll_sfrom_f(float val)
	{
	return __aeabi_d2lz(val);
	}

EXPORT_C unsigned long long _ll_usfrom_f(float val)
	{
	return __aeabi_d2ulz(val);
	}

} // extern "C"
