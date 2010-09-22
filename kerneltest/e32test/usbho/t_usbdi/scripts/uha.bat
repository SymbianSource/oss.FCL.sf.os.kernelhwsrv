REM Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
REM All rights reserved.
REM This component and the accompanying materials are made available
REM under the terms of the License "Eclipse Public License v1.0"
REM which accompanies this distribution, and is available
REM at the URL "http://www.eclipse.org/legal/epl-v10.html".
REM 
REM Initial Contributors:
REM Nokia Corporation - initial contribution.
REM 
REM Contributors:
REM 
REM Description:
REM 
REM
 
REM t_usbdi -role=host -cases=0472,0473,0474,0477,0478,0479,0480,0481,0483,0484,0485,0486,0487,0488,0489,0490,0491,0492,0493,0494,0495,0496,0497,0498,0499,0500,1229,1230,1231,1234,1235,1236,0475,0476
REM VASCO REMOVE 0473 since remote wakeup is not supported by vasco
REM VASCO REMOVE 0486(ou1cimx1#563256)
REM VASCO REMOVE 0499(ou1cimx1#553963). 
REM VASCO REMOVE 0496/0497/0498(ou1cimx1#553913)
REM VASCO REMOVE manual 0482(ou1cimx1#554096)
t_usbdi -role=host -cases=0472,0474,0477,0478,0479,0480,0481,0483,0484,0485,0487,0488,0489,0490,0491,0492,0493,0494,0495,0500,1229,1230,1231,1234,1235,1236,0475,0476