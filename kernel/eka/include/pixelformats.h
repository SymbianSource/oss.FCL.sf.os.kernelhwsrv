// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// pixel format UIDs
// More details, e.g. the lay-out of each pixel formats, are found in the document "Pixel Formats"
// //EPOC/master/cedar/generic/base/documentation/Base_Pixel_Formats.doc
// 
//

/**
 @file
 @publishedPartner
 @prototype
*/

#ifndef __PIXELFORMATS_H__
#define __PIXELFORMATS_H__

typedef enum
	{
	/**
	 Unknown pixel format
	 */
	EUidPixelFormatUnknown = 0,
	/**
	 32-bit RGB pixel format without alpha, with 8 bits are reserved for each color
	 */
	EUidPixelFormatXRGB_8888 = 0x10275496,
	/**
	 32-bit BGR pixel format without alpha, where 8 bits are reserved for each color
	 */
	EUidPixelFormatBGRX_8888 = 0x102754A8,
	/**
	 32-bit BGR pixel format without alpha, where 8 bits are reserved for each color
	 */
	EUidPixelFormatXBGR_8888 = 0x10273766,
	/**
	 32-bit BGRA pixel format with alpha, using 8 bits per channel
	 */
	EUidPixelFormatBGRA_8888 = 0x102754A9,
	/**
	 32-bit ARGB pixel format with alpha, using 8 bits per channel
	 */
	EUidPixelFormatARGB_8888 = 0x10275498,
	/**
	 32-bit ABGR pixel format with alpha, using 8 bits per channel
	 */
	EUidPixelFormatABGR_8888 = 0x10275499,
	/**
	 32-bit ARGB pixel format with alpha (pre-multiplied), using 8 bits per channel
	 */
	EUidPixelFormatARGB_8888_PRE = 0x1027549A,
	/**
	 32-bit ABGR pixel format with alpha (pre-multiplied), using 8 bits per channel
	 */
	EUidPixelFormatABGR_8888_PRE = 0x1027549B,
	/**
	 32-bit BGRA pixel format with alpha (pre-multiplied), using 8 bits per channel
	 */
	EUidPixelFormatBGRA_8888_PRE = 0x10275497,
	/**
	 32-bit pixel format using 10 bits each for red, green, and blue, and 2 bits for
	 alpha
	 */
	EUidPixelFormatARGB_2101010 = 0x1027549D,
	/**
	 32-bit pixel format using 10 bits each for blue, green, and red color and 2
	 bits for alpha
	 */
	EUidPixelFormatABGR_2101010 = 0x1027549C,
	/**
	 24-bit BGR pixel format with 8 bits per channel
	 */
	EUidPixelFormatBGR_888 = 0x102754A7,
	/**
	 24-bit RGB pixel format with 8 bits per channel
	 */
	EUidPixelFormatRGB_888 = 0x10275495,
	/**
	 16-bit RGB pixel format with 5 bits for red, 6 bits for green, and 5 bits for
	 blue
	 */
	EUidPixelFormatRGB_565 = 0x1027549E,
	/**
	 16-bit BGR pixel format with 5 bits for blue, 6 bits for green, and 5 bits for
	 red
	 */
	EUidPixelFormatBGR_565 = 0x10273765,
	/**
	 16-bit pixel format where 5 bits are reserved for each color and 1 bit is
	 reserved for alpha
	 */
	EUidPixelFormatARGB_1555 = 0x1027549F,
	/**
	 16-bit pixel format where 5 bits are reserved for each color
	 */
	EUidPixelFormatXRGB_1555 = 0x102754A0,
	/**
	 16-bit ARGB pixel format with 4 bits for each channel
	 */
	EUidPixelFormatARGB_4444 = 0x102754A1,
	/**
	 16-bit ARGB texture format using 8 bits for alpha, 3 bits each for red and
	 green, and 2 bits for blue
	 */
	EUidPixelFormatARGB_8332 = 0x102754A4,
	/**
	 16-bit pixel format where 5 bits are reserved for each color
	 */
	EUidPixelFormatBGRX_5551 = 0x102754A5,
	/**
	 16-bit pixel format where 5 bits are reserved for each color and 1 bit is reserved for alpha
	 */
	EUidPixelFormatBGRA_5551 = 0x102754AA,
	/**
	 16-bit BGRA pixel format with 4 bits for each channel
	 */
	EUidPixelFormatBGRA_4444 = 0x102754AC,
	/**
	 16-bit RGB pixel format using 4 bits for each color, without alpha channel
	 */
	EUidPixelFormatBGRX_4444 = 0x102754A6,
	/**
	 16-bit color indexed with 8 bits of alpha
	 */
	EUidPixelFormatAP_88 = 0x102754AB,
	/**
	 16-bit RGB pixel format using 4 bits for each color, without alpha channel
	 */
	EUidPixelFormatXRGB_4444 = 0x10273764,
	/**
	 16-bit BGR pixel format using 4 bits for each color, without alpha channel
	 */
	EUidPixelFormatXBGR_4444 = 0x102754B0,
	/**
	 8-bit RGB texture format using 3 bits for red, 3 bits for green, and 2 bits for blue
	 */
	EUidPixelFormatRGB_332 = 0x102754A2,
	/**
	 8-bit alpha only
	 */
	EUidPixelFormatA_8 = 0x102754A3,
	/**
	 8-bit BGR texture format using 3 bits for blue, 3 bits for green, and 2 bits for red
	 */
	EUidPixelFormatBGR_332 = 0x102754B1,
	/**
	 8-bit indexed (i.e. has a palette)
	 */
	EUidPixelFormatP_8 = 0x10273763,
	/**
	 4-bit indexed (i.e. has a palette)
	 */
	EUidPixelFormatP_4 = 0x102754AD,
	/**
	 2-bit indexed (i.e. has a palette)
	 */
	EUidPixelFormatP_2 = 0x102754AE,
	/**
	 1-bit indexed (i.e. has a palette)
	 */
	EUidPixelFormatP_1 = 0x102754AF,
	/**
	 YUV - 4:2:0 format, 8 bits per sample, Y00Y01Y10Y11UV.
	 */
	EUidPixelFormatYUV_420Interleaved = 0x10273767,
	/**
	 YUV - Three arrays Y, U, V. 4:2:0 format, 8 bits per sample, Y00Y01Y02Y03...U0...V0...
	 */
	EUidPixelFormatYUV_420Planar = 0x10273768,
	/**
	 YUV 4:2:0 image format. Planar, 12 bits per pixel, Pixel order: Y00Y01Y02Y03...V0...U0...
	 */
	EUidPixelFormatYUV_420PlanarReversed = 0x1027376D,
	/**
	 YUV - Two arrays, one is all Y, the other is U and V. 4:2:0 format, 8 bits per sample, Y00Y01Y02Y03...U0V0...
	 */
	EUidPixelFormatYUV_420SemiPlanar = 0x1027376C,
	/**
	 YUV - Organized as UYVY. 4:2:2 format, 8 bits per sample, UY0VY1
	 */
	EUidPixelFormatYUV_422Interleaved = 0x10273769,
	/**
	 YUV - Three arrays Y, U, V
	 */
	EUidPixelFormatYUV_422Planar = 0x102737DA,
	/**
	 YUV - Organized as YUYV. 4:2:2 format, 8 bits per sample, Y0UY1V
	 */
	EUidPixelFormatYUV_422Reversed = 0x102754B2,
	/**
	 YUV - Two arrays, one is all Y, the other is U and V
	 */
	EUidPixelFormatYUV_422SemiPlanar = 0x102754B3,
	/**
	 YUV 4:2:2 image format. 16 bits per pixel, 8 bits per sample, Pixel order: Y1VY0U.
	 */
	EUidPixelFormatYUV_422InterleavedReversed = 0x1027376A,
	/**
	 YUV 4:2:2 image format. Interleaved, 16 bits per pixel, 8 bits per sample, Pixel order: Y0Y1UV.
	 */
	EUidPixelFormatYUV_422Interleaved16bit = 0x102737D9,
	/**
	 YUV - 4:4:4 format, each pixel contains equal parts YUV
	 */
	EUidPixelFormatYUV_444Interleaved = 0x1027376B,
	/**
	 YUV - 4:4:4 format, three arrays Y, U, V
	 */
	EUidPixelFormatYUV_444Planar = 0x102737DB,
	/**
	 8-bit grayscale.
	 */
	EUidPixelFormatL_8 = 0x102858EC,
	/**
	 4-bit grayscale.
	 */
	EUidPixelFormatL_4 = 0x102858ED,
	/**
	 2-bit grayscale.
	 */
	EUidPixelFormatL_2 = 0x102858EE,
	/**
	 1-bit grayscale (monochrome).
	 */
	EUidPixelFormatL_1 = 0x102858EF,
	/**
	 Speed Tagged JPEG
	 */
	EUidPixelFormatSpeedTaggedJPEG = 0x102869FB,
	/**
	 JPEG
	*/
	EUidPixelFormatJPEG = 0x102869FC

	} TUidPixelFormat;

#endif
