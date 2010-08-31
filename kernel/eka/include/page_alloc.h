// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// kernel\eka\include\page_alloc.h
// 
//

#ifndef __KERNEL_MODE__ 

const int MAXSMALLPAGEBITS = 68<<3;
#define MINPAGEPOWER	PAGESHIFT+2

struct paged_bitmap
{
	public:
		inline paged_bitmap() : iBase(0), iNbits(0) {}
		void Init(unsigned char* p, unsigned size, unsigned bit);
//
		inline unsigned char* Addr() const;
		inline unsigned Size() const;
//
		inline void Set(unsigned ix, unsigned bit);
		inline unsigned operator[](unsigned ix) const;
		bool Is(unsigned ix, unsigned len, unsigned bit) const;
		void Set(unsigned ix, unsigned len, unsigned val);
		void Setn(unsigned ix, unsigned len, unsigned bit);
		unsigned Bits(unsigned ix, unsigned len) const;	// little endian
		int Find(unsigned start, unsigned bit) const;
	private:
		unsigned char* iBase;
		unsigned iNbits;
};

#endif  // __KERNEL_MODE__
