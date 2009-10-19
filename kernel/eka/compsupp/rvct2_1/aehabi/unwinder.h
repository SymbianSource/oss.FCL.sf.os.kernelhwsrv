/* unwinder.h
 *
 * Copyright 2002-2003 ARM Limited.
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
 * RCS $Revision: 1.7 $
 * Checkin $Date: 2003/09/01 12:45:25 $
 * Revising $Author: achapman $
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

  /* Virtual Register Set*/
        
  typedef enum {
    _UVRSC_CORE = 0,      /* integer register */
    _UVRSC_VFP = 1,       /* vfp */
    _UVRSC_FPA = 2,       /* fpa */
    _UVRSC_WMMXD = 3,     /* Intel WMMX data register */
    _UVRSC_WMMXC = 4      /* Intel WMMX control register */
  } _Unwind_VRS_RegClass;
  
  typedef enum {
    _UVRSD_UINT32 = 0,
    _UVRSD_VFPX = 1,
    _UVRSD_FPAX = 2,
    _UVRSD_UINT64 = 3,
    _UVRSD_FLOAT = 4,
    _UVRSD_DOUBLE = 5
  } _Unwind_VRS_DataRepresentation;
  
  typedef enum {
    _UVRSR_OK = 0,
    _UVRSR_NOT_IMPLEMENTED = 1,
    _UVRSR_FAILED = 2
  } _Unwind_VRS_Result;

  _Unwind_VRS_Result _Unwind_VRS_Set(_Unwind_Context *context,
                                     _Unwind_VRS_RegClass regclass,
                                     uint32_t regno,
                                     _Unwind_VRS_DataRepresentation representation,
                                     void *valuep);
  
  _Unwind_VRS_Result _Unwind_VRS_Get(_Unwind_Context *context,
                                     _Unwind_VRS_RegClass regclass,
                                     uint32_t regno,
                                     _Unwind_VRS_DataRepresentation representation,
                                     void *valuep);
  
  _Unwind_VRS_Result _Unwind_VRS_Pop(_Unwind_Context *context,
                                     _Unwind_VRS_RegClass regclass,
                                     uint32_t descriminator,
                                     _Unwind_VRS_DataRepresentation representation);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* defined UNWINDER_H */
