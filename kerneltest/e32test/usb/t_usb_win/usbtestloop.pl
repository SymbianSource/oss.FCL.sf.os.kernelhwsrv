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

# unless (-e "h4testsusbcv.bat")
# 	{
# 	print "Cannot find h4testsusbcv.bat\n";
# 	exit 0;
# 	}
unless (-e "h4tests.bat")
	{
	print "Cannot find h4tests.bat\n";
	exit 0;
	}

while (1)
	{
	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
# Sod USBCV for the time being
#	if ($wday == 5) # It's Friday !
#		{
#		print localtime().": Start h4testsusbcv.bat\n";
#		system "h4testsusbcv.bat";
#		print localtime().": Execution of h4testsusbcv.bat complete\n";
#		}
#	else
#		{
		print localtime().": Start h4tests.bat\n";
		system "h4tests.bat";
		print localtime().": Execution of h4tests.bat complete\n";
#		}
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
	my $prefix = sprintf("%04d%02d%02d%02d%02d%02d", $year+1900, $mon+1, $mday, $hour, $min, $sec);
	system "COPY /Y ..\\Results\\h4tests.log ..\\${prefix}_h4tests.log";
	system "COPY /Y ..\\Results\\h4performance.log ..\\${prefix}_h4performance.log";
	}
