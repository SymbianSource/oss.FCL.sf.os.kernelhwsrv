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


ifndef HALPATH
HALPATH := ..
endif

ifndef SOURCE
SOURCE := hal
endif

#MDIR := $(call generated,generatedcpp/hal) # abld
#MDIR := $(call generated,base/lubbock) # raptor
MDIR := $(call generatedcpp)

MAKMAKE : $(MDIR)/$(PREFIX)values.cpp $(MDIR)/$(PREFIX)config.cpp

FREEZE :

LIB :

CLEANLIB :

RESOURCE :

FINAL :

BLD SAVESPACE : $(MDIR)/$(PREFIX)values.cpp $(MDIR)/$(PREFIX)config.cpp

RELEASABLES :

CLEAN :
	-$(ERASE) $(call slash2generic,$(MDIR)/$(PREFIX)values.cpp) 
	-$(ERASE) $(call slash2generic,$(MDIR)/$(PREFIX)config.cpp) 
#	-$(ERASE) $(MDIR)/$(PREFIX)values.cpp
#	-$(ERASE) $(MDIR)/$(PREFIX)config.cpp

$(MDIR)/$(PREFIX)values.cpp : $(SOURCE)/values.hda $(EPOCROOT)epoc32/include/platform/hal_data.h
	-$(call createdir,"$(MDIR)")
	perl $(HALPATH)/hal/halcfg.pl $(EPOCROOT)epoc32/include/platform/hal_data.h $(SOURCE)/values.hda $(MDIR)/$(PREFIX)values.cpp

$(MDIR)/$(PREFIX)config.cpp : $(SOURCE)/config.hcf $(EPOCROOT)epoc32/include/platform/hal_data.h
	-$(call createdir,"$(MDIR)")
	perl $(HALPATH)/hal/halcfg.pl -x $(EPOCROOT)epoc32/include/platform/hal_data.h $(SOURCE)/config.hcf $(MDIR)/$(PREFIX)config.cpp

