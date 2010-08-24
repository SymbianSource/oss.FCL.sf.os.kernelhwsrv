// Copyright (c) 2000-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb/t_usbdev/src/config.cpp
// USB Test Program T_USB_DEVICE.
// Reading and converting the XML configuration file.
//
//

#include "general.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "configTraces.h"
#endif
#include "config.h"

_LIT(KCfgLDD,"LDD");
_LIT(KCfgLDD1,"ENDPOINTS");
_LIT(KCfgLDD2,"SOFTCONNECT");
_LIT(KCfgLDD3,"SELFPOWER");
_LIT(KCfgLDD4,"REMOTEWAKEUP");
_LIT(KCfgLDD5,"HIGHSPEED");
_LIT(KCfgLDD6,"FEATURES");
_LIT(KCfgLDD7,"MAXPOWER");
_LIT(KCfgLDD8,"EPSTALL");
_LIT(KCfgLDD9,"SPEC");
_LIT(KCfgLDD10,"VID");
_LIT(KCfgLDD11,"PID");
_LIT(KCfgLDD12,"RELEASE");
_LIT(KCfgLDD13,"MANUFACTURER");
_LIT(KCfgLDD14,"PRODUCT");
_LIT(KCfgLDD15,"SERIALNUMBER");
_LIT(KCfgLDD16,"OTG");
_LIT(KCfgIF,"INTERFACE");
_LIT(KCfgIFS,"ALT_SETTING");
_LIT(KCfgIF1,"CLASS");
_LIT(KCfgIF2,"SUBCLASS");
_LIT(KCfgIF3,"PROTOCOL");
_LIT(KCfgIF4,"DESCRIPTOR");
_LIT(KCfgIF5,"BANDWIDTH_IN");
_LIT(KCfgIF6,"BANDWIDTH_OUT");
_LIT(KCfgEP,"ENDPOINT");
_LIT(KCfgEP1,"SIZE");
_LIT(KCfgEP2,"INTERVAL");
_LIT(KCfgEP3,"HSINTERVAL");
_LIT(KCfgEP4,"HSTRANSACTIONS");
_LIT(KCfgEP5,"DMA");
_LIT(KCfgEP6,"DOUBLEBUFF");
_LIT(KCfgEP7,"EXTRA");
_LIT(KCfgEP8,"BUFFERSIZE");
_LIT(KCfgEP9,"READSIZE");


_LIT(KAttributeName,"name=");
_LIT(KAttributeNumber,"number=");
_LIT(KAttributeType,"type=");
_LIT(KAttributeDirection,"direction=");

_LIT(KEpBulk,"\"BULK\"");
_LIT(KEpInterrupt,"\"INTERRUPT\"");
_LIT(KEpIsochronous,"\"ISOCHRONOUS\"");
_LIT(KEpIn,"\"IN\"");
_LIT(KEpOut,"\"OUT\"");

static const TInt8 KMaxXMLNesting = 3;						// max 3 levels of xml nesting

static const TPtrC xmlKeys[] =
	{
	(TDesC&)KCfgLDD, (TDesC&)KCfgLDD1, (TDesC&)KCfgLDD2, (TDesC&)KCfgLDD3, (TDesC&)KCfgLDD4, (TDesC&)KCfgLDD5, (TDesC&)KCfgLDD6,
	(TDesC&)KCfgLDD7, (TDesC&)KCfgLDD8, (TDesC&)KCfgLDD9, (TDesC&)KCfgLDD10, (TDesC&)KCfgLDD11, (TDesC&)KCfgLDD12, (TDesC&)KCfgLDD13,
	(TDesC&)KCfgLDD14, (TDesC&)KCfgLDD15, (TDesC&)KCfgLDD16,
	(TDesC&)KCfgIF, (TDesC&)KCfgIFS, (TDesC&)KCfgIF1, (TDesC&)KCfgIF2, (TDesC&)KCfgIF3, (TDesC&)KCfgIF4, (TDesC&)KCfgIF5, (TDesC&)KCfgIF6,
	(TDesC&)KCfgEP, (TDesC&)KCfgEP1, (TDesC&)KCfgEP2, (TDesC&)KCfgEP3, (TDesC&)KCfgEP4, (TDesC&)KCfgEP5, (TDesC&)KCfgEP6, (TDesC&)KCfgEP7, (TDesC&)KCfgEP8, (TDesC&)KCfgEP9
	};

enum xmlIndex
	{
	ExiLdd = 0,						// xmlKeys index for LDD
	ExiEndpoints,
	ExiSoftconnect,
	ExiSelfPower,
	ExiRemoteWakeup,
	ExiHighSpeed,
	ExiFeatures,
	ExiMaxPower,
	ExiEpStall,
	ExiSpec,
	ExiVID,
	ExiPID,
	ExiRelease,
	ExiManufacturer,
	ExiProduct,
	ExiSerialNumber,
	ExiOTG,
	ExiInterface,					// xmlKeys index for Interface
	ExiSetting,
	ExiClass,
	ExiSubclass,
	ExiProtocol,
	ExiDescriptor,
	ExiBandwidthIn,
	ExiBandwidthOut,
	ExiEndpoint,					// xmlKeys index for Endpoint
	ExiSize,
	ExiInterval,
	ExiHSInterval,
	ExiHSTransactions,
	ExiDMA,
	ExiDoubleBuff,
	ExiExtra,
	ExiBufferSize,
	ExiReadSize,
	ExiLAST
	};

// This array provides the index into xmlKeys for each level of xml key
// the first index for level n being defined by xmlLevels[n]
// and the last index for level n being defined by xmlLevels[n+1] - 1
// this means this must have two more entries than the number of nesting levels
// and the last entry must be the size of xmlKeys
static const TUint8 xmlLevels[] =
	{
	ExiLdd,ExiLdd+1,ExiSetting+1,ExiEndpoint+1,ExiLAST
	};

LDDConfig::LDDConfig (TPtrC aName)
	{
	iPtrNext = NULL;
	iIFPtr = NULL;
	iName = aName;
	iSoftConnect = ETrue;
	iSelfPower = ETrue;
	iRemoteWakeup = ETrue;
	iHighSpeed = ETrue;
	iEPStall = ETrue;
	iNumEndpoints = 15;
	iNumChannels = 0;
	iFeatures = 1;
	iMaxPower = 50;
	iSpec = 0x0200;
	iVid = 0x0E22;
	iPid = 0x1111;
	iRelease = 0x0305;
	iManufacturer = NULL;
	iProduct = NULL;
	iSerialNumber = NULL;
	iOtg_A = ETrue;
	iOtg_altA = ETrue;
	iOtg_altB = ETrue;
	};

IFConfig::IFConfig(TUint8 aNumber)
	{
	iPtrNext = NULL;
	iNumber = aNumber;
	iAlternateSetting = EFalse;
	#ifdef USB_SC
	iInfoPtr = new TUsbcScInterfaceInfo(0xFF,0xFF,0xFF);
	#else
	iInfoPtr = new TUsbcInterfaceInfo(0xFF,0xFF,0xFF);
	iBandwidthIn = EUsbcBandwidthINDefault;
	iBandwidthOut = EUsbcBandwidthOUTDefault;
	#endif
	}

ConfigPtrs::ConfigPtrs (LDDConfigPtr * aLDDPtrPtr)
	{
	iNextLDDPtrPtr = aLDDPtrPtr;
	iThisLDDPtr = * aLDDPtrPtr;
	iThisIFPtr = NULL;
	iNextIFPtrPtr = &iThisIFPtr;		// initialised to prevent warning
	}

extern RTest test;
extern TBool gVerbose;
extern TBool gSkip;
extern TBool gStopOnFail;
extern TInt gSoakCount;

bool ProcessConfigFile (RFile aConfigFile,CConsoleBase* iConsole, LDDConfigPtr * LDDPtrPtr)
	{
	TUSB_PRINT ("Processing Configuration File");
	OstTrace0 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS, "Processing Configuration File");

	TBuf8<100> configBuf;
	TBuf<101> stringBuf;
	bool done = false;
	bool error = false;
	TInt rStatus;
	XMLState state = EEmpty;
	TChar nextChar(' ');
	TChar lastChar(' ');
	TBuf<50> keyString;
	TBuf<50> endkeyString;
	TBuf<50> attributeString;
	TBuf<50> valueString;
	TInt level = -1;
	TInt levelKeys[KMaxXMLNesting+1];

	* LDDPtrPtr = NULL;
	ConfigPtrsPtr cpPtr = new ConfigPtrs (LDDPtrPtr);

	while (!done && !error)
		{
		rStatus = aConfigFile.Read((TDes8&)configBuf);
		if (rStatus != KErrNone)
			{
			error = true;
			TUSB_PRINT1("Config file error %d", rStatus);
			OstTrace1(TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP01, "Config file error %d", rStatus);
			}
		else
			{
			if (configBuf.Length() == 0)
				{
				done = true;
				}
			else
				{
				stringBuf.Copy(configBuf);
				for (TInt i = 0; i < stringBuf.Length() && !error; i++)
					{
					lastChar = nextChar;
					nextChar = stringBuf[i];
					if (((nextChar == '<') && !((state == EEmpty) || (state == EValue))) ||
						((nextChar == '>') && ((state == EEmpty) || (state == EValue))) ||
						((nextChar == '/') && (lastChar != '<')))
						{
						error = true;
						TUSB_PRINT2 ("Config File Syntax Error at index %d of %s",i,stringBuf.PtrZ());
						OstTraceExt2 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP02, "Config File Syntax Error at index %d of %S",i,stringBuf);
						}
					switch (state)
						{
						case EEmpty:
							if (nextChar == '<')
								{
								state = EStartKey;
								}
							else
								if (!nextChar.IsSpace())
									{
									error = true;
									TUSB_PRINT2 ("Config File Syntax Error at index %d of %s",i,stringBuf.PtrZ());
									OstTraceExt2 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP03, "Config File Syntax Error at index %d of %S",i,stringBuf);
									}
							break;

						case EStartKey:
							if (nextChar == '/')
								{
								state = EEndKey;
								endkeyString.SetLength(0);
								}
							else
								{
								if (nextChar == '>')
									{
									level++;
									if (level > KMaxXMLNesting)
										{
										error = true;
										TUSB_PRINT1 ("Config File Too Many levels %s",stringBuf.PtrZ());
										OstTraceExt1 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP04, "Config File Too Many levels %S",stringBuf);
										}
									else
										{
										levelKeys[level] = CheckXmlKey (keyString,level);
										if (levelKeys[level] < 0)
											{
											error = true;
											TUSB_PRINT1 ("Invalid XML key %s",keyString.PtrZ());
											OstTraceExt1 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP05, "Invalid XML key %S",keyString);
											}
										else
											{
											if (CheckAttribute(iConsole,cpPtr,levelKeys[level],attributeString))
												{
												state = EValue;
												TUSB_VERBOSE_PRINT2 ("Start key: %s level %d",keyString.PtrZ(),level);
												if(gVerbose)
												    {
												    OstTraceExt2 (TRACE_VERBOSE, CONFIGPTRS_CONFIGPTRS_DUP06, "Start key: %S level %d",keyString,level);
												    }
												}
											else
												{
												error = true;
												TUSB_PRINT1 ("No attribute for XML key %s",keyString.PtrZ());
												OstTraceExt1 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP07, "No attribute for XML key %S",keyString);
												}
											}
										}
									}
								else
									{
									if (lastChar == '<')
										{
										keyString.SetLength(0);
										valueString.SetLength(0);
										attributeString.SetLength(0);
										if (nextChar.IsSpace())
											{
											error = true;
											TUSB_PRINT2 ("Config File Syntax Error at index %d of %s",i,stringBuf.PtrZ());
											OstTraceExt2 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP08, "Config File Syntax Error at index %d of %S",i,stringBuf);
											}
										}
									if (nextChar.IsSpace())
										{
										state = EAttribute;
										}
									else
										{
										keyString.Append(nextChar);
										}
									}
								}
							break;

						case EEndKey:
							if (nextChar == '>')
								{
								if (levelKeys[level] != CheckXmlKey (endkeyString,level))
									{
									error = true;
									TUSB_PRINT1 ("Invalid XML end key %s",endkeyString.PtrZ());
									OstTraceExt1 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP09, "Invalid XML end key %S",endkeyString);
									}
								else
									{
									if (CheckValue(iConsole,cpPtr,levelKeys[level],valueString))
										{
										state = EEmpty;
										TUSB_VERBOSE_PRINT2 ("End Key: %s value %s",endkeyString.PtrZ(),valueString.PtrZ());
										if(gVerbose)
										    {
										    OstTraceExt2 (TRACE_VERBOSE, CONFIGPTRS_CONFIGPTRS_DUP10, "End Key: %S value %S",endkeyString,valueString);
										    }
										level--;
										valueString.SetLength(0);
										}
									else
										{
										error = true;
										TUSB_PRINT2 ("Incorrect value string %s for XML key %s",valueString.PtrZ(),endkeyString.PtrZ());
										OstTraceExt2 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP11, "Incorrect value string %S for XML key %S",valueString,endkeyString);
										}
									}
								}
							if (nextChar.IsSpace())
								{
									error = true;
									TUSB_PRINT2 ("Config File Syntax Error at index %d of %s",i,stringBuf.PtrZ());
									OstTraceExt2 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP12, "Config File Syntax Error at index %d of %S",i,stringBuf);
								}
							else
								{
								endkeyString.Append(nextChar);
								}
							break;

						case EAttribute:
							if (nextChar == '>')
								{
								level++;
								if (level > KMaxXMLNesting)
									{
									error = true;
									TUSB_PRINT1 ("Config File Too Many levels %s",stringBuf.PtrZ());
									OstTraceExt1 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP13, "Config File Too Many levels %s",stringBuf);
									}
								else
									{
									levelKeys[level] = CheckXmlKey (keyString,level);
									if (levelKeys[level] < 0)
										{
										error = true;
										TUSB_PRINT1 ("Invalid XML key %s",keyString.PtrZ());
										OstTraceExt1 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP14, "Invalid XML key %s",keyString);
										}
									else
										{
										if (CheckAttribute(iConsole,cpPtr,levelKeys[level],attributeString))
											{
											state = EValue;
											TUSB_VERBOSE_PRINT3 ("Start key: %s level %d attribute %s",keyString.PtrZ(),level,attributeString.PtrZ());
											if(gVerbose)
											    {
											    OstTraceExt3 (TRACE_VERBOSE, CONFIGPTRS_CONFIGPTRS_DUP15, "Start key: %S level %d attribute %S",keyString,level,attributeString);
											    }
											}
										else
											{
											error = true;
											TUSB_PRINT2 ("Incorrect attribute %s for XML key %s",attributeString.PtrZ(),keyString.PtrZ());
											OstTraceExt2 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP16, "Incorrect attribute %s for XML key %s",attributeString,keyString);
											}
										}
									}
								}
							else
								{
								attributeString.Append(nextChar);
								}
							break;

						case EValue:
							if (nextChar == '<')
								{
								state = EStartKey;
								}
							else
								{
								// Don't add any leading spaces
								if (!nextChar.IsSpace() || valueString.Length() != 0)
									{
									valueString.Append(nextChar);
									}
								}
							break;
						}
					}
				}
			}
		}

	delete cpPtr;

	return !error;
	}



TBool CheckAttribute (CConsoleBase* iConsole, ConfigPtrsPtr cpPtr,TInt aKeyIndex, TPtrC aDes)
	{
	TBool retValue = ETrue;
	TBuf<50> attrib = aDes;
	TInt typePos;
	TInt dirPos;
	TInt typeLen;
	TInt dirLen;
	TUint epType = 0;
	TUint epDir = 0;
	TUint ifNumber = 0;

	_LIT (KQuote,"\"");

	switch (aKeyIndex)
		{
		// level 0 index LDD
		case ExiLdd :
			if (attrib.Find(KAttributeName) == 0)
				{
				attrib.Delete(0,((TDesC&)KAttributeName).Length());
				if (attrib[0] == ((TDesC&)KQuote)[0] && attrib[attrib.Length()-1] == ((TDesC&)KQuote)[0])
					{
					TUSB_VERBOSE_PRINT1 ("LDD with attribute name %s",attrib.PtrZ());
					if(gVerbose)
					    {
					    OstTraceExt1 (TRACE_VERBOSE, CONFIGPTRS_CONFIGPTRS_DUP17, "LDD with attribute name %s",attrib);
					    }
					cpPtr->iThisLDDPtr = new LDDConfig (attrib.MidTPtr(1,attrib.Length()-2));
					*cpPtr->iNextLDDPtrPtr = cpPtr->iThisLDDPtr;
					cpPtr->iNextLDDPtrPtr = &(cpPtr->iThisLDDPtr->iPtrNext);
					cpPtr->iNextIFPtrPtr = &(cpPtr->iThisLDDPtr->iIFPtr);
					}
				else
					retValue = EFalse;
				}
			else
				{
				retValue = EFalse;
				}
			break;

		//	level 1 index INTERFACE
		case ExiInterface :
			if (attrib.Find(KAttributeNumber) == 0)
				{
				attrib.Delete(0,((TDesC&)KAttributeNumber).Length());
				if (attrib[0] == ((TDesC&)KQuote)[0] && attrib[attrib.Length()-1] == ((TDesC&)KQuote)[0])
					{
					if (TDesToTUint(attrib.MidTPtr(1,attrib.Length()-2), &ifNumber))
						{
						if (cpPtr->iThisLDDPtr == NULL)
							{
							TUSB_PRINT ("No LDD container for interface");
							OstTrace0 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP18, "No LDD container for interface");
							retValue = EFalse;
							}
						}
					else
						{
						TUSB_PRINT2 ("Number conversion error %s %d",attrib.PtrZ(),ifNumber);
						OstTraceExt2 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP19, "Number conversion error %S %u",attrib,ifNumber);
						retValue = EFalse;
						}
					}
				else
					{
					TUSB_PRINT1 ("Attribute number not in \"\" %s",attrib.PtrZ());
					OstTraceExt1 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP20, "Attribute number not in \"\" %s",attrib);
					retValue = EFalse;
					}
				}
			if (retValue)
				{
	 			TUSB_VERBOSE_PRINT1 ("Interface number %d",ifNumber);
	 			if(gVerbose)
	 			    {
	 			    OstTrace1 (TRACE_VERBOSE, CONFIGPTRS_CONFIGPTRS_DUP21, "Interface number %d",ifNumber);
	 			    }
				cpPtr->iThisIFPtr = new IFConfig ((TUint8)ifNumber);
				* cpPtr->iNextIFPtrPtr = cpPtr->iThisIFPtr;
				cpPtr->iNextIFPtrPtr = &cpPtr->iThisIFPtr->iPtrNext;
				cpPtr->iThisLDDPtr->iNumChannels++;
				}
			break;


		//	level 1 index Setting
		case ExiSetting :
			if (aDes.Length() != 0)
				{
				retValue = EFalse;
				}
			else
				{
				if (cpPtr->iThisLDDPtr == NULL)
					{
					TUSB_PRINT ("No LDD container for interface");
					OstTrace0 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP22, "No LDD container for interface");
					retValue = EFalse;
					}
				else
					{
					TUSB_VERBOSE_PRINT ("Alternate Interface Setting");
					if(gVerbose)
					    {
					    OstTrace0 (TRACE_VERBOSE, CONFIGPTRS_CONFIGPTRS_DUP23, "Alternate Interface Setting");
					    }
					cpPtr->iThisIFPtr = new IFConfig (0);
					* cpPtr->iNextIFPtrPtr = cpPtr->iThisIFPtr;
					cpPtr->iNextIFPtrPtr = &cpPtr->iThisIFPtr->iPtrNext;
					cpPtr->iThisIFPtr->iAlternateSetting = ETrue;
					}
				}
			break;

		//	level 2 index ENDPOINT
		case ExiEndpoint :
			typePos = attrib.Find(KAttributeType);
			dirPos = attrib.Find(KAttributeDirection);

			if (typePos == KErrNotFound || dirPos == KErrNotFound)
				{
				retValue = EFalse;
				}
			else
				{
				if (typePos < dirPos)
					{
					typePos += ((TDesC&)KAttributeType).Length();
					typeLen = dirPos - typePos;
					dirPos += ((TDesC&)KAttributeDirection).Length();
					dirLen = attrib.Length() - dirPos;
					}
				else
					{
					dirPos += ((TDesC&)KAttributeDirection).Length();
					dirLen = typePos - dirPos;
					typePos += ((TDesC&)KAttributeType).Length();
					typeLen = attrib.Length() - typePos;
					}
				TPtr attribPtr = attrib.MidTPtr(typePos,typeLen);
				attribPtr.UpperCase();
				attribPtr.TrimAll();
				if (attribPtr == KEpBulk)
					{
					epType = KUsbEpTypeBulk;
					}
				else
					{
					if (attribPtr == KEpInterrupt)
						{
						epType = KUsbEpTypeInterrupt;
						}
					else
						{
						if (attribPtr == KEpIsochronous)
							{
							epType = KUsbEpTypeIsochronous;
							}
						else
							{
							retValue = EFalse;
							}
						}
					}
				attribPtr = attrib.MidTPtr(dirPos,dirLen);
				attribPtr.UpperCase();
				attribPtr.TrimAll();
				if (attribPtr == KEpIn)
					{
					epDir = KUsbEpDirIn;
					}
				else
					{
					if (attribPtr == KEpOut)
						{
						epDir = KUsbEpDirOut;
						}
					else
						{
						retValue = EFalse;
						}
					}
				if (retValue)
					{
					if (cpPtr->iThisIFPtr == NULL)
						{
						TUSB_PRINT ("No Interface container for Endpoint");
						OstTrace0 (TRACE_NORMAL, CONFIGPTRS_CONFIGPTRS_DUP24, "No Interface container for Endpoint");
						retValue = EFalse;
						}
					else
						{
						TUint epIndex = cpPtr->iThisIFPtr->iInfoPtr->iTotalEndpointsUsed;
						TUSB_VERBOSE_PRINT2 ("Endpoint with type %d %d",epType,epDir);
						if(gVerbose)
						    {
						    OstTraceExt2 (TRACE_VERBOSE, CONFIGPTRS_CONFIGPTRS_DUP25, "Endpoint with type %u %u",(TUint32)epType,(TUint32)epDir);
						    }
						cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iType = epType;
						cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iDir = epDir;
						#ifdef USB_SC
						cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iBufferSize = 65536;
						cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iReadSize = 4096;
						#endif
						cpPtr->iThisIFPtr->iEpDMA[epIndex] = EFalse;
						cpPtr->iThisIFPtr->iEpDoubleBuff[epIndex] = EFalse;
						cpPtr->iThisIFPtr->iInfoPtr->iTotalEndpointsUsed++;
						}
					}
				}
			break;

		default :
			if (aDes.Length() != 0)
				{
				retValue = EFalse;
				}
		}

	return retValue;
	}

TBool CheckValue (CConsoleBase* iConsole, ConfigPtrsPtr cpPtr, TInt aKeyIndex, TPtrC16 aDes)
	{
	TBool retValue = ETrue;
	TBool boolValue;
	TUint uintValue;
	TInt epIndex = -1;

	if (cpPtr->iThisIFPtr != NULL)
		{
		epIndex = cpPtr->iThisIFPtr->iInfoPtr->iTotalEndpointsUsed -1;
		}

	TUSB_VERBOSE_PRINT2 ("CheckValue keyIndex %d %s",aKeyIndex,aDes.Ptr());
	if(gVerbose)
	    {
	    OstTraceExt2 (TRACE_VERBOSE, CONFIGPTRS_CONFIGPTRS_DUP26, "CheckValue keyIndex %d %s",aKeyIndex,aDes);
	    }
	switch (aKeyIndex)
		{
		case ExiLdd:						// xmlKeys index for LDD
		case ExiInterface:					// xmlKeys index for Interface
		case ExiEndpoint:					// xmlKeys index for Endpoint
			if (aDes.Length() != 0)
				{
				retValue = EFalse;
				}
			break;

		case ExiEndpoints:
			retValue = TDesToTUint (aDes, &uintValue);
			if (uintValue == 0 || uintValue > 128)
				{
				retValue = EFalse;
				}
			else
				{
				cpPtr->iThisLDDPtr->iNumEndpoints = uintValue;
				}
			break;

		case ExiSoftconnect:
			retValue = TDesToBool (aDes, &boolValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iSoftConnect = boolValue;
				}
			break;

		case ExiSelfPower:
			retValue = TDesToBool (aDes, &boolValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iSelfPower = boolValue;
				}
			break;

		case ExiRemoteWakeup:
			retValue = TDesToBool (aDes, &boolValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iRemoteWakeup = boolValue;
				}
			break;

		case ExiHighSpeed:
			retValue = TDesToBool (aDes, &boolValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iHighSpeed = boolValue;
				}
			break;

		case ExiFeatures:
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iFeatures = uintValue;
				}
			break;

		case ExiMaxPower:
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisLDDPtr == NULL || uintValue > 50)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iMaxPower = uintValue;
				}
			break;

		case ExiEpStall:
			retValue = TDesToBool (aDes, &boolValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iEPStall = boolValue;
				}
			break;

		case ExiSpec:
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iSpec = uintValue;
				}
			break;

		case ExiVID:
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iVid = uintValue;
				}
			break;

		case ExiPID:
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iPid = uintValue;
				}
			break;

		case ExiRelease:
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisLDDPtr == NULL)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisLDDPtr->iRelease = uintValue;
				}
			break;

		case ExiManufacturer:
			cpPtr->iThisLDDPtr->iManufacturer = aDes.Alloc();
			break;

		case ExiProduct:
			cpPtr->iThisLDDPtr->iProduct = aDes.Alloc();
			break;

		case ExiSerialNumber:
			cpPtr->iThisLDDPtr->iSerialNumber = aDes.Alloc();
			break;

		case ExiOTG:
			break;

		case ExiClass:
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisIFPtr == NULL || uintValue > 0xFF)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisIFPtr->iInfoPtr->iClass.iClassNum = uintValue;
				}
			break;

		case ExiSubclass:
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisIFPtr == NULL || uintValue > 0xFF)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisIFPtr->iInfoPtr->iClass.iSubClassNum = uintValue;
				}
			break;

		case ExiProtocol:
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisIFPtr == NULL || uintValue > 0xFF)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisIFPtr->iInfoPtr->iClass.iProtocolNum = uintValue;
				}
			break;

		case ExiDescriptor:
			cpPtr->iThisIFPtr->iInfoPtr->iString = aDes.Alloc();
			break;

		case ExiBandwidthIn:
			#ifdef USB_SC
			retValue = EFalse;
			#else
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisIFPtr == NULL || uintValue > 3)
				retValue = EFalse;
			if (retValue)
				{
				switch (uintValue)
					{
				case 0:
					cpPtr->iThisIFPtr->iBandwidthIn = EUsbcBandwidthINDefault;
					break;
				case 1:
					cpPtr->iThisIFPtr->iBandwidthIn = EUsbcBandwidthINPlus1;
					break;
				case 2:
					cpPtr->iThisIFPtr->iBandwidthIn = EUsbcBandwidthINPlus2;
					break;
				case 3:
					cpPtr->iThisIFPtr->iBandwidthIn = EUsbcBandwidthINMaximum;
					break;
					}
				}
			#endif
			break;

		case ExiBandwidthOut:
			#ifdef USB_SC
			retValue = EFalse;
			#else
			retValue = TDesToTUint (aDes, &uintValue);
			if (cpPtr->iThisIFPtr == NULL || uintValue > 3)
				retValue = EFalse;
			if (retValue)
				{
				switch (uintValue)
					{
				case 0:
					cpPtr->iThisIFPtr->iBandwidthOut = EUsbcBandwidthOUTDefault;
					break;
				case 1:
					cpPtr->iThisIFPtr->iBandwidthOut = EUsbcBandwidthOUTPlus1;
					break;
				case 2:
					cpPtr->iThisIFPtr->iBandwidthOut = EUsbcBandwidthOUTPlus2;
					break;
				case 3:
					cpPtr->iThisIFPtr->iBandwidthOut = EUsbcBandwidthOUTMaximum;
					break;
					}
				}
			#endif
			break;

		case ExiSize:
			retValue = TDesToTUint (aDes, &uintValue);
			if (epIndex < 0)
				retValue = EFalse;
			if (retValue)
				{
				TBool defaultIF = cpPtr->iThisLDDPtr->iIFPtr == cpPtr->iThisIFPtr;
				switch (cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iType)
					{
					case KUsbEpTypeBulk :
						if (cpPtr->iThisLDDPtr->iHighSpeed)
							{
							if (uintValue != 512)
								retValue = EFalse;
							}
						else
							{
							if (!(uintValue == 8 || uintValue == 16 || uintValue == 32 || uintValue == 64))
								retValue = EFalse;
							}
						break;

					case KUsbEpTypeInterrupt :
						if ((defaultIF && uintValue > 64) ||
							(!cpPtr->iThisLDDPtr->iHighSpeed && uintValue > 64) ||
							(!defaultIF && cpPtr->iThisLDDPtr->iHighSpeed && uintValue > 1024))
							retValue = EFalse;
						break;

					case KUsbEpTypeIsochronous :
						if ((defaultIF && uintValue > 0) ||
							(!defaultIF && !cpPtr->iThisLDDPtr->iHighSpeed && uintValue > 1023) ||
							(!defaultIF && cpPtr->iThisLDDPtr->iHighSpeed && uintValue > 1024))
							retValue = EFalse;
						break;
					}
				if (retValue)
					{
					cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iSize = uintValue;
					}
				}

			break;

		case ExiInterval:
			retValue = TDesToTUint (aDes, &uintValue);
			if (epIndex < 0)
				retValue = EFalse;
			if (retValue)
				{
				switch (cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iType)
					{
					case KUsbEpTypeBulk :
						retValue = EFalse;
						break;

					case KUsbEpTypeInterrupt :
						if (uintValue < 1 || uintValue > 255)
							retValue = EFalse;
						break;

					case KUsbEpTypeIsochronous :
						if (uintValue < 1 || uintValue > 16)
							retValue = EFalse;
						break;
					}
				if (retValue)
					{
					cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iInterval = uintValue;
					}
				}
			break;

		case ExiHSInterval:
			retValue = TDesToTUint (aDes, &uintValue);
			if (epIndex < 0 || !cpPtr->iThisLDDPtr->iHighSpeed)
				retValue = EFalse;
			if (retValue)
				{
				switch (cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iType)
					{
					case KUsbEpTypeBulk :
						if (uintValue > 255)
							retValue = EFalse;
						break;

					case KUsbEpTypeInterrupt :
						if (uintValue < 1 || uintValue > 16)
							retValue = EFalse;
						break;

					case KUsbEpTypeIsochronous :
						if (uintValue < 1 || uintValue > 16)
							retValue = EFalse;
						break;
					}
				if (retValue)
					{
					cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iInterval_Hs = uintValue;
					}
				}
			break;

		case ExiHSTransactions:
			retValue = TDesToTUint (aDes, &uintValue);
			if (epIndex < 0 || !cpPtr->iThisLDDPtr->iHighSpeed)
				retValue = EFalse;
			if (retValue)
				{
				if (uintValue > 2)
					retValue = EFalse;
				if (retValue)
					{
					cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iTransactions = uintValue;
					}
				}
			break;

		case ExiDMA:
			retValue = TDesToBool (aDes, &boolValue);
			if (epIndex < 0)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisIFPtr->iEpDMA[epIndex] = boolValue;
				}
			break;

		case ExiDoubleBuff:
			#ifdef USB_SC
			retValue = EFalse;
			#else
			retValue = TDesToBool (aDes, &boolValue);
			if (epIndex < 0)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisIFPtr->iEpDoubleBuff[epIndex] = boolValue;
				}
			#endif
			break;

		case ExiExtra:
			retValue = TDesToTUint (aDes, &uintValue);
			if (epIndex < 0)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iExtra = uintValue;
				}
			break;

		case ExiBufferSize:
			#ifdef USB_SC
			retValue = TDesToTUint (aDes, &uintValue);
			if (epIndex < 0 || uintValue < 4096)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iBufferSize = uintValue;
				}
			#else
			retValue = EFalse;
			#endif
			break;

		case ExiReadSize:
			#ifdef USB_SC
			retValue = TDesToTUint (aDes, &uintValue);
			if (epIndex < 0 || uintValue < 1024)
				retValue = EFalse;
			if (retValue)
				{
				cpPtr->iThisIFPtr->iInfoPtr->iEndpointData[epIndex].iReadSize = uintValue;
				}
			#else
			retValue = EFalse;
			#endif
			break;
		}

	return retValue;
	}

TInt CheckXmlKey (TPtrC aKey,TInt aLevel)
	{
	TInt keyIndex = -1;

	for (TInt i = xmlLevels[aLevel]; i < xmlLevels[aLevel+1]; i++)
		{
		if (aKey == xmlKeys[i])
			{
			keyIndex = i;
			break;
			}
		}


	return keyIndex;
	}

TBool TDesToTUint (TPtrC aDes, TUint * aValue)
	{
	_LIT (KHexPrefix,"0x");
	TBuf<50> numDes = aDes;
	TBool hexBase = EFalse;
	TBool conversionOK = ETrue;
	TUint8 desIndex = 0;
	* aValue = 0;

	if (numDes.LeftTPtr(((TDesC&)KHexPrefix).Length()) == KHexPrefix)
		{
		hexBase = ETrue;
		desIndex = ((TDesC&)KHexPrefix).Length();
		}

	while (desIndex < numDes.Length() && conversionOK)
		{
		if (hexBase)
			{
			TUint maxValue = 0xFFFFFFFF >> 4;
			if (((TChar)numDes[desIndex]).IsHexDigit() && * aValue <= maxValue)
				{
				* aValue <<= 4;
				* aValue += ((TChar)numDes[desIndex]).IsDigit() ? numDes[desIndex] - '0' : ((TChar)numDes[desIndex]).GetUpperCase() - 'A' + 10;
				}
			else
				{
				conversionOK = EFalse;
				* aValue = 111;
				}
			}
		else
			{
			TUint maxValue = 0xFFFFFFFF / 10;
			if (((TChar)numDes[desIndex]).IsDigit())
				{
				TUint digit = numDes[desIndex] - '0';
				if ((* aValue < maxValue) || (* aValue == maxValue && digit <= 5))
					{
					* aValue *= 10;
					* aValue += digit;
					}
				else
					{
					conversionOK = EFalse;
					* aValue = 222;
					}
				}
			else
				{
				conversionOK = EFalse;
				* aValue = 333;
				}

			}
		desIndex++;
		}

	return conversionOK;
	}

TBool TDesToBool (TPtrC aDes, TBool * aValue)
	{
	_LIT (KBoolY,"Y");
	_LIT (KBoolN,"N");
	TBool conversionOK = ETrue;
	TBuf<50> boolDes = aDes;

	boolDes.TrimAll();
	boolDes.UpperCase();
	if (boolDes == KBoolY)
		{
		* aValue = ETrue;
		}
	else
		{
		if (boolDes == KBoolN)
			{
			* aValue = EFalse;
			}
		else
			{
			conversionOK = EFalse;
			}
		}

	return conversionOK;
	}
