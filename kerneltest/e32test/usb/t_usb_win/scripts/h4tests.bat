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

..\t_usb_win /L="..\Results\h4tests.log"       /P="H4Device" "sanity.uts"
..\t_usb_win /L="..\Results\h4tests.log"       /P="H4Device" "singleif1.uts"
..\t_usb_win /L="..\Results\h4tests.log"       /P="H4Device" "singleif2.uts"
..\t_usb_win /L="..\Results\h4tests.log"       /P="H4Device" "multif1.uts"
..\t_usb_win /L="..\Results\h4tests.log"       /P="H4Device" "multif2.uts"
..\t_usb_win /L="..\Results\h4performance.log" /P="H4Device" "streambm.uts"
..\t_usb_win /L="..\Results\h4tests.log"       /P="H4Device" "mstore.uts"
