// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32reg.h
// 
//

#ifndef __E32REG_H__
#define __E32REG_H__

/**
* 
* This is the ISO standard for region definitions. 
* http://userpage.chemie.fu-berlin.de/diverse/doc/ISO_3166.html
* It is used by localed, and can be retrieved by USer::Region()
*/

/**
@publishedAll
@released

Region identity enumeration. 
*/
enum TRegionCode
	{
	/** AFGHANISTAN */
	ERegAFG   =  4,
	
	/** ALBANIA */
	ERegALB   =  8,

	/** ANTARCTICA */                                           
	ERegATA   =  10,
	
	/** ALGERIA */                                              
	ERegDZA   =  12,

	/** AMERICAN SAMOA */                                       
	ERegASM   =  16,

	/** ANDORRA */                                             
	ERegAND   =  20,

	/** ANGOLA */                                              
	ERegAGO   =  24,
	
	/** ANTIGUA AND BARBUDA */                                  
	ERegATG   =  28,
	
	/** AZERBAIJAN */                                           
	ERegAZE   =  31, 

	/** ARGENTINA */                                            
	ERegARG   =  32,
	
	/** AUSTRALIA */                                            
	ERegAUS   =  36,

	/** AUSTRIA */                                              
	ERegAUT   =  40,

	/** BAHAMAS */                                              
	ERegBHS   =  44,

	/** BAHRAIN */                                              
	ERegBHR   =  48,

	/** BANGLADESH */                                           
	ERegBGD   =  50,

	/** ARMENIA */                                              
	ERegARM   =  51, 	

	/** BARBADOS */                                             
	ERegBRB   =  52,

	/** BELGIUM */                                              
	ERegBEL   =  56,

	/** BERMUDA */                                              
	ERegBMU   =  60,

	/** BHUTAN */                                               
	ERegBTN   =  64,

	/** BOLIVIA */                                              
	ERegBOL   =  68,

	/** BOSNIA AND HERZEGOWINA */                               
	ERegBIH   =  70,

	/** BOTSWANA */                                           
	ERegBWA   =  72,

	/** BOUVET ISLAND */                                        
	ERegBVT   =  74,

	/** BRAZIL */                                               
	ERegBRA   =  76,	

	/** BELIZE */                                               
	ERegBLZ   =  84,

	/** BRITISH INDIAN OCEAN TERRITORY */                       
	ERegIOT   =  86,	

	/** SOLOMON ISLANDS */                                
	ERegSLB   =  90,
	
	/** VIRGIN ISLANDS (BRITISH) */
	ERegVGB   =  92,

	/** BRUNEI DARUSSALAM */                                    
	ERegBRN   =  96,

	/** BULGARIA */                                             
	ERegBGR   =  100,
	
	/** MYANMAR */                                            
	ERegMMR   =  104,	

	/** BURUNDI */                                              
	ERegBDI   =  108,
	
	/** BELARUS */                                              
	ERegBLR   =  112,  
	
	/** CAMBODIA */                                             
	ERegKHM   =  116,

	/** CAMEROON */                                             
	ERegCMR   =  120,

	/** CANADA */                                               
	ERegCAN   =  124,

	/** CAPE VERDE */                                           
	ERegCPV   =  132,

	/** CAYMAN ISLANDS */                                       
	ERegCYM   =  136,

	/** CENTRAL AFRICAN REPUBLIC */                             
	ERegCAF   =  140,
	
	/** SRI LANKA */                                     
	ERegLKA   =  144,

	/** CHAD */                                                 
	ERegTCD   =  148,

	/** CHILE */                                                
	ERegCHL   =  152,

	/** CHINA */                                                
	ERegCHN   =  156,
	
	/** TAIWAN */                                           
	ERegTWN   =  158,

	/** CHRISTMAS ISLAND */                                     
	ERegCXR   =  162,

	/** COCOS (KEELING) ISLANDS */                              
	ERegCCK   =  166,

	/** COLOMBIA */                                             
	ERegCOL   =  170,

	/** COMOROS */                                             
	ERegCOM   =  174,
	
	/** MAYOTTE */                                          
	ERegMYT   =  175, 

	/** CONGO, Republic of */                                  
	ERegCOG   =  178,

	/** CONGO, Democratic Republic of (was Zaire) */          
	ERegCOD   =  180,

	/** COOK ISLANDS */                                     
	ERegCOK   =  184,

	/** COSTA RICA */                                          
	ERegCRI   =  188,

	/** CROATIA (local name: Hrvatska) */                  
	ERegHRV   =  191,  
	   
	/** CUBA */                                              
	ERegCUB   =  192,

	/** CYPRUS */                                             
	ERegCYP   =  196,

	/** CZECH REPUBLIC */                                      
	ERegCZE   =  203, 
	
	/** BENIN */                                                
	ERegBEN   =  204,

	/** DENMARK */                                          
	ERegDNK   =  208,

	/** DOMINICA */                                        
	ERegDMA   =  212,

	/** DOMINICAN REPUBLIC */                                  
	ERegDOM   =  214,

	/** ECUADOR */                                          
	ERegECU   =  218,

	/** EL SALVADOR */                                      
	ERegSLV   =  222,

	/** EQUATORIAL GUINEA */                               
	ERegGNQ   =  226,
	
	/** ETHIOPIA */                                       
	ERegETH   =  231,

	/** ERITREA */                                          
	ERegERI   =  232,

	/** ESTONIA */                                       
	ERegEST   =  233, 

	/** FAROE ISLANDS */                                   
	ERegFRO   =  234,
	
	/** FALKLAND ISLANDS (MALVINAS) */                   
	ERegFLK   =  238,
	
	/** SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS */    
	ERegSGS   =  239,

	/** FIJI */                                            
	ERegFJI   =  242,

	/** FINLAND */                                         
	ERegFIN   =  246,
	
	/** AALAND ISLANDS */
	ERegALA   =  248,

	/** FRANCE */                                          
	ERegFRA   =  250,

	/** FRENCH GUIANA */                                   
	ERegGUF   =  254,

	/** FRENCH POLYNESIA */                                 
	ERegPYF   =  258,

	/** FRENCH SOUTHERN TERRITORIES */                     
	ERegATF   =  260,	

	/** DJIBOUTI */                                           
	ERegDJI   =  262,

	/** GABON */                                          
	ERegGAB   =  266,
	
	/** GEORGIA */                                        
	ERegGEO   =  268, 

	/** GAMBIA */                                          
	ERegGMB   =  270,	

	/** PALESTINIAN TERRITORY, Occupied */                 
	ERegPSE   =  275,

	/** GERMANY */                                           
	ERegDEU   =  276,

	/** GHANA */                                             
	ERegGHA   =  288,

	/** GIBRALTAR */                                       
	ERegGIB   =  292,	

	/** KIRIBATI */                                         
	ERegKIR   =  296,

	/** GREECE */                                              
	ERegGRC   =  300,

	/** GREENLAND */                                        
	ERegGRL   =  304,

	/** GRENADA */                                        
	ERegGRD   =  308,

	/** GUADELOUPE */                                     
	ERegGLP   =  312,

	/** GUAM */                                          
	ERegGUM   =  316,

	/** GUATEMALA */                                         
	ERegGTM   =  320,

	/** GUINEA */                                           
	ERegGIN   =  324,

	/** GUYANA */                                            
	ERegGUY   =  328,

	/** HAITI */                                            
	ERegHTI   =  332,

	/** HEARD AND MC DONALD ISLANDS */                       
	ERegHMD   =  334,	

	/** VATICAN CITY STATE (HOLY SEE)  */
	ERegVAT   =  336,

	/** HONDURAS */                                         
	ERegHND   =  340,

	/** HONG KONG */                                         
	ERegHKG   =  344,

	/** HUNGARY */                                         
	ERegHUN   =  348,

	/** ICELAND */                                          
	ERegISL   =  352,

	/** INDIA */                                         
	ERegIND   =  356,

	/** INDONESIA */                                       
	ERegIDN   =  360,

	/** IRAN (ISLAMIC REPUBLIC OF) */                      
	ERegIRN   =  364,

	/** IRAQ */                                             
	ERegIRQ   =  368,

	/** IRELAND */                                          
	ERegIRL   =  372,

	/** ISRAEL */                                            
	ERegISR   =  376,

	/** ITALY */                                              
	ERegITA   =  380,
	
	/** COTE D'IVOIRE */                                      
	ERegCIV   =  384,

	/** JAMAICA */                                             
	ERegJAM   =  388,

	/** JAPAN */                                            
	ERegJPN   =  392,
	
	/** KAZAKHSTAN */                                        
	ERegKAZ   =  398, 

	/** JORDAN */                                           
	ERegJOR   =  400,

	/** KENYA */                                           
	ERegKEN   =  404,

	/** KOREA, DEMOCRATIC PEOPLE'S REPUBLIC OF */            
	ERegPRK   =  408,

	/** KOREA, REPUBLIC OF */                             
	ERegKOR   =  410,

	/** KUWAIT */                                            
	ERegKWT   =  414,

	/** KYRGYZSTAN */                                     
	ERegKGZ   =  417, 

	/** LAO PEOPLE'S DEMOCRATIC REPUBLIC */               
	ERegLAO   =  418,

	/** LEBANON */                                              
	ERegLBN   =  422,

	/** LESOTHO */                                               
	ERegLSO   =  426,
	
	/** LATVIA */                                               
	ERegLVA   =  428, 

	/** LIBERIA */                                              
	ERegLBR   =  430,

	/** LIBYAN ARAB JAMAHIRIYA */                               
	ERegLBY   =  434,

	/** LIECHTENSTEIN */                                        
	ERegLIE   =  438,

	/** LITHUANIA */                                            
	ERegLTU   =  440, 

	/** LUXEMBOURG */                                           
	ERegLUX   =  442,

	/** MACAU */                                                
	ERegMAC   =  446,

	/** MADAGASCAR */                                           
	ERegMDG   =  450,

	/** MALAWI */                                               
	ERegMWI   =  454,

	/** MALAYSIA */                                             
	ERegMYS   =  458,

	/** MALDIVES */      
	ERegMDV   =  462,

	/** MALI */                                                 
	ERegMLI   =  466,

	/** MALTA */                                                
	ERegMLT   =  470,

	/** MARTINIQUE */                                      
	ERegMTQ   =  474,

	/** MAURITANIA */                                       
	ERegMRT   =  478,

	/** MAURITIUS */                                         
	ERegMUS   =  480,

	/** MEXICO */                                           
	ERegMEX   =  484,

	/** MONACO */                                          
	ERegMCO   =  492,

	/** MONGOLIA */                                             
	ERegMNG   =  496,
	
	/** MOLDOVA, REPUBLIC OF */                              
	ERegMDA   =  498, 

	/** MONTSERRAT */                                        
	ERegMSR   =  500,

	/** MOROCCO */                                            
	ERegMAR   =  504,

	/** MOZAMBIQUE */                                        
	ERegMOZ   =  508,

	/** OMAN */                                              
	ERegOMN   =  512,

	/** NAMIBIA */                                            
	ERegNAM   =  516,

	/** NAURU */                                               
	ERegNRU   =  520,

	/** NEPAL */                                               
	ERegNPL   =  524,

	/** NETHERLANDS */                                        
	ERegNLD   =  528,

	/** NETHERLANDS ANTILLES */                             
	ERegANT   =  530,
	
	/** ARUBA */                                                
	ERegABW   =  533,

	/** NEW CALEDONIA */                                      
	ERegNCL   =  540,	
	
	/** VANUATU */                                             
	ERegVUT   =  548,

	/** NEW ZEALAND */                                          
	ERegNZL   =  554,

	/** NICARAGUA */                                         
	ERegNIC   =  558,

	/** NIGER */                                             
	ERegNER   =  562,

	/** NIGERIA */                                         
	ERegNGA   =  566,

	/** NIUE */                                              
	ERegNIU   =  570,

	/** NORFOLK ISLAND */                                     
	ERegNFK   =  574,	

	/** NORWAY */                                              
	ERegNOR   =  578,

	/** NORTHERN MARIANA ISLANDS */                          
	ERegMNP   =  580,
	
	/** UNITED STATES MINOR OUTLYING ISLANDS */              
	ERegUMI   =  581,	

	/** MICRONESIA, FEDERATED STATES OF */                   
	ERegFSM   =  583,
	
	/** MARSHALL ISLANDS */                                     
	ERegMHL   =  584,
	
	/** PALAU */                                             
	ERegPLW   =  585,

	/** PAKISTAN */                                            
	ERegPAK   =  586,

	/** PANAMA */                                              
	ERegPAN   =  591,

	/** PAPUA NEW GUINEA */                                  
	ERegPNG   =  598,

	/** PARAGUAY */                                          
	ERegPRY   =  600,

	/** PERU */                                             
	ERegPER   =  604,

	/** PHILIPPINES */                                     
	ERegPHL   =  608,

	/** PITCAIRN */                                        
	ERegPCN   =  612,

	/** POLAND */                                          
	ERegPOL   =  616,

	/** PORTUGAL */                                         
	ERegPRT   =  620,
	
	/** GUINEA-BISSAU */                                      
	ERegGNB   =  624,
	
	/** TIMOR-LESTE */                                       
	ERegTLS   =  626,

	/** PUERTO RICO */                                   
	ERegPRI   =  630,

	/** QATAR */                                            
	ERegQAT   =  634,

	/** REUNION */                                         
	ERegREU   =  638,

	/** ROMANIA */                                         
	ERegROU   =  642,

	/** RUSSIAN FEDERATION */                               
	ERegRUS   =  643,

	/** RWANDA */                                        
	ERegRWA   =  646,

	/** SAINT HELENA */                                    
	ERegSHN   =  654,

	/** SAINT KITTS AND NEVIS */                               
	ERegKNA   =  659,
	
	/** ANGUILLA */                                             
	ERegAIA   =  660, 

	/** SAINT LUCIA */                                 
	ERegLCA   =  662,

	/** SAINT PIERRE AND MIQUELON */                       
	ERegSPM   =  666,

	/** SAINT VINCENT AND THE GRENADINES */              
	ERegVCT   =  670,

	/** SAN MARINO */                                   
	ERegSMR   =  674,

	/** SAO TOME AND PRINCIPE */                        
	ERegSTP   =  678,

	/** SAUDI ARABIA */                                  
	ERegSAU   =  682,

	/** SENEGAL */                                         
	ERegSEN   =  686,

	/** SEYCHELLES */                                      
	ERegSYC   =  690,

	/** SIERRA LEONE */                                      
	ERegSLE   =  694,

	/** SINGAPORE */                                     
	ERegSGP   =  702,

	/** SLOVAKIA */                                       
	ERegSVK   =  703, 
	
	/** VIET NAM */
	ERegVNM   =  704,
		
	/** SLOVENIA */                                      
	ERegSVN   =  705, 

	/** SOMALIA  */                                        
	ERegSOM   =  706,

	/** SOUTH AFRICA */                                    
	ERegZAF   =  710,
	
	/** ZIMBABWE */
	ERegZWE   =  716,

	/** SPAIN */                                          
	ERegESP   =  724,

	/** WESTERN SAHARA */
	ERegESH   =  732,

	/** SUDAN */                                          
	ERegSDN   =  736,

	/** SURINAME */                                        
	ERegSUR   =  740,

	/** SVALBARD AND JAN MAYEN ISLANDS */                    
	ERegSJM   =  744,

	/** SWAZILAND */                                       
	ERegSWZ   =  748,

	/** SWEDEN */                                          
	ERegSWE   =  752,

	/** SWITZERLAND */                                   
	ERegCHE   =  756,

	/** SYRIAN ARAB REPUBLIC */                             
	ERegSYR   =  760,

	/** TAJIKISTAN */                                      
	ERegTJK   =  762, 

	/** THAILAND */                                       
	ERegTHA   =  764,

	/** TOGO */                                           
	ERegTGO   =  768,

	/** TOKELAU */      
	ERegTKL   =  772,

	/** TONGA */                                       
	ERegTON   =  776,

	/** TRINIDAD AND TOBAGO */                               
	ERegTTO   =  780,
	
	/** UNITED ARAB EMIRATES */                          
	ERegARE   =  784,

	/** TUNISIA */                                        
	ERegTUN   =  788,

	/** TURKEY */                                        
	ERegTUR   =  792,

	/** TURKMENISTAN */                                       
	ERegTKM   =  795, 

	/** TURKS AND CAICOS ISLANDS */                      
	ERegTCA   =  796,

	/** TUVALU */                                          
	ERegTUV   =  798,

	/** UGANDA */                                            
	ERegUGA   =  800,

	/** UKRAINE */                                         
	ERegUKR   =  804,
	
	/** MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF */           
	ERegMKD   =  807,
	
	/** EGYPT */                                           
	ERegEGY   =  818,

	/** UNITED KINGDOM  */                                  
	ERegGBR   =  826,
	
	/** TANZANIA, UNITED REPUBLIC OF */                   
	ERegTZA   =  834,

	/** UNITED STATES */                                     
	ERegUSA   =  840,

	/** VIRGIN ISLANDS (U.S.) */
	ERegVIR   =  850,
	
	/** BURKINA FASO */                                         
	ERegBFA   =  854,	
	
	/** URUGUAY */                                          
	ERegURY   =  858,

	/** UZBEKISTAN */                                        
	ERegUZB   =  860,
	
	/** VENEZUELA  */
	ERegVEN   =  862,
		
	/** WALLIS AND FUTUNA ISLANDS */
	ERegWLF   =  876,

	/** SAMOA */                                        
	ERegWSM   =  882,

	/** YEMEN */
	ERegYEM   =  887,
	
	/** SERBIA AND MONTENEGRO */                         
	ERegSCG   =  891,

	/** ZAMBIA */
	ERegZMB   =  894

	};

#endif
