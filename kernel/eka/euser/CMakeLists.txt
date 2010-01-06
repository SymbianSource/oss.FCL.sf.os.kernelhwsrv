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

#add_definitions(-DUNICODE)
add_definitions(-D__VC32__)
add_definitions(-D_UNICODE)
add_definitions(-D__LEAVE_EQUALS_THROW__)
add_definitions(-D__WINS__)
#__CPU_X86 is declared by __WINS__ 
#add_definitions(-D__CPU_X86)
add_definitions(-D__DLL__)

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
uc_trp.cpp uc_utl.cpp uc_exec.cpp
uc_dll.cpp
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

#define our library
add_library (emulator SHARED ../euser/epoc/win32/emulator.cpp)
target_link_libraries(emulator kernel32)

add_library (euser SHARED ${source})
add_dependencies(euser genexec emulator)

add_library (estub ../euser/epoc/win32/uc_stub.cpp)

#define vs IDE folders
source_group(Sources REGULAR_EXPRESSION ".+\\.cpp$")
source_group(Includes REGULAR_EXPRESSION ".+\\.h$") 	


