REM
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

t_usb_device z:\test\vascoif0.xml /script=sanity.uts
t_usb_device z:\test\vascoif0a3.xml /script=singleif1.uts
t_usb_device z:\test\vascoif0a3.xml /script=singleif2.uts
t_usb_device z:\test\vascoif0a3if1a2if2.xml /script=multif1.uts
t_usb_device z:\test\vascoif0a3if1a2if2.xml /script=multif2.uts
t_usb_device z:\test\vascobm.xml /script=streambm.uts
t_usb_device z:\test\vascoif0a3.xml /script=mstore.uts

t_usb_scdevice z:\test\scvascoif0.xml /script=sanity.uts
t_usb_scdevice z:\test\scvascoif0a3.xml /script=singleif1.uts
t_usb_scdevice z:\test\scvascoif0a3.xml /script=singleif2.uts
t_usb_scdevice z:\test\scvascoif0a3if1a2if2.xml /script=multif1.uts
t_usb_scdevice z:\test\scvascoif0a3if1a2if2.xml /script=multif2.uts
t_usb_scdevice z:\test\scvascobm.xml /script=streambm.uts
t_usb_scdevice z:\test\scvascoif0a3.xml /script=mstore.uts
