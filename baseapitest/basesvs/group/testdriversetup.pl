#
# Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of "Eclipse Public License v1.0"
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
use Cwd;

my $theEpocRoot=$ENV{EPOCROOT};
my $epoc32Location="$theEpocRoot.\\epoc32";

my $currentDirectory=cwd;
$currentDirectory =~ s/Group//i;
$currentDirectory =~ s/\//\\/g;

my $currentDrive = substr($currentDirectory,0,2);

my	$cmd="TestDriver config";
my	$suite="file:/$currentDrive$epoc32Location\\testdriver\\testproduct\\base.driver#base";
$suite =~ s.\\./.g;
$suite =~ s\/./\/\g;

$cmd .= " --bldclean OFF";
$cmd .= " --bldmake OFF";
$cmd .= " -e $currentDrive$theEpocRoot";
$cmd .= " -x $currentDrive$epoc32Location\\testdriver\\testproduct";
$cmd .= " --repos $currentDrive$epoc32Location\\testdriver\\Repository";
$cmd .= " -c $currentDrive$epoc32Location\\testdriver\\Results";
$cmd .= " -i $currentDirectory";
$cmd .= " -s $suite";
$cmd .= " --source $currentDirectory";
$cmd .= " --tp $currentDirectory";
$cmd .= " --platsec  ON";
$cmd .= " --testexec ON";
$cmd .= " --commdb overwrite";
$cmd .= " --testtimelimit 600000";
system("$cmd");
