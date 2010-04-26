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

project (euser)

cmake_minimum_required(VERSION 2.6)

#Add the given source files to our source prepending the sourcepath
macro(add_source)
	foreach(mySource ${ARGV})
		string(REGEX REPLACE "(^.+)" "${sourcepath}\\1" newsource ${mySource})
		set(source ${source} ${newsource})
	endforeach(mySource) 
endmacro(add_source) 

#Set common definitions using add_definitions
#Target specific definitions will be add using set_target_properties
#add_definitions(-DUNICODE)
add_definitions(-D__VC32__)
add_definitions(-D_UNICODE)
add_definitions(-D__LEAVE_EQUALS_THROW__)
add_definitions(-D__WINS__)
#__CPU_X86 is declared by __WINS__ 
#add_definitions(-D__CPU_X86)
#add_definitions(-D__DLL__)


#set(commonDefines "__VC32__; _UNICODE;")




include_directories (../euser ../euser/unicode ../include 
../include/nkern ../include/nkern/win32 ../include/kernel ../include/kernel/win32 ../include/memmodel/emul/win32 ../include/memmodel/emul)


#Adding the sources from cbase
set (sourcepath ../euser/cbase/)
add_source(
ub_act.cpp ub_array.cpp ub_bma.cpp ub_buf.cpp
ub_circ.cpp ub_cln.cpp ub_cons.cpp ub_dtim.cpp
ub_obj.cpp ub_svr.cpp ub_polsvr.cpp
ub_tim.cpp ub_utl.cpp ub_tque.cpp
)


#Add euser sources
set (sourcepath ../euser/)
add_source(
us_evnt.cpp locmapping.cpp us_exec.cpp us_func.cpp
us_graph.cpp us_hand.cpp us_ksvr.cpp us_des.cpp
us_lex16.cpp us_lex8.cpp us_loc.cpp us_mes.cpp
us_parse.cpp us_power.cpp us_property.cpp us_que.cpp
us_ref.cpp us_regn.cpp us_test.cpp us_time.cpp
us_trp.cpp us_utl.cpp us_mqueue.cpp us_encode.cpp
us_decode.cpp us_secure.cpp us_htab.cpp us_rwlock.cpp
us_shbuf.cpp
)


#Adding the sources from common
set (sourcepath ../common/)
add_source(
array.cpp des8.cpp des16.cpp alloc.cpp heap.cpp
mem.cpp secure.cpp
)

#Adding the sources from common/win32
set (sourcepath ../common/win32/)
add_source(
atomics.cpp
)

#Adding the sources from maths
set (sourcepath ../euser/maths/)
add_source(
um_asin.cpp um_atan.cpp um_dtor.cpp um_exp.cpp
um_frac.cpp um_int.cpp um_ln.cpp um_log.cpp
um_mod.cpp um_pow.cpp um_pow10.cpp
um_rand.cpp um_rtod.cpp um_sin.cpp um_spec.cpp
um_sqrt.cpp um_tan.cpp um_utl.cpp
)


#Adding the sources from epoc/win32
set (sourcepath ../euser/epoc/win32/)
add_source(
uc_i64.cpp uc_realx.cpp
uc_trp.cpp uc_utl.cpp 
uc_dll.cpp
#uc_exec.cpp
)

#Adding the sources from epoc/symc
set (sourcepath ../euser/epoc/symc/)
add_source(
uc_exec.cpp
)


#Adding the sources from epoc
set (sourcepath ../euser/epoc/)
add_source(
up_lib.cpp up_utl.cpp
)

#Adding the sources from unicode
set (sourcepath ../euser/unicode/)
add_source(
collate.cpp unicode.cpp unitable.cpp Compare.cpp
)

#define our target

### EMULATOR: used by euser and kernel for emulation
add_library (emulator SHARED ../euser/epoc/win32/emulator.cpp) #Compile the KLIB target entry point which is l_entry.cpp:  ../kernel/win32/l_entry.cpp
#target_link_libraries(emulator kernel32) #kernel32
#set_target_properties(emulator PROPERTIES COMPILE_FLAGS "/Zl") #No default library
#set_target_properties(emulator PROPERTIES LINK_FLAGS "/ENTRY:_Win32DllMain") #Change the entry point

### EUSER: many user library
add_library (euser SHARED ${source})
add_dependencies(euser genexec emulator)
target_link_libraries(euser emulator)
set_target_properties(euser PROPERTIES COMPILE_DEFINITIONS "__DLL__")


### ESTUB: not sure why need that yet
add_library (estub STATIC ../euser/epoc/win32/uc_stub.cpp)
#set_target_properties(estub PROPERTIES LINK_FLAGS /ENTRY:E32Bootstrap)

### SCPPNWDL_KERN: used by kernel. It contains new operators override.
#To be linked by kernel
add_library (scppnwdl_kern STATIC ../euser/epoc/win32/scppnwdl.cpp)
add_dependencies(scppnwdl_kern euser)

### EPOC: emulator executable
add_executable(epoc ../euser/epoc/win32/uc_exe.cpp ../euser/epoc/win32/uc_epoc.cpp) #Compile the EXE target entry point which is uc_exe.cpp 
add_dependencies(epoc estub euser)
target_link_libraries(epoc euser)
#set_target_properties(epoc PROPERTIES LINK_FLAGS "/ENTRY:_E32Bootstrap /SUBSYSTEM:WINDOWS /NODEFAULTLIB")
#target_link_libraries(epoc msvcrt)
#set_target_properties(epoc PROPERTIES COMPILE_FLAGS "/Zl")
#set_target_properties(epoc PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")
#set_target_properties(epoc PROPERTIES LINK_FLAGS "/NODEFAULTLIB")

 #set_target_properties(target1 target2 ... PROPERTIES prop1 value1 prop2 value2 ...)


#No need for emulator here
#add_executable(epoc ./euser/epoc/win32/uc_epoc.cpp)

 
 
#define vs IDE folders
source_group(Sources REGULAR_EXPRESSION ".+\\.cpp$")
source_group(Includes REGULAR_EXPRESSION ".+\\.h$") 	


