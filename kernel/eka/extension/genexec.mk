# Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
#
# Description:
#

# To guarantee there is a slash at the end of EPOCROOT in case there is not.
# This is needed to ensure compatibility with SBSv1. 
TMPROOT:=$(subst \,/,$(EPOCROOT))
EPOCROOT:=$(patsubst %/,%,$(TMPROOT))/

include $(EPOCROOT)epoc32/tools/shell/$(notdir $(basename $(SHELL))).mk


XINCDIR := $(INC_PATH)
XINCKDIR := $(INC_PATH)/kernel

PROCEED:=all
ifneq "$(PBUILDPID)" ""
	ifneq "$(PLATFORM)" "$(__firstplat)"
		PROCEED:=skip
	endif
endif

.PHONY : MAKMAKE FREEZE LIB CLEANLIB RESOURCE FINAL BLD SAVESPACE RELEASABLES CLEAN
.PHONY : all skip

MAKMAKE : $(PROCEED)

FREEZE :

LIB : $(PROCEED)

CLEANLIB :

RESOURCE :

FINAL :

BLD SAVESPACE : $(PROCEED)

RELEASABLES :

CLEAN :
	-$(ERASE) $(call slash2generic,$(XINCDIR)/exec_enum.h) 
	-$(ERASE) $(call slash2generic,$(XINCDIR)/exec_user.h) 
	-$(ERASE) $(call slash2generic,$(XINCKDIR)/exec_kernel.h) 

all: $(XINCDIR)/exec_enum.h $(XINCDIR)/exec_user.h $(XINCKDIR)/exec_kernel.h

$(XINCDIR)/exec_enum.h : $(EXTRA_SRC_PATH)/execs.txt $(EXTRA_SRC_PATH)/genexec.pl
	perl $(EXTRA_SRC_PATH)/genexec.pl -i $(EXTRA_SRC_PATH)/execs.txt -e $(XINCDIR)/exec_enum.h -u $(XINCDIR)/exec_user.h -k $(XINCKDIR)/exec_kernel.h

$(XINCDIR)/exec_user.h : $(EXTRA_SRC_PATH)/execs.txt $(EXTRA_SRC_PATH)/genexec.pl
	perl $(EXTRA_SRC_PATH)/genexec.pl -i $(EXTRA_SRC_PATH)/execs.txt -e $(XINCDIR)/exec_enum.h -u $(XINCDIR)/exec_user.h -k $(XINCKDIR)/exec_kernel.h

$(XINCKDIR)/exec_kernel.h :  $(EXTRA_SRC_PATH)/execs.txt $(EXTRA_SRC_PATH)/genexec.pl
	perl $(EXTRA_SRC_PATH)/genexec.pl -i $(EXTRA_SRC_PATH)/execs.txt -e $(XINCDIR)/exec_enum.h -u $(XINCDIR)/exec_user.h -k $(XINCKDIR)/exec_kernel.h


skip:
	echo GENEXEC skipped for $(PLATFORM) $(CFG)
