/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*/


#ifndef _OPENC_H_
#define _OPENC_H_

#include <e32base.h>
#include <e32std.h>
#include <e32cons.h>            // Console
#include <f32file.h>

#include <e32math.h>

extern RFs gFileSession;

// Math functions
double log(double);
double exp(double);
double fabs(double);
double floor(double);
double sqrt(double);
double erf(double);
double erfc(double);

double pow(double, double);

// Math trigonometric functions
double sin(double);
double cos(double);


// data types
typedef RFile FILE;
#define EOF (-1)
typedef TUint32     u_int32_t;
typedef TInt32      int32_t;

#define SEEK_SET    0   /* set file offset to offset */
#define SEEK_CUR    1   /* set file offset to current plus offset */
#define SEEK_END    2   /* set file offset to EOF plus offset */


// stdio functions
int     printf(const char * __restrict, ...);
int     scanf(const char * __restrict, ...);
int     sprintf(char * __restrict, const char * __restrict, ...);
int     puts ( const char * str );
int     putchar ( int character );
char*   strcpy(char* aDest, const char* aSrc);

FILE    *fopen(const char * __restrict, const char * __restrict);
int     fclose(FILE *);
int     fprintf(FILE * __restrict, const char * __restrict, ...);
int     fscanf(FILE * __restrict, const char * __restrict, ...);
TUint32  fread(void * __restrict, TUint32, TUint32, FILE * __restrict);
int     fseek(FILE *, long, int);

int     fflush(FILE *);

// stdlib functions
void*   calloc(TUint32, TUint32);
void    free(void *);

void    qsort (void* base, TUint32 nmemb, TUint32 size, int (*compar)(const void*, const void*));

void    exit (int status);

// Other utility functions
void ReadStringFromConsole(TDes& aString);
TInt ReadIntL(TInt& aValue);

#endif /* _OPENC_H_ */
