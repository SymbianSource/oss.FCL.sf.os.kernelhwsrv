/* unwind_env.h
 *
 * Copyright 2003 ARM Limited.
 */
/*
  Licence
  
  1. Subject to the provisions of clause 2, ARM hereby grants to LICENSEE a 
  perpetual, non-exclusive, nontransferable, royalty free, worldwide licence 
  to use this Example Implementation of Exception Handling solely for the 
  purpose of developing, having developed, manufacturing, having 
  manufactured, offering to sell, selling, supplying or otherwise 
  distributing products which comply with the Exception Handling ABI for the 
  ARM Architecture specification. All other rights are reserved to ARM or its 
  licensors.
  
  2. THIS EXAMPLE IMPLEMENTATION OF EXCEPTION HANDLING  IS PROVIDED "AS IS" 
  WITH NO WARRANTIES EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED 
  TO ANY WARRANTY OF SATISFACTORY QUALITY, MERCHANTABILITY, NONINFRINGEMENT 
  OR FITNESS FOR A PARTICULAR PURPOSE.
*/
/*
 * RCS $Revision: 1.3 $
 * Checkin $Date: 2003/10/23 13:57:32 $
 * Revising $Author: agrant $
 */

/* Environment definition - abstractions and requirements - to aid
 * portability of the ARM exceptions code.
 */

#ifndef UNWINDENV_H
#define UNWINDENV_H

/* ---------------------------------------------------------------------- */

/* Source language
 *
 * The compiler is expected to define preprocessor symbols as follows:
 * __cplusplus when compiling in C++ mode.
 * __thumb when compiling to Thumb code.
 *
 * Some use is made of embedded assembly language, introduced by __asm.
 * This is described in ARM's toolchain documentation. Some edits may be
 * required for other compilers. The compiler should define one or more of:
 * __TARGET_ARCH_4T __TARGET_ARCH_4TXM __TARGET_ARCH_5T __TARGET_ARCH_5TXM
 * __TARGET_ARCH_5TE __TARGET_ARCH_6
 * so the correct assembly wrappers are generated for certain functions.
 *
 * __APCS_INTERWORK should be defined if ARM/Thumb interworking is required.
 *
 * For all the above symbols, if your compiler does not provide appropriate
 * definitions, add them here.
 *
 * Some source language extensions are also used.
 */

/* ---------------------------------------------------------------------- */

/* Library structure
 *
 * ARM's private make system contains an automated facility for compiling
 * source files multiple times to create multiple object files. The source
 * regions intended to constitute object file xxx.o are delimited by
 * #ifdef xxx_c / #endif directives. The exact preprocessor symbols used
 * for this conditionalisation are described in a comment at the start of
 * each file. When porting to a different system, compilations must be
 * performed with these preprocessor symbols appropriately defined
 * (or remove the conditionalisation).
 *
 * ARM declares (or redeclares) some routines as weak in order that
 * references to them are weak, so that the static linker will not load
 * unwanted code. This is achieved by decorating routine declarations
 * with appropriate language extensions. Note that compilers supporting
 * similar features but via a different syntax may require edits to
 * the library source.
 *
 * Define those decorations here (define as empty if not required):
 */

#define WEAKDECL __weak        /* token in C and C++ */
#define WEAKASMDECL [WEAK]     /* token in assembler */

/* ---------------------------------------------------------------------- */

/* Source language support and language extensions */

/* It is possible to compile the C++ semantics code using a compiler
 * which does not support C++ exceptions; this was useful to ARM whilst
 * ARM's compiler was being developed, and the facility has not been
 * removed. C++ exceptions syntax is conditionalised by
 * #ifdef ARM_EXCEPTIONS_ENABLED / #endif. Define ARM_EXCEPTIONS_ENABLED
 * by some means here if you want a usable library:
 */

#ifdef __cplusplus
extern "C" {
  /* For conditionalisation, specifically on ARM_EXCEPTIONS_ENABLED */
#include "basics.h"
}
#endif

/* The following definitions of syntax decoration may be empty if the
 * facility is not required. Note that compilers supporting similar
 * features but via a different syntax may require edits to the library
 * source.
 * 
 * Define the decorations here (define as empty if not required):
 */
  
/* If the compiler understands noreturn functions: */
#define NORETURNDECL __declspec(noreturn)

/* ---------------------------------------------------------------------- */

/* Types */

/* The implementation requires types uint8_t, uint16_t, uint32_t and
 * uint64_t to be defined as unsigned integers of the appropriate number
 * of bits.
 *
 * Do that here:
 */

#include <stdint.h>

/* The C++ semantics support requires definition of the RTTI object
 * layout. We use the same structures and names as the generic C++
 * ABI for Itanium.
 *
 * Define those structures here:
 */

#ifdef __cplusplus
extern "C" {
#include "cxxabi.h"
}
#endif

/* ---------------------------------------------------------------------- */

/* External requirements */

/* The C++ exception-handling 'globals' should be allocated per-thread.
 * The Exceptions ABI does not specify how this happens, but it is
 * intended that the details are localised to __cxa_get_globals.
 *
 * In the ARM implementation of __cxa_get_globals, it is assumed that a
 * zero-initialised location in a known per-thread place is somehow
 * obtainable, and can be assigned (by __cxa_get_globals) a pointer to
 * the allocated globals data structure. The macro EH_GLOBALS should be
 * defined here to yield a suitable address of type void*. This is used
 * only in __cxa_get_globals.
 *
 * Define it here:
 */

#ifdef __cplusplus
extern "C" {
  /* for __user_libspace() machinery */
#include <interns.h>
#define EH_GLOBALS libspace.eh_globals
}
#endif


/* A routine is required for C++ derived class to base class conversion.
 * This is used once, in __cxa_type_match. It is likely that suitable
 * code exists as part of the RTTI support code. Therefore access it
 * via a macro:
 * DERIVED_TO_BASE_CONVERSION(PTR, P_NEW_PTR, CLASS_INFO, BASE_INFO)
 *   Convert PTR from a pointer to a derived class (described by
 *   CLASS_INFO) to a pointer to a base class (described by BASE_INFO)
 *   and store the resulting pointer in P_NEW_PTR. Return true (or
 *   non-zero) if the base class was found and the conversion was done,
 *   otherwise return false (or zero).
 *
 * Define the macro here:
 */

#ifdef __cplusplus
/* In the ARM implementation, a suitable routine exists elsewhere in the
 * C++ runtime library, where it is part of the dynamic_cast mechanism.
 */
extern "C" int __derived_to_base_conversion(void** p_ptr, void** p_new_ptr,
                                            const std::type_info * class_info,
                                            const std::type_info * base_info,
                                            char** access_flags, int use_access_flags);

#define DERIVED_TO_BASE_CONVERSION(PTR, P_NEW_PTR, CLASS_INFO, BASE_INFO) \
  __derived_to_base_conversion(&(PTR), (P_NEW_PTR), (CLASS_INFO), (BASE_INFO), NULL, 0)
#endif


/* ---------------------------------------------------------------------- */

/* Runtime debug support
 *
 * Here we define the interface to a "bottleneck function" to be called
 * by exception handling code at 'interesting' points during execution,
 * and breakpointable by a debugger.
 *
 * This is not part of the Exceptions ABI but is expected to be
 * standardised elsewhere, probably in a Debug ABI.
 *
 * If you don't want this, define DEBUGGER_BOTTLENECK as a dummy, e.g.
 * #define DEBUGGER_BOTTLENECK(UCBP,LANG,ACTIVITY,ARG) (void)0
 */

#ifdef __cplusplus
extern "C" {
#endif

  struct _Unwind_Control_Block;

  typedef enum {
    _UASUBSYS_CPP      = 0x00,
    _UASUBSYS_UNWINDER = 0xff
  } _Unwind_Activity_subsystem;

  typedef enum {
    _UAACT_STARTING     = 0x0,
    _UAACT_ENDING       = 0x1,
    _UAACT_BARRIERFOUND = 0x2,
    _UAACT_PADENTRY     = 0x3,
    _UAACT_CPP_TYPEINFO = 0x80
  } _Unwind_Activity_activity;

  typedef enum {
    _UAARG_ENDING_UNSPECIFIED           = 0x0,
    _UAARG_ENDING_TABLECORRUPT          = 0x1,
    _UAARG_ENDING_NOUNWIND              = 0x2,
    _UAARG_ENDING_VRSFAILED             = 0x3,
    /* C++ only: */
    _UAARG_ENDING_CPP_BADOPCODE         = 0x4,
    /* Unwinder only: */
    _UAARG_ENDING_UNWINDER_LOOKUPFAILED = 0x4,
    _UAARG_ENDING_UNWINDER_BUFFERFAILED = 0x5
  } _Unwind_Activity_arg;

  void _Unwind_Activity(struct _Unwind_Control_Block *ucbp, uint32_t reason, uint32_t arg);
#define DEBUGGER_BOTTLENECK(UCBP,LANG,ACTIVITY,ARG) \
  _Unwind_Activity((UCBP),(((LANG)<<24)|ACTIVITY),(uint32_t)(ARG))

#ifdef __cplusplus
}
#endif


/* ---------------------------------------------------------------------- */

/* Printed diagnostics
 *
 * These may be useful for debugging purposes during development, provided
 * the execution environment supports diagnostics via printf.
 *
 * #define PR_DIAGNOSTICS for printed diagnostics from the personality routine.
 * #define VRS_DIAGNOSTICS for printed diagnostics about VRS operations.
 * #define UNWIND_ACTIVITY_DIAGNOSTICS for printed information from _Unwind_Activity.
 */

/* ---------------------------------------------------------------------- */

#endif /* defined UNWINDENV_H */
