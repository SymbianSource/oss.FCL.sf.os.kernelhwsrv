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

CALL cd ..\..\..\..\..\kernel\eka\rombuild

CALL rom -v h4hrp -i armv5 -b udeb -t t_usbdi -s --define=USE_USB_HOST,TEST_USB_HOST,SYMBIAN_INCLUDE_USB_OTG_HOST

:makebin
set TheDrive=F
if exist %1: set TheDrive=%1

CALL DEL /Q sys$rom.bin
CALL rename h4HRPARMV5D.img sys$rom.bin
CALL copy /y sys$rom.bin %theDrive%:
CALL chkdsk /x %theDrive%:
CALL cd ..\..\..\kerneltest\e32test\usbho\t_usbdi\rom
