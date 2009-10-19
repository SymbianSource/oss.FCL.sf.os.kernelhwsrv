// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\bench\t_whet.cpp
// Whetstone Benchmarks
// 
//

 
#include <e32math.h>
#include <e32std.h>
#include <e32std_private.h>
#include <f32file.h>
#include <e32svr.h>
#include <e32test.h>
#include <hal.h>
#include "../server/t_server.h"

const TInt KSections=9;

LOCAL_D TReal64 loop_time[KSections];
LOCAL_D TReal64 loop_mops[KSections];
LOCAL_D TReal64 loop_mflops[KSections];
LOCAL_D TReal64 TimeUsed;
LOCAL_D TReal64 mwips;
LOCAL_D TBufC<50> headings[KSections];
LOCAL_D TReal64 Check;
LOCAL_D TReal64 results[KSections];

LOCAL_D	TBuf<0x100> buf;
LOCAL_D TBuf8<0x100> buf8;
LOCAL_D const TFileName pathName = _L("C:\\E32-MATH\\");
#ifdef T_WHET_WITH_VFP
LOCAL_D const TFileName fileName = _L("WHETVFP.RES");
#else
LOCAL_D const TFileName fileName = _L("WHET.RES");
#endif
LOCAL_D RFile outfile;

#ifdef T_WHET_WITH_VFP
GLDEF_D RTest test(_L("TReal64 C/C++ Whetstone Benchmark with VFP"));
#else
GLDEF_D RTest test(_L("TReal64 C/C++ Whetstone Benchmark"));
#endif

LOCAL_C void appendString(const TDesC& aString)
	{

	buf=_L("");
	buf.AppendFormat(_L("%S\n"),&aString);
	buf8.Copy(buf);			// Unicode
	outfile.Write(buf8);
	}

LOCAL_C void pa(TReal64* e,TReal64 t,TReal64 t2)	
	{
	
	TInt j;
    for (j=0;j<6;j++)
		{
        e[0]=(e[0]+e[1]+e[2]-e[3])*t;
        e[1]=(e[0]+e[1]-e[2]+e[3])*t;
        e[2]=(e[0]-e[1]+e[2]+e[3])*t;
        e[3]=(-e[0]+e[1]+e[2]+e[3])/t2;
        }
    }

LOCAL_C void po(TReal64* e1,TInt j,TInt k,TInt l)
	{
    
	e1[j]=e1[k];
    e1[k]=e1[l];
    e1[l]=e1[j];
    }

LOCAL_C void p3(TReal64 *x,TReal64 *y,TReal64 *z,TReal64 t,TReal64 t1,TReal64 t2)
	{
    
	*x=*y;
    *y=*z;
    *x=t*(*x+*y);
    *y=t1*(*x+*y);
    *z=(*x+*y)/t2;
    }

LOCAL_C void pout(const TDesC& aTitle,TReal64 aOps,TInt aType,TReal64 aChecknum,TReal64 aTime,
				  TInt aCalibrate,TInt aSection)
	{
	
    TReal64 mops,mflops;

    Check+=aChecknum;
    loop_time[aSection]=aTime;
    headings[aSection]=aTitle;
    TimeUsed+=aTime;

    if (aCalibrate==1)
        results[aSection]=aChecknum;

    if (aCalibrate==0)
        {              
        test.Printf(_L("%- 20S%- 18g"),&headings[aSection],results[aSection]);    
       
        if (aType==1)
			{
			if (aTime>0)
				mflops=aOps/(1E+6*aTime);
            else
				mflops=0.0;

            loop_mops[aSection]=99999.0;
            loop_mflops[aSection]=mflops;
            test.Printf(_L("%- 34g%- 7g\n"),loop_mflops[aSection],loop_time[aSection]);                
            }
		else
			{
            if (aTime>0)
				mops=aOps/(1E+6*aTime);
            else
				mops=0.0;

            loop_mops[aSection]=mops;
            loop_mflops[aSection]=0.0; 
            test.Printf(_L("                 %- 17g%- 7g\n"),loop_mops[aSection],loop_time[aSection]);
            }
        }     
    }

LOCAL_C void whetstones(TInt xtra,TInt x100,TInt calibrate)
	{
	
	TTime timea,timeb;                 
    TReal64 t=0.49999975;
	const TReal64 t0=t;            

    TReal64 t1=0.50000025;
    TReal64 t2=2.0;
               
    Check=0.0;

    TInt n1,n2,n3,n4,n5,n6,n7,n8,n1mult; 
    n1 = 12*x100;
    n2 = 14*x100;
    n3 = 345*x100;
    n4 = 210*x100;
    n5 = 32*x100;
    n6 = 899*x100;
    n7 = 616*x100;
    n8 = 93*x100;
    n1mult = 10;
	
	// Section 1, Array elements
    TReal64 e1[] = 
		{1.0,-1.0,-1.0,-1.0};
	TInt i,ix;

    timea.HomeTime();
	for (ix=0; ix<xtra; ix++)
		{
		for(i=0; i<n1*n1mult; i++)
			{
			e1[0]=(e1[0]+e1[1]+e1[2]-e1[3])*t;
			e1[1]=(e1[0]+e1[1]-e1[2]+e1[3])*t;
			e1[2]=(e1[0]-e1[1]+e1[2]+e1[3])*t;
			e1[3]=(-e1[0]+e1[1]+e1[2]+e1[3])*t;
			}
        t=1.0-t;
		}
    t=t0;                    
    timeb.HomeTime();
	TReal64 time=I64REAL(timeb.Int64()-timea.Int64())/(TReal64(n1mult)*1E+6);

    pout(_L("N1 floating point "),TReal64(n1*16)*TReal64(xtra),1,e1[3],time,calibrate,1);

    // Section 2, Array as parameter
    
	timea.HomeTime();
    for (ix=0; ix<xtra; ix++)
		{
		for(i=0; i<n2; i++)
            pa(e1,t,t2);
        t=1.0-t;
        }
	t= t0; 
	timeb.HomeTime();
	time=I64REAL(timeb.Int64()-timea.Int64())/1E+6;

    pout(_L("N2 floating point "),TReal64(n2*96)*TReal64(xtra),1,e1[3],time,calibrate,2);

    // Section 3, Conditional jumps
	TInt j=1;

    timea.HomeTime();       
	for (ix=0; ix<xtra; ix++)
		{
        for(i=0; i<n3; i++)
			{
            if(j==1)
				j = 2;
            else
				j = 3;
            if(j>2)
				j = 0;
            else
				j = 1;
            if(j<1)
				j = 1;
			else
				j = 0;
            }
        }
	timeb.HomeTime();
	time=I64REAL(timeb.Int64()-timea.Int64())/1E+6;
    
	pout(_L("N3 if then else   "),TReal64(n3*3)*TReal64(xtra),2,TReal64(j),time,calibrate,3);

    // Section 4, Integer arithmetic
	j=1;
    TInt k=2;
    TInt l=3;
    
	timea.HomeTime();
    for (ix=0; ix<xtra; ix++)
		{
        for(i=0; i<n4; i++)
			{
            j=j*(k-j)*(l-k);
            k=l*k-(l-j)*k;
            l=(l-k)*(k+j);
            e1[l-2]=j+k+l;
            e1[k-2]=j*k*l;
            }
        }
	timeb.HomeTime();
	time=I64REAL(timeb.Int64()-timea.Int64())/1E+6;
        
	TReal64 x=e1[0]+e1[1];

    pout(_L("N4 fixed point    "),TReal64(n4*15)*TReal64(xtra),2,x,time,calibrate,4);
     
    // Section 5, Trig functions
    x=0.5;
	TReal64 y=0.5;
	TReal64 trg1,trg2,trg3,trg4;
    
	timea.HomeTime();
	for (ix=0; ix<xtra; ix++)
		{
		for(i=1; i<n5; i++)
			{
			Math::Sin(trg1,x);
			Math::Cos(trg2,x);
			Math::Cos(trg3,x+y);
			Math::Cos(trg4,x-y);
            Math::ATan(x,(t2*trg1*trg2)/(trg3+trg4-1.0));
			x*=t;
			Math::Sin(trg1,y);
			Math::Cos(trg2,y);
			Math::Cos(trg3,x+y);
			Math::Cos(trg4,x-y);
			Math::ATan(y,(t2*trg1*trg2)/(trg3+trg4-1.0));
            y*=t;
            }
        t=1.0-t;
        }
    t=t0;
    timeb.HomeTime();
	time=I64REAL(timeb.Int64()-timea.Int64())/1E+6;

    pout(_L("N5 sin,cos etc.   "),TReal64(n5*26)*TReal64(xtra),2,y,time,calibrate,5);
  
    // Section 6, Procedure calls
    x=1.0;
    y=1.0;
    TReal64 z=1.0;
    
	timea.HomeTime();
    for (ix=0; ix<xtra; ix++)
		{
		for(i=0; i<n6; i++)
			p3(&x,&y,&z,t,t1,t2);
        }
    timeb.HomeTime();
	time=I64REAL(timeb.Int64()-timea.Int64())/1E+6;

    pout(_L("N6 floating point "),TReal64(n6*6)*TReal64(xtra),1,z,time,calibrate,6);
  
    // Section 7, Array refrences
    j = 0;
    k = 1;
    l = 2;

    TReal64 e2[]=
		{1.0,2.0,3.0};
    
	timea.HomeTime();
    for (ix=0; ix<xtra; ix++)
		{
        for(i=0;i<n7;i++)
			po(&e2[0],j,k,l);
        }
    timeb.HomeTime();
	time=I64REAL(timeb.Int64()-timea.Int64())/1E+6;

    pout(_L("N7 assignments    "),TReal64(n7*3)*TReal64(xtra),2,e2[2],time,calibrate,7);
        
	// Section 8, Standard functions
    x=0.75;
    
	timea.HomeTime();
    for (ix=0; ix<xtra; ix++)
		{
        for(i=0; i<n8; i++)
			{
			Math::Log(trg1,x);
			Math::Exp(trg1,trg1/t1);
			Math::Sqrt(x,trg1);
			}
		}
	timeb.HomeTime();
	time=I64REAL(timeb.Int64()-timea.Int64())/1E+6;


    pout(_L("N8 exp,sqrt etc.  "),TReal64(n8*4)*TReal64(xtra),2,x,time,calibrate,8);
    }

GLDEF_C void CallTestsL(void)
	{

	TInt calibrate=1;
    TInt xtra=1;
    TInt section;
    TInt x100=100;
    TInt duration=100;
   
    test.Title();

#ifdef T_WHET_WITH_VFP
	TInt supportedModes;
	if (HAL::Get(HALData::EHardwareFloatingPoint, supportedModes) != KErrNone)
		{
		test.Printf(_L("No VFP hardware, skipping test\n"));
		return;
		}
#endif

	// connect and make directory for write file
	test.Start(_L("Making directory..."));
	TInt r=TheFs.MkDirAll(pathName);
	test(r==KErrNone || r==KErrAlreadyExists);
	test(TheFs.SetSessionPath(pathName)==KErrNone);
	test(outfile.Replace(TheFs,fileName,EFileWrite)==KErrNone);
            
	// calibrate
	test.Next(_L("Calibrating...\n"));
	TInt count=10;
	
	while (count>0)
		{
		TimeUsed=0;     
		whetstones(xtra,x100,calibrate);

		test.Printf(_L("%g Seconds     %d Passes (x 100)\n"),TimeUsed,xtra);
		calibrate++;
		count--;

		if (TimeUsed>2)
			count=0;
		else
			xtra*=5;
		}
	
	if (TimeUsed>0) 
		{
		TRealX TimeX(TimeUsed);
		TInt TimeInt=TInt(TimeX);
		xtra*=duration/TimeInt;
		}
	if (xtra<1) 
		xtra=1;
       
	calibrate=0;
  
	test.Printf(_L("\nUse %d  passes (x 100)\n"),xtra);

	test.Printf(_L("\nTReal64 C/C++ Whetstone Benchmark\n"));
	test.Printf(_L("\nLoop content        Result            MFLOPS          MOPS             Seconds\n\n"));

	TimeUsed=0;
	whetstones(xtra,x100,calibrate);
	
	test.Printf(_L("\nMWIPS                                 "));
	if (TimeUsed>0)
		mwips=(TReal64)(xtra*x100)/(10.0*TimeUsed);
    else
		mwips=0;
    
	test.Printf(_L("%- 17g                 %- 7g\n\n"),mwips,TimeUsed);
     
	if (Check==TReal64(0.0)) 
		test.Printf(_L("Wrong answer  "));
              
	// Add results to output file whets.res
	appendString(_L("Whetstone TReal64 Benchmark in C/C++\n"));
	appendString(_L("\n\n"));
	appendString(_L("Loop content        Result              MFLOPS           MOPS                 Seconds\n\n")); 
                           
	for (section=1; section<KSections; section++)
		{
		buf=_L("");
		buf.AppendFormat(_L("%- 20S%- 20g"),&headings[section],results[section]);
		buf8.Copy(buf);		// Unicode
		outfile.Write(buf8);

		if (loop_mops[section]==99999.0f)
			{      
			buf=_L("");
			buf.AppendFormat(_L("%- 39g%- 8g\n"),loop_mflops[section],loop_time[section]);
			buf8.Copy(buf);		// Unicode
			outfile.Write(buf8);	
			}
		else
			{
			buf=_L("");			
			buf.AppendFormat(_L("                  %- 21g%- 8g\n"),loop_mops[section],loop_time[section]);
			buf8.Copy(buf);		// Unicode
			outfile.Write(buf8);	
			}
		}

	buf=_L("");			
	buf.AppendFormat(_L("\nMWIPS                                   %- 18g                     %- 8g\n\n"),mwips,TimeUsed);
	buf8.Copy(buf);			// Unicode
	outfile.Write(buf8);	

	outfile.Close();
	test.Printf(_L("\nResults are in file whets.res"));

    test.End();
	return;
	}


