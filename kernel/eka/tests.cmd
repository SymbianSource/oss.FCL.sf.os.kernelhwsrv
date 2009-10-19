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

cd \e32test\group
call bldmake bldfiles
call abld test makefile arm4
call abld test makefile armi
call abld test makefile thumb
call abld test makefile mawd
call abld test makefile misa
call abld test makefile sawd
call abld test library arm4
call abld test library armi
call abld test library thumb
call abld test library mawd
call abld test library misa
call abld test library sawd
call abld test target arm4
call abld test target armi
call abld test target thumb
call abld test target mawd
call abld test target misa
call abld test target sawd
call abld test romfile arm4
call abld test romfile armi
call abld test romfile thumb
call abld test romfile mawd
call abld test romfile misa
call abld test romfile sawd

cd \f32test\group
call bldmake bldfiles
call abld test makefile arm4
call abld test makefile armi
call abld test makefile thumb
call abld test library arm4
call abld test library armi
call abld test library thumb
call abld test target arm4
call abld test target armi
call abld test target thumb
call abld test romfile arm4
call abld test romfile armi
call abld test romfile thumb
