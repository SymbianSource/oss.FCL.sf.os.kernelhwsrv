// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

// Some internals of the Unicode collation system.
#ifndef __COLLATEIMP_H__
#define __COLLATEIMP_H__

#include <e32std.h>
#include "collate.h"
#include "CompareImp.h"

//Forward declarations
struct TCollationKeyTable;

//External declarations
const TCollationKeyTable* StandardCollationMethod();

/**
@internalComponent
*/
struct TCollationKey
	{
	enum { KHighValue = 0x00FFFFFF, KFlagIsStarter = 0x80000000 };
	TUint32 Level(TInt aLevel) const
		{
		static const TUint32 mask[3] = { 0xFFFF0000, 0xFF00, 0xFC };
		return aLevel == 3 ? iHigh & KHighValue : iLow & mask[aLevel];
		}
	TBool IsStarter() const
        { 
        return (TBool)(iHigh & (TUint32)KFlagIsStarter);
        }
    
    enum {KLevel0KeySize=2, KLevel1KeySize=1,KLevel2KeySize=1,KLevel3KeySize=3 };
        
	static TInt MaxSizePerKey(TInt aLevel)
		{
		if (aLevel==0)
			return KLevel0KeySize;
		if (aLevel==1 || aLevel==2)
			return KLevel1KeySize;
		return KLevel3KeySize;			
		}

	void AppendToDescriptor(TPtr8 aLevelBuffer,TInt aLevel) const
		{
		TBuf8<4> buffer;
		switch (aLevel)
			{
			//for each level need to check for zero key
			//i.e. only append non zero key
			case 0:
				{
				if (((iLow>>16)&0xFFFF)!=0)
					{
					buffer.SetLength(KLevel0KeySize);
					buffer[0]=(TUint8)((iLow>>24)&0xFF);
					buffer[1]=(TUint8)((iLow>>16)&0xFF);
					}
				break;
				}
			case 1:
				{
				if (((iLow>>8)&0xFF)!=0)
					{
					buffer.SetLength(KLevel1KeySize);
					buffer[0]=(TUint8)((iLow>>8)&0xFF);
					}
				break;
				}
			case 2:
				{
				if ((iLow&0xFC)!=0)
					{
					buffer.SetLength(KLevel2KeySize);
					buffer[0]=(TUint8)(iLow&0xFC);
					}
				break;
				}
			case 3:
				{
				if ((iHigh&0xFFFFFF)!=0)
					{
					buffer.SetLength(KLevel3KeySize);
					buffer[0]=(TUint8)((iHigh>>16)&0xFF);
					buffer[1]=(TUint8)((iHigh>>8)&0xFF);
					buffer[2]=(TUint8)(iHigh&0xFF);
					}
				break;
				}
			}
			aLevelBuffer.Append(buffer);
		}
 
	TUint32 iLow;				// primary, secondary and tertiary keys
	TUint32 iHigh;				// quaternary key; usually the Unicode value
	};

/**
@internalComponent
*/
struct TKeyInfo
	{
	enum { EMaxKeys = 8 };

	TCollationKey iKey[EMaxKeys];	// the keys
	TInt iKeys;						// the number of keys returned
	TInt iCharactersConsumed;		// number of characters consumed from the input to generate the keys
	};

/**
Steps through a decomposed unicode string (using iDecompStrIt iterator), 
outputting raw collation keys.
Every Increment() call will move to the next collation key (from iKey array), if available.
Every GetCurrentKey() call will retrieve current collation key, if available.
@internalComponent
*/
class TCollationValueIterator
	{
public:
	inline TCollationValueIterator(const TCollationMethod& aMethod);
	void SetSourceIt(TUTF32Iterator& aSourceIt);
	TBool GetCurrentKey(TCollationKey& aKey);
    TBool GetCurrentKey(TInt aLevel, TUint32& aKey);
    TUint32 GetNextNonZeroKey(TInt aLevel);
    TBool MatchChar(TChar aMatch);
    TBool AtCombiningCharacter();
    TInt SkipCombiningCharacters();
	TBool Increment();
	inline TBool IgnoringNone() const;
	inline const TCollationMethod& CollationMethod() const;
	const TText16* CurrentPositionIfAtCharacter();

private:
	TBool ProduceCollationKeys();
	void GetNextRawKeySequence();
	void GetKeyFromTable(const TCollationKeyTable* aTable);

private:
	TCanonicalDecompositionIteratorCached iDecompStrIt;//Used to iterate through the canonically decomposed input string
	// Current position in the underlying iterator (if well defined)
	// of the start of the keys stored in iKey.
	const TText16* iCurrentPosition;
	const TCollationMethod& iMethod;//Current (locale dependend) collation method
	TKeyInfo iKey;//Each ProduceCollationKeys() call fills it with the longest possible collation keys sequence
    TInt iCurrentKeyPos;//Current position in iKey array. Incremented/set to 0 after each Increment() call
	};

#include "CollateImp.inl"

#endif //__COLLATEIMP_H__
