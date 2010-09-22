REM Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

..\t_usb_win /L="..\Results\sanity.log"       /P="VascoDevice" "sanity.uts"
..\t_usb_win /L="..\Results\singleif1.log"       /P="VascoDevice" "singleif1.uts"
..\t_usb_win /L="..\Results\singleif2.log"       /P="VascoDevice" "singleif2.uts"
..\t_usb_win /L="..\Results\multif1.log"       /P="VascoDevice" "multif1.uts"
..\t_usb_win /L="..\Results\multif2.log"       /P="VascoDevice" "multif2.uts"
..\t_usb_win /L="..\Results\streambm.log" /P="VascoDevice" "streambm.uts"
..\t_usb_win /L="..\Results\mstore.log"       /P="VascoDevice" "mstore.uts"
