/* unwinder.c
 *
 * Copyright 2002-2005 ARM Limited. All rights reserved.
 *
 * Your rights to use this code are set out in the accompanying licence
 * text file LICENCE.txt (ARM contract number LEC-ELA-00080 v1.0).
 */

/* Portions copyright Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies). */

/*
 * RCS $Revision: 92986 $
 * Checkin $Date: 2005-10-13 15:56:12 +0100 (Thu, 13 Oct 2005) $
 * Revising $Author: achapman $
 */

/* Language-independent unwinder implementation */

/* This source file is compiled automatically by ARM's make system into
 * multiple object files. The source regions constituting object file
 * xxx.o are delimited by ifdef xxx_c / endif directives.
 *
 * The source regions currently marked are:
 * unwinder_c
 * unwind_activity_c
 */

#ifndef __EPOC32__
#include <stddef.h>
#include <stdlib.h>
#else
#include <e32def.h>
#endif
/* Environment: */
#include "unwind_env.h"
/* Language-independent unwinder declarations: */
#include "unwinder.h"

#ifdef __EPOC32__
/* Symbian specific support */
#include "symbian_support.h"
#endif

/* Define UNWIND_ACTIVITY_DIAGNOSTICS for printed information from _Unwind_Activity */
/* Define VRS_DIAGNOSTICS for printed diagnostics about VRS operations */

#if defined(VRS_DIAGNOSTICS) || defined(UNWIND_ACTIVITY_DIAGNOSTICS)
#ifndef __EPOC32__
extern int printf(const char *, ...);
#endif
#endif

#ifdef SUPPORT_NESTED_EXCEPTIONS
extern _Unwind_Control_Block *AllocSavedUCB();
extern void FreeSavedUCB(_Unwind_Control_Block *context);
#endif

#ifdef unwinder_c

/* =========================                      ========================= */
/* ========================= Virtual register set ========================= */
/* =========================                      ========================= */

/* The approach taken by this implementation is to use the real machine
 * registers to hold all but the values of core (integer)
 * registers. Consequently the implementation must use only the core
 * registers except when manipulating the virtual register set. Non-core
 * registers are saved only on first use, so the single implementation can
 * cope with execution on processors which lack certain registers.  The
 * registers as they were at the start of the propagation must be preserved
 * over phase 1 so that the machine state is correct at the start of phase
 * 2. This requires a copy to be taken (which can be stack allocated). During
 * a stack unwind (phase 1 or phase 2), the "current" virtual register set is
 * implemented as core register values held in a data structure, and non-core
 * register values held in the registers themselves. To ensure that all
 * original register values are available at the beginning of phase 2, the
 * core registers are saved in a second structure at the start of phase 1 and
 * the non-core registers are demand-saved into another part of the data
 * structure that holds the current core registers during the phase 1 stack
 * unwind.
 */
/* Extent to which the access routines are implemented:
 * _Unwind_VRS_Get and _Unwind_VRS_Set implement only access to the core registers.
 * _Unwind_VRS_Pop implements only popping of core and vfp registers.
 * There is no support here for the Intel WMMX registers, but space is nevertheless
 * reserved in the virtual register set structure to indicate whether demand-saving
 * of those registers is required (as they are unsupported, it never is). The space
 * costs nothing as it is required for alignment.
 * The level of supported functionality is compliant with the requirements of the
 * Exceptions ABI.
 */

typedef unsigned char bool;
struct core_s  { uint32_t r[16]; };        /* core integer regs */
struct vfp_s   { uint64_t d[32]; };        /* VFP registers saved in FSTMD format */

/* Phase 1 virtual register set includes demand-save areas */
/* The phase 2 virtual register set must be a prefix of the phase 1 set */
typedef struct phase1_virtual_register_set_s {
  /* demand_save flag == 1 means save the registers in the demand-save area */
  bool demand_save_vfp_low;
  bool demand_save_vfp_high;
  bool demand_save_wmmxd;
  bool demand_save_wmmxc;
  struct core_s core;      /* current core registers */
  struct vfp_s  vfp;       /* demand-saved vfp registers */
} phase1_virtual_register_set;

/* Phase 2 virtual register set has no demand-save areas */
/* The phase 2 virtual register set must be a prefix of the phase 1 set */
/* The assembly fragments for _Unwind_RaiseException and _Unwind_Resume create
 * a phase2_virtual_register_set_s by hand so be careful.
 */
typedef struct phase2_virtual_register_set_s {
  /* demand_save flag == 1 means save the registers in the demand-save area */
  /* Always 0 in phase 2 */
  bool demand_save_vfp_low;
  bool demand_save_vfp_high;
  bool demand_save_wmmxd;
  bool demand_save_wmmxc;
  struct core_s core;      /* current core registers */
} phase2_virtual_register_set;

/* -- Helper macros for the embedded assembly */

#if defined(__TARGET_ARCH_5T)  || defined(__TARGET_ARCH_5TXM) || \
    defined(__TARGET_ARCH_5TE) || defined(__TARGET_ARCH_6) || \
    defined(__TARGET_ARCH_6T2) || defined(__TARGET_ARCH_7_A) /* || ... */
  #define ARCH_5T_OR_LATER 1
#else
  #define ARCH_5T_OR_LATER 0
#endif

#if defined(__APCS_INTERWORK) && !ARCH_5T_OR_LATER
  #define OLD_STYLE_INTERWORKING 1
#else
  #define OLD_STYLE_INTERWORKING 0
#endif

#if defined(__TARGET_ARCH_4T) || defined(__TARGET_ARCH_4TXM) || ARCH_5T_OR_LATER
  #define HAVE_BX 1
#else
  #define HAVE_BX 0
#endif

#if defined(__TARGET_ARCH_THUMBNAIL)
  #define THUMBNAIL 1
#else
  #define THUMBNAIL 0
#endif

#if HAVE_BX
  #define RET_LR bx lr
#else
  #define RET_LR mov pc,lr
#endif

/* ----- Routines: ----- */

/* ----- Helper routines, private ----- */

/* R_ARM_PREL31 is a place-relative 31-bit signed relocation.  The
 * routine takes the address of a location that was relocated by
 * R_ARM_PREL31, and returns an absolute address.
 */
static FORCEINLINE uint32_t __ARM_resolve_prel31(void *p)
{
  return (uint32_t)((((*(int32_t *)p) << 1) >> 1) + (int32_t)p);
}

/* ----- Helper routines, private but external ----- */

/* Note '%0' refers to local label '0' */
#if defined(__thumb)
#define MAYBE_SWITCH_TO_ARM_STATE SWITCH_TO_ARM_STATE
#define MAYBE_CODE16 code16
#else
#define MAYBE_SWITCH_TO_ARM_STATE /* nothing */
#define MAYBE_CODE16              /* nothing */
#endif
__asm void __ARM_Unwind_VRS_VFPpreserve_low(void *vfpp)
{
vfp_d0 CN 0;
  /* Preserve the low vfp registers in the passed memory */
#if defined(__thumb)
  macro;
  SWITCH_TO_ARM_STATE;
1
  align 4;
2
  assert (%2 - %1) = 0;
  bx pc;
  nop;
  code32;
  mend;
#endif

  MAYBE_SWITCH_TO_ARM_STATE;
  stc   p11,vfp_d0,[r0],{0x20};  /* 0xec800b20  FSTMIAD r0,{d0-d15} */
  RET_LR;
  MAYBE_CODE16;
}

__asm void __ARM_Unwind_VRS_VFPpreserve_high(void *vfpp)
{
vfp_d16 CN 0;                      /* =16 when used with stcl */
  /* Preserve the high vfp registers in the passed memory */
  MAYBE_SWITCH_TO_ARM_STATE;
  stcl  p11,vfp_d16,[r0],{0x20};  /* 0xecc00b20  FSTMIAD r0,{d16-d31} */
  RET_LR;
  MAYBE_CODE16;
}

__asm void __ARM_Unwind_VRS_VFPrestore_low(void *vfpp)
{
  /* Restore the low vfp registers from the passed memory */
vfp_d0 CN 0;
  MAYBE_SWITCH_TO_ARM_STATE;
  ldc   p11,vfp_d0,[r0],{0x20};  /* 0xec900b20  FLDMIAD r0,{d0-d15} */
  RET_LR;
  MAYBE_CODE16;
}

__asm void __ARM_Unwind_VRS_VFPrestore_high(void *vfpp)
{
  /* Restore the high vfp registers from the passed memory */
vfp_d16 CN 0;                      /* =16 when used with ldcl */
  MAYBE_SWITCH_TO_ARM_STATE;
  ldcl   p11,vfp_d16,[r0],{0x20};  /* 0xecd00b20  FLDMIAD r0,{d16-d31} */
  RET_LR;
  MAYBE_CODE16;
}


__asm NORETURNDECL void __ARM_Unwind_VRS_corerestore(void *corep)
{
  /* We rely here on corep pointing to a location in the stack,
   * as we briefly assign it to sp. This allows us to safely do
   * ldmia's which restore sp (if we use a different base register,
   * the updated sp may be used by the handler of any data abort
   * that occurs during the ldmia, and the stack gets overwritten).
   * By hypothesis this is preserve8 but the load of sp means the
   * assembler can't infer that.
   */
#if THUMBNAIL
  preserve8;
  mov.w   r13, r0;
  ldmia.w r13!,{r0-r12};
  ldr.w   r14, [r13, #4]   /* lr */
  ldr.w   r12, [r13, #4*2] /* pc */
  ldr.w   r13, [r13, #0]   /* sp */
  bx      r12
  
#else
  preserve8;
  MAYBE_SWITCH_TO_ARM_STATE;
#if OLD_STYLE_INTERWORKING
  mov   r13, r0;
  ldmia r13!,{r0-r12};
  ldr   r12,[r13, #4*2]; /* pc */
  ldmia r13,{r13-r14};
  bx    r12;
#else

  #if __ARMCC_VERSION < 300000
  mov   r13, r0;
  ldmia r13,{r0-r15};
  #else
  mov r14, r0;
  ldmia r14!, {r0-r12};
  ldr r13, [r14], #4;
  ldmia r14, {r14,r15};
  #endif

#endif
  MAYBE_CODE16;
#endif
}


/* ----- Development support ----- */

#ifdef VRS_DIAGNOSTICS
static void debug_print_vrs_vfp(uint32_t base, uint64_t *lp)
{
  int c = 0;
  int i;
  for (i = 0; i < 16; i++) {
    printf("D%-2d  0x%16.16llx    ", i + base, *lp);
    lp++;
    if (c++ == 1) {
      c = 0;
      printf("\n");
    }
  }
}


static void debug_print_vrs(_Unwind_Context *context)
{
  phase1_virtual_register_set *vrsp = (phase1_virtual_register_set *)context;
  int i;
  int c;
  printf("------------------------------------------------------------------------\n");
  c = 0;
  for (i = 0; i < 16; i++) {
    printf("r%-2d  0x%8.8x    ", i, vrsp->core.r[i]);
    if (c++ == 3) {
      c = 0;
      printf("\n");
    }
  }

  printf("-----\n");
  if (vrsp->demand_save_vfp_low == 1)
    printf("VFP low registers not saved\n");
  else
    debug_print_vrs_vfp(0, &vrsp->vfp.d[0]);
  printf("-----\n");
  if (vrsp->demand_save_vfp_high == 1)
    printf("VFP high registers not saved\n");
  else
    debug_print_vrs_vfp(16, &vrsp->vfp.d[16]);
  printf("------------------------------------------------------------------------\n");
}
#endif


/* ----- Public routines ----- */

EXPORT_C _Unwind_VRS_Result _Unwind_VRS_Set(_Unwind_Context *context,
                                            _Unwind_VRS_RegClass regclass,
                                            uint32_t regno,
                                            _Unwind_VRS_DataRepresentation representation,
                                            void *valuep)
{
  phase1_virtual_register_set *vrsp = (phase1_virtual_register_set *)context;
  switch (regclass) {
  case _UVRSC_CORE:
    {
      if (representation != _UVRSD_UINT32 || regno > 15)
        return _UVRSR_FAILED;
       vrsp->core.r[regno] = *(uint32_t *)valuep;
       return _UVRSR_OK;
    }
  case _UVRSC_VFP:
  case _UVRSC_WMMXD:
  case _UVRSC_WMMXC:
    return _UVRSR_NOT_IMPLEMENTED;
  default:
    break;
  }
  return _UVRSR_FAILED;
}


EXPORT_C _Unwind_VRS_Result _Unwind_VRS_Get(_Unwind_Context *context,
                                            _Unwind_VRS_RegClass regclass,
                                            uint32_t regno,
                                            _Unwind_VRS_DataRepresentation representation,
                                            void *valuep)
{
  phase1_virtual_register_set *vrsp = (phase1_virtual_register_set *)context;
  switch (regclass) {
  case _UVRSC_CORE:
    {
      if (representation != _UVRSD_UINT32 || regno > 15)
        return _UVRSR_FAILED;
      *(uint32_t *)valuep = vrsp->core.r[regno];
      return _UVRSR_OK;
    }
  case _UVRSC_VFP:
  case _UVRSC_WMMXD:
  case _UVRSC_WMMXC:
    return _UVRSR_NOT_IMPLEMENTED;
  default:
    break;
  }
  return _UVRSR_FAILED;
}


#define R_SP 13

EXPORT_C _Unwind_VRS_Result _Unwind_VRS_Pop(_Unwind_Context *context,
                                            _Unwind_VRS_RegClass regclass,
                                            uint32_t descriminator,
                                            _Unwind_VRS_DataRepresentation representation)
{
  phase1_virtual_register_set *vrsp = (phase1_virtual_register_set *)context;
  switch (regclass) {
  case _UVRSC_CORE:
    {
      /* If SP is included in the mask, the loaded value is used in preference to
       * the writeback value, but only on completion of the loading.
       */
      uint32_t mask, *vsp, *rp, sp_loaded;
      if (representation != _UVRSD_UINT32)
        return _UVRSR_FAILED;
      vsp = (uint32_t *)vrsp->core.r[R_SP];
      rp = (uint32_t *)&vrsp->core;
      mask = descriminator & 0xffff;
      sp_loaded = mask & (1 << R_SP);
      while (mask != 0) {
        if (mask & 1) {
#ifdef VRS_DIAGNOSTICS
          printf("VRS Pop r%d\n", rp - &vrsp->core.r[0]);
#endif
          *rp = *vsp++;
        }
        rp++;
        mask >>= 1;
      }
      if (!sp_loaded)
        vrsp->core.r[R_SP] = (uint32_t)vsp;
      return _UVRSR_OK;
    }
  case _UVRSC_VFP:
    {
      uint32_t start = descriminator >> 16;
      uint32_t count = descriminator & 0xffff;
      bool some_low = start < 16;
      bool some_high = start + count > 16;
      if ((representation != _UVRSD_VFPX && representation != _UVRSD_DOUBLE) ||
          (representation == _UVRSD_VFPX && some_high) ||
          (representation == _UVRSD_DOUBLE && start + count > 32))
        return _UVRSR_FAILED;
      if (some_low && vrsp->demand_save_vfp_low == 1) { /* Demand-save over phase 1 */
        vrsp->demand_save_vfp_low = 0;
        __ARM_Unwind_VRS_VFPpreserve_low(&vrsp->vfp.d[0]);
      }
      if (some_high && vrsp->demand_save_vfp_high == 1) { /* Demand-save over phase 1 */
        vrsp->demand_save_vfp_high = 0;
        __ARM_Unwind_VRS_VFPpreserve_high(&vrsp->vfp.d[16]);
      }
      /* Now recover from the stack into the real machine registers.
       * Note for _UVRSD_VFPX we assume FSTMX standard format 1.
       * Do this by saving the current VFP registers to a memory area,
       * moving the in-memory values into that area, and
       * restoring from the whole area.
       * Must be careful as the 64-bit values saved by FSTMX might be
       * only 32-bit aligned.
       */
      {
        struct unaligned_vfp_reg_s { uint32_t w1; uint32_t w2; };
        struct unaligned_vfp_reg_s *vsp;
        struct vfp_s temp_vfp;
        if (some_low)
          __ARM_Unwind_VRS_VFPpreserve_low(&temp_vfp.d[0]);
        if (some_high)
          __ARM_Unwind_VRS_VFPpreserve_high(&temp_vfp.d[16]);
        vsp = (struct unaligned_vfp_reg_s *)vrsp->core.r[R_SP];
        while (count--) {
          struct unaligned_vfp_reg_s *v =
            (struct unaligned_vfp_reg_s *)&temp_vfp.d[start++];
          *v = *vsp++;
#ifdef VRS_DIAGNOSTICS
          printf("VRS Pop D%d = 0x%llx\n", start - 1, temp_vfp.d[start - 1]);
#endif
        }
        vrsp->core.r[R_SP] = (uint32_t)((uint32_t *)vsp +
                                        (representation == _UVRSD_VFPX ?
                                         1 : /* +1 to skip the format word */
                                         0));
        if (some_low)
          __ARM_Unwind_VRS_VFPrestore_low(&temp_vfp.d[0]);
        if (some_high)
          __ARM_Unwind_VRS_VFPrestore_high(&temp_vfp.d[16]);
      }
      return _UVRSR_OK;
    }
  case _UVRSC_WMMXD:
  case _UVRSC_WMMXC:
    return _UVRSR_NOT_IMPLEMENTED;
  default:
    break;
  }
  return _UVRSR_FAILED;
}



/* =========================              ========================= */
/* ========================= The unwinder ========================= */
/* =========================              ========================= */


/* This implementation uses the UCB unwinder_cache as follows:
 * reserved1 is documented in the EABI as requiring initialisation to 0.
 *  It is used to manage nested simultaneous propagation. If the value is 0,
 *  the UCB is participating in no propagations. If the value is 1, the UCB
 *  is participating in one propagation. Otherwise the value is a pointer to
 *  a structure holding saved UCB state from the next propagation out.
 *  The structure used is simply a mallocated UCB.
 * reserved2 is used to preserve the call-site address over calls to a
 *  personality routine and cleanup.
 * reserved3 is used to cache the PR address.
 * reserved4 is used by the Symbian implementation to cache the ROM exeception 
 *  search table
 * reserved5 is used by the symbian implementation to cache the 
 *  TExceptionDescriptor for the executable of the 'current' frame
 */

#define NESTED_CONTEXT      unwinder_cache.reserved1
#define SAVED_CALLSITE_ADDR unwinder_cache.reserved2
#define PR_ADDR             unwinder_cache.reserved3

/* Index table entry: */

#ifndef __EPOC32__  // Symbian OS defines this in symbian_support.h
typedef struct __EIT_entry {
  uint32_t fnoffset; /* Place-relative */
  uint32_t content;
} __EIT_entry;
#endif

/* Private defines etc: */

static const uint32_t EXIDX_CANTUNWIND = 1;
static const uint32_t uint32_highbit = 0x80000000;

/* ARM C++ personality routines: */

typedef _Unwind_Reason_Code (*personality_routine)(_Unwind_State,
                                                   _Unwind_Control_Block *,
                                                   _Unwind_Context *);

WEAKDECL _Unwind_Reason_Code __aeabi_unwind_cpp_pr0(_Unwind_State state, _Unwind_Control_Block *,
                                                    _Unwind_Context *context);
IMPORT_C WEAKDECL _Unwind_Reason_Code __aeabi_unwind_cpp_pr1(_Unwind_State state, _Unwind_Control_Block *,
                                                             _Unwind_Context *context);
IMPORT_C WEAKDECL _Unwind_Reason_Code __aeabi_unwind_cpp_pr2(_Unwind_State state, _Unwind_Control_Block *,
                                                             _Unwind_Context *context);


/* Various image symbols: */

struct ExceptionTableInfo {
  uint32_t EIT_base;
  uint32_t EIT_limit;
};

#ifndef __EPOC32__
/* We define __ARM_ETInfo to allow access to some linker-generated
   names that are not legal C identifiers. __ARM_ETInfo is extern only
   because of scope limitations of the embedded assembler */
extern const struct ExceptionTableInfo __ARM_ETInfo;
#define EIT_base \
    ((const __EIT_entry *)(__ARM_ETInfo.EIT_base + (const char *)&__ARM_ETInfo))
#define EIT_limit \
    ((const __EIT_entry *)(__ARM_ETInfo.EIT_limit + (const char *)&__ARM_ETInfo))

#endif


/* ----- Index table processing ----- */

/* find_and_expand_eit_entry is a support function used in both phases to set
 * ucb.pr_cache and internal cache.
 * Call with a pointer to the ucb and the return address to look up.
 *
 * The table is contained in the half-open interval
 * [EIT_base, EIT_limit) and is an ordered array of __EIT_entrys.
 * Perform a binary search via C library routine bsearch.
 * The table contains only function start addresses (encoded as offsets), so
 * we need to special-case the end table entry in the comparison function,
 * which we do by assuming the function it describes extends to end of memory.
 * This causes us problems indirectly in that we would like to fault as
 * many attempts as possible to look up an invalid return address. There are
 * several ways an invalid return address can be obtained from a broken
 * program, such as someone corrupting the stack or broken unwind instructions
 * recovered the wrong value. It is plausible that many bad return addresses
 * will be either small integers or will point into the heap or stack, hence
 * it's desirable to get the length of that final function roughly right.
 * Here we make no attempt to do it. Code exclusively for use in toolchains
 * which define a suitable limit symbol could make use of that symbol.
 * Alternatively (QoI) a smart linker could augment the index table with a
 * dummy EXIDX_CANTUNWIND entry pointing just past the last real function.
 */

#ifndef __EPOC32__
static int EIT_comparator(const void *ck, const void *ce)
{
  uint32_t return_address = *(const uint32_t *)ck;
  const __EIT_entry *eitp = (const __EIT_entry *)ce;
  const __EIT_entry *next_eitp = eitp + 1;
  uint32_t next_fn;
  if (next_eitp != EIT_limit)
    next_fn = __ARM_resolve_prel31((void *)&next_eitp->fnoffset);
  else
    next_fn = 0xffffffffU;
  if (return_address < __ARM_resolve_prel31((void *)&eitp->fnoffset)) return -1;
  if (return_address >= next_fn) return 1;
  return 0;
}
#endif


static _Unwind_Reason_Code find_and_expand_eit_entry_V2(_Unwind_Control_Block *ucbp,
                                                     uint32_t return_address)
{
  /* Search the index table for an entry containing the specified return
   * address. Subtract the 2 from the return address, as the index table
   * contains function start addresses (a trailing noreturn BL would
   * appear to return to the first address of the next function (perhaps
   * +1 if Thumb); a leading BL would appear to return to function start
   * + instruction size (perhaps +1 if Thumb)).
   */

#ifndef __EPOC32__
  const __EIT_entry *base = EIT_base;
  size_t nelems = EIT_limit - EIT_base;
  __EIT_entry *eitp;

  return_address -= 2;

  eitp = (__EIT_entry *) bsearch(&return_address, base, nelems,
                                 sizeof(__EIT_entry), EIT_comparator);
#else
  const __EIT_entry *base = EIT_base(ucbp);
  size_t nelems = EIT_limit(ucbp) - base;
  __EIT_entry *eitp;

  return_address -= 2;

  // This must succeed on SymbianOS or else an error will have occured already.
  eitp = SearchEITV2(return_address, base, nelems);
#endif

  if (eitp == NULL) {
    /* The return address we have was not found in the EIT.
     * This breaks the scan and we have to indicate failure.
     */
    ucbp->PR_ADDR = NULL;
    DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_UNWINDER, _UAACT_ENDING, _UAARG_ENDING_UNWINDER_LOOKUPFAILED);
    return _URC_FAILURE;
  }

  /* Cache the function offset */

  ucbp->pr_cache.fnstart = __ARM_resolve_prel31((void *)&eitp->fnoffset);

  /* Can this frame be unwound at all? */

  if (eitp->content == EXIDX_CANTUNWIND) {
    ucbp->PR_ADDR = NULL;
    DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_UNWINDER, _UAACT_ENDING, _UAARG_ENDING_NOUNWIND);
    return _URC_FAILURE;
  }

  /* Obtain the address of the "real" __EHT_Header word */

  if (eitp->content & uint32_highbit) {
    /* It is immediate data */
    ucbp->pr_cache.ehtp = (_Unwind_EHT_Header *)&eitp->content;
    ucbp->pr_cache.additional = 1;
  } else {
    /* The content field is a 31-bit place-relative offset to an _Unwind_EHT_Entry structure */
    ucbp->pr_cache.ehtp = (_Unwind_EHT_Header *)__ARM_resolve_prel31((void *)&eitp->content);
    ucbp->pr_cache.additional = 0;
  }

  /* Discover the personality routine address */

  if (*(uint32_t *)(ucbp->pr_cache.ehtp) & uint32_highbit) {
    /* It is immediate data - compute matching pr */
    uint32_t idx = ((*(uint32_t *)(ucbp->pr_cache.ehtp)) >> 24) & 0xf;
    if (idx == 0) ucbp->PR_ADDR = (uint32_t)&__aeabi_unwind_cpp_pr0;
    else if (idx == 1) ucbp->PR_ADDR = (uint32_t)&__aeabi_unwind_cpp_pr1;
    else if (idx == 2) ucbp->PR_ADDR = (uint32_t)&__aeabi_unwind_cpp_pr2;
    else { /* Failed */
      ucbp->PR_ADDR = NULL;
      DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_UNWINDER, _UAACT_ENDING, _UAARG_ENDING_TABLECORRUPT);
      return _URC_FAILURE;
    }
  } else {
    /* It's a place-relative offset to pr */
    ucbp->PR_ADDR = __ARM_resolve_prel31((void *)(ucbp->pr_cache.ehtp));
  }
  return _URC_OK;
}

static _Unwind_Reason_Code find_and_expand_eit_entry_V1(_Unwind_Control_Block *ucbp,
                                                     uint32_t return_address)
{
  /* Search the index table for an entry containing the specified return
   * address. The EIT contains function offsets relative to the base of the
   * execute region so adjust the return address accordingly.
   */

#ifndef __EPOC32__
  uint32_t return_address_offset = ADDR_TO_ER_RO_OFFSET(return_address, ucbp);
  const __EIT_entry *base = EIT_base;
  size_t nelems = EIT_limit - EIT_base;

   const __EIT_entry *eitp =
     (const __EIT_entry *) bsearch(&return_address_offset, base, nelems, 
                                   sizeof(__EIT_entry), EIT_comparator);
  if (eitp == NULL) {
    /* The return address we have was not found in the EIT.
     * This breaks the scan and we have to indicate failure.
     */
    ucbp->PR_ADDR = NULL;
    DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_UNWINDER, _UAACT_ENDING, _UAARG_ENDING_UNWINDER_LOOKUPFAILED);
    return _URC_FAILURE;
  }
#else
  /* Shouldn't we subtract 2 from here just like in the V2 lookup? 
   */
  uint32_t return_address_offset = ADDR_TO_ER_RO_OFFSET(return_address, ucbp);
  const __EIT_entry *base = EIT_base(ucbp);
  size_t nelems = EIT_limit(ucbp) - base;

  // This must succeed or else an error will have occured already.
  const __EIT_entry *eitp = SearchEITV1(return_address_offset, base, nelems);

#endif


  /* Cache the function offset */

  ucbp->pr_cache.fnstart = ER_RO_OFFSET_TO_ADDR(eitp->fnoffset, ucbp);

  /* Can this frame be unwound at all? */

  if (eitp->content == EXIDX_CANTUNWIND) {
    ucbp->PR_ADDR = NULL;
    DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_UNWINDER, _UAACT_ENDING, _UAARG_ENDING_NOUNWIND);
    return _URC_FAILURE;
  }

  /* Obtain the address of the "real" __EHT_Header word */
  if (eitp->content & uint32_highbit) {
    /* It is immediate data */
    ucbp->pr_cache.ehtp = (_Unwind_EHT_Header *)&eitp->content;
    ucbp->pr_cache.additional = 1;
  } else {
    /* The content field is a segment relative offset to an _Unwind_EHT_Entry structure */
    ucbp->pr_cache.ehtp = (_Unwind_EHT_Header *)ER_RO_OFFSET_TO_ADDR(eitp->content, ucbp);
    ucbp->pr_cache.additional = 0;
  }

  /* Discover the personality routine address */

  if (*(uint32_t *)(ucbp->pr_cache.ehtp) & uint32_highbit) {
    /* It is immediate data - compute matching pr */
    uint32_t idx = ((*(uint32_t *)(ucbp->pr_cache.ehtp)) >> 24) & 0xf;

    if (idx == 0) ucbp->PR_ADDR = (uint32_t)&__aeabi_unwind_cpp_pr0;
    else if (idx == 1) ucbp->PR_ADDR = (uint32_t)&__aeabi_unwind_cpp_pr1;
    else if (idx == 2) ucbp->PR_ADDR = (uint32_t)&__aeabi_unwind_cpp_pr2;
    else { /* Failed */
      ucbp->PR_ADDR = NULL;
      DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_UNWINDER, _UAACT_ENDING, _UAARG_ENDING_TABLECORRUPT);
      return _URC_FAILURE;
    }
  } else {
    /* Execute region offset to PR */
    ucbp->PR_ADDR = ER_RO_OFFSET_TO_ADDR(*(uint32_t *)(ucbp->pr_cache.ehtp), ucbp);

  }
  return _URC_OK;
}

static _Unwind_Reason_Code find_and_expand_eit_entry(_Unwind_Control_Block *ucbp,
                                                     uint32_t return_address)
{
  ValidateExceptionDescriptor(return_address, ucbp);
  if (EHABI_V2(ucbp))
    return find_and_expand_eit_entry_V2(ucbp, return_address);
  else
    return find_and_expand_eit_entry_V1(ucbp, return_address);
}


/* ----- Unwinding: ----- */

/* Fwd decl */
static NORETURNDECL void unwind_next_frame(_Unwind_Control_Block *ucbp, phase2_virtual_register_set *vrsp);

/* Helper fn: If the demand_save flag in a phase1_virtual_register_set was
 * zeroed, the registers were demand-saved. This function restores from
 * the save area.
*/
static FORCEINLINE void restore_non_core_regs(phase1_virtual_register_set *vrsp)
{
  if (vrsp->demand_save_vfp_low == 0)
    __ARM_Unwind_VRS_VFPrestore_low(&vrsp->vfp.d[0]);
  if (vrsp->demand_save_vfp_high == 0)
    __ARM_Unwind_VRS_VFPrestore_high(&vrsp->vfp.d[16]);
}

/* _Unwind_RaiseException is the external entry point to begin unwinding */
__asm _Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Control_Block *ucbp)
{
  extern __ARM_Unwind_RaiseException;

#if THUMBNAIL

  /* Create a phase2_virtual_register_set on the stack */
  /* Save the core registers, carefully writing the original sp value */
  /* Note we account for the pc but do not actually write it's value here */
  str.w    r14,[sp, #-8]!;
  add.w    r14, r13, #8;
  str.w    r14,[sp, #-4]!  /* pushed 3 words => 3 words */
  stmfd.w  sp!,{r0-r12};   /* pushed 13 words => 16 words */
  /* Write zeroes for the demand_save bytes so no saving occurs in phase 2 */
  mov.w    r1,#0;
  str.w    r1,[sp,#-4]!;   /* pushed 1 word => 17 words */
  mov.w    r1,sp;
  sub.w    sp,sp,#4;       /* preserve 8 byte alignment => 18 words */

  /* Now pass to C (with r0 still valid) to do the real work.
   * r0 = ucbp, r1 = phase2_virtual_register_set.
   * If we get control back, pop the stack and return preserving r0.
   */

  /* on arch 5T and later the linker will fix 'bl' => 'blx' as
     needed */
  bl.w     __ARM_Unwind_RaiseException;
  ldr.w    r14,[sp,#16*4];
  add.w    sp,sp,#18*4;
  bx lr;

#else

  MAYBE_SWITCH_TO_ARM_STATE;

  /* Create a phase2_virtual_register_set on the stack */
  /* Save the core registers, carefully writing the original sp value */
  #if __ARMCC_VERSION < 300000
  stmfd sp!,{r13-r15};  /* pushed 3 words => 3 words */
  #else
  stmdb r13, {r14,r15};
  str r13, [r13,#-3*4];
  sub r13, r13, #3*4;
  #endif
  stmfd sp!,{r0-r12};   /* pushed 13 words => 16 words */
  /* Write zeroes for the demand_save bytes so no saving occurs in phase 2 */
  mov r1,#0;
  str r1,[sp,#-4]!;     /* pushed 1 word => 17 words */
  mov r1,sp;
  sub sp,sp,#4;         /* preserve 8 byte alignment => 18 words */

  /* Now pass to C (with r0 still valid) to do the real work.
   * r0 = ucbp, r1 = phase2_virtual_register_set.
   * If we get control back, pop the stack and return preserving r0.
   */

#if OLD_STYLE_INTERWORKING
  ldr r2,Unwind_RaiseException_Offset;
  add r2,r2,pc;
  mov lr,pc;
Offset_Base
  bx    r2;
#else
  /* on arch 5T and later the linker will fix 'bl' => 'blx' as
     needed */
  bl  __ARM_Unwind_RaiseException;
#endif
  ldr r14,[sp,#16*4];
  add sp,sp,#18*4;
  RET_LR;
#if OLD_STYLE_INTERWORKING
Unwind_RaiseException_Offset dcd __ARM_Unwind_RaiseException - Offset_Base;
#endif
  MAYBE_CODE16;

#endif

#ifndef __EPOC32__
  /* Alternate symbol names for difficult symbols.
   * It is possible no functions included in the image require
   * a handler table. Therefore make only a weak reference to
   * the handler table base symbol, which may be absent.
   */
  align 4
  extern |.ARM.exidx$$Base|;
  extern |.ARM.exidx$$Limit|;
  extern |.ARM.extab$$Base| WEAKASMDECL;
  export __ARM_ETInfo;
  /* these are offsets for /ropi */
__ARM_ETInfo /* layout must match struct ExceptionTableInfo */
eit_base   dcd |.ARM.exidx$$Base|  - __ARM_ETInfo; /* index table base */
eit_limit  dcd |.ARM.exidx$$Limit| - __ARM_ETInfo; /* index table limit */
#endif
}


/* __ARM_Unwind_RaiseException performs phase 1 unwinding */

_Unwind_Reason_Code __ARM_Unwind_RaiseException(_Unwind_Control_Block *ucbp,
                                                phase2_virtual_register_set *entry_VRSp)
{
  phase1_virtual_register_set phase1_VRS;

  /* Is this a nested simultaneous propagation?
   * (see comments with _Unwind_Complete)
   */
  if (ucbp->NESTED_CONTEXT == 0) {
    /* No - this is only propagation */
    ucbp->NESTED_CONTEXT = 1;
  } else {
#ifdef SUPPORT_NESTED_EXCEPTIONS
    /* Yes - cache the state elsewhere and restore it when the propagation ends */
    /* This representation wastes space and uses malloc; do better?
     * On the other hand will it ever be used in practice?
     */
    _Unwind_Control_Block *saved_ucbp = AllocSavedUCB();
    if (ucbp == NULL) {
      DEBUGGER_BOTTLENECK(ucbp, _UASUBSYS_UNWINDER, _UAACT_ENDING, _UAARG_ENDING_UNWINDER_BUFFERFAILED);
      return _URC_FAILURE;
    }
    saved_ucbp->unwinder_cache = ucbp->unwinder_cache;
    saved_ucbp->barrier_cache = ucbp->barrier_cache;
    saved_ucbp->cleanup_cache = ucbp->cleanup_cache;
    ucbp->NESTED_CONTEXT = (uint32_t)saved_ucbp;
#else
    abort();
#endif
  }

  /* entry_VRSp contains the core registers as they were when
   * _Unwind_RaiseException was called.  Copy the call-site address to r15
   * then copy all the registers to phase1_VRS for the phase 1 stack scan.
   */

  entry_VRSp->core.r[15] = entry_VRSp->core.r[14];
  phase1_VRS.core = entry_VRSp->core;

  /* For phase 1 only ensure non-core registers are saved before use.
   * If WMMX registers are supported, initialise their flags here and
   * take appropriate action elsewhere.
   */

  phase1_VRS.demand_save_vfp_low = 1;
  phase1_VRS.demand_save_vfp_high = 1;
#ifdef __EPOC32__
  /* Set up Symbian specific caches in the _Unwind_Control_Block's 
     unwinder_cache. 
  */
  InitialiseSymbianSpecificUnwinderCache(phase1_VRS.core.r[15], ucbp);
#endif


  /* Now perform a virtual unwind until a propagation barrier is met, or
   * until something goes wrong.  If something does go wrong, we ought (I
   * suppose) to restore registers we may have destroyed.
   */

  while (1) {

    _Unwind_Reason_Code pr_result;

    /* Search the index table for the required entry.  Cache the index table
     * pointer, and obtain and cache the addresses of the "real" __EHT_Header
     * word and the personality routine.
     */

    if (find_and_expand_eit_entry(ucbp, phase1_VRS.core.r[15]) != _URC_OK) {
      restore_non_core_regs(&phase1_VRS);
      /* Debugger bottleneck fn called during lookup */
      return _URC_FAILURE;
    }

    /* Call the pr to decide what to do */

    pr_result = ((personality_routine)ucbp->PR_ADDR)(_US_VIRTUAL_UNWIND_FRAME,
                                                     ucbp,
                                                     (_Unwind_Context *)&phase1_VRS);

    if (pr_result == _URC_HANDLER_FOUND) break;
    if (pr_result == _URC_CONTINUE_UNWIND) continue;

    /* If we get here some sort of failure has occurred in the
     * pr and probably the pr returned _URC_FAILURE
     */
    restore_non_core_regs(&phase1_VRS);
    return _URC_FAILURE;
  }

  /* Propagation barrier located... restore entry register state of non-core regs */

  restore_non_core_regs(&phase1_VRS);

  /* Initiate real unwinding */
  unwind_next_frame(ucbp, entry_VRSp);
  /* Unreached, but keep compiler quiet: */
  return _URC_FAILURE;
}


/* unwind_next_frame performs phase 2 unwinding */

static NORETURNDECL void unwind_next_frame(_Unwind_Control_Block *ucbp, phase2_virtual_register_set *vrsp)
{
  while (1) {

    _Unwind_Reason_Code pr_result;

    /* Search the index table for the required entry.  Cache the index table
     * pointer, and obtain and cache the addresses of the "real" __EHT_Header
     * word and the personality routine.
     */

    if (find_and_expand_eit_entry(ucbp, vrsp->core.r[15]) != _URC_OK)
      abort();

    /* Save the call-site address and call the pr to do whatever it
     * wants to do on this new frame.
     */

    ucbp->SAVED_CALLSITE_ADDR = vrsp->core.r[15];
    pr_result = ((personality_routine)ucbp->PR_ADDR)(_US_UNWIND_FRAME_STARTING, ucbp,
                                                     (_Unwind_Context *)vrsp);

    if (pr_result == _URC_INSTALL_CONTEXT) {
      /* Upload the registers */
      __ARM_Unwind_VRS_corerestore(&vrsp->core);
    } else if (pr_result == _URC_CONTINUE_UNWIND)
      continue;
    else
      abort();
  }
}


/* _Unwind_Resume is the external entry point called after a cleanup
 * to resume unwinding. It tail-calls a helper function,
 * __ARM_Unwind_Resume, which never returns.
 */
__asm NORETURNDECL void _Unwind_Resume(_Unwind_Control_Block *ucbp)
{
  extern __ARM_Unwind_Resume;

#if THUMBNAIL

  /* Create a phase2_virtual_register_set on the stack */
  /* Save the core registers, carefully writing the original sp value */
  /* Note we account for the pc but do not actually write it's value here */
  str.w    r14,[sp, #-8]!;
  add.w    r14, r13, #8;
  str.w    r14,[sp, #-4]!    /* pushed 3 words => 3 words */
  stmfd.w  sp!,{r0-r12};     /* pushed 13 words => 16 words */
  /* Write zeroes for the demand_save bytes so no saving occurs in phase 2 */
  mov.w    r1,#0;
  str.w    r1,[sp,#-4]!;     /* pushed 1 word => 17 words */
  mov.w    r1,sp;
  sub.w    sp,sp,#4;         /* preserve 8 byte alignment => 18 words */

  /* Now pass to C (with r0 still valid) to do the real work.
   * r0 = ucbp, r1 = phase2_virtual_register_set.
   * This call never returns.
   */

  mov      pc,r2

#else

  MAYBE_SWITCH_TO_ARM_STATE;

  /* Create a phase2_virtual_register_set on the stack */
  /* Save the core registers, carefully writing the original sp value */

  #if __ARMCC_VERSION < 300000
  stmfd sp!,{r13-r15};  /* pushed 3 words => 3 words */
  #else
  stmdb r13, {r14,r15};
  str r13, [r13,#-3*4];
  sub r13, r13, #3*4;
  #endif

  stmfd sp!,{r0-r12};   /* pushed 13 words => 16 words */
  /* Write zeroes for the demand_save bytes so no saving occurs in phase 2 */
  mov r1,#0;
  str r1,[sp,#-4]!;     /* pushed 1 word => 17 words */
  mov r1,sp;
  sub sp,sp,#4;         /* preserve 8 byte alignment => 18 words */

  /* Now pass to C (with r0 still valid) to do the real work.
   * r0 = ucbp, r1 = phase2_virtual_register_set.
   * This call never returns.
   */

#ifdef __APCS_INTERWORK
  ldr r2,Unwind_Resume_Offset;
  add r2,r2,pc;
  bx    r2;
Unwind_Resume_Offset dcd __ARM_Unwind_Resume - .;
#else
  b __ARM_Unwind_Resume;
#endif
  MAYBE_CODE16;

#endif
}


/* Helper function for _Unwind_Resume */

NORETURNDECL void __ARM_Unwind_Resume(_Unwind_Control_Block *ucbp,
                                  phase2_virtual_register_set *entry_VRSp)
{
  _Unwind_Reason_Code pr_result;

  /* Recover saved state */

  entry_VRSp->core.r[15] = ucbp->SAVED_CALLSITE_ADDR;

  /* Call the cached PR and dispatch */

  pr_result = ((personality_routine)ucbp->PR_ADDR)(_US_UNWIND_FRAME_RESUME, ucbp,
                                                   (_Unwind_Context *)entry_VRSp);

  if (pr_result == _URC_INSTALL_CONTEXT) {
   /* Upload the registers */
    __ARM_Unwind_VRS_corerestore(&entry_VRSp->core);
  } else if (pr_result == _URC_CONTINUE_UNWIND)
    unwind_next_frame(ucbp, entry_VRSp);
  else
    abort();
}


/* _Unwind_Complete is called at the end of a propagation.
 * If we support multiple simultaneous propagations, restore the cached state
 * of the previous propagation here.
 */

void _Unwind_Complete(_Unwind_Control_Block *ucbp)
{
  _Unwind_Control_Block *context = (_Unwind_Control_Block *)ucbp->NESTED_CONTEXT;
  if ((uint32_t)context == 0) abort();  /* should be impossible */
  if ((uint32_t)context == 1) {
    /* This was the only ongoing propagation of this object */
    ucbp->NESTED_CONTEXT--;
    return;
  }
#ifdef SUPPORT_NESTED_EXCEPTIONS
  /* Otherwise we copy the state back from the cache structure pointed to
   * by ucbp->NESTED_CONTEXT.
   */
  /* This first one updates ucbp->NESTED_CONTEXT */
  ucbp->unwinder_cache = context->unwinder_cache;
  ucbp->barrier_cache = context->barrier_cache;
  ucbp->cleanup_cache = context->cleanup_cache;
  FreeSavedUCB(context);
#else
  abort();
#endif
}

/* _Unwind_DeleteException can be used to invoke the exception_cleanup
 * function after catching a foreign exception.
 */

void _Unwind_DeleteException(_Unwind_Control_Block *ucbp)
{
  if (ucbp->exception_cleanup != NULL)
    (ucbp->exception_cleanup)(_URC_FOREIGN_EXCEPTION_CAUGHT, ucbp);
}

#endif /* unwinder_c */
#ifdef unwind_activity_c

/* Runtime debug "bottleneck function": */
/* (not in the current Exceptions EABI document) */

void _Unwind_Activity(_Unwind_Control_Block *ucbp, uint32_t reason, uint32_t arg)
{
#ifdef UNWIND_ACTIVITY_DIAGNOSTICS
  uint32_t who = reason >> 24;
  uint32_t activity = reason & 0xffffff;
  printf("_Unwind_Activity: UCB=0x%8.8x Reason=(", (uint32_t)ucbp);
  switch (who) {
  case _UASUBSYS_UNWINDER:
    printf("unw,");
    if (activity >= 0x80)
      printf("%x) Arg=0x%8.8x\n", activity, arg);
    break;
  case _UASUBSYS_CPP:
    printf("C++,");
    if (activity >= 0x80) {
      if (activity == _UAACT_CPP_TYPEINFO)
        printf("typeinfo) Typeinfo=0x%8.8x\n", arg);
      else
        printf("%x) Arg=0x%8.8x\n", activity, arg);
    }
    break;
  default:
    printf("???,");
    if (activity >= 0x80)
      printf("%x) Arg=0x%8.8x\n", activity, arg);
    break;
  }
  if (activity < 0x80) {
    switch (activity) {
    case _UAACT_STARTING:
      printf("starting) Typeinfo=0x%8.8x\n", arg);
      break;
    case _UAACT_ENDING:
      printf("ending) Cause=%d\n", arg);
      break;
    case _UAACT_BARRIERFOUND:
      printf("barrierfound) Pad=0x%8.8x\n", arg);
      break;
    case _UAACT_PADENTRY:
      printf("padentry) Pad=0x%8.8x\n", arg);
      break;
    default:
      printf("%x) Arg=0x%8.8x\n", activity, arg);
      break;
    }
  }
#endif
}

#endif /* unwind_activity_c */
