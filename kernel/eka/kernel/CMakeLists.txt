# Copyright (c) 2009 Stéphane Lenclud.
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of the License "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Stéphane Lenclud.
#

project (kernel)

cmake_minimum_required(VERSION 2.6)

#Add the given source files to our source prepending the sourcepath
macro(add_source)
	foreach(mySource ${ARGV})
		string(REGEX REPLACE "(^.+)" "${sourcepath}\\1" newsource ${mySource})
		set(source ${source} ${newsource})
	endforeach(mySource) 
endmacro(add_source) 

add_definitions(-DUNICODE)
add_definitions(-D__VC32__)
add_definitions(-D_UNICODE)
add_definitions(-D__LEAVE_EQUALS_THROW__)
add_definitions(-D__WINS__)
add_definitions(-D__EXPORT_MEMCPY__)				
add_definitions(-D__IN_KERNEL__)
add_definitions(-D__KERNEL_MODE__)

#__CPU_X86 is declared by __WINS__ 
#add_definitions(-D__CPU_X86)
#add_definitions(-D__DLL__)

include_directories (../euser ../euser/unicode ../include 
../include/nkern ../include/nkern/win32 ../include/kernel ../include/kernel/win32 ../include/memmodel/emul/win32 ../include/memmodel/emul)

#Define source for ekern
set (sourcepath ../nkern/)
add_source(
nkern.cpp nkerns.cpp sched.cpp dfcs.cpp nk_timer.cpp
nk_old.cpp
)

set (sourcepath ../common/win32/)
add_source(
atomics.cpp
)

set (sourcepath ../nkern/win32/)
add_source(
vectors.cpp ncsched.cpp nctimer.cpp ncutilf.cpp ncutils.cpp
ncthrd.cpp ncglob.cpp
)

set (sourcepath ../common/win32/)
add_source(
seh.cpp
)

set (sourcepath ../kernel/win32/)
add_source(
cboot.cpp
cglobals.cpp cexec.cpp cinit.cpp
ckernel.cpp csched.cpp
cutils.cpp cache.cpp
)
#k_entry.cpp


set (sourcepath ../kernel/)
add_source(
sglobals.cpp device.cpp eventq.cpp
sexec.cpp sinit.cpp skernel.cpp
scodeseg.cpp sipc.cpp notifier.cpp 
object.cpp power.cpp server.cpp
stimer.cpp sutils.cpp random.cpp
sprocess.cpp sthread.cpp 
smqueue.cpp
sproperty.cpp
ssharedio.cpp
ssecure.cpp
emi.cpp
objectix.cpp
sbtrace.cpp
stest.cpp
sshbuf.cpp smap.cpp
)
				
set (sourcepath ../klib/)
add_source(
dbase.cpp kdes8.cpp
kheap.cpp bma.cpp
)

#
set (sourcepath ../common/)
add_source(
alloc.cpp array.cpp des8.cpp heap.cpp mem.cpp
secure.cpp
)

set (sourcepath ../memmodel/emul/)
add_source(
pglobals.cpp
nvram.cpp
)

#
set (sourcepath ../memmodel/emul/win32/)
add_source(
zrom.cpp
mchunk.cpp mglobals.cpp minit.cpp mkernel.cpp
mcodeseg.cpp mprocess.cpp mutils.cpp mshbuf.cpp
)

#add_executable(ekern ${source})
add_library(ekern SHARED ${source})
add_dependencies(ekern genexec emulator)
#install(TARGETS ekern DESTINATION ./symbian/bin)



#
#Copy ekern.dll to be next to epoc.exe
#

get_target_property(LIB_NAME ekern LOCATION)
get_target_property(DEST_NAME epoc LOCATION)
set(target_suffix .dll)
#Extract path from full filename
string(REGEX REPLACE "(^.+)/epoc.exe" "\\1" DEST_NAME ${DEST_NAME})
set(DEST_NAME ${DEST_NAME}/ekern${target_suffix})

#message(${DEST_NAME})

add_custom_command (
 	TARGET ekern
   	POST_BUILD
   	COMMAND ${CMAKE_COMMAND} -E copy ${LIB_NAME} ${DEST_NAME}
 	)



#define vs IDE folders
source_group(Sources REGULAR_EXPRESSION ".+\\.cpp$")
source_group(Includes REGULAR_EXPRESSION ".+\\.h$") 	


