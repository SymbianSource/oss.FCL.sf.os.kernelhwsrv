#ifndef __TESTLITERALS_H
#define __TESTLITERALS_H

/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* @file testliterals.h
* @internalComponent
* 
*
*/



namespace NUnitTesting_USBDI
	{
// Warning Debug Print

#define __PRINT_CONTROL_TRANSFER_TIMER_COMPARISON_WARNING \
RDebug::Printf("\n\n"); \
RDebug::Printf("**********************************FAILURE WARNING**************************************"); \
RDebug::Printf("The time taken to complete a control transfer with a concurrent bulk transfer"); \
RDebug::Printf("is significantly longer than"); \
RDebug::Printf("the time taken to complete a control transfer without a concurrent bulk transfer."); \
RDebug::Printf("This is likely to be the fault of the USB client stack on the remote."); \
RDebug::Printf("Use a tracer tool (e.g. Ellisys) to check that the time difference is all found in"); \
RDebug::Printf("the time taken between the control transfer's SetUp transaction and its IN transaction."); \
RDebug::Printf("This test will now FAIL to alert you to this WARNING!"); \
RDebug::Printf("**********************************FAILURE WARNING**************************************"); \
RDebug::Printf("\n\n");


// French Literals - various lengths
	
_LIT8(KLiteralFrench4,
"\
1: \
Elle etait dechaussee, elle etait decoiffee, \
Assise, les pieds nus, parmi les joncs penchants; \
Moi qui passais par la, je crus voir une fee \
Et je lui dis: Veux-tu t'en venir dans les champs? \
2: \
Elle me regarda de ce regard supreme \
Qui reste a la beaute quand nous en triomphons, \
Et je lui dis: Veux-tu, c'est le mois ou l'on aime, \
Veux-tu nous en aller sou les arbres profonds? \
3: \
Elle essuya ses pieds a l'herbe de la rive; \
Elle me regarda pour la seconde fois, \
Et ta belle folatre alors devint pensive. \
Oh! Comme les oiseaux chantaient au fond du bois! \
4: \
Comme l'eau caressait doucement le rivage! \
Je vis venir a moi, dans les grands roseaux verts, \
La belle fille heureuse, effaree et sauvage, \
Ses cheveux dans les yeux, riant au travers. \
"); // 1446 bytes


_LIT8(KLiteralFrench8,
"\
1: \
Elle etait dechaussee, elle etait decoiffee, \
Assise, les pieds nus, parmi les joncs penchants; \
Moi qui passais par la, je crus voir une fee \
Et je lui dis: Veux-tu t'en venir dans les champs? \
2: \
Elle me regarda de ce regard supreme \
Qui reste a la beaute quand nous en triomphons, \
Et je lui dis: Veux-tu, c'est le mois ou l'on aime, \
Veux-tu nous en aller sou les arbres profonds? \
3: \
Elle essuya ses pieds a l'herbe de la rive; \
Elle me regarda pour la seconde fois, \
Et ta belle folatre alors devint pensive. \
Oh! Comme les oiseaux chantaient au fond du bois! \
4: \
Comme l'eau caressait doucement le rivage! \
Je vis venir a moi, dans les grands roseaux verts, \
La belle fille heureuse, effaree et sauvage, \
Ses cheveux dans les yeux, riant au travers. \
Je repete... \
1: \
Elle etait dechaussee, elle etait decoiffee, \
Assise, les pieds nus, parmi les joncs penchants; \
Moi qui passais par la, je crus voir une fee \
Et je lui dis: Veux-tu t'en venir dans les champs? \
2: \
Elle me regarda de ce regard supreme \
Qui reste a la beaute quand nous en triomphons, \
Et je lui dis: Veux-tu, c'est le mois ou l'on aime, \
Veux-tu nous en aller sou les arbres profonds? \
3: \
Elle essuya ses pieds a l'herbe de la rive; \
Elle me regarda pour la seconde fois, \
Et ta belle folatre alors devint pensive. \
Oh! Comme les oiseaux chantaient au fond du bois! \
4: \
Comme l'eau caressait doucement le rivage! \
Je vis venir a moi, dans les grands roseaux verts, \
La belle fille heureuse, effaree et sauvage, \
Ses cheveux dans les yeux, riant au travers. \
"); // 1519 bytes



// English Literals - various lengths

_LIT8(KLiteralEnglish2,
"\
1: \
\"You are old, father William,\" the young man said, \
\"And your hair has become very white; \
And yet you incessantly stand on your head-- \
Do you think, at your age, it is right?\" \
2: \
\"In my youth,\" father William replied to his son, \
\"I feared it might injure the brain; \
But, now that I'm perfectly sure I have none, \
Why, I do it again and again.\"\
"); //  344 bytes


_LIT8(KLiteralEnglish5,
"\
1: \
\"You are old, father William,\" the young man said, \
\"And your hair has become very white; \
And yet you incessantly stand on your head-- \
Do you think, at your age, it is right?\" \
2: \
\"In my youth,\" father William replied to his son, \
\"I feared it might injure the brain; \
But, now that I'm perfectly sure I have none, \
Why, I do it again and again.\" \
3: \
\"You are old,\" said the youth, \"as I mentioned before, \
And you have grown most uncommonly fat; \
Yet you turned a back-somersault in at the door-- \
Pray what is the reason for that?\" \
4: \
\"In my youth,\" said the sage, as he shook his grey locks, \
\"I kept all my limbs very supple \
By the use of this ointment - one shilling a box-- \
Allow me to sell you a couple?\" \
5: \
\"You are old,\" said the youth, \"and your jaws are too weak \
For anything tougher than suet; \
Yet you finished the goose, with the bones and the beak-- \
Pray, how did you manage to do it?\" \
"); //893 bytes


_LIT8(KLiteralEnglish8,
"\
1: \
\"You are old, father William,\" the young man said, \
\"And your hair has become very white; \
And yet you incessantly stand on your head-- \
Do you think, at your age, it is right?\" \
2: \
\"In my youth,\" father William replied to his son, \
\"I feared it might injure the brain; \
But, now that I'm perfectly sure I have none, \
Why, I do it again and again.\" \
3: \
\"You are old,\" said the youth, \"as I mentioned before, \
And you have grown most uncommonly fat; \
Yet you turned a back-somersault in at the door-- \
Pray what is the reason for that?\" \
4: \
\"In my youth,\" said the sage, as he shook his grey locks, \
\"I kept all my limbs very supple \
By the use of this ointment - one shilling a box-- \
Allow me to sell you a couple?\" \
5: \
\"You are old,\" said the youth, \"and your jaws are too weak \
For anything tougher than suet; \
Yet you finished the goose, with the bones and the beak-- \
Pray, how did you manage to do it?\" \
6: \
\"In my youth,\" said his father, \"I took to the law, \
And argued each case with my wife; \
And the muscular strength, which it gave to my jaw, \
Has lasted the rest of my life.\" \
7: \
\"You are old,\" said the youth, \"one would hardly suppose \
That your eye was as steady as ever;\
Yet you balanced an eel on the end of your nose-- \
What made you so awfully clever?\" \
8: \
\"I have answered three questions, and that is enough,\" \
Said his father. \"Don't give yourself airs! \
Do you think I can listen all day to such stuff? \
Be off, or I'll kick you down stairs.\" \
"); // 1438 bytes

_LIT8(KLiteralEnglish16,
"\
1: \
\"You are old, father William,\" the young man said, \
\"And your hair has become very white; \
And yet you incessantly stand on your head-- \
Do you think, at your age, it is right?\" \
2: \
\"In my youth,\" father William replied to his son, \
\"I feared it might injure the brain; \
But, now that I'm perfectly sure I have none, \
Why, I do it again and again.\" \
3: \
\"You are old,\" said the youth, \"as I mentioned before, \
And you have grown most uncommonly fat; \
Yet you turned a back-somersault in at the door-- \
Pray what is the reason for that?\" \
4: \
\"In my youth,\" said the sage, as he shook his grey locks, \
\"I kept all my limbs very supple \
By the use of this ointment - one shilling a box-- \
Allow me to sell you a couple?\" \
5: \
\"You are old,\" said the youth, \"and your jaws are too weak \
For anything tougher than suet; \
Yet you finished the goose, with the bones and the beak-- \
Pray, how did you manage to do it?\" \
6: \
\"In my youth,\" said his father, \"I took to the law, \
And argued each case with my wife; \
And the muscular strength, which it gave to my jaw, \
Has lasted the rest of my life.\" \
7: \
\"You are old,\" said the youth, \"one would hardly suppose \
That your eye was as steady as ever;\
Yet you balanced an eel on the end of your nose-- \
What made you so awfully clever?\" \
8: \
\"I have answered three questions, and that is enough,\" \
Said his father. \"Don't give yourself airs! \
Do you think I can listen all day to such stuff? \
Be off, or I'll kick you down stairs.\" \
From 9 to 16, just simply repeats 1 to 8:\
9: \
\"You are old, father William,\" the young man said, \
\"And your hair has become very white; \
And yet you incessantly stand on your head-- \
Do you think, at your age, it is right?\" \
10: \
\"In my youth,\" father William replied to his son, \
\"I feared it might injure the brain; \
But, now that I'm perfectly sure I have none, \
Why, I do it again and again.\" \
11: \
\"You are old,\" said the youth, \"as I mentioned before, \
And you have grown most uncommonly fat; \
Yet you turned a back-somersault in at the door-- \
Pray what is the reason for that?\" \
12: \
\"In my youth,\" said the sage, as he shook his grey locks, \
\"I kept all my limbs very supple \
By the use of this ointment - one shilling a box-- \
Allow me to sell you a couple?\" \
13: \
\"You are old,\" said the youth, \"and your jaws are too weak \
For anything tougher than suet; \
Yet you finished the goose, with the bones and the beak-- \
Pray, how did you manage to do it?\" \
14: \
\"In my youth,\" said his father, \"I took to the law, \
And argued each case with my wife; \
And the muscular strength, which it gave to my jaw, \
Has lasted the rest of my life.\" \
15: \
\"You are old,\" said the youth, \"one would hardly suppose \
That your eye was as steady as ever;\
Yet you balanced an eel on the end of your nose-- \
What made you so awfully clever?\" \
16: \
\"I have answered three questions, and that is enough,\" \
Said his father. \"Don't give yourself airs! \
Do you think I can listen all day to such stuff? \
Be off, or I'll kick you down stairs.\" \
");
	}

#endif