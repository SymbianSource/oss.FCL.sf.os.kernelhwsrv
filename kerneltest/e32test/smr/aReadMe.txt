Shadow Memory Region (SMR) Manual Test Suite
============================================


Introduction
------------

This folder contains the manual tests for the Shadow Memory Region feature
that is part of the kernel bootstrap PIL. The reference test variant for this 
feature is the NaviEngine, NE1 PSL. This feature allows Loader software in a 
local media boot scenario to shadow data held in local media partition into 
RAM which is then reserved by the bootstrap for later access by kernel 
extensions. The feature was introduced frist during the development of the 
Hardware Configuration Repository component.

The folder contains an overlay of 'os' tree source code that modifies 
several components to enable unit testing of this feature with existing NAND 
User Data partitions separately from the new tool support added to support 
SMR partitions.

This unit test suite was initially developed in the depot @
//EPOC/dv3/personal/2008/stephenm/sfm-baseH1-test1/os/...

There are three main parts to the test suite which relate to individual MBC #
files as follows:
	smrtest_h4_production.mbc
		Used to build ROMs for H4 to test SMR results in zero regressions.
		Test cases S01.xx
	smrtest_ne1_production.mbc
		Used to build ROMs for NE1 to test for zero regressions when no SMRs 
		are shadowed. 
		Test cases S02.xx
	smrtest_h4_production.mbc
		Used to build ROMs for H4 to test SMR results in zero regressions.
		Test cases S03.xx

Test cases listed in spreadsheet "aTestCases.xls". The memory model used does
not have a bearing on the testing of SMR as it is MM independent. Multiple
Memory Model ROMs should be used for H4 and NE1 uni-core testing
and Flexible Memory Model ROMs for NE1 multi-core SMP testing.


Build & Execute Instructions Summary
------------------------------------

S01.xx tests
	Build the Base tree and H4 variant tree
	Build exports from the e32tests\smr\h4 project
	Build H4 roms using smrtest_h4_production.mbc
	Execute tests as explained in aTestCases.xls
		
S02.xx tests
	Build the Base tree and NE1 variant tree		
	Build exports and test binaries from the e32tests\smr\ne1 project
	Build exports and test binaries from the e32tests\smr\ne1\flex project
	Build NE1 roms using smrtest_ne1_production.mbc	
	Execute tests as explained in aTestCases.xls
	
S03.01.xx tests
S03.02.xx tests
	Build the Base tree and NE1 variant tree	
	Unzip the e32test\smr\os-patch.zip files onto your os source tree (patches coreldr)
	Clean and then re-build the \os\boardsupport\naviengine\naviengineunistore2 
		component (builds patched coreldr)
	Build exports and test binaries from the e32tests\smr\ne1 project
	Build exports and test binaries from the e32tests\smr\ne1\flex project
	Build NE1 roms using smrtest_ne1_autotest.mbc	
	Execute tests as explained in aTestCases.xls
		
S03.03.xx test
	Modify \os\boardsupport\naviengine\navienginebsp\ne1_tb\config.inc to 
		define CFG_DebugBootRom & CFG_InitDebugPort to enable bootstrap tracing
	Build the Base tree and NE1 variant tree	
	Unzip the e32test\smr\os-patch.zip files onto your os source tree (patches coreldr)
	Clean and then re-build the \os\boardsupport\naviengine\naviengineunistore2 
		component (builds patched coreldr)
	Build exports and test binaries from the e32tests\smr\ne1 project
	Build exports and test binaries from the e32tests\smr\ne1\flex project
	Build NE1 roms using smrtest_ne1_autotest.mbc	
	Execute test S03.03.01 as explained in aTestCases.xls
	
	Modify \os\kernelhwsrv\kernel\eka\drivers\unistore2\srca\xsr\util\ONBL2\ONBL2.CPP
		to uncomment line 627 (comment line 626) in ShadowHCR() routine so that 
		it create an invalid sized SMRIB with 128 bytes, i.e. 8 entries but is 
		the 7 maximum so will lead to a fault in the bootstrap.
	Re-build the CoreLdr component for NE1 non FMM 
		(\os\boardsupport\naviengine\naviengineunistore2)
	Execute test S03.03.02 as explained in aTestCases.xls
	
	Revert edits to Core Loader.
	Revert edits to the NE1 config.inc source.
		
S03.04.01 tests
	Modify \os\boardsupport\naviengine\navienginebsp\ne1_tb\config.inc to 
		define CFG_DebugBootRom & CFG_InitDebugPort to enable bootstrap tracing
	Build the Base tree and NE1 variant tree	
	Unzip the e32test\smr\os-patch.zip files onto your os source tree (patches coreldr)
	Clean and then re-build the \os\boardsupport\naviengine\naviengineunistore2 
		component (builds patched coreldr)
	Build exports and test binaries from the e32tests\smr\ne1 project
	Build exports and test binaries from the e32tests\smr\ne1\flex project
	Build NE1 roms using smrtest_ne1_autotest.mbc	
	Modify \os\kernelhwsrv\kernel\eka\drivers\unistore2\srca\xsr\util\ONBL2\ONBL2.CPP
		to uncomment line 608 (comment line 607) in ShadowHCR() routine so that
		it stores the size of the SMRs as not multiples of 4096bytes so will 
		lead to a fault in bootstrap.
	Re-build the CoreLdr component for NE1 non FMM 
		(\os\boardsupport\naviengine\naviengineunistore2)
	Execute test S03.04.01 as explained in aTestCases.xls

	Revert edits to Core Loader source.
	Revert edits to the NE1 config.inc source.


Build SMR Test Projects
-----------------------

	Build H4 Multiple Memory Model version:
	cd \os\kernelhwsrv\kerneltest\e32test\smr\h4
	bldmake -f=smr_h4bld.inf bldfiles
	abld test build

	Build NE1 Multiple Memory Model version:
	cd \os\kernelhwsrv\kerneltest\e32test\smr\ne1
	bldmake -f=smr_ne1bld.inf bldfiles
	abld test build
	
	Build NE1 Flexible Memory Model / SMP version:
	cd \os\kernelhwsrv\kerneltest\e32test\smr\ne1\flex
	bldmake -f=smr_ne1smpbld.inf bldfiles
	abld test build
	

	
Build ROMs for Test Suite
-------------------------

	Smoke test H4:
		cd \os\kernelhwsrv\kernel\eka\rombuild
		metabld smrtest_h4_production.mbc > \logs\h4p.log 2>&1
		scanlog \logs\h4p.log
		
	Smoke test NE1:
		cd \os\kernelhwsrv\kernel\eka\rombuild
		metabld smrtest_ne1_production.mbc > \logs\ne1p.log 2>&1
		scanlog \logs\ne1p.log
	
	Functional (+ve & -ve) tests for NE1:
		cd \os\kernelhwsrv\kernel\eka\rombuild
		metabld smrtest_ne1_autotest.mbc > \logs\ne1at.log 2>&1
		scanlog \logs\ne1at.log
	

Notes
-----

*** variant.mmh Files

\os\kernelhwsrv\kerneltest\e32test\smr\ne1\variant.mmh is a direct unmodified 
copy of the version in \os\boardsupport\naviengine\navienginebsp\ne1_tb\variant.mmh.
Please ensure this is aligned before building NE1 tests. Same for 
\os\kernelhwsrv\kerneltest\e32test\smr\ne1\flex\variant.mmh which is a copy of
\os\boardsupport\naviengine\navienginebspflexible\variant.mmh

