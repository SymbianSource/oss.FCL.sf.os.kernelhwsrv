/*
* Copyright (c) 1995-2007 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


/**
@internalTechnology
@file

Implementation of the Netcards.exe adapter selection application.
This version writes settings to an ethernet.ini file only. Those settings 
should be either copied into epoc.ini (EKA2) or extracted into correct setup 
files: ethermac.dat and etherdriver.dat (EKA1) using an appropriate tool.
*/

/*
 * Desired design of maximum size and alignment.
 * These are implementation specific.
 */
#define _SS_MAXSIZE 128                  // Maximum size.
#define _SS_ALIGNSIZE (sizeof(__int64))  // Desired alignment.

/*
 * Definitions used for sockaddr_storage structure paddings design.
 */
/**
 * This version of netcards.c has been written to work and run on Windows 2000
 * and Windows XP. It has been compiled against both WinPCap 4.0.
 *
 * We identify the version of the compiler from the macros set by E32PLAT.pm 
 * and if .NET assume that winsock2 will be used (although this is not needed).
 */

#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof (short))
#define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof (short) + _SS_PAD1SIZE \
                                                    + _SS_ALIGNSIZE))

struct sockaddr_storage {
    short ss_family;               // Address family.
    char __ss_pad1[_SS_PAD1SIZE];  // 6 byte pad, this is to make
                                   // implementation specific pad up to
                                   // alignment field that follows explicit
                                   // in the data structure.
    __int64 __ss_align;            // Field to force desired structure.
    char __ss_pad2[_SS_PAD2SIZE];  // 112 byte pad to achieve desired size;
                                   // _SS_MAXSIZE value minus size of
                                   // ss_family, __ss_pad1, and
                                   // __ss_align fields is 112.
};

// Forward declare struct _RPC_ASYNC_STATE, to avoid a warning in rpcasync.h
struct _RPC_ASYNC_STATE;

// include WinPcap
// The dance with warnings is necessary to avoid warnings such as
//
// \MSVC6\VC98\include\qos.h(236) : warning C4201: nonstandard extension used : 
// 	nameless struct/union 
//
// when we compile at warning level 4.
// 

#pragma warning(disable:4514)
#pragma warning(push,3)
#include "pcap.h"
#pragma warning(pop)

//other includes
#include "packet32.h"
#include "ntddndis.h"
#include <windows.h>

#include <stdio.h>
#include <malloc.h>
#include <conio.h>


#define EPOC_INI_FILE			"ethernet.ini"

#define EPOC_INI_FILE_TEMP		"__temp__ethernet__.ini"

#define	ETHER_NIF_ENTRY			"ETHER_NIF"			
#define	ETHER_MAC_ENTRY			"ETHER_MAC"
#define	ETHER_SPEED_ENTRY		"ETHER_SPEED"

#define MAX_VALUE		80
#define MAX_LINE		100
#define MAX_OID_DATA	256

#define MAX_ADAPTER_LEN	1024
#define MAX_ADAPTERS	10

#define OID_802_3_CURRENT_ADDRESS		   		0x01010102


//
// Global list of supported adapter names.
//
char	AdapterList[MAX_ADAPTERS][MAX_ADAPTER_LEN];


//
// Replace or write new 'value' for 'entry' in epoc.ini file
// returns 0 if ok, negative value if sth wrong
//
int replace_in_inifile(char * entry, char* value, BOOL valUnicode );


//
// Gets the adapter speed and mac address. Returns 0 if no mac is available,
// otherwise returns non-zero if successful.
//
int get_adapter_speed_and_mac(char* adapterName, UINT* speed, unsigned char* mac);

//
// Main entry point.
//
int main(int argc, char* argv[])
{
	char err[PCAP_ERRBUF_SIZE + 1];
	

	// string that contains a list of the network adapters
	pcap_if_t *adapterlist, *adapter;

	int					adapters=0, adapterToUse, i;
	UINT				speed_Mbps = 0;
	unsigned char		mac[6];

	FILE				*inifile;

	char				speed_value[10];
	char				*MAC_value = malloc( 13*sizeof(char) );
	char				*temp2 = malloc( 13*sizeof(char) );
	int					interfaceArg = 0;
	int					MACmode = 0;

	if ( argc>1 && argv[1] )
	{
		interfaceArg = atoi(argv[1])%10;
		MACmode = atoi(argv[1])/10;	//used for set ethernet/WLAN MAC address 
	}
	//printf("interfaceArg=%d, MACmode=%d\n", interfaceArg, MACmode);

	//
	// Obtain the name of the adapters installed on this machine.
	//
	// The result of this function is obtained querying the registry, 
	// and therefore the format of the result in Windows NTx is different
	// from the one in Windows 9x. Windows 9x uses the ASCII encoding method
	// to store a string, while Windows NTx uses UNICODE.
	//
	// In addition different versions of PCAP vary this behaviour meaning
	// that the returned data cannot be assumed to be one format or the
	// other - in other words we must check it ourselves.
	//
	printf("\nAdapters installed:\n");
	
	if(pcap_findalldevs(&adapterlist, err) < 0)
		{
		printf("Error - pcap_findalldevs: %s\n", err);
		return (1);
		}
	for(adapter = adapterlist; adapter; adapter = adapter->next)
		{
		memcpy(AdapterList[adapters], adapter->name, strlen(adapter->name));
		if(get_adapter_speed_and_mac(AdapterList[adapters], &speed_Mbps, mac))
			{
			printf("\n%3d - %s\n", adapters+1, AdapterList[adapters]);
			adapters++;
			}
		else
			{
			printf("\nN/A - %s\n",AdapterList[adapters]);			
			}
		printf("        %s\n", adapter->description);
		}
	printf("\n");

	if (adapters>1)
		{
		if((interfaceArg>adapters)||(interfaceArg<1) )
			{
			do
				{
				printf("Select the number of the adapter to use : ");
				scanf("%d",&adapterToUse);
				if (adapterToUse > adapters  ||  adapterToUse < 1)
					{
					printf("\nThe number must be between 1 and %d\n",adapters);
					}
				} while (adapterToUse > adapters  ||  adapterToUse < 1);
			}
		else
			{
			adapterToUse = interfaceArg;
			}
  		}
	else
		{
		adapterToUse = adapters;
 		}

	MAC_value[0] = '\0';
	temp2[0] = '\0';
	speed_value[0] = '\0';
	
	//
	// Open the adapter if available...
	//
	if (adapterToUse > 0  &&  adapterToUse <= adapters)
	{
		printf("\nUsing adapter %d\n", adapterToUse);

		if (get_adapter_speed_and_mac(AdapterList[adapterToUse-1], &speed_Mbps, mac))
		{
			if (speed_Mbps == 0)
			{
				printf("Could not read Ethernet card's speed\n");
			}

			printf("Physical address read: %02x:%02x:%02x:%02x:%02x:%02x\n",
				   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

			if(MACmode == 0)	//ethernet MAC
				{
				mac[0] += 2; // changing address from global to local
				}
			
			for( i=0; i<6; i++ )
			{
				strcpy( temp2, MAC_value);
				if( mac[i] > 15 )
				{
					// has 2 hex digits
					sprintf( MAC_value, "%s%x", temp2, mac[i]);
				}
				else
				{
					sprintf( MAC_value, "%s0%x", temp2, mac[i]);
				}
			}
		}
		else
		{
			printf("Problem with opening adapter (packet.lib issue)\n");
			return (1);
		}
	}
	else
	{
		printf("\nNo supported adapters found\n");
	}

	//
	// Write the settings to the INI file...
	//
	printf("Writing settings to %s.\n\n", EPOC_INI_FILE);
	
	inifile = fopen(EPOC_INI_FILE, "a"); // to create if does exist
	if ( NULL != inifile )
	{
		fclose(inifile);
	}
	else
	{
		printf("Can't create or access %s.\n\n", EPOC_INI_FILE);
		return (1);
	}
		
	if ( 0 != replace_in_inifile( ETHER_NIF_ENTRY, AdapterList[adapterToUse-1], FALSE ) )
	{
		printf("Couldn't write adapter name to %s file\n", EPOC_INI_FILE);
		return (1);
	}
		

	if ( 0 != replace_in_inifile( ETHER_MAC_ENTRY, MAC_value, FALSE ) )
	{
		printf("Couldn't write MAC address to %s file\n", EPOC_INI_FILE);
		return (1);
	}


	if( 0 != speed_Mbps )
	{
		sprintf( speed_value, "%dMbps", speed_Mbps);
	}	

	if ( 0 != replace_in_inifile( ETHER_SPEED_ENTRY, speed_value, FALSE ) )
	{
		printf("Couldn't write speed value to %s file\n", EPOC_INI_FILE);
		return (1);
	}
	
	//printf("Netcards has written settings to %s.\n\n", EPOC_INI_FILE);
	free(MAC_value);
	free(temp2);
	pcap_freealldevs(adapterlist);

	return (0);
} // main


int get_adapter_speed_and_mac(char* adapterName, UINT* speed, unsigned char* mac)
{
	LPADAPTER	lpAdapter;
	int			retVal = 0;

	//
	// Open the adapter and get the speed and MAC address...
	//
	lpAdapter = PacketOpenAdapter(adapterName);
		
	if (lpAdapter)
	{
		PPACKET_OID_DATA	pOidData = malloc(sizeof(PACKET_OID_DATA) + MAX_OID_DATA);

		//
		// Get the link speed first. We use this method rather than call
		// PacketGetNetType() as it works better with PCAP3.1...
		//
		pOidData->Oid    = OID_GEN_LINK_SPEED;
		pOidData->Length = MAX_OID_DATA;

		if (PacketRequest(lpAdapter, FALSE , pOidData))
		{
			*speed = *((UINT*)pOidData->Data) / 10000; // OID is in 100 bps units.
		}
		else
		{
			*speed = 0;
		}

		//
		// Check the adapter is 802.3 based, cable connected and has a MAC address.
		//
		pOidData->Oid    = OID_GEN_MEDIA_IN_USE;
		pOidData->Length = MAX_OID_DATA;

		if (PacketRequest(lpAdapter, FALSE , pOidData)  &&
			*((UINT*)pOidData->Data) == NdisMedium802_3)
		{
			pOidData->Oid    = OID_GEN_MEDIA_CONNECT_STATUS;
			pOidData->Length = MAX_OID_DATA;

			if (PacketRequest(lpAdapter, FALSE , pOidData)  &&
				*((UINT*)pOidData->Data) == NdisMediaStateConnected)
			{
				pOidData->Oid    = OID_802_3_CURRENT_ADDRESS;
				pOidData->Length = MAX_OID_DATA;
			
				if (PacketRequest(lpAdapter, FALSE , pOidData))
				{
	  				mac[0] = pOidData->Data[0];
	  				mac[1] = pOidData->Data[1];
	  				mac[2] = pOidData->Data[2];
	  				mac[3] = pOidData->Data[3];
	  				mac[4] = pOidData->Data[4];
	  				mac[5] = pOidData->Data[5];

					retVal = 1;
				}
			}
		}

		free(pOidData);
	
		PacketCloseAdapter(lpAdapter);
	}

	return retVal;
} // get_adapter_speed_and_mac


int replace_in_inifile(char * entry_str, char* value, BOOL valUnicode)
{
	int err = 0; // 0 - ok, negative sth wrong

	int replaced = 0;
	int len = strlen(entry_str);

	FILE *	file;
	FILE *	tmp_file;

	char*  s = malloc(MAX_LINE);
	char *line = malloc(MAX_LINE);

	if ( NULL == (tmp_file = fopen(EPOC_INI_FILE_TEMP, "w")) ) 
	{
		printf( "Could not create '%s'\n", EPOC_INI_FILE_TEMP );
		return -1;
	}

	if ( NULL == (file  = fopen(EPOC_INI_FILE, "r+")) )
	{
		fclose( tmp_file );
		remove( EPOC_INI_FILE_TEMP  );
		printf( "Could not open '%s'\n", EPOC_INI_FILE );
		return -1;
	}

	rewind(file);
	
	
	while( fgets(line, MAX_LINE, file) != NULL)
    {
		if (sscanf( line, "%s", s ) > 0) // to trim blank chars
		{
			s[len] = '\0';
			if( 0 == strcmp(entry_str, s))
			{
				fprintf(tmp_file, "%s=", entry_str);
				
				if( valUnicode )
				{
					fwprintf(tmp_file, L"%s\n", value);
				}
				else
				{
					fprintf(tmp_file, "%s\n", value);
				}
				
				replaced = 1;
			}
			else
			{
				if( EOF == fputs(line, tmp_file) )
				{
					err = -1;
					break;
				}
			}
		}
	}
        
	free(line);
	free(s); 

	if( (0 == replaced) && (0 == err) )
	{
		// no entry encountered - add new
		if( 0 != fseek( tmp_file, 0, SEEK_END ) )
		{
			err = -1;
		}
	
		fprintf( tmp_file, "\n%s=", entry_str);
		if ( valUnicode )
		{
			fwprintf( tmp_file, L"%s\n", value);
		}
		else
		{
			fprintf( tmp_file, "%s\n", value);
		}
	}


	if ( 0 != fclose(file ) )
	{
		printf( "Could not close %s file\n", EPOC_INI_FILE );
		return -1;
	}

	if ( 0 != fclose( tmp_file ) )
	{
		printf( "Could not close %s file\n", EPOC_INI_FILE_TEMP );
		return -1;
	}


	if( remove( EPOC_INI_FILE  ) == -1 )
	{
		printf( "Could not overwrite %s file\n", EPOC_INI_FILE );
		return -1;
	}
	
	if( rename( EPOC_INI_FILE_TEMP, EPOC_INI_FILE ) != 0 )
	{
		printf( "\nCould not rename '%s' to '%s'\n", EPOC_INI_FILE_TEMP, EPOC_INI_FILE );
		return -1;
	}	
		
	return 0;
} // replace_in_inifile
