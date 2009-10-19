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
//

/**
 @file
 @internalTechnology
 
 Gets a 32-bit integer value which is in big-endian format from a byte stream.
 
 @param aPtr A pointer to a byte stream.
 @return A 32-bit long integer value in native machine format.
 
*/
inline TUint32 BigEndian::Get32(const TUint8 *aPtr)
    {
    return (aPtr[0]<<24) | (aPtr[1]<<16) | (aPtr[2]<<8) | aPtr[3];
    }

/**
    Inserts a 32-bit value into a byte stream in big-endian format.

    @param aPtr A pointer to a byte stream.
    @param aVal A 32-bit long integer value in native machine format.
 */
inline void BigEndian::Put32(TUint8 *aPtr, TUint32 aVal)
    {
    aPtr[0] = aVal >> 24;
    aPtr[1] = (aVal >> 16) & 0xff;
    aPtr[2] = (aVal >> 8) & 0xff;
    aPtr[3] = aVal & 0xff;
    }
/**
    Gets a 16-bit value integer which is in big-endian format from a byte stream.

    @param aPtr A pointer to a byte stream.
    @return A 16-bit long integer value in native machine format.
 */
inline TUint16 BigEndian::Get16(const TUint8 *aPtr)
    {
    return (aPtr[0]<<8) | aPtr[1];
    }
/**
    Inserts a 16-bit value into a byte stream in big-endian format.

    @param aPtr A pointer to a byte stream.
    @param aVal A 16-bit long integer value in native machine format.
 */
inline void BigEndian::Put16(TUint8 *aPtr, TUint16 aVal)
    {
    aPtr[0] = aVal >> 8;
    aPtr[1] = aVal & 0xff;
    }



/**
    Gets a 32-bit integer value which is in little-endian format from a byte
    stream.

    @param aPtr A pointer to a byte stream.
    @return A 32-bit long integer value in native machine format.
 */
inline TUint32 LittleEndian::Get32(const TUint8 *aPtr)
    {
    return (aPtr[3]<<24) | (aPtr[2]<<16) | (aPtr[1]<<8) | aPtr[0];
    }

/**
    Inserts a 32-bit value into a byte stream in little-endian format.

    @param aPtr A pointer to a byte stream.
    @param aVal A 32-bit long integer value in native machine format.
 */
inline void LittleEndian::Put32(TUint8 *aPtr, TUint32 aVal)
    {
    aPtr[3] = aVal >> 24;
    aPtr[2] = (aVal >> 16) & 0xff;
    aPtr[1] = (aVal >> 8) & 0xff;
    aPtr[0] = aVal & 0xff;
    }
/**
    Gets a 16-bit value integer which is in little-endian format from a byte
    stream.

    @param aPtr A pointer to a byte stream.
    @return A 16-bit long integer value in native machine format.
 */
inline TUint16 LittleEndian::Get16(const TUint8 *aPtr)
    {
    return (aPtr[1]<<8) | aPtr[0];
    }
/**
    Inserts a 16-bit value into a byte stream in little-endian format.

    @param aPtr A pointer to a byte stream.
    @param aVal A 16-bit long integer value in native machine format.
 */
inline void LittleEndian::Put16(TUint8 *aPtr, TUint16 aVal)
    {
    aPtr[1] = aVal >> 8;
    aPtr[0] = aVal & 0xff;
    }



