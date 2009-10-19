@rem
@rem Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
@rem All rights reserved.
@rem This component and the accompanying materials are made available
@rem under the terms of the License "Eclipse Public License v1.0"
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
@rem @echo off

@if exist temp_result1.txt del temp_result1.txt
@if exist temp_result2.txt del temp_result2.txt
@if exist temp_result3.txt del temp_result3.txt
@if exist FoldDecomp.inl del FoldDecomp.inl

@perl -w UnicodeCompositionEx.pl CompositionExclusions-5_0_0.txt < UnicodeData-5_0_0.txt >>temp_result1.txt
@perl -w UnicodeAddFolded.pl CaseFolding-5_0_0.txt < temp_result1.txt >> temp_result2.txt
@perl -w UnicodeMaxDecompose.pl < temp_result2.txt >> temp_result3.txt
@perl -w FoldAndDecompTables.pl < temp_result3.txt >> FoldDecomp.inl

@if exist temp_result1.txt del temp_result1.txt
@if exist temp_result2.txt del temp_result2.txt
@if exist temp_result3.txt del temp_result3.txt
