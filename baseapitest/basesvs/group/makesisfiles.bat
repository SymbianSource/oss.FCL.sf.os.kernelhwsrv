@rem
@rem Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
@rem All rights reserved.
@rem This component and the accompanying materials are made available
@rem under the terms of "Eclipse Public License v1.0"
@rem which accompanies this distribution, and is available
@rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
@rem
@rem Initial Contributors:
@rem Nokia Corporation - initial contribution.
@rem
@rem Contributors:
@rem
@rem Description: 
@rem

copy ..\release\armv5\urel\T_SfSrv.exe .
copy ..\release\armv5\urel\t_prompt.exe .
copy ..\release\armv5\urel\t_fat32server.exe .
copy ..\release\armv5\urel\t_perf.exe .

copy ..\release\armv5\urel\T_TESTFSY1.fsy
copy ..\release\armv5\urel\T_TestFXT.fxt

call makesis t_sfsrv.pkg t_sfsrv.sis
call makesis t_perf.pkg t_perf.sis
call makesis t_fat32.pkg t_fat32.sis
call makesis dualdrive.pkg dualdrive.sis

call signsis -s t_sfsrv.sis t_sfsrv.sis zeus.cer SymbianACS.key caforstat
call signsis -s t_perf.sis t_perf.sis zeus.cer SymbianACS.key caforstat
call signsis -s t_fat32.sis t_fat32.sis zeus.cer SymbianACS.key caforstat
call signsis -s dualdrive.sis dualdrive.sis zeus.cer SymbianACS.key caforstat