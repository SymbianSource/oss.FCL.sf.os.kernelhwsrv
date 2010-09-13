# Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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


SOURCE_COPY=$(EPOCROOT)epoc32/release/$(PLATFORM_PATH)/$(CFG_PATH)/$(SOURCES)
TARGET_COPY=$(EPOCROOT)epoc32/release/$(PLATFORM_PATH)/$(CFG_PATH)/$(TARGET)

$(TARGET_COPY) : $(SOURCE_COPY)
	$(call cpfeature,"$(SOURCE_COPY)","$(TARGET_COPY)")
#	perl $(EPOCROOT)epoc32/tools/copyfeaturevariants.pl "$(SOURCE_COPY)" "$(TARGET_COPY)"
#	$(CP) "$?" "$@"

#
# The targets invoked by abld...
#

MAKMAKE BLD SAVESPACE FREEZE LIB CLEANLIB RESOURCE :
	@echo Nothing to do for "$@"

CLEAN : 
	-$(ERASE) $(TARGET_COPY)

RELEASABLES : 
	@echo $(TARGET_COPY)

# we have to wait until the SOURCE_COPY is built before we can copy it...
#
FINAL : $(TARGET_COPY)

