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

# To ensure that EPOCROOT always ends with a forward slash. 
TMPROOT:=$(subst \,/,$(EPOCROOT))
EPOCROOT:=$(patsubst %/,%,$(TMPROOT))/

ifndef CPU
CPU := arm
endif

ifndef LINKBASE
LINKBASE := 0x00000000
endif

include $(EPOCROOT)epoc32/tools/shell/$(notdir $(basename $(SHELL))).mk

PROCEED:=build
ifneq "$(PBUILDPID)" ""
        ifneq "$(CFG)" "UREL"
                PROCEED:=skip
        endif
endif

COPY := $(call ecopy)

ifeq "$(CPU)" "x86"
        ifeq "$(MEMMODEL)" "direct"
                BLDSGL:=s
        else
                ifeq "$(MEMMODEL)" "flexible"
                        BLDSGL:=f
                else
                        BLDSGL:=
                endif
        endif
        ifdef SMP
                BLDSMP:=smp
        else
                BLDSMP:=
        endif
        EPOCROOT:=$(subst /,\,$(EPOCROOT))
        EPOCBLDABS := $(EPOCROOT)epoc32\build\tasm$(PBUILDPID)\$(BLDSGL)$(VNAME)$(BLDSMP)
else
        DRIVELETTER := $(shell cd 2>NUL)
        DRIVELETTER_2 := $(word 1,$(subst \, ,$(DRIVELETTER)))
        EPOCBLDABS_1 := $(subst $(TO_ROOT),,$(EPOCBLD))
        EPOCBLDABS_2 := $(subst $(DRIVELETTER_2),,$(EPOCBLDABS_1))
        
        EPOCBLDABS := $(call epocbldabs,$(DRIVELETTER_2),$(EPOCBLDABS_2))/$(NAME)
endif

EPOCINC := $(INC_PATH)
EPOCKERNINC := $(EPOCINC)/kernel
EPOCCPUINC := $(EPOCKERNINC)/$(CPU)
EPOCMMINC := $(INC_PATH)/memmodel/epoc/$(MEMMODEL)/$(CPU)
EPOCTRG := $(EPOCROOT)epoc32/release/$(PLATFORM_PATH)
TRG := $(EPOCTRG)/$(NAME).bin
TEMPTRG := $(EPOCBLDABS)/$(NAME).bin

ifdef EXTRA_INC_PATH
ASMINCPATH := $(EXTRA_INC_PATH)
endif

ifeq "$(CPU)" "arm"
ASMINCPATH := . $(EPOCBLDABS) $(ASMINCPATH) $(EXTENSION_ROOT) $(EPOCCPUINC)
ARMASM_OUT := $(shell armasm 2>&1)
ARMASM_OUT_4 := $(word 4,$(ARMASM_OUT))
ARMASM_OUT_6 := $(word 6,$(ARMASM_OUT))

# Use GCC toolchain if no other is available
TOOLVER := GCC

RVCTSTR := $(strip $(findstring RVCT, $(ARMASM_OUT_4)))
ifeq "$(RVCTSTR)" "RVCT"
        TOOLVER := RVCT
        OP := --
endif
ifeq "$(ARMASM_OUT_6)" "2.37"
        TOOLVER := 211
endif
endif

ifeq "$(MEMMODEL)" "direct"
CFG_MM := CFG_MMDirect
endif
ifeq "$(MEMMODEL)" "moving"
CFG_MM := CFG_MMMoving
endif
ifeq "$(MEMMODEL)" "multiple"
CFG_MM := CFG_MMMultiple
endif
ifeq "$(MEMMODEL)" "flexible"
CFG_MM := CFG_MMFlexible
endif
ifndef CFG_MM
$(error Memory model unknown)
endif

ASM_MACROS += $(CFG_MM)
ifdef SMP
        ASM_MACROS += SMP
endif

ifeq "$(CPU)" "x86"
        ifndef BASEINCLUDES
                BASEINCLUDES := bootcpu.inc bootmacr.inc
        endif
        ifndef BASESOURCES
                BASESOURCES := bootmain.asm bootcpu.asm bootutil.asm
        endif
        GENINCLUDES := $(GENINCLUDES) x86boot.h
        ASMINCPATH := . 
        ASM := tasm
        LINK := tlink
        EXE2BIN := exe2bin
        SRCEXT := asm
        INCEXT := inc
        OBJEXT := obj
        EXEEXT := exe
        
        ASMINCPATHCMD := $(foreach dir,$(ASMINCPATH),$(join /i,$(call slash2generic,$(dir))))   
        ASM_MACRO_CMD := $(foreach macro,$(ASM_MACROS),/d$(macro)=1)
        AFLAGS := /l /m3 /ML /W-PDC $(ASM_MACRO_CMD) $(ASMINCPATHCMD)
        LFLAGS := /m /s /n /3 /c
        ASMTYP := TASM
        LINKFILE :=
        define do_asm
                cd $(EPOCBLDABS) && $(CP) $(call slash2generic,$<) .
                cd $(EPOCBLDABS) && $(ASM) $(AFLAGS) $(notdir $<)
        endef
        define do_link
                cd $(EPOCBLDABS) && $(LINK) $(LFLAGS) $(filter %.$(OBJEXT),$(foreach obj,$^,$(notdir $(obj)))), temp.exe
                cd $(EPOCBLDABS) && $(COPY) temp.exe $@
                cd $(EPOCBLDABS) && $(ERASE) temp.exe
        endef
        define do_strip
                cd $(EPOCBLDABS) && $(COPY) $< temp.exe
                cd $(EPOCBLDABS) && $(EXE2BIN) temp.exe temp.bin
                cd $(EPOCBLDABS) && $(COPY) temp.bin $@
                cd $(EPOCBLDABS) && $(ERASE) temp.exe temp.bin
        endef
endif
ifeq "$(CPU)" "arm"
        ifeq "$(TOOLVER)" "211"
                ASM := armasm
                LINK := armlink
                SRCEXT := s
                INCEXT := inc
                OBJEXT := o
                EXEEXT := in
                ASMINCPATHCMD := $(foreach dir,$(ASMINCPATH),$(join -I ,$(dir)))
                ASM_MACRO_CMD := $(foreach macro,$(ASM_MACROS),-predefine "$(macro) SETL {TRUE}")
                AFLAGS := $(ASM_ARM211_VARIANTFLAGS) -apcs 3/32bit/nosw -Length 0 -Width 200 $(ASM_MACRO_CMD) $(ASMINCPATHCMD)
                LFLAGS := -Base $(LINKBASE) -Data 0xf0000000 -Entry $(LINKBASE) -Bin -map
                SYMOPT := -symbols
                ASMTYP := ARMASM
                LINKFILE :=
                define do_asm
                        $(ASM) $(AFLAGS) -o $@ -LIST $(join $(basename $@),.lst) $<
                endef
                define do_link
                        $(LINK) $(LFLAGS) $(SYMOPT) $(join $(basename $@),.sym) -o $@ $(filter %.$(OBJEXT),$^)
                endef
                define do_strip
                        @if exist $@ $(ERASE) $(call slash2generic,$@) 
                        $(COPY) $< $@
                endef
        endif
        ifeq "$(TOOLVER)" "RVCT"
                ASM_MACROS += USE_CXSF
                ASM := armasm
                LINK := armlink
                FROMELF := fromelf
                SRCEXT := s
                INCEXT := inc
                OBJEXT := o
                EXEEXT := in
                ASMINCPATHCMD := $(foreach dir,$(ASMINCPATH),$(join -I ,$(dir)))
                ASM_MACRO_CMD := $(foreach macro,$(ASM_MACROS),$(OP)predefine "$(macro) SETL {TRUE}")
                AFLAGS := -g $(OP)keep $(ASM_MACRO_CMD) $(ASMINCPATHCMD) $(ASM_RVCT_VARIANTFLAGS)
                LFLAGS := $(OP)ro-base $(LINKBASE) $(OP)entry $(LINKBASE) $(OP)map
                SYMOPT := $(OP)symdefs
                ASMTYP := ARMASM
                LINKFILE :=
                define do_asm
                        $(ASM) $(AFLAGS) -o $@ $(OP)LIST $(join $(basename $@),.lst) $<
                endef
                define do_link
                        $(LINK) $(LFLAGS) $(SYMOPT) $(join $(basename $@),.sym) -o $@ $(filter %.$(OBJEXT),$^)
                        $(COPY) $@ $(join $(basename $(TRG)),.sym)
                endef
                define do_strip
                        $(FROMELF) $(OP)bin $(OP)output $@ $<
                endef
        endif
        ifeq "$(TOOLVER)" "GCC"
                ASM_MACROS += USE_CXSF GNU_ASM
                ASM := as
                LINK := ld
                STRIP := strip
                SRCEXT := s
                INCEXT := ginc
                OBJEXT := o
                EXEEXT := in
                ASMINCPATHCMD := $(foreach dir,$(ASMINCPATH),$(join -I ,$(dir)))
                ASM_MACRO_CMD := $(foreach macro,$(ASM_MACROS),--defsym $(macro)=1 )
                AFLAGS := -mapcs-32 -R -n $(ASM_MACRO_CMD) -I- $(ASMINCPATHCMD)
                LFLAGS := -n -x --section-alignment 4 --file-alignment 2 -no-whole-archive
                SYMOPT := -symdefs
                ASMTYP := AS
                PROCESS_INCLUDES := 1
                ifndef LINKFILE
                        LINKFILE := bootstrap.lnk
                endif
                define do_asm
                        perl $(EPOCROOT)epoc32/tools/armasm2as.pl $< $(join $(basename $@),.ss)
                        $(ASM) $(AFLAGS) -acdhlms=$(join $(basename $@),.lst) -o $@ $(join $(basename $@),.ss)
                endef
                define do_link
                        if exist $(join $(basename $@),.lnk) $(ERASE) $(call slash2generic,$(join $(basename $@),.lnk)) 
                        $(COPY) $(subst /,\,$(filter %.lnk,$^)) $(join $(basename $@),.lnk)
                        $(LINK) -M -o $@ $(filter %.$(OBJEXT),$^) $(LFLAGS) --script=$(join $(basename $@),.lnk) >$(join $(basename $@),.map)
                endef
                define do_strip
                        $(STRIP) -O binary -o $(TEMPTRG) $<
                        $(COPY) $(TEMPTRG) $@
                        $(ERASE) $(call slash2generic,$(TEMPTRG)) 
                endef
        endif
endif



# Generic source files
ifndef BASESOURCES
BASESOURCES := bootmain.s bootcpu.s bootutils.s
endif

# Path for generic source files
ifndef BASESRCPATH
BASESRCPATH := $(E32PATH)/eka/kernel/$(CPU)
endif


# Generated include files
GENINCLUDES := $(foreach f,$(GENINCLUDES),$(basename $(f)).$(INCEXT))
GENINCLUDES := $(GENINCLUDES) e32rom.$(INCEXT) kernboot.$(INCEXT)
GENINCLUDES := $(GENINCLUDES) bootdefs.$(INCEXT)
ifneq "$(MEMMODEL)" "direct"
GENINCLUDES := $(GENINCLUDES) mmboot.$(INCEXT)
endif

# Headers from which GENINCLUDES are generated
GENHEADERS = $(foreach inc,$(GENINCLUDES),$(basename $(inc)).h)

# Non-generated generic include files
ifndef BASEINCLUDES
BASEINCLUDES := bootcpu.inc bootmacro.inc
endif
BASEINCLUDES := $(foreach f,$(BASEINCLUDES),$(basename $(f)).$(INCEXT))
INCLUDES := $(foreach f,$(INCLUDES),$(basename $(f)).$(INCEXT))

# Generic object files
BASEOBJECTS = $(foreach src, $(BASESOURCES), $(basename $(src)).$(OBJEXT))

# Platform specific object files
OBJECTS = $(foreach src, $(SOURCES), $(basename $(src)).$(OBJEXT))

# Object files with paths
FULLBASEOBJECTS = $(addprefix $(EPOCBLDABS)/,$(BASEOBJECTS))
FULLOBJECTS = $(addprefix $(EPOCBLDABS)/,$(OBJECTS))
LINKOBJECTS = $(FULLBASEOBJECTS) $(FULLOBJECTS)

# Generated include files with paths
FULLGENINCLUDES = $(addprefix $(EPOCBLDABS)/,$(GENINCLUDES))

# Tell make where to look for things
vpath %.h . $(EXTRA_INC_PATH) $(EPOCINC) $(EPOCKERNINC) $(EPOCCPUINC) $(EPOCMMINC)
vpath %.inc . $(EXTRA_INC_PATH) $(EXTENSION_ROOT) $(EPOCINC) $(EPOCKERNINC) $(EPOCCPUINC) $(EPOCMMINC) $(EPOCBLDABS)
vpath %.ginc $(EPOCBLDABS)
vpath %.$(SRCEXT) . $(EXTRA_SRC_PATH) $(EXTENSION_ROOT) $(BASESRCPATH)
vpath %.$(OBJEXT) $(EPOCBLDABS)
vpath %.lnk . $(EXTENSION_ROOT) $(EPOCCPUINC)

# How to link the object files 
$(EPOCBLDABS)/$(NAME).$(EXEEXT): $(LINKOBJECTS) $(LINKFILE) $(call pipe,$(EPOCBLDABS)) 
	$(do_link)

# How to strip linked object to binary
$(TRG): $(EPOCBLDABS)/$(NAME).$(EXEEXT)
	$(do_strip)

# How to assemble the source files
ifdef PROCESS_INCLUDES
FULLBASEINCLUDES := $(addprefix $(EPOCBLDABS)/,$(BASEINCLUDES))
FULLINCLUDES := $(addprefix $(EPOCBLDABS)/,$(INCLUDES))

$(FULLBASEINCLUDES) : $(EPOCBLDABS)/%.$(INCEXT) : %.inc $(call pipe,$(EPOCBLDABS))
	perl $(EPOCROOT)epoc32/tools/armasm2as.pl $< $@

$(FULLINCLUDES) : $(EPOCBLDABS)/%.$(INCEXT) : %.inc $(call pipe,$(EPOCBLDABS))
	perl $(EPOCROOT)epoc32/tools/armasm2as.pl $< $@

$(FULLBASEOBJECTS) : $(EPOCBLDABS)/%.$(OBJEXT) : %.$(SRCEXT) $(FULLINCLUDES) $(FULLBASEINCLUDES) $(FULLGENINCLUDES) $(call pipe,$(EPOCBLDABS))
	$(do_asm)

$(FULLOBJECTS) : $(EPOCBLDABS)/%.$(OBJEXT) : %.$(SRCEXT) $(FULLINCLUDES) $(FULLBASEINCLUDES) $(FULLGENINCLUDES) $(call pipe,$(EPOCBLDABS))
	$(do_asm)

else

ifeq "$(CPU)" "x86"
FULLBASEINCLUDES := $(addprefix $(EPOCBLDABS)/,$(BASEINCLUDES))
FULLINCLUDES := $(addprefix $(EPOCBLDABS)/,$(INCLUDES))

$(FULLBASEINCLUDES) $(FULLINCLUDES) : $(EPOCBLDABS)/%.inc : %.inc
	$(CP) $(call slash2generic,$<) $(call slash2generic,$@) 

$(FULLBASEOBJECTS) $(FULLOBJECTS) : $(EPOCBLDABS)/%.$(OBJEXT) : %.$(SRCEXT) $(FULLBASEINCLUDES) $(FULLGENINCLUDES) $(FULLINCLUDES)
	$(do_asm)

else
$(FULLBASEOBJECTS) $(FULLOBJECTS) : $(EPOCBLDABS)/%.$(OBJEXT) : %.$(SRCEXT) $(BASEINCLUDES) $(FULLGENINCLUDES) $(INCLUDES) $(call pipe,$(EPOCBLDABS))
	$(do_asm)

endif
endif

# How to translate the .h files to .inc
$(FULLGENINCLUDES) : $(EPOCBLDABS)/%.$(INCEXT) : %.h $(call pipe,$(EPOCBLDABS))
	perl $(EPOCROOT)epoc32/tools/h2inc.pl $< $@ $(ASMTYP)


# How to make the working directories
$(EPOCBLDABS) $(EPOCTRG) :
	$(call ifnotexistd,$(call slash2generic,$@))

# Makmake targets
.PHONY : MAKMAKE FREEZE LIB CLEANLIB RESOURCE FINAL BLD SAVESPACE RELEASABLES CLEAN
.PHONY : build skip

MAKMAKE :
	echo Nothing to do
	echo $(BASESRCPATH)

FREEZE :
	echo Nothing to do
	echo $(BASESRCPATH)

LIB :
	echo Nothing to do
	echo $(BASESRCPATH)

CLEANLIB :
	echo Nothing to do
	echo $(BASESRCPATH)

RESOURCE :
	echo Nothing to do
	echo $(BASESRCPATH)

FINAL :
	echo Nothing to do

BLD SAVESPACE : $(PROCEED)

RELEASABLES :
	@echo $(TRG)

CLEAN :
	-$(ERASE) $(call slash2generic,$(TRG)) 
	-$(ERASE) $(call slash2generic,$(EPOCBLDABS)/*.*) 

build: $(EPOCTRG) $(EPOCBLDABS) $(TRG)
	echo Bootstrap built for $(PLATFORM) $(CFG)

skip:
	echo Bootstrap build skipped for $(PLATFORM) $(CFG)
