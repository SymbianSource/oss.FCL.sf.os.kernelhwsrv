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

project (eka)

cmake_minimum_required(VERSION 2.6)

#add_definitions(-DUNICODE)
add_definitions(-D__VC32__)
add_definitions(-D_UNICODE)
add_definitions(-D__LEAVE_EQUALS_THROW__)
add_definitions(-D__WINS__)
#__CPU_X86 is declared by __WINS__ 
#add_definitions(-D__CPU_X86)
add_definitions(-D__DLL__)

include_directories (./euser ./euser/unicode ./include)

#Adding the sources from cbase
set (sourcepath ./euser/cbase/)
set (source ${source}
${sourcepath}ub_act.cpp ${sourcepath}ub_array.cpp ${sourcepath}ub_bma.cpp ${sourcepath}ub_buf.cpp
${sourcepath}ub_circ.cpp ${sourcepath}ub_cln.cpp ${sourcepath}ub_cons.cpp ${sourcepath}ub_dtim.cpp
${sourcepath}ub_obj.cpp ${sourcepath}ub_svr.cpp ${sourcepath}ub_polsvr.cpp
${sourcepath}ub_tim.cpp ${sourcepath}ub_utl.cpp ${sourcepath}ub_tque.cpp
)

#Add euser sources
set (sourcepath ./euser/)
set (source ${source}
${sourcepath}us_evnt.cpp ${sourcepath}locmapping.cpp ${sourcepath}us_exec.cpp ${sourcepath}us_func.cpp
${sourcepath}us_graph.cpp ${sourcepath}us_hand.cpp ${sourcepath}us_ksvr.cpp ${sourcepath}us_des.cpp
${sourcepath}us_lex16.cpp ${sourcepath}us_lex8.cpp ${sourcepath}us_loc.cpp ${sourcepath}us_mes.cpp
${sourcepath}us_parse.cpp ${sourcepath}us_power.cpp ${sourcepath}us_property.cpp ${sourcepath}us_que.cpp
${sourcepath}us_ref.cpp ${sourcepath}us_regn.cpp ${sourcepath}us_test.cpp ${sourcepath}us_time.cpp
${sourcepath}us_trp.cpp ${sourcepath}us_utl.cpp ${sourcepath}us_mqueue.cpp ${sourcepath}us_encode.cpp
${sourcepath}us_decode.cpp ${sourcepath}us_secure.cpp ${sourcepath}us_htab.cpp ${sourcepath}us_rwlock.cpp
${sourcepath}us_shbuf.cpp
)

#Adding the sources from common
set (sourcepath ./common/)
set (source ${source}
${sourcepath}array.cpp ${sourcepath}des8.cpp ${sourcepath}des16.cpp ${sourcepath}alloc.cpp ${sourcepath}heap.cpp
${sourcepath}mem.cpp ${sourcepath}secure.cpp
)

#Adding the sources from common/win32
set (sourcepath ./common/win32/)
set (source ${source}
${sourcepath}atomics.cpp
)

#Adding the sources from maths
set (sourcepath ./euser/maths/)
set (source ${source}
${sourcepath}um_asin.cpp ${sourcepath}um_atan.cpp ${sourcepath}um_dtor.cpp ${sourcepath}um_exp.cpp
${sourcepath}um_frac.cpp ${sourcepath}um_int.cpp ${sourcepath}um_ln.cpp ${sourcepath}um_log.cpp
${sourcepath}um_mod.cpp ${sourcepath}um_pow.cpp ${sourcepath}um_pow10.cpp
${sourcepath}um_rand.cpp ${sourcepath}um_rtod.cpp ${sourcepath}um_sin.cpp ${sourcepath}um_spec.cpp
${sourcepath}um_sqrt.cpp ${sourcepath}um_tan.cpp ${sourcepath}um_utl.cpp
)

#Adding the sources from epoc/win32
set (sourcepath ./euser/epoc/win32/)
set (source ${source}
${sourcepath}uc_i64.cpp ${sourcepath}uc_realx.cpp
${sourcepath}uc_trp.cpp ${sourcepath}uc_utl.cpp ${sourcepath}uc_exec.cpp
)

#Adding the sources from epoc
set (sourcepath ./euser/epoc/)
set (source ${source}
${sourcepath}up_lib.cpp ${sourcepath}up_utl.cpp
)

#Adding the sources from unicode
set (sourcepath ./euser/unicode/)
set (source ${source}
${sourcepath}collate.cpp ${sourcepath}unicode.cpp ${sourcepath}unitable.cpp ${sourcepath}Compare.cpp
)

#define our library
add_library (emulator SHARED ./euser/epoc/win32/emulator.cpp)
target_link_libraries(emulator kernel32)

add_library (euser SHARED ${source})
target_link_libraries(euser emulator)

add_library (estub ./euser/epoc/win32/uc_stub.cpp)

add_executable(epoc ./euser/epoc/win32/uc_epoc.cpp)
target_link_libraries(epoc estub euser)







#define vs IDE folders
source_group(Sources REGULAR_EXPRESSION ".+\\.cpp$")
source_group(Includes REGULAR_EXPRESSION ".+\\.h$") 	

#TODO: generate the exec enum
#perl genexec.pl -i execs.txt -e ../include/exec_enum.h -u ../include/exec_user.h -k ../include/exec_kernel.h
