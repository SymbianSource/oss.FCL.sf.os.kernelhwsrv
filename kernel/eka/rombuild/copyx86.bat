@echo off
rem
rem Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
rem All rights reserved.
rem This component and the accompanying materials are made available
rem under the terms of the License "Eclipse Public License v1.0"
rem which accompanies this distribution, and is available
rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
rem

rem Nokia Corporation - initial contribution.
rem
rem Contributors:
rem
rem Description:
rem
del %1.zip
zip %1.zip %1.img
copy /b \os\boardsupport_internal\x86pc\pcboot\inflate.bin+%1.zip %2\E32ROM.IMG
