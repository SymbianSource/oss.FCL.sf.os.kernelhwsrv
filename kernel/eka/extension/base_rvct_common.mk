# Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
# Some functions that are commonly used by base FLM

define base__compile
$(1) : $(2) : $(3)
	$(call startrule,base__compile) \
	$(CC) $(ARMCCFLAGS) $$< -o $$@ \
	$(call endrule,base__compile)

CLEANTARGETS := $(CLEANTARGETS) $(1)
endef

define base__h2inc
$(1) : $(2)
	$(call startrule,base__h2inc) \
 	$(PERL) $(EPOCROOT)/epoc32/tools/h2inc.pl $$< $$@ ARMASM \
	$(call endrule,base__h2inc)

CLEANTARGETS := $(CLEANTARGETS) $(1)
endef

define base__asm
$(1) : $(2) : $(3)
	$(call startrule,base__asm) \
	$(ASM) $(AFLAGS) -o $$@ --LIST $(join $(basename $(1)),.lst) $$< \
	$(call endrule,base__asm)

CLEANTARGETS := $(CLEANTARGETS) $(1) $(join $(basename $(1)),.lst)
endef

define base__link
$(1) : $(2)
	$(call startrule,base__link) \
	$(LD) $(LFLAGS) -o $$@ $(FULLOBJECTS) \
	$(call endrule,base__link)

CLEANTARGETS := $(CLEANTARGETS) $(1)
endef

define base__strip
$(1) : $(2)
	$(call startrule,base__strip) \
	$(FROMELF) --bin --output $$@ $$< \
	$(call endrule,base__strip)

CLEANTARGETS := $(CLEANTARGETS) $(1)
endef

define base__omapsig
$(1) : $(2)
	$(call startrule,base__omapsig) \
	$(PERL) $(EPOCROOT)/epoc32/tools/omapsig.pl $(LINKBASE) $$< $$@ \
	$(call endrule,base__omapsig)

CLEANTARGETS := $(CLEANTARGETS) $(1)
endef

