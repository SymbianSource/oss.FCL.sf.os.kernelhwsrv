// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_multin.h
// 
//

// #define __TRACE__

#include <e32test.h>

class MProducer
	{
public:
	virtual TPtrC Produce()=0;
	};

class MConsumer
	{
public:
	virtual void Consume(const TDesC& aStr)=0;
	virtual void Consume(const TDesC& aStr1,const TDesC& aStr2);
	virtual void Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3);
	virtual void Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3,const TDesC& aStr4);
	};

class MPipe : public MProducer,public MConsumer
	{
public:
	TPtrC Produce();
	void Consume(const TDesC& aStr);
	void Consume(const TDesC& aStr1,const TDesC& aStr2);
	void Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3);
	void Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3,const TDesC& aStr4);
private:
	TBuf<0x100> iBuf;
	};


enum TSpecies {EProducer,EConsumer,EPipe,EVirtual};

class TBase
	{
public:
	virtual ~TBase() {}
	virtual TSpecies Species() const=0;
	virtual MProducer* Producer();
	virtual MConsumer* Consumer();
	};

class TProducer : public TBase,public MProducer
	{
public:
	TSpecies Species() const;
	TPtrC Produce();
	};

class TConsumer : public TBase,public MConsumer
	{
public:
	TSpecies Species() const;
	void Consume(const TDesC& aStr);
	};

class TPipe : public TBase,public MPipe
	{
public:
	TSpecies Species() const;
	};

class TVirProducer : public virtual TBase,public MProducer
	{
public:
	~TVirProducer() {} // gcc workaround
	TSpecies Species() const;
	MProducer* Producer();
	TPtrC Produce();
	};

class TVirConsumer : public virtual TBase,public MConsumer
	{
public:
	~TVirConsumer() {} // gcc workaround
	TSpecies Species() const;
	MConsumer* Consumer();
	void Consume(const TDesC& aStr);
	};

class TVirPipe : public TVirProducer,public TVirConsumer
	{
public:
	~TVirPipe() {} // gcc workaround
	TSpecies Species() const;
	MProducer* Producer();
	MConsumer* Consumer();
	TPtrC Produce();
	void Consume(const TDesC& aStr);
	void Consume(const TDesC& aStr1,const TDesC& aStr2);
	void Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3);
	void Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3,const TDesC& aStr4);
private:
	TBuf<0x100> iBuf;
	};

