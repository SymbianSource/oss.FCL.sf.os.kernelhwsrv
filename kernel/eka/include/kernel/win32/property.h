// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\win32\property.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

struct Property
	{
	static inline const char* GetString(const char* aProperty, const char* aDefault = NULL);
	static inline TInt GetInt(const char* aProperty, TInt aDefault = 0);
	static inline TBool GetBool(const char* aProperty, TBool aDefault = EFalse);
	static inline TInt MapFilename(TDes& aBuffer, const TDesC& aFilename);
	};

inline const char* Property::GetString(const char* aProperty, const char* aDefault)
	{
	const char* val = aDefault;
	Kern::HalFunction(EHalGroupEmulator,EEmulatorHalStringProperty,(TAny*)aProperty,&val);
	return val;
	}

inline TInt Property::GetInt(const char* aProperty, TInt aDefault)
	{
	TInt val = aDefault;
	Kern::HalFunction(EHalGroupEmulator,EEmulatorHalIntProperty,(TAny*)aProperty,&val);
	return val;
	}

inline TBool Property::GetBool(const char* aProperty, TBool aDefault)
	{
	TBool val = aDefault;
	Kern::HalFunction(EHalGroupEmulator,EEmulatorHalBoolProperty,(TAny*)aProperty,&val);
	return val;
	}

inline TInt Property::MapFilename(TDes& aBuffer, const TDesC& aFilename)
	{
	return Kern::HalFunction(EHalGroupEmulator,EEmulatorHalMapFilename,(TAny*)&aFilename,&aBuffer);
	}
