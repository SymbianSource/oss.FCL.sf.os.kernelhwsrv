/*
* Description: 
* The original NIST Statistical Test Suite code is placed in public domain.
* (http://csrc.nist.gov/groups/ST/toolkit/rng/documentation_software.html) 
* 
* This software was developed at the National Institute of Standards and Technology by 
* employees of the Federal Government in the course of their official duties. Pursuant
* to title 17 Section 105 of the United States Code this software is not subject to 
* copyright protection and is in the public domain. The NIST Statistical Test Suite is
* an experimental system. NIST assumes no responsibility whatsoever for its use by other 
* parties, and makes no guarantees, expressed or implied, about its quality, reliability, 
* or any other characteristic. We would appreciate acknowledgment if the software is used.
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef _CONFIG_H_
#define	_CONFIG_H_

//#define	WINDOWS32
//#define	PROTOTYPES
//#define	LITTLE_ENDIAN
//#define	LOWHI

/*
 * AUTO DEFINES (DON'T TOUCH!)
 */

#ifndef	CSTRTD
typedef char *CSTRTD;
#endif
#ifndef	BSTRTD
typedef unsigned char *BSTRTD;
#endif

#ifndef	BYTE
typedef unsigned char BYTE;
#endif
#ifndef	UINT
typedef unsigned int UINT;
#endif
#ifndef	USHORT
typedef unsigned short USHORT;
#endif
#ifndef	ULONG
typedef unsigned long ULONG;
#endif
#ifndef	DIGIT
typedef USHORT DIGIT;	/* 16-bit word */
#endif
#ifndef	DBLWORD
typedef ULONG DBLWORD;  /* 32-bit word */
#endif

#ifndef	WORD64
typedef ULONG WORD64[2];  /* 64-bit word */
#endif

#endif /* _CONFIG_H_ */

#if defined(__cplusplus)
}
#endif
