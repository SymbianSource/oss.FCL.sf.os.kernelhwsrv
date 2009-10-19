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
// f32test\plugins\version_2\crypto_encryption\src\encryptionmanager.cpp
// user functionality exposed
// includes
// 
//

#include "encryption_plugin.h"
#include "encryptionmanager.h"
#include <e32test.h>
//plug-in takes care of addition and removal of entries from the file


/**
Adds the file to the database
@param aFileName Name of the file to be added
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
 */
inline TInt EncryptionManager::AddFile(TDesC& aFileName)
	{
	TFileName8 filename;
	filename.Copy(aFileName);
	return (iPlugin.AddFile(filename));
	}
/**
Removes the specified file from the database
@param aFileName Name of the file to be removed
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
 */
inline TInt EncryptionManager::RemoveFile(TDesC& aFileName, TInt aDecrypt)
	{
	TFileName8 filename;
	filename.Copy(aFileName);
	return(iPlugin.RemoveFile(filename,aDecrypt));
	}

/**
Displays the encryption manager menu and asks the user for his response
@param aKeyCode The keycode pressed by the user which is to be returned to the caller
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
 */
TInt EncryptionManager::DisplayMenuAndGetChoice(TInt& aKeyCode)
	{
	TBuf<0x100> choice;
	switch (iState)
		{
	case EPluginNotLoaded:
		{
		FOREVER
			{
			choice.Zero();
			iConsole->ClearScreen();
			iConsole->Printf(_L("(L)oad Plug-in \n (Q)uit\nEnterChoice:"));

			ReadFromConsole(choice);
			if (choice.Length()==0)
				continue;
			aKeyCode=choice[0];
			if ((aKeyCode!='L')&&(aKeyCode!='l')&&(aKeyCode!='q')&&(aKeyCode!='Q'))
				{
				iConsole->Printf(_L("Wrong choice\n"));
				
				iConsole->Getch();
				}
			else
				break;
			}	
		break;
		}
	case EPluginLoaded:
		{
		FOREVER
			{
			choice.Zero();
			iConsole->ClearScreen();
			iConsole->Printf(_L("(U)nload Plug-in \n (E)nable Plug-in (Q)uit\nEnterChoice:"));
			ReadFromConsole(choice);
			if (choice.Length()==0)
				continue;
			aKeyCode=choice[0];
			if ((aKeyCode!='U')&&(aKeyCode!='u')&&(aKeyCode!='q')&&(aKeyCode!='Q')&&(aKeyCode!='E')&&(aKeyCode!='e'))
				{
				iConsole->Printf(_L("Wrong choice\n"));
				iConsole->Getch();
				}
			else
				break;
			}	
		break;
		}
	case EEncryptionEnabled:
		{
		FOREVER
			{
			choice.Zero();
			iConsole->ClearScreen();
			iConsole->Printf(_L("(P)rint List of encrypted files (A)dd File\n (R)emove File (D)isable (Q)uit\nEnterChoice:"));
			ReadFromConsole(choice);
			if (choice.Length()==0)
				continue;
			aKeyCode=choice[0];
			if ((aKeyCode!='A')&&(aKeyCode!='a')&&(aKeyCode!='R')&&(aKeyCode!='r')&&(aKeyCode!='q')&&(aKeyCode!='Q')&&(aKeyCode!='D')&&(aKeyCode!='d')&&(aKeyCode!='P')&&(aKeyCode!='p'))
				{
				iConsole->Printf(_L("Wrong choice\n"));
				iConsole->Getch();
				}
			else
				break;
			}	
		break;
		}
	default:
		return KErrGeneral;
		}
	return KErrNone;
	}

/**
Enables encryption
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
inline TInt EncryptionManager::EnableEncryption()
	{
	TInt ret=iPlugin.EnableEncryption();
	if (ret!=KErrNone)
		return ret;
	iState=EEncryptionEnabled;
	return ret;
	}

/**
Disables encryption
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
inline TInt EncryptionManager::DisableEncryption()
	{
	TInt ret=iPlugin.DisableEncryption();
	if (ret!=KErrNone)
		return ret;	
	iState=EPluginLoaded;
	return ret;
	}

/**
Prints the list of encrypted files as stored in the database
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
TInt EncryptionManager::PrintListOfEncFiles()
	{
	TFileName8 filename8;
	TFileName filename;
	iDatabase.Reset();
	iConsole->Printf(_L("\nList of encrypted files\n==========================\n"));
	for (TInt i=0;;i++)
		{
		TInt ret=iPlugin.GetDatabaseEntry(i,filename8);
		if (ret!=KErrNone)
			break;
		iDatabase.Append(filename8);
		filename.Copy(filename8);
		iConsole->Printf(_L("%d. %S\n"),i+1,&filename);
		}
	return KErrNone;
	}

/**
Initialises/gets the current state of the system and connects to the file server
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
TInt EncryptionManager::Init()
	{
	TInt ret=iFs.Connect();
	if (ret!=KErrNone)
		return ret;
	//get the current state and then display the menu
	iState=EPluginNotLoaded;
	if (IsPluginMounted())
		{
		iState=EPluginLoaded;	
		if (IsEncryptionEnabled())
			iState=EEncryptionEnabled;
		}
	iConsole=Console::NewL(_L("Encryption ManagerApp"), TSize(KConsFullScreen, KConsFullScreen));
	return KErrNone;
	}

/**
The main loop of the application when no command line arguments are specified
*/
void EncryptionManager::StateMachine()
	{

	TInt keycode;
	//this is main loop area of the application
	do
		{
		TInt ret=KErrNone;
		ret=DisplayMenuAndGetChoice(keycode);
		if (ret!=KErrNone)
			User::Panic(_L("Illegal state\n"),0);	
		switch (keycode)
			{
		case 'L':
		case 'l':
			ret=LoadPlugin();
			if (ret!=KErrNone)
				{
				iConsole->Printf(_L("\nLoading failed\n"));
				iConsole->Getch();
				}
			break;
		case 'U':
		case 'u':
			ret=UnloadPlugin();
			if (ret!=KErrNone)
				{
				iConsole->Printf(_L("\nUnLoading failed\n"));
				iConsole->Getch();
				}
			break;
		case 'E':
		case 'e':
			ret=EnableEncryption();
			if (ret!=KErrNone)
				{
				iConsole->Printf(_L("\nEnabled encryption failed\n"));
				iConsole->Getch();
				}
			break;
		case 'P':
		case 'p':
			ret=PrintListOfEncFiles();
			if (ret!=KErrNone)
				{
				iConsole->Printf(_L("\n Error in retrieving the list\n"));
				}
			iConsole->Getch();
			break;
		case 'A':
		case 'a':
			{
			TFileName filename;
			iConsole->Printf(_L("\nEnter filename with entire path\n:"));
			if (ReadFromConsole(filename))
				{
				ret=AddFile(filename);
				if (ret!=KErrNone)
					{
					iConsole->Printf(_L("\n Adding file failed\n"));
					iConsole->Getch();
					}
			}
			else 
				continue;
			break;
			}
		case 'R':
		case 'r':
			{
			iConsole->Printf(_L("\nRemove file\n=====\n"));
			TFileName filename;
			PrintListOfEncFiles();
			if (ReadFromConsole(filename))
				{
				iConsole->Printf(_L("\nDo you want to decrypt this file?\n  :"));
				keycode=iConsole->Getch();
				if (keycode=='y'||keycode=='Y')
					{
					ret=RemoveFile(filename,1);
					if (ret!=KErrNone)
						{
						iConsole->Printf(_L("\nRemoving file failed\n"));
						iConsole->Getch();
						}
					}
				else if (keycode=='n'||keycode=='N')
					{
					ret=RemoveFile(filename,0);
					if (ret!=KErrNone)
						{
						iConsole->Printf(_L("\nRemoving file failed\n"));
						iConsole->Getch();
						}
					}
				else 
					{
					iConsole->Printf(_L("\nWrong choice\n"));
					iConsole->Getch();
					continue;
					}
					
				}
			else 
				continue;
			break;
			}
		case 'D':
		case 'd':
			ret=DisableEncryption();
			if (ret!=KErrNone)
				{
				iConsole->Printf(_L("\nDisable encryption failed\n"));
				iConsole->Getch();
				}
			break;
		case 'Q':
		case 'q':
			if (iState!=EPluginNotLoaded)
				iPlugin.Commit();
			return;
		default:
			continue;
			};
		}
	while(keycode!='q');
	}

/**
Closes the RPlugin and RFs instances
 */
EncryptionManager::~EncryptionManager()
	{
	iPlugin.Close();
	iFs.Close();
	delete iConsole;
	}

/**
Checks whether the plug-in is mounted
@return ETrue, if plug-in is mounted
		else, EFalse
 */
inline TBool EncryptionManager::IsPluginMounted()
	{
	TInt ret=iPlugin.Open(iFs,KEncryptionPos);
	if ((ret==KErrNone)||(ret==KErrAlreadyExists))
		return ETrue;
	else 
		return EFalse;
	}

/**
Checks whether encryption is enabled 
@return ETrue, if encryption is enabled
       	else, EFalse
*/
inline TBool EncryptionManager::IsEncryptionEnabled()
	{
	return (iPlugin.IsEncryptionEnabled());
	}

/**
Loads and mounts the encryption plug-in.
Opens a channel to the plug-in 
and populates the database array in RAM from the file
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
TInt EncryptionManager::LoadPlugin()
	{
	TInt r=KErrNone;
	r = iFs.AddPlugin(KEncryptionPluginFileName);
	if (r == KErrAlreadyExists) 
		r = KErrNone;
	if (r!=KErrNone)
		return r;
	r = iFs.MountPlugin(KEncryptionPluginName);
	if (r == KErrInUse)
		r = KErrNone;
	if (r!=KErrNone)
		return r;
	r=iPlugin.Open(iFs,KEncryptionPos);
	if (r!=KErrNone)
		return r;
	//open a channel to the plug-in
	iState=EPluginLoaded; 
	r=iPlugin.PopulateDatabase();
	if (r!=KErrNone)
		return r;
	
	return KErrNone;
	}

/**
Commits the RAM database entries to the disk.Closes the channel to the plug-in.
Unloads and unmounts the encryption plug-in.
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
TInt EncryptionManager::UnloadPlugin()
	{
	TInt ret=KErrNone;
	ret=iPlugin.Commit();
	if (ret!=KErrNone)
		return ret;
	iPlugin.Close();
	ret=iFs.DismountPlugin(KEncryptionPluginName);
	if (ret!=KErrNone)
		return ret;
	ret=iFs.RemovePlugin(KEncryptionPluginName);
	iState=EPluginNotLoaded;
	
	return ret;
	}
GLDEF_C TInt E32Main()
    	{
   
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();
	EncryptionManager encman;
	encman.Init();
	TInt ret;
	ret=encman.ParseCmdLineArgs();
	if (ret==KNoCmdLineArgs)//no command line arguments
		encman.StateMachine();
	else 
		{
		if (ret!=KErrNone)
			{
			User::Panic(_L("Illegal Args!"),0);
			}
		}
	delete cleanup;
	return KErrNone;
	}

/**
Parses the command line arguments and performs the defined actions
@reurn KErrNone, if successful
	KNoCmdLineArgs, if no command line arguments are specified
	KErrArgument, if illegal arguments are specified
	otherwise, one of the other system-wide error 
        codes. 
*/
TInt EncryptionManager::ParseCmdLineArgs()
	{
	TInt ret=KErrNone;
	User::CommandLine(iCmd);
	TLex lex(iCmd);
	TBuf<0x100> token;

	token=lex.NextToken();
	
	if (token.Length()==0)
		return KNoCmdLineArgs;//no arguments=> menu driven implementation
	token.LowerCase();
	TInt choice=token[0];
	
	while (token.Length()!=0)
		{
		choice=token[0];
		switch (iState)
			{
		case EPluginNotLoaded:
			{
			if (choice=='l')
				{
				ret=LoadPlugin();
				if (ret!=KErrNone)
					return ret;
				}
			else if (choice=='q')
				{
				return KErrNone;
				}
			else 
				return KErrArgument;
			break;
			}
			
		case EPluginLoaded:
			{
			if (choice=='u')
				{
				//unload plug-in
				ret=UnloadPlugin();
				if (ret!=KErrNone)
					return ret;
				}
			else if (choice=='e')
				{
				//enable encryption
				ret=EnableEncryption();
				if (ret!=KErrNone)
					return ret;
				}
			else if (choice=='q')
				return KErrNone;
			else
				return KErrArgument;
			break;
			}
		case EEncryptionEnabled:
			{
			if (choice=='d')
				{
				ret=DisableEncryption();
				if (ret!=KErrNone)
					return ret;
				}
			else if (choice=='a')
				{
				//add file
				token=lex.NextToken();//the file name
				if (token.Length()==0)
					return KErrArgument;
			
				ret=AddFile(token);
				if (ret!=KErrNone)
					return ret;
				}
			else if (choice=='r')
				{
				//remove file
				token=lex.NextToken();//Y/N =>decrypt or not
				if (token.Length()==0)
					return KErrArgument;
			
				TInt decrypt=0;//0=>N 1=> Y
				if ((token[0]=='y')||(token[0]=='Y'))
					decrypt=1;
				else if ((token[0]=='n')||(token[0]=='N'))
					decrypt=0;
				else 
					return KErrArgument;
				token=lex.NextToken();//file name
				if (token.Length()==0)
					return KErrArgument;
		
				ret=RemoveFile(token,decrypt);
				if (ret!=KErrNone)
					return ret;
				}
			else if (choice=='q')
				{
				return KErrNone;
				}
			else 
				{
				return KErrArgument;
				}
			break;
			}	
		default:
			return KErrGeneral;
			}

		token=lex.NextToken();
		token.LowerCase();
		}
	return ret;
	}

/**
Checks for valid key input.
@param	aKey keycode 
@return 	ETrue if key input is a displayable character
			EFalse if key input is not a displayable character.
*/
static TBool IsCharacterKey(TKeyCode aKey)
	{
	if(aKey >= EKeySpace&& aKey < EKeyDelete)
		{
		return ETrue;
		}
	return EFalse;
	}

/**
Gets the string input from the console.
@param 	aString String to be read into from console 
@return 	ETrue if the enter key is pressed or buffer length is exceeded
		EFalse if the escape key has been pressed
*/
TBool EncryptionManager::ReadFromConsole(TDes& aString)
	{
	// This infinte loop terminates when user hits an "enter" key
	// Or the maximum size of the buffer is hit
	FOREVER
		{
		// Get a key(character) from the console
		TKeyCode keycode = iConsole->Getch();  
		switch(keycode)
			{
		case EKeyEnter:
			return ETrue;

		case EKeyBackspace:		
			if(0 != aString.Length())
				{
				// Back-space only takes the cursor one position back
				// So to delete a character blank-space is inserted at
				// that position and later cursor is again adjusted.
				iConsole->Printf(_L("%c%c%c"), EKeyBackspace, EKeySpace,EKeyBackspace);
				// Delete the character from the target string also. 
				aString.Delete(aString.Length() - 1, 1);
				}
			break;
			
		case EKeyEscape:	
			return EFalse;

		default:
			// IsCharacterKey will return true if keycode is a displayable
			// character else it will return false.
			if(IsCharacterKey(keycode))
				{
				TInt maxBufferLength = aString.MaxLength();
				if(aString.Length() == maxBufferLength)
					{
					continue;
					}
				iConsole->Printf(_L("%c"), keycode);
	            		aString.Append(keycode);
				}
			else 
				iConsole->Printf(_L("garbage"));
			}
		}
	}
