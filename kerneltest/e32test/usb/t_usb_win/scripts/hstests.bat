REM Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

..\t_usb_win /L="..\Results\HStests.log"       /P="HSDevice" "sanity.uts"
..\t_usb_win /L="..\Results\HStests.log"       /P="HSDevice" "singleif1.uts"
..\t_usb_win /L="..\Results\HStests.log"       /P="HSDevice" "singleif2.uts"
..\t_usb_win /L="..\Results\HStests.log"       /P="HSDevice" "multif1.uts"
..\t_usb_win /L="..\Results\HStests.log"       /P="HSDevice" "multif2.uts"
..\t_usb_win /L="..\Results\HSperformance.log" /P="HSDevice" "streambm.uts"
..\t_usb_win /L="..\Results\HStests.log"       /P="HSDevice" "mstore.uts"
