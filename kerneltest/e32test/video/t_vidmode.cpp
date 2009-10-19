// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\video\t_vidmode.cpp
// Tests the video driver kernel extension
// 
//

#include <hal.h>
#include <e32svr.h>
#include <videodriver.h>

GLDEF_C TInt E32Main()
	{
	TBuf<256> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TInt mode;
	lex.Val(mode);

	TInt screens;
	HAL::Get(HAL::EDisplayNumberOfScreens, screens);
	for(TInt i=0;i<screens;i++)
		{
		TInt nmodes;
		HAL::Get(i, HAL::EDisplayNumModes, nmodes);
		if (mode<0 || mode>=nmodes)
			return KErrArgument;

		HAL::Set(i, HAL::EDisplayMode, mode);

		TInt xres, yres, bpp, addr;
		bpp=mode;
		HAL::Get(i, HAL::EDisplayXPixels, xres);
		HAL::Get(i, HAL::EDisplayYPixels, yres);
		HAL::Get(i, HAL::EDisplayBitsPerPixel, bpp);
		HAL::Get(i, HAL::EDisplayMemoryAddress, addr);

		TInt bpl=mode;
		HAL::Get(i, HAL::EDisplayOffsetBetweenLines, bpl);

		TInt ofp=mode;
		HAL::Get(i, HAL::EDisplayOffsetToFirstPixel, ofp);

		RDebug::Print(_L("xres=%d yres=%d bpp=%d"), xres, yres, bpp);
		RDebug::Print(_L("addr=%08x bpl=%d ofp=%d"), addr, bpl, ofp);

		TInt xb, yb;
		for (yb=0; yb<yres/16; ++yb)
			{
			for (xb=0; xb<xres/16; ++xb)
				{
	//			TUint c=(5*(xb+yb))&0xff;
				TUint c=(xb*xb+yb*yb)&0xff;
				TUint r=c&7;
				TUint g=(c>>3)&7;
				TUint b=(c>>6);
				TUint c16=(b<<14)|(g<<8)|(r<<2);
				c16 |= (c16<<16);
				TUint c8=c|(c<<8);
				c8 |= (c8<<16);
				TUint c32=(b<<22)|(g<<13)|(r<<5);
				TInt baddr=addr+ofp+yb*16*bpl+xb*16*bpp/8;
				TInt l;
				for (l=0; l<16; ++l, baddr+=bpl)
					{
					TUint32* p=(TUint32*)baddr;
					if (bpp==8)
						*p++=c8, *p++=c8, *p++=c8, *p++=c8;
					else if (bpp==16)
						*p++=c16, *p++=c16, *p++=c16, *p++=c16,
						*p++=c16, *p++=c16, *p++=c16, *p++=c16;
	//					c16+=((l&3==3))?0x20:0x00;
					else
						*p++=c32+0x0, *p++=c32+0x1, *p++=c32+0x2, *p++=c32+0x3,
						*p++=c32+0x4, *p++=c32+0x5, *p++=c32+0x6, *p++=c32+0x7,
						*p++=c32+0x8, *p++=c32+0x9, *p++=c32+0xa, *p++=c32+0xb,
						*p++=c32+0xc, *p++=c32+0xd, *p++=c32+0xe, *p++=c32+0xf,
						c32+=0x100;
					}
				}
			}
		}

	return KErrNone;
	}
