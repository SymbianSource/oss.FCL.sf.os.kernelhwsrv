#
# Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#
#

use strict;

chdir "scripts" or die "$!";

print "======================================\n";
print "USBTESTLOOP starts\n";
print localtime()."\n";
print "======================================\n";

unless (-e "vascotests.bat")
	{
	print "Cannot find vascotests.bat\n";
	exit 0;
	}

while (1)
	{
	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);

		print localtime().": Start vascotests.bat\n";
		system "vascotests.bat";
		print localtime().": Execution of vascotests.bat complete\n";
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
	my $prefix = sprintf("%04d%02d%02d%02d%02d%02d", $year+1900, $mon+1, $mday, $hour, $min, $sec);
	system "COPY /Y ..\\Results\\vascotests.log ..\\${prefix}_vascotests.log";
	system "COPY /Y ..\\Results\\vascoperformance.log ..\\${prefix}_vascoperformance.log";
	}
