/* unwinder.h
 *
 * Copyright 2002-2005 ARM Limited. All rights reserved.
 *
 * Your rights to use this code are set out in the accompanying licence
 * text file LICENCE.txt (ARM contract number LEC-ELA-00080 v2.0).
 */
/*
 * RCS $Revision: 91721 $
 * Checkin $Date: 2005-08-08 19:30:07 +0100 (Mon, 08 Aug 2005) $
 * Revising $Author: drodgman $
 */

/* Language-independent unwinder header public defines */

#ifndef UNWINDER_H
#define UNWINDER_H

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum {
    _URC_OK = 0,       /* operation completed successfully */
    _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
    _URC_HANDLER_FOUND = 6,
    _URC_INSTALL_CONTEXT = 7,
    _URC_CONTINUE_UNWIND = 8,
    _URC_FAILURE = 9   /* unspecified failure of some kind */
  } _Unwind_Reason_Code;
  
  typedef enum {
    _US_VIRTUAL_UNWIND_FRAME = 0,
    _US_UNWIND_FRAME_STARTING = 1,
    _US_UNWIND_FRAME_RESUME = 2
  } _Unwind_State;
  
  typedef struct _Unwind_Control_Block _Unwind_Control_Block;
  typedef struct _Unwind_Context _Unwind_Context;
  typedef uint32_t _Unwind_EHT_Header;
  
  
  /* UCB: */
  
  struct _Unwind_Control_Block {
    char exception_class[8];
    void (*exception_cleanup)(_Unwind_Reason_Code, _Unwind_Control_Block *);
    /* Unwinder cache, private fields for the unwinder's use */
    struct {
      uint32_t reserved1;     /* init reserved1 to 0, then don't touch */
      uint32_t reserved2;
      uint32_t reserved3;
      uint32_t reserved4;
      uint32_t reserved5;
    } unwinder_cache;
    /* Propagation barrier cache (valid after phase 1): */
    struct {
      uint32_t sp;
      uint32_t bitpattern[5];
    } barrier_cache;
    /* Cleanup cache (preserved over cleanup): */
    struct {
      uint32_t bitpattern[4];
    } cleanup_cache;
    /* Pr cache (for pr's benefit): */
    struct {
      uint32_t fnstart;             /* function start address */
      _Unwind_EHT_Header *ehtp;     /* pointer to EHT entry header word */
      uint32_t additional;          /* additional data */
      uint32_t reserved1;
    } pr_cache;
    long long int :0;               /* Force alignment of next item to 8-byte boundary */
  };
  
  /* Interface functions: */
  
  _Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Control_Block *ucbp);
  NORETURNDECL void _Unwind_Resume(_Unwind_Control_Block *ucbp);
  void _Unwind_Complete(_Unwind_Control_Block *ucbp);
  void _Unwind_DeleteException(_Unwind_Control_Block *ucbp); 

  /* Virtual Register Set*/
        
  typedef enum {
    _UVRSC_CORE = 0,      /* integer register */
    _UVRSC_VFP = 1,       /* vfp */
                          /* 2: was fpa (obsolete) */
    _UVRSC_WMMXD = 3,     /* Intel WMMX data register */
    _UVRSC_WMMXC = 4      /* Intel WMMX control register */
  } _Unwind_VRS_RegClass;
  
  typedef enum {
    _UVRSD_UINT32 = 0,
    _UVRSD_VFPX = 1,
                          /* 2: was fpa (obsolete) */
    _UVRSD_UINT64 = 3,
    _UVRSD_FLOAT = 4,
    _UVRSD_DOUBLE = 5
  } _Unwind_VRS_DataRepresentation;
  
  typedef enum {
    _UVRSR_OK = 0,
    _UVRSR_NOT_IMPLEMENTED = 1,
    _UVRSR_FAILED = 2
  } _Unwind_VRS_Result;

  IMPORT_C _Unwind_VRS_Result _Unwind_VRS_Set(_Unwind_Context *context,
                                              _Unwind_VRS_RegClass regclass,
                                              uint32_t regno,
                                              _Unwind_VRS_DataRepresentation representation,
                                              void *valuep);
  
  IMPORT_C _Unwind_VRS_Result _Unwind_VRS_Get(_Unwind_Context *context,
                                              _Unwind_VRS_RegClass regclass,
                                              uint32_t regno,
                                              _Unwind_VRS_DataRepresentation representation,
                                              void *valuep);
  
  IMPORT_C _Unwind_VRS_Result _Unwind_VRS_Pop(_Unwind_Context *context,
                                              _Unwind_VRS_RegClass regclass,
                                              uint32_t descriminator,
                                              _Unwind_VRS_DataRepresentation representation);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* defined UNWINDER_H */
