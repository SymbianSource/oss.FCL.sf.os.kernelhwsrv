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
// e32test\system\t_multin.cpp
// Overview:
// Test multiple inheritance with and without virtual bases. 
// API Information:
// N/A
// Details:
// - Test multiple inheritance with virtual bases. Using a virtual
// base class, copy one or more strings from the "producer" object 
// to the "consumer" object. Verify that results are as expected.
// - Test multiple inheritance without virtual bases. Using a non-
// virtual base class, copy one or more strings from the "producer" 
// object to the "consumer" object. Verify that results are as expected.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include "t_multin.h"

LOCAL_D RTest test(_L("T_MULTIN"));

void MConsumer::Consume(const TDesC& aStr1,const TDesC& aStr2)
	{
#if defined(__TRACE__)
	test.Printf(_L("MConsumer::Consume(s,s)\n"));
	//test.Getch();
#endif
	TBuf<0x100> b;b.Format(_L("%S%S"),&aStr1,&aStr2);Consume(b);
	}

void MConsumer::Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3)
	{
#if defined(__TRACE__)
	test.Printf(_L("MConsumer::Consume(s,s,s)\n"));
	//test.Getch();
#endif
	TBuf<0x100> b;b.Format(_L("%S%S%S"),&aStr1,&aStr2,&aStr3);Consume(b);
	}

void MConsumer::Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3,const TDesC& aStr4)
	{
#if defined(__TRACE__)
	test.Printf(_L("MConsumer::Consume(s,s,s,s)\n"));
	//test.Getch();
#endif
	TBuf<0x100> b;b.Format(_L("%S%S%S%S"),&aStr1,&aStr2,&aStr3,&aStr4);Consume(b);
	}

TPtrC MPipe::Produce()
	{
#if defined(__TRACE__)
	test.Printf(_L("MPipe::Produce\n"));
	//test.Getch();
#endif
	return(iBuf);
	}

void MPipe::Consume(const TDesC& aStr)
	{
#if defined(__TRACE__)
	test.Printf(_L("MPipe::Consume(s)\n"));
	//test.Getch();
#endif
#if defined(__TRACE__)
	{
	TInt l=*(TInt*)&aStr;
	TInt t=l>>28;
	l&=0xfffffff;	
	test.Printf(_L("aStr type=%d,aStr length=%d\n"),t,l);
//
	TText* p=NULL;
	switch (t)
		{
	case 0:
		p=(TText*)((TInt*)&aStr+1);
		break;
	case 1:
		p=*(TText**)((TInt*)&aStr+1);
		break;
	case 2:
		p=*(TText**)((TInt*)&aStr+2);
		break;
	case 3:
		p=(TText*)((TInt*)&aStr+2);
		break;
	case 4:
		p=(TText*)(*(TInt**)((TInt*)&aStr+2)+1);
		break;
		}
//
	test.Printf(_L("aStr=\"%.3s...\"\n"),p);
	}
	//test.Getch();
#endif
#if defined(__TRACE__)
	{
	TInt l=*(TInt*)&iBuf;
	TInt t=l>>28;
	l&=0xfffffff;	
	TInt m=((TInt*)&iBuf)[1];
	test.Printf(_L("iBuf type=%d,iBuf length=%d,iBuf max length=%d\n"),t,l,m);
//
	TText* p=NULL;
	switch (t)
		{
	case 0:
		p=(TText*)((TInt*)&iBuf+1);
		break;
	case 1:
		p=*(TText**)((TInt*)&iBuf+1);
		break;
	case 2:
		p=*(TText**)((TInt*)&iBuf+2);
		break;
	case 3:
		p=(TText*)((TInt*)&iBuf+2);
		break;
	case 4:
		p=(TText*)(*(TInt**)((TInt*)&iBuf+2)+1);
		break;
		}
//
	test.Printf(_L("iBuf=\"%.3s...\"\n"),p);
	}
	//test.Getch();
#endif
	iBuf=aStr;
	}

void MPipe::Consume(const TDesC& aStr1,const TDesC& aStr2)
	{
#if defined(__TRACE__)
	test.Printf(_L("MPipe::Consume(s,s)\n"));
	//test.Getch();
#endif
	iBuf.Format(_L("%S%S"),&aStr1,&aStr2);
	}

void MPipe::Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3)
	{
#if defined(__TRACE__)
	test.Printf(_L("MPipe::Consume(s,s,s)\n"));
	//test.Getch();
#endif
	iBuf.Format(_L("%S%S%S"),&aStr1,&aStr2,&aStr3);
	}

void MPipe::Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3,const TDesC& aStr4)
	{
#if defined(__TRACE__)
	test.Printf(_L("MPipe::Consume(s,s,s,s)\n"));
	//test.Getch();
#endif
	iBuf.Format(_L("%S%S%S%S"),&aStr1,&aStr2,&aStr3,&aStr4);
	}

MProducer* TBase::Producer()
	{
#if defined(__TRACE__)
	test.Printf(_L("TBase::Producer\n"));
	//test.Getch();
#endif
	switch(Species())
		{
	case EProducer:
		return((TProducer*)this);
	case EPipe:
		return((TPipe*)this);
	default:
		return(NULL);
		}
	}

MConsumer* TBase::Consumer()
	{
#if defined(__TRACE__)
	test.Printf(_L("TBase::Consumer\n"));
	//test.Getch();
#endif
	switch(Species())
		{
	case EConsumer:
		return((TConsumer*)this);
	case EPipe:
		return((TPipe*)this);
	default:
		return(NULL);
		}
	}

TPtrC TProducer::Produce()
	{
#if defined(__TRACE__)
	test.Printf(_L("TProducer::Produce\n"));
	//test.Getch();
#endif
	return(_L("*"));
	}

TSpecies TProducer::Species() const
	{
#if defined(__TRACE__)
	test.Printf(_L("TProducer::Species\n"));
	//test.Getch();
#endif
	return(EProducer);
	}

TSpecies TConsumer::Species() const
	{
#if defined(__TRACE__)
	test.Printf(_L("TConsumer::Species\n"));
	//test.Getch();
#endif
	return(EConsumer);
	}

void TConsumer::Consume(const TDesC& aStr)
	{
#if defined(__TRACE__)
	test.Printf(_L("TConsumer::Consume\n"));
	//test.Getch();
#endif
	test.Printf(_L("Consumed: %S\n"),&aStr);
	}

TSpecies TPipe::Species() const
	{
#if defined(__TRACE__)
	test.Printf(_L("TPipe::Species\n"));
	//test.Getch();
#endif
	return(EPipe);
	}

TSpecies TVirProducer::Species() const
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirProducer::Species\n"));
	//test.Getch();
#endif
	return(EVirtual);
	}

TPtrC TVirProducer::Produce()
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirProducer::Produce\n"));
	//test.Getch();
#endif
	return(_L("*"));
	}

TSpecies TVirConsumer::Species() const
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirConsumer::Species\n"));
	//test.Getch();
#endif
	return(EVirtual);
	};

MConsumer* TVirConsumer::Consumer()
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirConsumer::Consumer\n"));
	//test.Getch();
#endif
	return(this);
	}

void TVirConsumer::Consume(const TDesC& aStr)
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirConsumer::Consume\n"));
	//test.Getch();
#endif
	test.Printf(_L("Consumed: %S\n"),&aStr);
	}

TSpecies TVirPipe::Species() const
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirPipe::Species\n"));
	//test.Getch();
#endif
	return(EVirtual);
	};

MProducer* TVirPipe::Producer()
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirPipe::Producer\n"));
	//test.Getch();
#endif
	return(this);
	}

MConsumer* TVirPipe::Consumer()
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirPipe::Consumer\n"));
	//test.Getch();
#endif
	return(this);
	}

TPtrC TVirPipe::Produce()
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirPipe::Produce\n"));
	//test.Getch();
#endif
	return(iBuf);
	}

void TVirPipe::Consume(const TDesC& aStr)
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirPipe::Consume(s)\n"));
	//test.Getch();
#endif
	iBuf=aStr;
	}

void TVirPipe::Consume(const TDesC& aStr1,const TDesC& aStr2)
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirPipe::Consume(s,s)\n"));
	//test.Getch();
#endif
	iBuf.Format(_L("%S%S"),&aStr1,&aStr2);
	}

void TVirPipe::Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3)
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirPipe::Consume(s,s,s)\n"));
	//test.Getch();
#endif
	iBuf.Format(_L("%S%S%S"),&aStr1,&aStr2,&aStr3);
	}

void TVirPipe::Consume(const TDesC& aStr1,const TDesC& aStr2,const TDesC& aStr3,const TDesC& aStr4)
	{
#if defined(__TRACE__)
	test.Printf(_L("TVirPipe::Consume(s,s,s,s)\n"));
	//test.Getch();
#endif
	iBuf.Format(_L("%S%S%S%S"),&aStr1,&aStr2,&aStr3,&aStr4);
	}

LOCAL_C MProducer& Producer(TBase& aBase)
	{
#if defined(__TRACE__)
	test.Printf(_L("Producer(TBase&)\n"));
	//test.Getch();
#endif
	MProducer* prod=aBase.Producer();
	test(prod!=NULL);
	return(*prod);
	}

LOCAL_C MConsumer& Consumer(TBase& aBase)
	{
#if defined(__TRACE__)
	test.Printf(_L("Consumer(TBase&)\n"));
	//test.Getch();
#endif
	MConsumer* cons=aBase.Consumer();
	test(cons!=NULL);
	return(*cons);
	}

LOCAL_C void testCopy1(MConsumer& aConsumer,MProducer& aProducer)
//
// Copy a string from the producer to the consumer.
//
	{

#if defined(__TRACE__)
	test.Printf(_L("testCopy1()\n"));
	//test.Getch();
#endif
	aConsumer.Consume(aProducer.Produce());
	}

LOCAL_C void testCopy2(MConsumer& aConsumer,MProducer& aProducer)
//
// Copy two strings from the producer to the consumer.
//
	{

#if defined(__TRACE__)
	test.Printf(_L("testCopy2()\n"));
	//test.Getch();
#endif
	TPtrC s1=aProducer.Produce();
	TPtrC s2=aProducer.Produce();
	aConsumer.Consume(s1,s2);
	}

LOCAL_C void testCopy3(MConsumer& aConsumer,MProducer& aProducer)
//
// Copy three strings from the producer to the consumer.
//
	{

#if defined(__TRACE__)
	test.Printf(_L("testCopy3()\n"));
	//test.Getch();
#endif
	TPtrC s1=aProducer.Produce();
	TPtrC s2=aProducer.Produce();
	TPtrC s3=aProducer.Produce();
	aConsumer.Consume(s1,s2,s3);
	}

LOCAL_C void testCopy4(MConsumer& aConsumer,MProducer& aProducer)
//
// Copy four strings from the producer to the consumer.
//
	{

#if defined(__TRACE__)
	test.Printf(_L("testCopy4()\n"));
	//test.Getch();
#endif
	TPtrC s1=aProducer.Produce();
	TPtrC s2=aProducer.Produce();
	TPtrC s3=aProducer.Produce();
	TPtrC s4=aProducer.Produce();
	aConsumer.Consume(s1,s2,s3,s4);
	}

LOCAL_C void testMulti()
//
// Test multiple inheritance without virtual bases.
//
	{

#if defined(__TRACE__)
	test.Printf(_L("testMulti()\n"));
	//test.Getch();
#endif
	test.Next(_L("without virtual base classes"));
//
	TProducer prod;
	TConsumer cons;
	TPipe pipe;
	testCopy1(Consumer(pipe),Producer(prod));
	testCopy2(Consumer(cons),Producer(pipe));
//
	testCopy3(Consumer(pipe),Producer(prod));
	testCopy4(Consumer(cons),Producer(pipe));
//
	testCopy4(Consumer(pipe),Producer(prod));
	testCopy3(Consumer(cons),Producer(pipe));
//
	testCopy2(Consumer(pipe),Producer(prod));
	testCopy1(Consumer(cons),Producer(pipe));
	}

LOCAL_C void testVirt()
//
// Test multiple inheritance with virtual bases.
//
	{

#if defined(__TRACE__)
	test.Printf(_L("testVirt()\n"));
	//test.Getch();
#endif
	test.Next(_L("with virtual base classes"));
//
	TVirProducer prod;
	TVirConsumer cons;
	TVirPipe pipe;
	testCopy1(Consumer(pipe),Producer(prod));
	testCopy2(Consumer(cons),Producer(pipe));
//
	testCopy3(Consumer(pipe),Producer(prod));
	testCopy4(Consumer(cons),Producer(pipe));
//
	testCopy4(Consumer(pipe),Producer(prod));
	testCopy3(Consumer(cons),Producer(pipe));
//
	testCopy2(Consumer(pipe),Producer(prod));
	testCopy1(Consumer(cons),Producer(pipe));
	}

GLDEF_C TInt E32Main()
//
// Test the multiple inheritance implementation.
//
	{
#if defined(__TRACE__)
	test.Printf(_L("E32Main()\n"));
	//test.Getch();
#endif
	test.Title();
//
	test.Start(_L("Multiple Inheritance"));
	testMulti();
//
	testVirt();
//
	test.End();
	test.Close();
	return(KErrNone);
	}

