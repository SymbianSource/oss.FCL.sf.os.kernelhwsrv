/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "ARM EABI LICENCE.txt"
* which accompanies this distribution, and is available
* in kernel/eka/compsupp.
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

#include <typeinfo>

EXPORT_C bool std::type_info::operator==(const std::type_info& rhs) const
	{
	if (this == &rhs)
		{
		return true;
		}

	const char* s1 = this->name();
	const char* s2 = rhs.name();

	while ( *s1 == *s2 && *s1 != '\0' )
		{
		s1++;
		s2++;
		}

	return ( *s1 == '\0' && *s2 == '\0' );
	}

EXPORT_C bool std::type_info::operator!=(const std::type_info& rhs) const
	{
	return !(*this == rhs);
	}

