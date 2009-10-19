@echo off
rem
rem Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
rem All rights reserved.
rem This component and the accompanying materials are made available
rem under the terms of the License "Eclipse Public License v1.0"
rem which accompanies this distribution, and is available
rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
rem
rem Initial Contributors:
rem Nokia Corporation - initial contribution.
rem
rem Contributors:
rem
rem Description:
rem
goto b%1
:b
:bdeb
set _where_=\Epoc32\Release\wins\deb\eshell.exe
goto doit
:brel
set _where_=\Epoc32\Release\wins\rel\eshell.exe
goto doit
:budeb
set _where_=\Epoc32\Release\wins\udeb\eshell.exe
goto doit
:burel
set _where_=\Epoc32\Release\wins\urel\eshell.exe
:doit
echo \F32\TSHELL\ESHELL...
%_where_%
set _where_=


