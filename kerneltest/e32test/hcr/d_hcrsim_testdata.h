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
// Hardware Configuration Respoitory Test Application
//

#ifndef D_HCRSIM_TESTDATA_H
#define D_HCRSIM_TESTDATA_H

const TText8* TestString1 = reinterpret_cast<const TText8*>("_");
const TText8* TestString64a = reinterpret_cast<const TText8*>("Two peanuts walked into a bar, and ... one was a-salted... !!! #");
const TText8* TestString64b = reinterpret_cast<const TText8*>("A termite walks into a bar and says, 'Is the bar tender here?' #");
const TText8* TestString512a = reinterpret_cast<const TText8*>("ENSUTIXABRMNOBXIEZTGSIYAPKNHESPUFWXCVGLHKKYHEMLOJMSSXYZWYCJWHJSJAPNDTJVGXBQKLECTVCOSGFHGLSCXNGSVWJBRLIRLEZSNWAHVVFKXWRLXJVEKCUKUBGJILRKSAAWBNCKCVSYTSZUSBPJDZNQEFXTKZAKCJQDCEHPBLZCNITSZASYRRXBDYNZAWBZBISWRESBPLAIEVQLFZJNJMOGEDUCDQLJEEUZLQLVUTHLODJDETGDIGBUBTZRKNXPLHUSYHNWALUQGSRWAISHLNZZTAWQHHDSCHIRNLDAZUBPQTHOBWMUJNZFQFTGCMAPQWKYEFJBPDFHUBABYRYXQCTQGULIUGHDVYPZUENQDATFRDSFQEXHLUZLNKIZGIUZTNCRZUSJCFXEABFQHBUBVSKQOUIUAGKVTVOIFYHSGNHLHQJYMBOLLOCKSZWQNBBEMFDOQJMBSADSSJEQXDOZJIBWZZWZWPRMVENMGJPSVVDZKGNCRIWIMQWM#");
const TText8* TestString512b = reinterpret_cast<const TText8*>("These two strings walk up to a bar. The first string walks in and orders and the bartender throws him out and yells 'I don't serve strings in this bar'. The other string ruffs himself up on the street and curls up and orders. The bartender shouts, 'Hey, didn't you hear what I told your buddy?'. The string says 'Yeah.' The bartender says, 'aren't you a string?' The string says, 'No, I'm a frayed knot...'*********************************************************************************************************?");

const TUint8 TestBinData[] =
	{
	0x18, 0xa1, 0x0b, 0xbc, 0x61, 0x41, 0xde, 0xc9,
	0xcd, 0xd2, 0xf9, 0x3b, 0xba, 0x99, 0xe5, 0xe2,
	0xfa, 0xe0, 0x13, 0xfe, 0x8f, 0xd4, 0xe3, 0x50,
	0x1a, 0x5e, 0xa3, 0x32, 0x39, 0x34, 0x1f, 0x16,
	0x9a, 0xa8, 0xb3, 0x61, 0x1d, 0x7d, 0x7f, 0xfd,
	0xb1, 0x58, 0xaf, 0x91, 0x53, 0xf7, 0x76, 0xc4,
	0xda, 0xdd, 0xa7, 0x7c, 0xf0, 0x94, 0x97, 0x6a,
	0xee, 0x30, 0x2e, 0xef, 0xef, 0x38, 0xc6, 0x9c,

	0xda, 0xa7, 0xdc, 0x4c, 0xb6, 0x1b, 0x0a, 0x41,
	0xfc, 0xed, 0x71, 0x1c, 0x47, 0x4e, 0xfa, 0x29,
	0x18, 0x14, 0x92, 0xd8, 0x01, 0x62, 0xd3, 0xda,
	0xf5, 0xcc, 0x2f, 0xbe, 0x0e, 0x2a, 0x25, 0x7b,
	0x91, 0xb6, 0x7d, 0xd1, 0x6a, 0xa4, 0x25, 0xe2,
	0x00, 0xdb, 0xa0, 0xf6, 0x8d, 0xfa, 0xa3, 0xc1,
	0xdf, 0x42, 0xe2, 0x2e, 0xb2, 0xaa, 0x91, 0xf4,
	0x74, 0xa1, 0x9e, 0xfa, 0xc2, 0xc8, 0x3d, 0xeb,

	0x5e, 0x34, 0xb9, 0xd1, 0xde, 0x68, 0x1e, 0x3a,
	0xf6, 0xb5, 0xc7, 0xbd, 0x84, 0x1d, 0x46, 0x47,
	0x44, 0x6b, 0xcb, 0x20, 0x5c, 0xa1, 0x72, 0x12,
	0x99, 0x65, 0x99, 0x88, 0x9e, 0x9a, 0xbb, 0x27,
	0xc9, 0xca, 0xd6, 0xb0, 0x1e, 0x81, 0xea, 0xa7,
	0x00, 0x53, 0x98, 0x9c, 0xf3, 0xa1, 0x64, 0x7b,
	0x41, 0xda, 0xa9, 0xd9, 0xc0, 0x44, 0xc0, 0x98,
	0x7b, 0x15, 0x67, 0xd4, 0x9c, 0xef, 0xf3, 0x75,

	0x42, 0x67, 0x46, 0x5d, 0xa0, 0xcf, 0x26, 0x91,
	0x29, 0xd7, 0xef, 0x42, 0x0a, 0x3b, 0x25, 0x26,
	0xbd, 0x23, 0x01, 0x09, 0x07, 0x57, 0x6c, 0xf1,
	0x18, 0xf8, 0x77, 0xd2, 0x21, 0xdc, 0xe0, 0x22,
	0x22, 0x43, 0xa2, 0x4f, 0x41, 0xfb, 0x19, 0x67,
	0x63, 0xab, 0xcd, 0xe7, 0x5e, 0x64, 0x57, 0x1b,
	0x81, 0x22, 0x80, 0xec, 0xc3, 0x68, 0x10, 0x91,
	0x56, 0x9b, 0x5f, 0xfe, 0xf6, 0x42, 0x26, 0x84,

	0xa9, 0xde, 0xa9, 0x84, 0x46, 0x77, 0xb0, 0xa0,
	0x88, 0x82, 0x5d, 0x4c, 0xf1, 0x60, 0x73, 0x32,
	0x44, 0xfb, 0xfa, 0x44, 0xec, 0xcc, 0xee, 0xf3,
	0x02, 0xd3, 0xdd, 0x5d, 0x4f, 0xc6, 0x0f, 0xf8,
	0xfe, 0x00, 0x45, 0x82, 0x5c, 0x7a, 0x7f, 0xbb,
	0x5a, 0x55, 0xf3, 0xb9, 0x27, 0x38, 0x97, 0x4c,
	0xa0, 0x1a, 0x6c, 0x5c, 0xe3, 0x9e, 0xef, 0x9a,
	0xd4, 0xc0, 0xd8, 0x7c, 0xc7, 0xd8, 0x91, 0xe3,

	0x31, 0xb8, 0x85, 0x58, 0x97, 0x05, 0xc6, 0x41,
	0x83, 0x65, 0x07, 0xfe, 0xd1, 0xc1, 0x8c, 0x53,
	0x19, 0x31, 0xfa, 0x05, 0x72, 0xc5, 0xa5, 0x12,
	0x68, 0xc6, 0x5f, 0x6e, 0x61, 0xae, 0x45, 0xb3,
	0x3c, 0x5f, 0xa4, 0x9b, 0x76, 0xe2, 0x69, 0xbf,
	0x94, 0x3d, 0x3e, 0x74, 0x26, 0x94, 0xc2, 0x39,
	0x1d, 0x3f, 0xf0, 0x9b, 0xca, 0xec, 0x49, 0xed,
	0x46, 0x94, 0xa8, 0xd0, 0x88, 0x47, 0x71, 0xdd,

	0xfd, 0x93, 0x00, 0x6b, 0xdd, 0xa1, 0xf6, 0xce,
	0x09, 0xae, 0x61, 0xfa, 0xc4, 0x15, 0x4e, 0xf6,
	0xfe, 0x85, 0xc5, 0xfe, 0x83, 0x89, 0xbc, 0xc7,
	0xda, 0x1f, 0x12, 0xc6, 0x0f, 0x6b, 0xff, 0xde,
	0x3d, 0x3e, 0x23, 0x6d, 0x19, 0x9b, 0xa3, 0x0d,
	0x43, 0xd2, 0x64, 0xfb, 0xb3, 0x6f, 0xf2, 0x8c,
	0xf9, 0x90, 0x13, 0x9a, 0x9f, 0xd8, 0x89, 0x44,
	0x7c, 0xa5, 0x23, 0xfc, 0x32, 0xa7, 0x83, 0x3b,

	0xac, 0x8e, 0xbd, 0xd0, 0xdf, 0xf0, 0x4c, 0x23,
	0x8c, 0x0b, 0x60, 0x64, 0x62, 0x94, 0x15, 0x04,
	0x32, 0x33, 0x9f, 0x61, 0x87, 0xdf, 0xe1, 0x0f,
	0x69, 0xac, 0x8a, 0xa7, 0x92, 0x53, 0x39, 0x83,
	0xe2, 0xfc, 0xa8, 0x4a, 0x4c, 0x8b, 0x79, 0xbf,
	0x16, 0x03, 0x98, 0xb0, 0xa8, 0x3f, 0xc6, 0x70,
	0xb5, 0x8b, 0x57, 0xcd, 0x0b, 0x69, 0x9e, 0xdb,
	0xc7, 0x01, 0x1e, 0x83, 0x3c, 0x8d, 0x03, 0x49,
	};

const TUint32 TestUint32Array[] =
	{
	0x793e5664, 0x155e1d7f, 0xb8ad360e, 0xb7189d5d,
	0xd9ab7a85, 0xb0b16e95, 0xa0b62ef8, 0x651a5fe5,
	0x5a5d5a4c, 0x45cdbd2f, 0x02bc70c1, 0xbbe5133a,
	0x0c295fc9, 0xcc528d8b, 0xcdf53fed, 0x512ff863,

	0x8652aadd, 0xcc3764de, 0x0a57a6a0, 0xe356632e,
	0x08263ada, 0x7965ecf7, 0xf5319b3f, 0x67fad9d0,
	0x949f0420, 0xd55d0edc, 0x21d12012, 0x34a0338a,
	0x17051cc2, 0xd3d2196d, 0x99205ede, 0x1e52bb42,

	0x808cce22, 0x724add02, 0xfe44cc89, 0x993e4e29,
	0xe9f4c095, 0xe2c4cc0b, 0xa6414bbd, 0xd7577d57,
	0xaf2b15fd, 0x9e501bb0, 0xbf95437f, 0xe8f5aa6d,
	0x98e8876e, 0x92b3e273, 0x6ddbccda, 0xda722a35,

	0xee538ecd, 0x39073bce, 0xec6cc575, 0xdbcf6341,
	0xa49d7a70, 0xbb5c63d3, 0xabfdf73b, 0x5751f08b,
	0x73a697bc, 0x0f53d8cd, 0x743e38f5, 0x1820be17,
	0xf8994bc8, 0x1ac47cde, 0x837e87ed, 0x66ec2a8f,

	0xd98d36f9, 0xd05cb4a4, 0xad452d8e, 0x298626e9,
	0xe32754a7, 0x553b86d2, 0x7efcde05, 0x07845600,
	0x273819ba, 0x1895bad5, 0x5784dadc, 0x83e23139,
	0x1f5d954a, 0xfa55fd72, 0x91de09a0, 0x22a01f23,

	0xcaa1943f, 0x69b7fc67, 0x9ac6207e, 0x83629c13,
	0xcd18b9f3, 0x80f0890b, 0x1654bce4, 0x871054c7,
	0x9687a5d0, 0x2cc5b5eb, 0x039d861f, 0x6c784a07,
	0x76fd12ef, 0x4532f870, 0xd15351fe, 0xeeecef42,

	0xc874f3bb, 0xb307487a, 0x8a643b70, 0x6ae14831,
	0x0b779890, 0x8e8941ff, 0xed9bcd24, 0xf8930f74,
	0x06b9ca58, 0x39113fb2, 0x8f3f182a, 0x91a1cc33,
	0xe6bded31, 0x89ab829c, 0xfcb3da92, 0x2caefdc1,

	0x5d6f2007, 0xe0bb4cbe, 0xd8179b0e, 0xde033137,
	0xc6ca5b37, 0x4d9601b2, 0xfae8cd8e, 0xfb2d2918,
	0x4276922e, 0xb1274a4c, 0x93a0ede6, 0x349cfbef,
	0xd665d40b, 0xd88e2f37, 0x4a54a266, 0xbb462ef0,
	};

const TInt32 TestInt32Array[] =
	{
	0x793e5664, 0x155e1d7f, 0xb8ad360e, 0xb7189d5d,
	0xd9ab7a85, 0xb0b16e95, 0xa0b62ef8, 0x651a5fe5,
	0x5a5d5a4c, 0x45cdbd2f, 0x02bc70c1, 0xbbe5133a,
	0x0c295fc9, 0xcc528d8b, 0xcdf53fed, 0x512ff863,

	0x8652aadd, 0xcc3764de, 0x0a57a6a0, 0xe356632e,
	0x08263ada, 0x7965ecf7, 0xf5319b3f, 0x67fad9d0,
	0x949f0420, 0xd55d0edc, 0x21d12012, 0x34a0338a,
	0x17051cc2, 0xd3d2196d, 0x99205ede, 0x1e52bb42,

	0x808cce22, 0x724add02, 0xfe44cc89, 0x993e4e29,
	0xe9f4c095, 0xe2c4cc0b, 0xa6414bbd, 0xd7577d57,
	0xaf2b15fd, 0x9e501bb0, 0xbf95437f, 0xe8f5aa6d,
	0x98e8876e, 0x92b3e273, 0x6ddbccda, 0xda722a35,

	0xee538ecd, 0x39073bce, 0xec6cc575, 0xdbcf6341,
	0xa49d7a70, 0xbb5c63d3, 0xabfdf73b, 0x5751f08b,
	0x73a697bc, 0x0f53d8cd, 0x743e38f5, 0x1820be17,
	0xf8994bc8, 0x1ac47cde, 0x837e87ed, 0x66ec2a8f,

	0xd98d36f9, 0xd05cb4a4, 0xad452d8e, 0x298626e9,
	0xe32754a7, 0x553b86d2, 0x7efcde05, 0x07845600,
	0x273819ba, 0x1895bad5, 0x5784dadc, 0x83e23139,
	0x1f5d954a, 0xfa55fd72, 0x91de09a0, 0x22a01f23,

	0xcaa1943f, 0x69b7fc67, 0x9ac6207e, 0x83629c13,
	0xcd18b9f3, 0x80f0890b, 0x1654bce4, 0x871054c7,
	0x9687a5d0, 0x2cc5b5eb, 0x039d861f, 0x6c784a07,
	0x76fd12ef, 0x4532f870, 0xd15351fe, 0xeeecef42,

	0xc874f3bb, 0xb307487a, 0x8a643b70, 0x6ae14831,
	0x0b779890, 0x8e8941ff, 0xed9bcd24, 0xf8930f74,
	0x06b9ca58, 0x39113fb2, 0x8f3f182a, 0x91a1cc33,
	0xe6bded31, 0x89ab829c, 0xfcb3da92, 0x2caefdc1,

	0x5d6f2007, 0xe0bb4cbe, 0xd8179b0e, 0xde033137,
	0xc6ca5b37, 0x4d9601b2, 0xfae8cd8e, 0xfb2d2918,
	0x4276922e, 0xb1274a4c, 0x93a0ede6, 0x349cfbef,
	0xd665d40b, 0xd88e2f37, 0x4a54a266, 0xbb462ef0,
	};

const TCategoryUid KTestCategories[] = {
	1,					// 0
	2,					// 1
	1000,				// 2
	0xfffffff0,			// 3
	0x10000001,			// 4
	0x10000002,			// 5
	0x10000003,			// 6
	0x10000004			// 7
	};

const TInt64 KTestI64One = I64LIT(-1);
const TInt64 KTestI64Two = KMinTInt64;
const TInt64 KTestI64Three = KMaxTInt64;
const TInt64 KTestU64One = UI64LIT(1);
const TInt64 KTestU64Two = KMaxTInt64;
const TInt64 KTestU64Three = UI64LIT(0);

SSettingC SettingsList[] = {
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[0], 2}, ETypeInt32, 0x0000, 0 }, {{ KMaxTInt32 }}},
	{{{ KTestCategories[0], 3}, ETypeInt32, 0x0000, 0 }, {{ KMinTInt32 }}},
	{{{ KTestCategories[0], 4}, ETypeInt16, 0x0000, 0 }, {{ -1 }}},
	{{{ KTestCategories[0], 5}, ETypeInt16, 0x0000, 0 }, {{ KMaxTInt16 }}},
	{{{ KTestCategories[0], 6}, ETypeInt16, 0x0000, 0 }, {{ KMinTInt16 }}},
	{{{ KTestCategories[0], 7}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[0], 8}, ETypeInt8, 0x0000, 0 }, {{ KMaxTInt8 }}},
	{{{ KTestCategories[0], 9}, ETypeInt8, 0x0000, 0 }, {{ KMinTInt8 }}},
	{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[0], 11}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[1], 1}, ETypeBool, 0x0000, 0 }, {{ 0x80 }}},
	{{{ KTestCategories[1], 2}, ETypeUInt32, 0x0000, 0 }, {{ 0xfffffffe }}},
	{{{ KTestCategories[1], 3}, ETypeUInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 4}, ETypeUInt32, 0x0000, 0 }, {{ KMaxTUint32 }}},
	{{{ KTestCategories[1], 5}, ETypeUInt16, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[1], 6}, ETypeUInt16, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 7}, ETypeUInt16, 0x0000, 0 }, {{ KMaxTUint16 }}},
	{{{ KTestCategories[1], 8}, ETypeUInt8, 0x0000, 0 }, {{ 254 }}},
	{{{ KTestCategories[1], 9}, ETypeUInt8, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 10}, ETypeUInt8, 0x0000, 0 }, {{ KMaxTUint8 }}},
	{{{ KTestCategories[1], 11}, ETypeLinAddr, 0x0000, 0 }, {{ NULL }}},
	{{{ KTestCategories[1], 12}, ETypeLinAddr, 0x0000, 0 }, {{ 0x1234abcd }}},
	{{{ KTestCategories[2], 0x1000}, ETypeBinData, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x2000}, ETypeBinData, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x3000}, ETypeBinData, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x4000}, ETypeText8, 0x0000, 0 }, {{ reinterpret_cast<TInt32>("") }}},
	{{{ KTestCategories[2], 0x5000}, ETypeText8, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestString1) }}},
	{{{ KTestCategories[2], 0x6000}, ETypeText8, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestString64a) }}},
	{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512a) }}},
	{{{ KTestCategories[2], 0x8000}, ETypeArrayInt32, 0x0000, sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[2], 0x9000}, ETypeArrayInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[3], 0x0000}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[3], 0x0010}, ETypeArrayUInt32, 0x0000, sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[3], 0x0021}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64One) }}},
	{{{ KTestCategories[3], 0x0030}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[3], 0x0031}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Three) }}},
	{{{ KTestCategories[3], 0x1000}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64One) }}},
	{{{ KTestCategories[3], 0x80000000}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	{{{ KTestCategories[3], 0xfffffffe}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	};

SSettingC SettingsListCorrupt1[] = {
	{{{ KTestCategories[0], 2}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[0], 3}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	};

SSettingC SettingsListCorrupt2[] = {
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[0], 2}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	};
#if !defined(__KERNEL_MODE__) || defined(HCRTEST_USERSIDE_INTERFACE)
const TUint32 KTestInvalidCategory = 0;
const TUint32 KTestInvalidSettingId = 5678;

// The following repositories are only used for reference by the test application
// so there is no point cluttering the device driver.

// For clarity, the following repositories are NOT ordered
// Comments denote an overridden setting

// Compiled+File
SSettingC SettingsList2[] = {
	// Existing settings from Compiled Repository (unchanged)
	//{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[0], 2}, ETypeInt32, 0x0000, 0 }, {{ KMaxTInt32 }}},
	{{{ KTestCategories[0], 3}, ETypeInt32, 0x0000, 0 }, {{ KMinTInt32 }}},
	//{{{ KTestCategories[0], 4}, ETypeInt16, 0x0000, 0 }, {{ -1 }}},
	{{{ KTestCategories[0], 5}, ETypeInt16, 0x0000, 0 }, {{ KMaxTInt16 }}},
	{{{ KTestCategories[0], 6}, ETypeInt16, 0x0000, 0 }, {{ KMinTInt16 }}},
	//{{{ KTestCategories[0], 7}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[0], 8}, ETypeInt8, 0x0000, 0 }, {{ KMaxTInt8 }}},
	{{{ KTestCategories[0], 9}, ETypeInt8, 0x0000, 0 }, {{ KMinTInt8 }}},
	//{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[0], 11}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[1], 1}, ETypeBool, 0x0000, 0 }, {{ 0x80 }}},
	//{{{ KTestCategories[1], 2}, ETypeUInt32, 0x0000, 0 }, {{ KMaxTUint32 - 1 }}},
	{{{ KTestCategories[1], 3}, ETypeUInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 4}, ETypeUInt32, 0x0000, 0 }, {{ KMaxTUint32 }}},
	//{{{ KTestCategories[1], 5}, ETypeUInt16, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[1], 6}, ETypeUInt16, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 7}, ETypeUInt16, 0x0000, 0 }, {{ KMaxTUint16 }}},
	// {{{ KTestCategories[1], 8}, ETypeUInt8, 0x0000, 0 }, {{ 254 }}},
	{{{ KTestCategories[1], 9}, ETypeUInt8, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 10}, ETypeUInt8, 0x0000, 0 }, {{ KMaxTUint8 }}},
	{{{ KTestCategories[1], 11}, ETypeLinAddr, 0x0000, 0 }, {{ NULL }}},
	// {{{ KTestCategories[1], 12}, ETypeLinAddr, 0x0000, 0 }, {{ 0x1234abcd }}},
	// {{{ KTestCategories[2], 0x1000}, ETypeBinData, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x2000}, ETypeBinData, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x3000}, ETypeBinData, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x4000}, ETypeText8, 0x0000, 0 }, {{ reinterpret_cast<TInt32>("") }}},
	{{{ KTestCategories[2], 0x5000}, ETypeText8, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestString1) }}},
	{{{ KTestCategories[2], 0x6000}, ETypeText8, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestString64a) }}},
	// {{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512) }, {{ reinterpret_cast<TInt32>(TestString512a) }}},
	// {{{ KTestCategories[2], 0x8000}, ETypeArrayInt32, 0x0000, sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[2], 0x9000}, ETypeArrayInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[3], 0x0000}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[3], 0x0010}, ETypeArrayUInt32, 0x0000, sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	// {{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	// {{{ KTestCategories[3], 0x0021}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64One) }}},
	{{{ KTestCategories[3], 0x0030}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[3], 0x0031}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Three) }}},
	{{{ KTestCategories[3], 0x1000}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64One) }}},
	{{{ KTestCategories[3], 0x80000000}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	// {{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	// {{{ KTestCategories[3], 0xfffffffe}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},

	// Existing settings override
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 444 }}},
	{{{ KTestCategories[0], 4}, ETypeInt16, 0x0000, 0 }, {{ -7632 }}},
	{{{ KTestCategories[0], 7}, ETypeInt8, 0x0000, 0 }, {{ 120 }}},
	{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[1], 2}, ETypeUInt32, 0x0000, 0 }, {{ 555555 }}},
	{{{ KTestCategories[1], 5}, ETypeUInt16, 0x0000, 0 }, {{ 60000 }}},
	{{{ KTestCategories[1], 8}, ETypeUInt8, 0x0000, 0 }, {{ 11 }}},
	{{{ KTestCategories[1], 12}, ETypeLinAddr, 0x0000, 0 }, {{ 0x0faece50 }}},
	{{{ KTestCategories[2], 0x1000}, ETypeBinData, 0x0000, 50 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512b) }}},
	{{{ KTestCategories[2], 0x8000}, ETypeArrayInt32, 0x0000, 6 * sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[3], 0x0021}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},	
	{{{ KTestCategories[3], 0xfffffffe}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	
	// New settings
	{{{ KTestCategories[4], 0x1000}, ETypeInt32, 0x0000, 0 }, {{ 333 }}},
	{{{ KTestCategories[4], 0x1010}, ETypeInt16, 0x0000, 0 }, {{ 17632 }}},
	{{{ KTestCategories[4], 0x1020}, ETypeInt8, 0x0000, 0 }, {{ 44 }}},
	{{{ KTestCategories[4], 0x1030}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[4], 0x1040}, ETypeUInt32, 0x0000, 0 }, {{ 0xba2713b3 }}},
	{{{ KTestCategories[4], 0x1050}, ETypeUInt16, 0x0000, 0 }, {{ 18 }}},
	{{{ KTestCategories[4], 0x1060}, ETypeUInt8, 0x0000, 0 }, {{ 80 }}},
	{{{ KTestCategories[4], 0x1070}, ETypeLinAddr, 0x0000, 0 }, {{ 0xdeadbeef }}},
	{{{ KTestCategories[5], 0x0080}, ETypeBinData, 0x0000, 50 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[5], 0x0093}, ETypeText8, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestString64b) }}},
	{{{ KTestCategories[5], 0x00a0}, ETypeArrayInt32, 0x0000, 6 * sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[5], 0x00b1}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[5], 0x00c2}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64One) }}},
	{{{ KTestCategories[5], 0x00d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	{{{ KTestCategories[5], 0xffffffff}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	};

// Compiled+File+Nand
SSettingC SettingsList3[] = {
	// Existing settings from Compiled Repository (unchanged)
	//{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[0], 2}, ETypeInt32, 0x0000, 0 }, {{ KMaxTInt32 }}},
	{{{ KTestCategories[0], 3}, ETypeInt32, 0x0000, 0 }, {{ KMinTInt32 }}},
	//{{{ KTestCategories[0], 4}, ETypeInt16, 0x0000, 0 }, {{ -1 }}},
	{{{ KTestCategories[0], 5}, ETypeInt16, 0x0000, 0 }, {{ KMaxTInt16 }}},
	{{{ KTestCategories[0], 6}, ETypeInt16, 0x0000, 0 }, {{ KMinTInt16 }}},
	//{{{ KTestCategories[0], 7}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[0], 8}, ETypeInt8, 0x0000, 0 }, {{ KMaxTInt8 }}},
	{{{ KTestCategories[0], 9}, ETypeInt8, 0x0000, 0 }, {{ KMinTInt8 }}},
	//{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[0], 11}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[1], 1}, ETypeBool, 0x0000, 0 }, {{ 0x80 }}},
	//{{{ KTestCategories[1], 2}, ETypeUInt32, 0x0000, 0 }, {{ KMaxTUint32 - 1 }}},
	{{{ KTestCategories[1], 3}, ETypeUInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 4}, ETypeUInt32, 0x0000, 0 }, {{ KMaxTUint32 }}},
	//{{{ KTestCategories[1], 5}, ETypeUInt16, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[1], 6}, ETypeUInt16, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 7}, ETypeUInt16, 0x0000, 0 }, {{ KMaxTUint16 }}},
	// {{{ KTestCategories[1], 8}, ETypeUInt8, 0x0000, 0 }, {{ 254 }}},
	{{{ KTestCategories[1], 9}, ETypeUInt8, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 10}, ETypeUInt8, 0x0000, 0 }, {{ KMaxTUint8 }}},
	{{{ KTestCategories[1], 11}, ETypeLinAddr, 0x0000, 0 }, {{ NULL }}},
	// {{{ KTestCategories[1], 12}, ETypeLinAddr, 0x0000, 0 }, {{ 0x1234abcd }}},
	// {{{ KTestCategories[2], 0x1000}, ETypeBinData, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x2000}, ETypeBinData, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x3000}, ETypeBinData, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x4000}, ETypeText8, 0x0000, 0 }, {{ reinterpret_cast<TInt32>("") }}},
	{{{ KTestCategories[2], 0x5000}, ETypeText8, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestString1) }}},
	{{{ KTestCategories[2], 0x6000}, ETypeText8, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestString64a) }}},
	// {{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512) }, {{ reinterpret_cast<TInt32>(TestString512a) }}},
	// {{{ KTestCategories[2], 0x8000}, ETypeArrayInt32, 0x0000, sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[2], 0x9000}, ETypeArrayInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[3], 0x0000}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[3], 0x0010}, ETypeArrayUInt32, 0x0000, sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	// {{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	// {{{ KTestCategories[3], 0x0021}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64One) }}},
	{{{ KTestCategories[3], 0x0030}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[3], 0x0031}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Three) }}},
	{{{ KTestCategories[3], 0x1000}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64One) }}},
	{{{ KTestCategories[3], 0x80000000}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	// {{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	// {{{ KTestCategories[3], 0xfffffffe}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},

	// Existing settings override (Core)
	//{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 444 }}},
	{{{ KTestCategories[0], 4}, ETypeInt16, 0x0000, 0 }, {{ -7632 }}},
	{{{ KTestCategories[0], 7}, ETypeInt8, 0x0000, 0 }, {{ 120 }}},
	//{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[1], 2}, ETypeUInt32, 0x0000, 0 }, {{ 555555 }}},
	{{{ KTestCategories[1], 5}, ETypeUInt16, 0x0000, 0 }, {{ 60000 }}},
	{{{ KTestCategories[1], 8}, ETypeUInt8, 0x0000, 0 }, {{ 11 }}},
	{{{ KTestCategories[1], 12}, ETypeLinAddr, 0x0000, 0 }, {{ 0x0faece50 }}},
	{{{ KTestCategories[2], 0x1000}, ETypeBinData, 0x0000, 50 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	//{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512b) }}},
	{{{ KTestCategories[2], 0x8000}, ETypeArrayInt32, 0x0000, 6 * sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	//{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[3], 0x0021}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	//{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	//{{{ KTestCategories[3], 0xfffffffe}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	
	// New settings (Core)
	{{{ KTestCategories[4], 0x1000}, ETypeInt32, 0x0000, 0 }, {{ 333 }}},
	{{{ KTestCategories[4], 0x1010}, ETypeInt16, 0x0000, 0 }, {{ 17632 }}},
	//{{{ KTestCategories[4], 0x1020}, ETypeInt8, 0x0000, 0 }, {{ 44 }}},
	{{{ KTestCategories[4], 0x1030}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	//{{{ KTestCategories[4], 0x1040}, ETypeUInt32, 0x0000, 0 }, {{ 3123123123u }}},
	{{{ KTestCategories[4], 0x1050}, ETypeUInt16, 0x0000, 0 }, {{ 18 }}},
	{{{ KTestCategories[4], 0x1060}, ETypeUInt8, 0x0000, 0 }, {{ 80 }}},
	{{{ KTestCategories[4], 0x1070}, ETypeLinAddr, 0x0000, 0 }, {{ 0xdeadbeef }}},
	//{{{ KTestCategories[5], 0x0080}, ETypeBinData, 0x0000, 50 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[5], 0x0093}, ETypeText8, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestString64b) }}},
	{{{ KTestCategories[5], 0x00a0}, ETypeArrayInt32, 0x0000, 6 * sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[5], 0x00b1}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[5], 0x00c2}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64One) }}},
	//{{{ KTestCategories[5], 0x00d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	//{{{ KTestCategories[5], 0xffffffff}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},

	// Existing Settings Override (Nand)
	// Over Compiler Repository
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 555666 }}},
	{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512a) }}},
	{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 32 * sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	{{{ KTestCategories[3], 0xfffffffe}, ETypeInt8, 0x0000, 0 }, {{ 100 }}},
	// Over File Repository
	{{{ KTestCategories[4], 0x1020}, ETypeInt8, 0x0000, 0 }, {{ -33 }}},
	{{{ KTestCategories[4], 0x1040}, ETypeUInt32, 0x0000, 0 }, {{ 999999 }}},
	{{{ KTestCategories[5], 0x0080}, ETypeBinData, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[5], 0x00d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64One) }}},
	{{{ KTestCategories[5], 0xffffffff}, ETypeUInt8, 0x0000, 0 }, {{ 25 }}},
	
	// New Settings (Nand)
	{{{ KTestCategories[6], 0x3000}, ETypeInt32, 0x0000, 0 }, {{ -987654 }}},
	{{{ KTestCategories[6], 0x3010}, ETypeInt16, 0x0000, 0 }, {{ -12345 }}},
	{{{ KTestCategories[6], 0x3020}, ETypeInt8, 0x0000, 0 }, {{ -100 }}},
	{{{ KTestCategories[6], 0x3030}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[6], 0x3040}, ETypeUInt32, 0x0000, 0 }, {{ 1234567890 }}},
	{{{ KTestCategories[6], 0x3050}, ETypeUInt16, 0x0000, 0 }, {{ 12345 }}},
	{{{ KTestCategories[6], 0x3060}, ETypeUInt8, 0x0000, 0 }, {{ 123 }}},
	{{{ KTestCategories[6], 0x3070}, ETypeLinAddr, 0x0000, 0 }, {{ 0xabcdef01 }}},
	{{{ KTestCategories[7], 0x2080}, ETypeBinData, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[7], 0x2093}, ETypeText8, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestString1) }}},
	{{{ KTestCategories[7], 0x20a0}, ETypeArrayInt32, 0x0000, sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[7], 0x20b1}, ETypeArrayUInt32, 0x0000, sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[7], 0x20c2}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[7], 0x20d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	{{{ KTestCategories[7], 0xffffffff}, ETypeInt8, 0x0000, 0 }, {{ -1 }}},
	};

// Compiled+Nand
SSettingC SettingsList4[] = {
	//{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[0], 2}, ETypeInt32, 0x0000, 0 }, {{ KMaxTInt32 }}},
	{{{ KTestCategories[0], 3}, ETypeInt32, 0x0000, 0 }, {{ KMinTInt32 }}},
	{{{ KTestCategories[0], 4}, ETypeInt16, 0x0000, 0 }, {{ -1 }}},
	{{{ KTestCategories[0], 5}, ETypeInt16, 0x0000, 0 }, {{ KMaxTInt16 }}},
	{{{ KTestCategories[0], 6}, ETypeInt16, 0x0000, 0 }, {{ KMinTInt16 }}},
	{{{ KTestCategories[0], 7}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[0], 8}, ETypeInt8, 0x0000, 0 }, {{ KMaxTInt8 }}},
	{{{ KTestCategories[0], 9}, ETypeInt8, 0x0000, 0 }, {{ KMinTInt8 }}},
	//{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[0], 11}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[1], 1}, ETypeBool, 0x0000, 0 }, {{ 0x80 }}},
	{{{ KTestCategories[1], 2}, ETypeUInt32, 0x0000, 0 }, {{ 0xfffffffe }}},
	{{{ KTestCategories[1], 3}, ETypeUInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 4}, ETypeUInt32, 0x0000, 0 }, {{ KMaxTUint32 }}},
	{{{ KTestCategories[1], 5}, ETypeUInt16, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[1], 6}, ETypeUInt16, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 7}, ETypeUInt16, 0x0000, 0 }, {{ KMaxTUint16 }}},
	{{{ KTestCategories[1], 8}, ETypeUInt8, 0x0000, 0 }, {{ 254 }}},
	{{{ KTestCategories[1], 9}, ETypeUInt8, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[1], 10}, ETypeUInt8, 0x0000, 0 }, {{ KMaxTUint8 }}},
	{{{ KTestCategories[1], 11}, ETypeLinAddr, 0x0000, 0 }, {{ NULL }}},
	{{{ KTestCategories[1], 12}, ETypeLinAddr, 0x0000, 0 }, {{ 0x1234abcd }}},
	{{{ KTestCategories[2], 0x1000}, ETypeBinData, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x2000}, ETypeBinData, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x3000}, ETypeBinData, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x4000}, ETypeText8, 0x0000, 0 }, {{ reinterpret_cast<TInt32>("") }}},
	{{{ KTestCategories[2], 0x5000}, ETypeText8, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestString1) }}},
	{{{ KTestCategories[2], 0x6000}, ETypeText8, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestString64a) }}},
	//{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512a) }}},
	{{{ KTestCategories[2], 0x8000}, ETypeArrayInt32, 0x0000, sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[2], 0x9000}, ETypeArrayInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[3], 0x0000}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	{{{ KTestCategories[3], 0x0010}, ETypeArrayUInt32, 0x0000, sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	//{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[3], 0x0021}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64One) }}},
	{{{ KTestCategories[3], 0x0030}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[3], 0x0031}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Three) }}},
	{{{ KTestCategories[3], 0x1000}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64One) }}},
	{{{ KTestCategories[3], 0x80000000}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	//{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	//{{{ KTestCategories[3], 0xfffffffe}, ETypeInt32, 0x0000, 0 }, {{ 0 }}},
	
	// Existing Settings Override (Nand)
	// Over Compiler Repository
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 555666 }}},
	{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512a) }}},
	{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 32 * sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	{{{ KTestCategories[3], 0xfffffffe}, ETypeInt8, 0x0000, 0 }, {{ 100 }}},
	// Over File Repository (will be new settings in this case)
	{{{ KTestCategories[4], 0x1020}, ETypeInt8, 0x0000, 0 }, {{ -33 }}},
	{{{ KTestCategories[4], 0x1040}, ETypeUInt32, 0x0000, 0 }, {{ 999999 }}},
	{{{ KTestCategories[5], 0x0080}, ETypeBinData, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[5], 0x00d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64One) }}},
	{{{ KTestCategories[5], 0xffffffff}, ETypeUInt8, 0x0000, 0 }, {{ 25 }}},

	// New Settings (Nand)
	{{{ KTestCategories[6], 0x3000}, ETypeInt32, 0x0000, 0 }, {{ -987654 }}},
	{{{ KTestCategories[6], 0x3010}, ETypeInt16, 0x0000, 0 }, {{ -12345 }}},
	{{{ KTestCategories[6], 0x3020}, ETypeInt8, 0x0000, 0 }, {{ -100 }}},
	{{{ KTestCategories[6], 0x3030}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[6], 0x3040}, ETypeUInt32, 0x0000, 0 }, {{ 1234567890 }}},
	{{{ KTestCategories[6], 0x3050}, ETypeUInt16, 0x0000, 0 }, {{ 12345 }}},
	{{{ KTestCategories[6], 0x3060}, ETypeUInt8, 0x0000, 0 }, {{ 123 }}},
	{{{ KTestCategories[6], 0x3070}, ETypeLinAddr, 0x0000, 0 }, {{ 0xabcdef01 }}},
	{{{ KTestCategories[7], 0x2080}, ETypeBinData, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[7], 0x2093}, ETypeText8, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestString1) }}},
	{{{ KTestCategories[7], 0x20a0}, ETypeArrayInt32, 0x0000, sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[7], 0x20b1}, ETypeArrayUInt32, 0x0000, sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[7], 0x20c2}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[7], 0x20d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	{{{ KTestCategories[7], 0xffffffff}, ETypeInt8, 0x0000, 0 }, {{ -1 }}},
	};

// Nand
SSettingC SettingsList5[] = {
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 555666 }}},
	{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512a) }}},
	{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 32 * sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	{{{ KTestCategories[3], 0xfffffffe}, ETypeInt8, 0x0000, 0 }, {{ 100 }}},
	{{{ KTestCategories[4], 0x1020}, ETypeInt8, 0x0000, 0 }, {{ -33 }}},
	{{{ KTestCategories[4], 0x1040}, ETypeUInt32, 0x0000, 0 }, {{ 999999 }}},
	{{{ KTestCategories[5], 0x0080}, ETypeBinData, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[5], 0x00d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64One) }}},
	{{{ KTestCategories[5], 0xffffffff}, ETypeUInt8, 0x0000, 0 }, {{ 25 }}},
	{{{ KTestCategories[6], 0x3000}, ETypeInt32, 0x0000, 0 }, {{ -987654 }}},
	{{{ KTestCategories[6], 0x3010}, ETypeInt16, 0x0000, 0 }, {{ -12345 }}},
	{{{ KTestCategories[6], 0x3020}, ETypeInt8, 0x0000, 0 }, {{ -100 }}},
	{{{ KTestCategories[6], 0x3030}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[6], 0x3040}, ETypeUInt32, 0x0000, 0 }, {{ 1234567890 }}},
	{{{ KTestCategories[6], 0x3050}, ETypeUInt16, 0x0000, 0 }, {{ 12345 }}},
	{{{ KTestCategories[6], 0x3060}, ETypeUInt8, 0x0000, 0 }, {{ 123 }}},
	{{{ KTestCategories[6], 0x3070}, ETypeLinAddr, 0x0000, 0 }, {{ 0xabcdef01 }}},
	{{{ KTestCategories[7], 0x2080}, ETypeBinData, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[7], 0x2093}, ETypeText8, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestString1) }}},
	{{{ KTestCategories[7], 0x20a0}, ETypeArrayInt32, 0x0000, sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[7], 0x20b1}, ETypeArrayUInt32, 0x0000, sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[7], 0x20c2}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[7], 0x20d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	{{{ KTestCategories[7], 0xffffffff}, ETypeInt8, 0x0000, 0 }, {{ -1 }}},
	};

// File+Nand
SSettingC SettingsList6[] = {
	// File
	//{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 444 }}},
	{{{ KTestCategories[0], 4}, ETypeInt16, 0x0000, 0 }, {{ -7632 }}},
	{{{ KTestCategories[0], 7}, ETypeInt8, 0x0000, 0 }, {{ 120 }}},
	//{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[1], 2}, ETypeUInt32, 0x0000, 0 }, {{ 555555 }}},
	{{{ KTestCategories[1], 5}, ETypeUInt16, 0x0000, 0 }, {{ 60000 }}},
	{{{ KTestCategories[1], 8}, ETypeUInt8, 0x0000, 0 }, {{ 11 }}},
	{{{ KTestCategories[1], 12}, ETypeLinAddr, 0x0000, 0 }, {{ 0x0faece50 }}},
	{{{ KTestCategories[2], 0x1000}, ETypeBinData, 0x0000, 50 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	//{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512b) }}},
	{{{ KTestCategories[2], 0x8000}, ETypeArrayInt32, 0x0000, 6 * sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	//{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[3], 0x0021}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	//{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	//{{{ KTestCategories[3], 0xfffffffe}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[4], 0x1000}, ETypeInt32, 0x0000, 0 }, {{ 333 }}},
	{{{ KTestCategories[4], 0x1010}, ETypeInt16, 0x0000, 0 }, {{ 17632 }}},
	//{{{ KTestCategories[4], 0x1020}, ETypeInt8, 0x0000, 0 }, {{ 44 }}},
	{{{ KTestCategories[4], 0x1030}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	//{{{ KTestCategories[4], 0x1040}, ETypeUInt32, 0x0000, 0 }, {{ 3123123123u }}},
	{{{ KTestCategories[4], 0x1050}, ETypeUInt16, 0x0000, 0 }, {{ 18 }}},
	{{{ KTestCategories[4], 0x1060}, ETypeUInt8, 0x0000, 0 }, {{ 80 }}},
	{{{ KTestCategories[4], 0x1070}, ETypeLinAddr, 0x0000, 0 }, {{ 0xdeadbeef }}},
	//{{{ KTestCategories[5], 0x0080}, ETypeBinData, 0x0000, 50 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[5], 0x0093}, ETypeText8, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestString64b) }}},
	{{{ KTestCategories[5], 0x00a0}, ETypeArrayInt32, 0x0000, 6 * sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[5], 0x00b1}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[5], 0x00c2}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64One) }}},
	//{{{ KTestCategories[5], 0x00d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	//{{{ KTestCategories[5], 0xffffffff}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},

	// Existing Settings Override (Nand)
	// Over Compiler Repository (new settings here)
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 555666 }}},
	{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512a) }}},
	{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 32 * sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	{{{ KTestCategories[3], 0xfffffffe}, ETypeInt8, 0x0000, 0 }, {{ 100 }}},
	// Over File Repository
	{{{ KTestCategories[4], 0x1020}, ETypeInt8, 0x0000, 0 }, {{ -33 }}},
	{{{ KTestCategories[4], 0x1040}, ETypeUInt32, 0x0000, 0 }, {{ 999999 }}},
	{{{ KTestCategories[5], 0x0080}, ETypeBinData, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[5], 0x00d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64One) }}},
	{{{ KTestCategories[5], 0xffffffff}, ETypeUInt8, 0x0000, 0 }, {{ 25 }}},

	// New Settings (Nand)
	{{{ KTestCategories[6], 0x3000}, ETypeInt32, 0x0000, 0 }, {{ -987654 }}},
	{{{ KTestCategories[6], 0x3010}, ETypeInt16, 0x0000, 0 }, {{ -12345 }}},
	{{{ KTestCategories[6], 0x3020}, ETypeInt8, 0x0000, 0 }, {{ -100 }}},
	{{{ KTestCategories[6], 0x3030}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[6], 0x3040}, ETypeUInt32, 0x0000, 0 }, {{ 1234567890 }}},
	{{{ KTestCategories[6], 0x3050}, ETypeUInt16, 0x0000, 0 }, {{ 12345 }}},
	{{{ KTestCategories[6], 0x3060}, ETypeUInt8, 0x0000, 0 }, {{ 123 }}},
	{{{ KTestCategories[6], 0x3070}, ETypeLinAddr, 0x0000, 0 }, {{ 0xabcdef01 }}},
	{{{ KTestCategories[7], 0x2080}, ETypeBinData, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[7], 0x2093}, ETypeText8, 0x0000, 1 }, {{ reinterpret_cast<TInt32>(TestString1) }}},
	{{{ KTestCategories[7], 0x20a0}, ETypeArrayInt32, 0x0000, 1 * sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[7], 0x20b1}, ETypeArrayUInt32, 0x0000, sizeof(TUint32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[7], 0x20c2}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[7], 0x20d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	{{{ KTestCategories[7], 0xffffffff}, ETypeInt8, 0x0000, 0 }, {{ -1 }}},
	};

// File
SSettingC SettingsList7[] = {
	{{{ KTestCategories[0], 1}, ETypeInt32, 0x0000, 0 }, {{ 444 }}},
	{{{ KTestCategories[0], 4}, ETypeInt16, 0x0000, 0 }, {{ -7632 }}},
	{{{ KTestCategories[0], 7}, ETypeInt8, 0x0000, 0 }, {{ 120 }}},
	{{{ KTestCategories[0], 10}, ETypeBool, 0x0000, 0 }, {{ EFalse }}},
	{{{ KTestCategories[1], 2}, ETypeUInt32, 0x0000, 0 }, {{ 555555 }}},
	{{{ KTestCategories[1], 5}, ETypeUInt16, 0x0000, 0 }, {{ 60000 }}},
	{{{ KTestCategories[1], 8}, ETypeUInt8, 0x0000, 0 }, {{ 11 }}},
	{{{ KTestCategories[1], 12}, ETypeLinAddr, 0x0000, 0 }, {{ 0x0faece50 }}},
	{{{ KTestCategories[2], 0x1000}, ETypeBinData, 0x0000, 50 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[2], 0x7000}, ETypeText8, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestString512b) }}},
	{{{ KTestCategories[2], 0x8000}, ETypeArrayInt32, 0x0000, 6 * sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[3], 0x0020}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[3], 0x0021}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64Two) }}},
	{{{ KTestCategories[3], 0xcccccccc}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Two) }}},
	{{{ KTestCategories[3], 0xfffffffe}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	{{{ KTestCategories[4], 0x1000}, ETypeInt32, 0x0000, 0 }, {{ 333 }}},
	{{{ KTestCategories[4], 0x1010}, ETypeInt16, 0x0000, 0 }, {{ 17632 }}},
	{{{ KTestCategories[4], 0x1020}, ETypeInt8, 0x0000, 0 }, {{ 44 }}},
	{{{ KTestCategories[4], 0x1030}, ETypeBool, 0x0000, 0 }, {{ ETrue }}},
	{{{ KTestCategories[4], 0x1040}, ETypeUInt32, 0x0000, 0 }, {{ 0xba2713b3 }}},
	{{{ KTestCategories[4], 0x1050}, ETypeUInt16, 0x0000, 0 }, {{ 18 }}},
	{{{ KTestCategories[4], 0x1060}, ETypeUInt8, 0x0000, 0 }, {{ 80 }}},
	{{{ KTestCategories[4], 0x1070}, ETypeLinAddr, 0x0000, 0 }, {{ 0xdeadbeef }}},
	{{{ KTestCategories[5], 0x0080}, ETypeBinData, 0x0000, 50 }, {{ reinterpret_cast<TInt32>(TestBinData) }}},
	{{{ KTestCategories[5], 0x0093}, ETypeText8, 0x0000, 64 }, {{ reinterpret_cast<TInt32>(TestString64b) }}},
	{{{ KTestCategories[5], 0x00a0}, ETypeArrayInt32, 0x0000, 6 * sizeof(TInt32) }, {{ reinterpret_cast<TInt32>(TestInt32Array) }}},
	{{{ KTestCategories[5], 0x00b1}, ETypeArrayUInt32, 0x0000, 512 }, {{ reinterpret_cast<TInt32>(TestUint32Array) }}},
	{{{ KTestCategories[5], 0x00c2}, ETypeInt64, 0x0000, sizeof(TInt64) }, {{ reinterpret_cast<TInt32>(&KTestI64One) }}},
	{{{ KTestCategories[5], 0x00d0}, ETypeUInt64, 0x0000, sizeof(TUint64) }, {{ reinterpret_cast<TInt32>(&KTestU64Three) }}},
	{{{ KTestCategories[5], 0xffffffff}, ETypeInt8, 0x0000, 0 }, {{ 1 }}},
	};
#endif // !defined(__KERNEL_MODE__) || defined(HCRTEST_USERSIDE_INTERFACE)
#endif // !D_HCRSIM_TESTDATA_H
