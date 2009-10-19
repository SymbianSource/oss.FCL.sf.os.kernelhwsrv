# Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# Called from MNT.BAT to do some PVCS stuff.
# 
#

use strict;

if($#ARGV !=1) {
  print <<EOH;
Usage: dopvcs.pl <rune1> <rune2>

Basically, only call this from mnt.bat

EOH
exit 1;	  
}

  
open(X, "/f32test/group/dir.prj") || die "Can't find the directory list, $!";

#Slurp
my @dirs=<X>;

close X;

# Zap comments
foreach (@dirs) {
  if(/!/) {
	s/^(.*)!.*/$1/;
  }
}

chomp @dirs;

# Now the weird runes

foreach (grep /\w/, @dirs) {
  system("t:\\pvcs\\$ARGV[0] F32TEST $_ LI $ARGV[1]");
}
