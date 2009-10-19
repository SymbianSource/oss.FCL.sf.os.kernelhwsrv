; template\config.inc
;
; Copyright (c) 1998-2001 Symbian Ltd. All rights reserved.
;
; Template bootstrap configuration file

; Include to enable tracing
;	GBLL	CFG_DebugBootRom

; Include one of these to select the CPU
;	GBLL	CFG_CPU_GENERIC_ARM4
;	GBLL	CFG_CPU_ARM710T
;	GBLL	CFG_CPU_ARM720T
;	GBLL	CFG_CPU_SA1
;	GBLL	CFG_CPU_ARM920T
;	GBLL	CFG_CPU_ARM925T
;	GBLL	CFG_CPU_ARM926J
;	GBLL	CFG_CPU_XSCALE
;	GBLL	CFG_CPU_ARM1136
	GBLL	CFG_CPU_ARM1176
;	GBLL	CFG_CORTEX_A8

; Include the following line if this is a bootloader bootstrap
;	GBLL	CFG_BootLoader
; TO DO:
; The following line needs to be removed for target hardware
	GBLL	CFG_Template

; If you want to supply a custom set of initial vectors (including reset vector) include the following line
;	GBLL	CFG_CustomVectors
;
; and provide a custom_vectors.inc file

; Variant Number, just an example:
	INIT_NUMERIC_CONSTANT	CFG_HWVD, 0x09080001

; On ARM architecture 6 processors, include the following line to override the threshold
; on total physical RAM size at which the multiple memory model switches into large address space mode
; i.e. size>threshold -> 2Gb per process, size<=threshold -> 1Gb per process
; Defaults to 32Mb.
;	INIT_NUMERIC_CONSTANT	CFG_ARMV6_LARGE_CONFIG_THRESHOLD, <value>

; For the direct memory model only, include the following line if you wish the exception vectors at the
; start of the bootstrap to be used at all times. This is only relevant if an MMU is present - this option
; is mandatory if not.
;	GBLL	CFG_UseBootstrapVectors
;
; If the above option is in use (including if no MMU is present) the following symbol should be defined
; to specify the offset from the bootstrap to the kernel image.
	INIT_NUMERIC_CONSTANT	KernelCodeOffset, 0x4000

; Include the following line if you wish to include the ROM autodetection code based on data bus
; capacitance and image repeats.
;	GBLL	CFG_AutoDetectROM

; Include the following line to minimise the initial kernel heap size
; On the direct memory model the size of the kernel data area (super page to end of kernel heap) 
; is rounded up to the next 1Mb if this is not included, 4K if it is.
; On the moving and multiple models, the size of the initial kernel heap area is rounded up to
; the next 64K if this is not included, 4K if it is.
;	GBLL	CFG_MinimiseKernelHeap

; Include the following line if default memory mapping should use shared memory.
; Should be defined on multicore (SMP) devices.
;	GBLL	CFG_USE_SHARED_MEMORY

; On the moving or multiple memory models, include either or both of the following lines to
; specify the size of the initial kernel heap
;	INIT_NUMERIC_CONSTANT	CFG_KernelHeapMultiplier, <multiplier>
;	INIT_NUMERIC_CONSTANT	CFG_KernelHeapBaseSize, <base>
;
; The initial kernel heap size is MAX( <base> + <multiplier> * N / 16, value specified in ROMBUILD )
; where N is the total physical RAM size in pages.
; <base> defaults to 24K and <multiplier> defaults to 9*16 (ie 9 bytes per page).

; Uncomment if using ARM1136 processor and ARM1136 Erratum 353494
; "Rare conditions can cause corruption of the Instruction Cache"
; is fixed on this hardware.
;
; NOTE: The boot table should use this macro to determine whether RONO or RORO permissions
; are used for the exception vectors. If the erratum is not fixed, RORO must be used.
;
;	GBLL	CFG_CPU_ARM1136_ERRATUM_353494_FIXED

; Uncomment if using ARM1136 processor and ARM1136 Erratum 364296
; "Possible Cache Data Corruption with Hit-Under-Miss"
; is fixed on this hardware.
;
;	GBLL	CFG_CPU_ARM1136_ERRATUM_364296_FIXED

; Uncomment if using ARM1136 processor and ARM1136 Erratum 399234
; "Write back data cache entry evicted by write through entry causes data corruption"
; is fixed on this hardware.
; Workaround
; The erratum may be avoided by marking all cacheable memory as one of write through or write back.
; This requires the memory attributes described in the translation tables to be modified by software
; appropriately, or the use of the remapping capability to remap write through regions to non cacheable.
;
; If this macro is enabled, it should be accompanied by:
; 		"macro __CPU_ARM1136_ERRATUM_399234_FIXED" in variant.mmh
;	GBLL	CFG_CPU_ARM1136_ERRATUM_399234_FIXED


; Uncomment if:
;	1)	using ARM1136 processor and ARM1136 Erratum 411920: "Invalidate Entire Instruction Cache
;	  	operation might fail to invalidate some lines if coincident with linefill"
;  	  	is fixed on this hardware, or
;	2)	using ARM1176 processor and ARM1176 Erratum 415045: "Invalidate Entire Instruction Cache
; 	  	operation might fail to invalidate some lines if coincident with linefill
;	  	is fixed on this hardware.
; Workaround:
;	1)	Disables the use of of prefetch range cache operations by setting RV bit in Auxiliary Ctrl Reg.
;	2)	Replaces Invalidate ICache operation with the sequence defined in the errata document.
; If this macro is enabled, it should be accompanied by:
; 	"macro __CPU_ARM1136_ERRATUM_411920_FIXED" in variant.mmh
;
;	GBLL	CFG_CPU_ARM1136_ERRATUM_411920_FIXED


; Uncomment if using ARM1136 processor and ARM1136 Erratum 415662: "Invalidate Instruction Cache by
; Index might corrupt cache when used with background prefetch range" is fixed on this hardware.
; Workaround:
; 	Disables the use of of prefetch range cache operations by setting RV bit in Auxiliary Ctrl Reg.
;
;	GBLL	CFG_CPU_ARM1136_ERRATUM_415662_FIXED



; These are deduced from the supplied configuration
;	CFG_ARMV6
;	CFG_MMUPresent
;	CFG_CachePresent
;	CFG_WriteBufferPresent
;	CFG_SplitCache
;	CFG_SplitTLB
;	CFG_AltDCachePresent
;	CFG_WriteBackCache
;	CFG_CacheWriteAllocate
;	CFG_CachePhysicalTag
;	CFG_CacheFlushByDataRead
;	CFG_CacheFlushByWaySetIndex
;	CFG_CacheFlushByLineAlloc
;	CFG_CachePolicyInPTE
;	CFG_TEX
;	CFG_SingleEntryDCacheFlush
;	CFG_SingleEntryICacheFlush
;	CFG_SingleEntryITLBFlush
;	CFG_SingleEntryTLBFlush
;	CFG_CacheTypeReg
;	CFG_BTBPresent
;	CFG_CARPresent
;	CFG_PrefetchBuffer
;	CFG_FCSE_Present
;	CFG_ASID_Present
;	CFG_IncludeRAMAllocator

	END
