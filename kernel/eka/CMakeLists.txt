# Copyright (c) 2009-2010 Stéphane Lenclud.
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of the License "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Stéphane Lenclud.
#

project (eka)

cmake_minimum_required(VERSION 2.6)


#Generate the exec enum using CMAKE custom command
#perl genexec.pl -i execs.txt -e ../include/exec_enum.h -u ../include/exec_user.h -k ../include/exec_kernel.h
add_custom_command (
	OUTPUT ./include/exec_enum.h ./include/exec_user.h ./include/kernel/exec_kernel.h
   	COMMAND perl ./kernel/genexec.pl -i ./kernel/execs.txt -e ./include/exec_enum.h -u ./include/exec_user.h -k ./include/kernel/exec_kernel.h
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}	
   	COMMENT "Generating exec headers..."
   	VERBATIM
 	)
	
#Tell cmake those file won't be available until build time
set_source_files_properties(./include/exec_enum.h PROPERTIES GENERATED 1)
set_source_files_properties(./include/exec_user.h PROPERTIES GENERATED 1)
set_source_files_properties(./include/kernel/exec_kernel.h PROPERTIES GENERATED 1)

#Custom target that will generate our exec headers 
add_custom_target(genexec DEPENDS ./include/exec_enum.h ./include/exec_user.h ./include/kernel/exec_kernel.h)

add_subdirectory(./euser)
add_subdirectory(./kernel)




