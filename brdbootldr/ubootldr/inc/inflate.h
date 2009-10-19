// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// ubootldr\inc\inflate.h
// 
//

#ifndef __INFLATE_H__
#define __INFLATE_H__
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(PROTO)
#  define OF(args)  args
#else
#  define OF(args)  ()
#endif

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

/* Compression methods (see algorithm.doc) */
#define STORED      0
#define COMPRESSED  1
#define PACKED      2
#define LZHED       3
/* methods 4 to 7 reserved */
#define DEFLATED    8
#define MAX_METHODS 9

#ifndef NULL
#define NULL	(0)
#endif

#define ZIP_WINDOW_SIZE 32768

/* PKZIP header definitions */
#define LOCSIG 0x04034b50L      /* four-byte lead-in (lsb first) */
#define LOCFLG 6                /* offset of bit flag */
#define  CRPFLG 1               /*  bit for encrypted entry */
#define  EXTFLG 8               /*  bit for extended local header */
#define LOCHOW 8                /* offset of compression method */
#define LOCTIM 10               /* file mod time (for decryption) */
#define LOCCRC 14               /* offset of crc */
#define LOCSIZ 18               /* offset of compressed size */
#define LOCLEN 22               /* offset of uncompressed length */
#define LOCFIL 26               /* offset of file name field length */
#define LOCEXT 28               /* offset of extra field length */
#define LOCHDR 30               /* size of local header, including sig */
#define EXTHDR 16               /* size of extended local header, inc sig */

extern uch* volatile inbuf_end;		/* pointer to last valid input byte+1 */
extern uch* volatile inptr;			/* pointer to next byte to be processed in inbuf */
extern uch* volatile outptr;		/* pointer to output data */

extern uch fill_inbuf();
extern void* malloc(unsigned);
extern void free(void*);

extern void process_block(int error);
extern int inflate();

#ifdef __cplusplus
}
#endif
#endif
