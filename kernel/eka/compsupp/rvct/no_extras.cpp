// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file is part of all compsupp DLLs. It makes the linker issue an error
// message if semihosting or heap functions get pulled in from RVCT's libraries.
// 
//

#if __ARMCC_VERSION > 300000
#pragma import(__use_no_semihosting)
#endif

#pragma import(__use_no_heap)

