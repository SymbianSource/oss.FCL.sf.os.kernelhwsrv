#!/usr/bin/perl -w
# Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of the License "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
#
# Description:
# USBinterop2
# 
#
# AUTHOR: Steve Elliott
# DESCRIPTION: This program is only for testing a perl script 
# 
#
# @ 2008
use strict;

# we're starting
print "* Perl Test Script\n";


my $i = 0;
for (my $i = 0; $i < 18; $i++)
{
	sleep 1;
	print ".";
}

# we're done, exit
print "\n* Done\n";


