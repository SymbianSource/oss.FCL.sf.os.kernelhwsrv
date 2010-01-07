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

start t_smpsoak.exe -p 40000 -b

trace 2 2
z:
cd test
format c:
format d:
runtests e32test.auto.bat -st -t 60
format c:
format d:
runtests loader.auto.bat -st -t 540
format c:
format d:
runtests f32test.auto.bat -d c -st -t 60
format c:
format d:
format c:
format d:
runtests f32test.auto.bat -d d -st -t 540 -p