@rem
@rem Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
@rem All rights reserved.
@rem This component and the accompanying materials are made available
@rem under the terms of the License "Eclipse Public License v1.0"
@rem which accompanies this distribution, and is available
@rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
@rem

@rem Nokia Corporation - initial contribution.
@rem
@rem Contributors:
@rem
@rem Description:
@rem

cd \e32
call abld export
call abld test export
cd \e32test\group
call abld export
call abld test export
cd \x86pc\single
call abld export
call abld test export
call abld library x86smp
call abld test library x86smp
call abld test target x86smp
cd \e32\rombuild
call rom -v sx86pc -i x86smp -x x86smp -t nktest %*

