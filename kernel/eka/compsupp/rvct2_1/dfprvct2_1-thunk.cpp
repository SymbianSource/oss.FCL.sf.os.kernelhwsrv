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
// Thunks for RVCT synonyms for AEABI functions
// 
//

extern "C" double __aeabi_l2d(long long val);
extern "C" double __aeabi_ul2d(unsigned long long val);
extern "C" long long __aeabi_d2lz(double val);
extern "C" unsigned long long __aeabi_d2ulz(double val);
extern "C" float __aeabi_l2f(long long val);
extern "C" float __aeabi_ul2f(unsigned long long val);
extern "C" long long __aeabi_f2lz(float val);
extern "C" unsigned long long __aeabi_f2ulz(float val);

extern "C"
{

__declspec(dllexport) double _ll_sto_d(long long val)
	{
	return __aeabi_l2d(val);
	}

__declspec(dllexport) double _ll_usto_d(unsigned long long val)
	{
	return __aeabi_ul2d(val);
	}

__declspec(dllexport) long long _ll_sfrom_d(double val)
	{
	return __aeabi_d2lz(val);
	}

__declspec(dllexport) unsigned long long _ll_usfrom_d(double val)
	{
	return __aeabi_d2ulz(val);
	}

__declspec(dllexport) float _ll_sto_f(long long val)
	{
	return __aeabi_l2d(val);
	}

__declspec(dllexport) float _ll_usto_f(unsigned long long val)
	{
	return __aeabi_ul2d(val);
	}

__declspec(dllexport) long long _ll_sfrom_f(float val)
	{
	return __aeabi_d2lz(val);
	}

__declspec(dllexport) unsigned long long _ll_usfrom_f(float val)
	{
	return __aeabi_d2ulz(val);
	}

};
