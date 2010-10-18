// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// 
//

template<typename T, unsigned N> class TEmergencyBuffer
	{
public:
	void Init();
public:
	void* Alloc(unsigned);
	void* Free(void*);
private:
	bool iIsOccupied[N];
	T iSlots[N];
private:
	TEmergencyBuffer();
	};

template<typename T, unsigned N> inline void TEmergencyBuffer<T,N>::Init()
	{
	for (int i=0; i < N; i++)
		{
		iIsOccupied[i] = 0;
		}
	}

template<typename T, unsigned N> void* TEmergencyBuffer<T,N>::Alloc(unsigned n)
	{
	if ( n <= sizeof(T) )
		{
		for (int i=0; i < N; i++)
			{
			bool& isOccupied = iIsOccupied[i];

			if ( ! isOccupied )
				{
				isOccupied = 1;
				return &iSlots[i];
				}
			}
		}

	return NULL;
	}

template<typename T, unsigned N> void* TEmergencyBuffer<T,N>::Free(void* p)
	{
	for (int i=0; i < N; i++)
		{
		const void* bp = &iSlots[i];
		bool& isOccupied = iIsOccupied[i];

		if ( bp == p && isOccupied )
			{
			isOccupied = 0;
			return p;
			}
		}

	return NULL;
	}


