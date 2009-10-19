// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//



/**
 @file
 @internalTechnology
 @released
*/

#ifndef __RM_DEBUG_KERNELDRIVER_H__
#define __RM_DEBUG_KERNELDRIVER_H__

#include <rm_debug_api.h>

/**
Used to store a value read from or written to an ARM register
*/
typedef TUint32 T4ByteRegisterValue;


/** 
Provides static methods for accessing the information stored in a TRegisterInfo
object.
*/
class Register
	{
public:
	static TBool IsCoreReg(const Debug::TRegisterInfo aRegister);
	static TBool IsCoproReg(const Debug::TRegisterInfo aRegister);
	static TUint32 GetCoreRegId(const Debug::TRegisterInfo aRegister);
	static TUint32 GetCRm(const Debug::TRegisterInfo aRegister);
	static TUint32 GetCRn(const Debug::TRegisterInfo aRegister);
	static TUint32 GetOpcode1(const Debug::TRegisterInfo aRegister);
	static TUint32 GetOpcode2(const Debug::TRegisterInfo aRegister);
	static TUint32 GetCoproNum(const Debug::TRegisterInfo aRegister);
	};

/**
Identify whether aRegister is a core register
@param aRegister register ID to analyse
@return ETrue if core register, EFalse otherwise
*/
inline TBool Register::IsCoreReg(const Debug::TRegisterInfo aRegister)
	{
	return ((aRegister & 0xff) == 0x0);
	}

/**
Identify whether aRegister is a coprocessor register
@param aRegister register ID to analyse
@return ETrue if coprocessor register, EFalse otherwise
*/
inline TBool Register::IsCoproReg(const Debug::TRegisterInfo aRegister)
	{
	return ((aRegister & 0xff) == 0x1);
	}

/**
Get the ID of the core register
@param aRegister register ID to analyse
@return ID of the core register
*/
inline TUint32 Register::GetCoreRegId(const Debug::TRegisterInfo aRegister)
	{
	return ((aRegister >> 8) & 0xff);
	}

/**
Get the CRm value of a coprocessor register
@param aRegister register ID to analyse
@return the CRm value of a coprocessor register
*/
inline TUint32 Register::GetCRm(const Debug::TRegisterInfo aRegister)
	{
	return ((aRegister >> 16) & 0xf);
	}

/**
Get the CRm value of a coprocessor register
@param aRegister register ID to analyse
@return the CRm value of a coprocessor register
*/
inline TUint32 Register::GetCRn(const Debug::TRegisterInfo aRegister)
	{
	return ((aRegister >> 20) & 0xf);
	}

/**
Get the Opcode1 value of a coprocessor register
@param aRegister register ID to analyse
@return the Opcode1 value of a coprocessor register
*/
inline TUint32 Register::GetOpcode1(const Debug::TRegisterInfo aRegister)
	{
	return ((aRegister >> 24) & 0x8);
	}
	
/**
Get the Opcode2 value of a coprocessor register
@param aRegister register ID to analyse
@return the Opcode2 value of a coprocessor register
*/
inline TUint32 Register::GetOpcode2(const Debug::TRegisterInfo aRegister)
	{
	return ((aRegister >> 27) & 0x8);
	}

/**
Get the coprocessor number of a coprocessor register
@param aRegister register ID to analyse
@return the coprocessor number of a coprocessor register
*/
inline TUint32 Register::GetCoproNum(const Debug::TRegisterInfo aRegister)
	{
	return ((aRegister >> 8) & 0xff);
	}

//
// class TCapsRM_DebugDriver
//
class TCapsRM_DebugDriver
{
public:
	TVersion	iVersion;
};

/**
Stores listings information for passing between the DSS and the kernel driver
*/
class TListInformation
{
public:
	inline TListInformation(const Debug::TListId aType=(Debug::TListId)NULL, const Debug::TListScope aListScope=(Debug::TListScope)NULL, TDes8* aBuffer=NULL, TUint32* aDataSize=NULL, TUint64 aTargetId=0)
		: iType(aType),
		  iListScope(aListScope),
		  iBuffer(aBuffer),
		  iDataSize(aDataSize),
		  iTargetId(aTargetId) {};
public:
	Debug::TListId iType;
	Debug::TListScope iListScope;
	TDes8* iBuffer;
	TUint32* iDataSize;
	TUint64 iTargetId;
};

/**
Data structure to hold information to the crash flash
(Possibly: Could be expanded to hold on configuration data too)
*/
class TFlashInfo
{
public:
	inline TFlashInfo(TUint32 aPos, TUint32* aSize, TDes8* aData)
		:iPos(aPos),
		iSize(aSize),
		iData(aData){};
public:
	TUint32 iPos;
	TUint32* iSize;	
	 TDes8* iData;	
};
//
// class TRM_DebugMemoryInfo
//
class TRM_DebugMemoryInfo
{
public:

	inline TRM_DebugMemoryInfo(const TUint32 aAddress, const TUint32 aLength, TDesC8 *aData)
				: iAddress(aAddress),
				  iLength(aLength),
				  iData(aData) {};
	
public:

	TUint32 iAddress;
	TUint32	iLength;
	TDesC8*	iData;
};


/**
@deprecated
This class is only used by TRK phase 1 functions.

@see TRM_DebugRegisterInformation which offers similar storage suitable for use
with the TRK pahse 2 API.
*/
class TRM_DebugRegisterInfo
{
public:

	inline TRM_DebugRegisterInfo(const TInt16 aFirstRegister, const TInt16 aLastRegister, TDesC8 *aValues)
				: iFirstRegister(aFirstRegister),
				  iLastRegister(aLastRegister),
				  iValues(aValues) {};
	
public:

	TInt16	iFirstRegister;
	TInt16	iLastRegister;
	TDesC8*	iValues;
};

/**
Structure used to store information about registers
*/
class TRM_DebugRegisterInformation
{
public:

	inline TRM_DebugRegisterInformation(const TDes8 *aRegisterIds=NULL, TDes8 *aRegisterValues=NULL, TDes8 *aRegisterFlags=NULL)
		: iRegisterIds(aRegisterIds),
		  iRegisterValues(aRegisterValues),
		  iRegisterFlags(aRegisterFlags) {};
	
public:

	const TDes8* iRegisterIds;
	TDes8* iRegisterValues;
	TDes8* iRegisterFlags;
};

//
// class TRM_DebugTaskInfo
//
class TRM_DebugTaskInfo
{
public:

	inline TRM_DebugTaskInfo(TUint32 aOtherId)
				: iId(0),
				  iOtherId(aOtherId),
				  iPriority(0) { iName.FillZ(); };

public:

	TUint32 iId;
	TUint32 iOtherId;
	TUint32 iPriority;	
	TBuf8<KMaxName> iName;
};

//
// class TRM_DebugStepInfo
//
class TRM_DebugStepInfo
{
public:

	inline TRM_DebugStepInfo(const TUint32 aStartAddress, const TUint32 aStopAddress, const TBool aStepInto)
				: iStartAddress(aStartAddress),
				  iStopAddress(aStopAddress),
				  iStepInto(aStepInto) {};

public:

	TUint32 iStartAddress;
	TUint32 iStopAddress;
	TBool iStepInto;
};


//
// class TRM_DebugDriverInfo
//
class TRM_DebugDriverInfo
{
public:

	TUint32 iPanic1Address;
	TUint32 iPanic2Address;
	TUint32 iException1Address;
	TUint32 iException2Address;
	TUint32 iLibraryLoadedAddress;
	TUint32 iUserLibraryEnd;
};


//
// class TRM_DebugProcessInfo
//
class TRM_DebugProcessInfo
{
public:

	inline TRM_DebugProcessInfo(TUint32 *aCodeAddress, TUint32 *aDataAddress)
				: iCodeAddress(aCodeAddress),
				  iDataAddress(aDataAddress) {};

public:

	TUint32* iCodeAddress;
	TUint32* iDataAddress;
};

//
// class TRM_DebugEventActionInfo
//
class TRM_DebugEventActionInfo
{
public:
	inline TRM_DebugEventActionInfo(TUint32 aEvent, TUint32 aAction, TUint64 aAgentId)
		: iEvent(aEvent),
		iAction(aAction),
		iAgentId(aAgentId) {};
public:
	TUint32 iEvent;
	TUint32 iAction;
	TUint64 iAgentId;
};

//
// class TRM_DebugEventInfo
//
class TRM_DebugEventInfo
{
public:
	inline TRM_DebugEventInfo(TDesC8& aProcessName, TUint32& aBufSize)
		: iProcessName(aProcessName),
		iBufSize(aBufSize) {};

public:
	TDesC8& iProcessName;
	TUint32& iBufSize;
};

//
// class TRMD_DebugAgentId
//
class TRM_DebugAgentId
{
public:
	inline TRM_DebugAgentId(TUint64 aAgentId)
		: iAgentId(aAgentId) {};

public:
	TUint64 iAgentId;
};

//
// Class TRMD_DebugCancelInfo
//
class TRMD_DebugCancelInfo
{
public:
	inline TRMD_DebugCancelInfo(TUint32 aCancelRequest,TDesC8& aProcessName, TUint64 aAgentId)
		: iCancelRequest(aCancelRequest),
		iProcessName(aProcessName),
		iAgentId(aAgentId) {};

	inline TRMD_DebugCancelInfo(void)
		: iCancelRequest(0),
	iAgentId(0)
	{
	};

public:
	TUint32 iCancelRequest;
	TBuf8<KMaxName> iProcessName;
	TUint64 iAgentId;
};

class TEventMetaData
	{
public:
	TBuf8<KMaxName> iTargetProcessName;
	TUint64 iDebugAgentProcessId;
	};

/**
@internalComponent
*/
class TSetBreakInfo
{
public:

	inline TSetBreakInfo(Debug::TBreakId* aBreakId,
		TUint64 aId,\
		TUint32 aAddress,\
		Debug::TArchitectureMode aMode,
		TBool aThreadSpecific)
				: iBreakId(aBreakId),
				  iId(aId),
				  iAddress(aAddress),
				  iMode(aMode),
       				  iThreadSpecific(aThreadSpecific) {};

inline TSetBreakInfo(void)
			: iBreakId((Debug::TBreakId*)0),
			  iId(0),
			  iAddress(0),
			  iMode(Debug::EArmMode),
       			  iThreadSpecific(ETrue) {};


public:
	Debug::TBreakId* iBreakId;
	TUint64 iId;
	TUint32 iAddress;
	Debug::TArchitectureMode iMode;
	TBool iThreadSpecific;
};

/**
@internalComponent
*/
class TModifyBreakInfo
{
public:

	inline TModifyBreakInfo(Debug::TBreakId aBreakId,\
		const TUint64 aThreadId,\
		const TUint32 aAddress,\
		const Debug::TArchitectureMode aMode)
				: iBreakId(aBreakId),
				  iThreadId(aThreadId),
				  iAddress(aAddress),
				  iMode(aMode) {};

public:
	const Debug::TBreakId iBreakId;
	const TUint64 iThreadId;
	const TUint32 iAddress;
	const Debug::TArchitectureMode iMode;
};

/**
@internalComponent
*/
class TModifyProcessBreakInfo
{
public:

	inline TModifyProcessBreakInfo(Debug::TBreakId aBreakId,\
		const TUint64 aProcessId,\
		const TUint32 aAddress,\
		const Debug::TArchitectureMode aMode)
				: iBreakId(aBreakId),
				  iProcessId(aProcessId),
				  iAddress(aAddress),
				  iMode(aMode) {};

public:
	const Debug::TBreakId iBreakId;
	const TUint64 iProcessId;
	const TUint32 iAddress;
	const Debug::TArchitectureMode iMode;
};

/**
@internalComponent
*/
class TGetBreakInfo
{
public:

	inline TGetBreakInfo(Debug::TBreakId aBreakId,\
		TUint64& aId,\
		TUint32& aAddress,\
		Debug::TArchitectureMode& aMode,
		TBool& aThreadSpecific)
				: iBreakId(aBreakId),
				  iId(&aId),
				  iAddress(&aAddress),
				  iMode(&aMode),
       				  iThreadSpecific(&aThreadSpecific) {};

	inline TGetBreakInfo()
				: iBreakId((Debug::TBreakId)0),
				  iId((TUint64*)0),
				  iAddress((TUint32*)0),
				  iMode((Debug::TArchitectureMode*)0),
       				  iThreadSpecific((TBool*)0)	{};

public:
	const Debug::TBreakId iBreakId;
	TUint64* iId;
	TUint32* iAddress;
	Debug::TArchitectureMode* iMode;
	TBool* iThreadSpecific;
};

//
// class RRM_DebugDriver
//
class RRM_DebugDriver : public RBusLogicalChannel
{
public:

	enum TControl
	{
		EControlSetBreak = 0,
		EControlClearBreak,
		EControlModifyBreak,
		EControlBreakInfo,
		EControlSuspendThread,
		EControlResumeThread,
		EControlStepRange,
		EControlReadMemory,
		EControlWriteMemory,
		EControlReadRegisters,
		EControlWriteRegisters,
		EControlGetStaticLibraryInfo,
		EControlGetDebugFunctionalityBufSize,
		EControlGetDebugFunctionality,
		EControlReadRegistersLegacy,
		EControlWriteRegistersLegacy,		
		EControlGetMemoryOperationMaxBlockSize,		
		EControlAttachProcess,
		EControlDetachProcess,
		EControlDetachAgent,
		EControlSetEventAction,
		EControlGetList,
		EControlStep,
		EControlIsDebuggable,
		EControlKillProcess,		
		EControlModifyProcessBreak,
	};
	
	enum TRequest
	{
		ERequestGetEvent=0x0, ERequestGetEventCancel=0x1
	};	
		
public:

	inline TInt Open(const TRM_DebugDriverInfo aDriverInfo);

	inline TInt	SetBreak(Debug::TBreakId &aBreakId,const TUint32 aThreadId, const TUint32 aAddress, const Debug::TArchitectureMode aThumbMode );
	inline TInt	SetProcessBreak(Debug::TBreakId &aBreakId,const TUint32 aProcessId, const TUint32 aAddress, const Debug::TArchitectureMode aThumbMode );
	
	inline TInt	ClearBreak(const TInt32 aBreakId);
	
	inline TInt	ModifyBreak(const Debug::TBreakId aBreakId, const TUint32 aThreadId, const TUint32 aAddress, const Debug::TArchitectureMode aArchitectureMode );
	inline TInt	ModifyProcessBreak(const Debug::TBreakId aBreakId, const TUint32 aProcessId, const TUint32 aAddress, const Debug::TArchitectureMode aArchitectureMode );
	
	inline TInt BreakInfo(const Debug::TBreakId aBreakId, TUint64& aId, TUint32& aAddress, Debug::TArchitectureMode& aMode, TBool& aThreadSpecific);
	
	inline TInt	SuspendThread(const TUint32 aThreadId);
	inline TInt	ResumeThread(const TUint32 aThreadId);
	inline TInt	StepRange(const TUint32 aThreadId, const TUint32 aStartAddress, const TUint32 aStopAddress, TBool aStepInto);
	inline TInt ReadMemory(const TUint32 aThreadId, const TUint32 aAddress, const TUint32 aLength, TDes8 &aData);
	inline TInt WriteMemory(const TUint32 aThreadId, const TUint32 aAddress, const TUint32 aLength, const TDesC8 &aData);
	inline TInt ReadRegisters(const TUint32 aThreadId, const TDes8 &aRegisterIds, TDes8 &aRegisterValues, TDes8 &aRegisterFlags);
	inline TInt WriteRegisters(const TUint32 aThreadId, const TDes8 &aRegisterIds, const TDes8 &aRegisterValues, TDes8 &aRegisterFlags);
	inline TInt ReadRegisters(const TUint32 aThreadId, const TInt32 aFirstRegister, const TInt32 aLastRegister, TDes8 &aValues);
	inline TInt WriteRegisters(const TUint32 aThreadId, const TInt32 aFirstRegister, const TInt32 aLastRegister, TDesC8 &aValues);
	inline void GetEvent(TDesC8& aProcessName, TUint64 aAgentId, TRequestStatus &aStatus, Debug::TEventInfo &aEventInfo);
	inline void CancelGetEvent(TDesC8& aProcessName, TUint64 aAgentId);
//	inline TInt GetProcessInfo(const TInt aIndex, TRM_DebugTaskInfo &aInfo);
//	inline TInt GetThreadInfo(const TInt aIndex, TRM_DebugTaskInfo &aInfo);
	inline TInt GetStaticLibraryInfo(const TInt aIndex, Debug::TEventInfo &aInfo);
	inline TInt GetDebugFunctionalityBufSize(TUint32 &aBufSize);
	inline TInt GetDebugFunctionality(TDes8& aDebugFunctionality);
	inline TInt GetMemoryOperationMaxBlockSize(TUint32 &aMaxSize);
	inline TInt AttachProcess(TDesC8& aProcessName, TUint64 aAgentId);
	inline TInt DetachProcess(TDesC8& aProcessName, TUint64 aAgentId);
	inline TInt DetachAgent(TUint64 aAgentId);
	inline TInt SetEventAction(TDesC8& aProcessName, Debug::TEventType aEvent, Debug::TKernelEventAction aEventAction, TUint64 aAgentId);
	inline TInt GetList(const Debug::TListId aType, const Debug::TListScope aListScope, const TUint64 aTargetId, const TUint64 aDebugProcessId, TDes8& aBuffer, TUint32& aDataSize);
	inline TInt Step(const TUint32 aThreadId, const TUint32 aNumSteps);
	inline TInt IsDebuggable(const TUint32 aProcessId);
	inline TInt KillProcess(const TUint32 aProcessId, const TInt32 aReason);
};

_LIT(KRM_DebugDriverName,"RM Debug Driver");

//priority set equal to that of KDfcThread0Priority defined in e32/kernel/sinit.cpp
const TInt KRmDebugDriverThreadPriority = 27;

// Version information
const TInt KMajorVersionNumber=2;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=0;


inline TInt RRM_DebugDriver::Open(const TRM_DebugDriverInfo aDriverInfo)
{
	TBuf8<32> buf;
	buf.Append((TUint8*)&aDriverInfo.iPanic1Address, 4);
	buf.Append((TUint8*)&aDriverInfo.iPanic2Address, 4);
	buf.Append((TUint8*)&aDriverInfo.iException1Address, 4);
	buf.Append((TUint8*)&aDriverInfo.iException2Address, 4);
	buf.Append((TUint8*)&aDriverInfo.iLibraryLoadedAddress, 4);
	buf.Append((TUint8*)&aDriverInfo.iUserLibraryEnd, 4);
	
	#ifdef EKA2
	return DoCreate(KRM_DebugDriverName, TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber), KNullUnit, NULL, &buf);
	#else
	return DoCreate(KRM_DebugDriverName, TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber), NULL, KNullUnit, NULL, &buf);
	#endif
}

inline TInt RRM_DebugDriver::SetBreak(Debug::TBreakId &aBreakId, const TUint32 aThreadId, const TUint32 aAddress, const Debug::TArchitectureMode aMode )
{
	TSetBreakInfo info(&aBreakId, aThreadId, aAddress, aMode, ETrue);
	return DoSvControl(EControlSetBreak, reinterpret_cast<TAny*>(&info),0);
}
inline TInt RRM_DebugDriver::SetProcessBreak(Debug::TBreakId &aBreakId, const TUint32 aProcessId, const TUint32 aAddress, const Debug::TArchitectureMode aMode )
{
	TSetBreakInfo info(&aBreakId, aProcessId, aAddress, aMode, EFalse);
	return DoSvControl(EControlSetBreak, reinterpret_cast<TAny*>(&info),0);
}

inline TInt RRM_DebugDriver::ClearBreak(const Debug::TBreakId aBreakId)
{
	return DoSvControl(EControlClearBreak, reinterpret_cast<TAny*>(aBreakId), 0);
}

inline TInt RRM_DebugDriver::ModifyBreak(const Debug::TBreakId aBreakId, const TUint32 aThreadId, const TUint32 aAddress, const Debug::TArchitectureMode aMode)
{
	TModifyBreakInfo info(aBreakId, aThreadId, aAddress, aMode);
	return DoControl(EControlModifyBreak, reinterpret_cast<TAny*>(&info), 0);
}

inline TInt RRM_DebugDriver::ModifyProcessBreak(const Debug::TBreakId aBreakId, const TUint32 aProcessId, const TUint32 aAddress, const Debug::TArchitectureMode aMode)
{
	TModifyProcessBreakInfo info(aBreakId, aProcessId, aAddress, aMode);
	return DoControl(EControlModifyProcessBreak, reinterpret_cast<TAny*>(&info), 0);
}

inline TInt RRM_DebugDriver::BreakInfo(const Debug::TBreakId aBreakId, TUint64& aId, TUint32& aAddress, Debug::TArchitectureMode& aMode, TBool& aThreadSpecific)
{
	TGetBreakInfo info(aBreakId, aId, aAddress, aMode, aThreadSpecific);
	return DoControl(EControlBreakInfo, reinterpret_cast<TAny*>(&info), 0);
}

inline TInt RRM_DebugDriver::SuspendThread(const TUint32 aThreadId)
{
	return DoControl(EControlSuspendThread, reinterpret_cast<TAny*>(aThreadId));
}

inline TInt RRM_DebugDriver::ResumeThread(const TUint32 aThreadId)
{
	return DoSvControl(EControlResumeThread, reinterpret_cast<TAny*>(aThreadId));
}

inline TInt RRM_DebugDriver::StepRange(const TUint32 aThreadId, const TUint32 aStartAddress, const TUint32 aStopAddress, TBool aStepInto)
{
	TRM_DebugStepInfo info(aStartAddress, aStopAddress, aStepInto);
	return DoSvControl(EControlStepRange, reinterpret_cast<TAny*>(aThreadId), (TAny*)&info);
}

inline TInt RRM_DebugDriver::ReadMemory(const TUint32 aThreadId, const TUint32 aAddress, const TUint32 aLength, TDes8 &aData)
{
	TRM_DebugMemoryInfo info(aAddress, aLength, &aData);
	return DoControl(EControlReadMemory, reinterpret_cast<TAny*>(aThreadId), (TAny*)&info);
}

inline TInt RRM_DebugDriver::WriteMemory(const TUint32 aThreadId, const TUint32 aAddress, const TUint32 aLength, const TDesC8 &aData)
{
	TRM_DebugMemoryInfo info(aAddress, aLength, (TDesC8*)&aData);
	return DoControl(EControlWriteMemory, reinterpret_cast<TAny*>(aThreadId), (TAny*)&info);
}

inline TInt RRM_DebugDriver::ReadRegisters(const TUint32 aThreadId, const TDes8 &aRegisterIds, TDes8 &aRegisterValues, TDes8 &aRegisterFlags)
	{
	TRM_DebugRegisterInformation info(&aRegisterIds, &aRegisterValues, &aRegisterFlags);
	return DoControl(EControlReadRegisters, reinterpret_cast<TAny*>(aThreadId), (TAny*)&info);	
	}

inline TInt RRM_DebugDriver::WriteRegisters(const TUint32 aThreadId, const TDes8 &aRegisterIds, const TDes8 &aRegisterValues, TDes8 &aRegisterFlags)
	{
	TRM_DebugRegisterInformation info(&aRegisterIds, (TDes8*)&aRegisterValues, &aRegisterFlags);
	return DoControl(EControlWriteRegisters, reinterpret_cast<TAny*>(aThreadId), (TAny*)&info);
	}

inline TInt RRM_DebugDriver::ReadRegisters(const TUint32 aThreadId, const TInt32 aFirstRegister, const TInt32 aLastRegister, TDes8 &aValues)
{
	TRM_DebugRegisterInfo info(aFirstRegister, aLastRegister, &aValues);
	return DoControl(EControlReadRegistersLegacy, reinterpret_cast<TAny*>(aThreadId), (TAny*)&info);
}

inline TInt RRM_DebugDriver::WriteRegisters(const TUint32 aThreadId, const TInt32 aFirstRegister, const TInt32 aLastRegister, TDesC8 &aValues)
{
	TRM_DebugRegisterInfo info(aFirstRegister, aLastRegister, &aValues);
	return DoControl(EControlWriteRegistersLegacy, reinterpret_cast<TAny*>(aThreadId), (TAny*)&info);
}

inline void RRM_DebugDriver::GetEvent(TDesC8& aProcessName, TUint64 aAgentId, TRequestStatus &aStatus, Debug::TEventInfo &aEventInfo)
{
	// temporary object not needed beyond the DoRequest call
	TEventMetaData eventMetaData;
	eventMetaData.iTargetProcessName.Copy(aProcessName);
	eventMetaData.iDebugAgentProcessId = aAgentId;
	DoRequest(ERequestGetEvent, aStatus, (TAny*)&aEventInfo, (TAny*)&eventMetaData);
}

inline void RRM_DebugDriver::CancelGetEvent(TDesC8& aProcessName, TUint64 aAgentId)
{
	TRMD_DebugCancelInfo info(ERequestGetEventCancel,aProcessName,aAgentId);
	DoCancel(reinterpret_cast<TInt>(&info));
}

inline TInt RRM_DebugDriver::GetStaticLibraryInfo(const TInt aIndex, Debug::TEventInfo &aInfo)
{
	return DoControl(EControlGetStaticLibraryInfo, reinterpret_cast<TAny*>(aIndex), (TAny*)&aInfo);
}

inline TInt RRM_DebugDriver::GetDebugFunctionalityBufSize(TUint32 &aBufSize)
{
	return DoControl(EControlGetDebugFunctionalityBufSize, reinterpret_cast<TAny*>(&aBufSize));
}

inline TInt RRM_DebugDriver::GetDebugFunctionality(TDes8& aDebugFunctionality)
{
	return DoControl(EControlGetDebugFunctionality,reinterpret_cast<TAny*>(&aDebugFunctionality));
}

inline TInt RRM_DebugDriver::GetMemoryOperationMaxBlockSize(TUint32 &aMaxSize)
{
	return DoControl(EControlGetMemoryOperationMaxBlockSize, reinterpret_cast<TAny*>(&aMaxSize));
}

inline TInt RRM_DebugDriver::AttachProcess(TDesC8& aProcessName, TUint64 aAgentId)
{
	TRM_DebugAgentId info(aAgentId);
	return DoControl(EControlAttachProcess,reinterpret_cast<TAny*>(&aProcessName),reinterpret_cast<TAny*>(&info));
}

inline TInt RRM_DebugDriver::DetachProcess(TDesC8& aProcessName, TUint64 aAgentId)
{
	TRM_DebugAgentId info(aAgentId);
	return DoControl(EControlDetachProcess,reinterpret_cast<TAny*>(&aProcessName),reinterpret_cast<TAny*>(&info));
}

inline TInt RRM_DebugDriver::DetachAgent(TUint64 aAgentId)
{
	TRM_DebugAgentId info(aAgentId);
	return DoControl(EControlDetachAgent,reinterpret_cast<TAny*>(&info),0);
}

inline TInt RRM_DebugDriver::SetEventAction(TDesC8& aProcessName, Debug::TEventType aEvent, Debug::TKernelEventAction aEventAction, TUint64 aAgentId)
{
	TRM_DebugEventActionInfo info (aEvent,aEventAction, aAgentId);
	return DoControl(EControlSetEventAction,reinterpret_cast<TAny*>(&aProcessName),(TAny*)&info);
}

inline TInt RRM_DebugDriver::GetList(const Debug::TListId aType, const Debug::TListScope aListScope, const TUint64 aTargetId, const TUint64 aDebugProcessId, TDes8& aBuffer, TUint32& aDataSize)
{
	TListInformation info(aType, aListScope, &aBuffer, &aDataSize, aTargetId);
	return DoControl(EControlGetList, (TAny*)&info);
}

inline TInt RRM_DebugDriver::Step(const TUint32 aThreadId, const TUint32 aNumSteps)
{
	return DoControl(EControlStep,reinterpret_cast<TAny*>(aThreadId),reinterpret_cast<TAny*>(aNumSteps));
}

inline TInt RRM_DebugDriver::IsDebuggable(const TUint32 aProcessId)
{
	return DoControl(EControlIsDebuggable,reinterpret_cast<TAny*>(aProcessId),NULL);
}

inline TInt RRM_DebugDriver::KillProcess(const TUint32 aProcessId, const TInt32 aReason)
{
	return DoControl(EControlKillProcess,reinterpret_cast<TAny*>(aProcessId),reinterpret_cast<TAny*>(aReason));
}

#endif // __RM_DEBUG_KERNELDRIVER_H__

