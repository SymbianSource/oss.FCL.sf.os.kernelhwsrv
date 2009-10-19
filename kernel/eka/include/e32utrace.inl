/**
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Trace API
* ---------------TTraceContext-----------------------
*
*/




/**
 @file
 @publishedPartner
 @prototype
 
 Define the context of a trace packet by setting its attributes.
 
 The module UID is defaulted according to the FW_DEFAULT_MODULEUID definition.
 The HasThreadIdentification is defaulted to the FW_DEFAULT_HAS_THREAD_IDENTIFICATION definition.
 The HasProgramCounter is defaulted to the FW_DEFAULT_HAS_PC definition.
 
 @param aClassification 	@see TClassification
 
*/
TTraceContext::TTraceContext(const TClassification aClassification)
:iModuleUid(FW_DEFAULT_MODULEUID), iClassification(aClassification), iHasThreadIdentification(FW_DEFAULT_HAS_THREAD_IDENTIFICATION), iHasProgramCounter(FW_DEFAULT_HAS_PC), iReserved1(0), iReserved2(0)
	{
	}

/**
 * Define the context of a trace packet by setting its attributes.
 *
 * The HasThreadIdentification is defaulted to the FW_DEFAULT_HAS_THREAD_IDENTIFICATION definition.
 * The HasProgramCounter is defaulted to the FW_DEFAULT_HAS_PC definition.
 * 
 * @param aModuleUid		@see TModuleUid
 * @param aClassification	@see TClassification
 */
TTraceContext::TTraceContext(const TModuleUid aModuleUid, const TClassification aClassification)
:iModuleUid(aModuleUid), iClassification(aClassification), iHasThreadIdentification(FW_DEFAULT_HAS_THREAD_IDENTIFICATION), iHasProgramCounter(FW_DEFAULT_HAS_PC), iReserved1(0), iReserved2(0)
	{
	}

/**
 * Define the context of a trace packet by setting its attributes.
 * 
 * The module UID is defaulted according to the FW_DEFAULT_MODULEUID definition.
 * 
 * @param aClassification 	@see TClassification
 * @param aHasThreadIdentification	Set whether to add thread identification automatically in the trace packet.
 * @param aHasProgramCounter			Set whether to add PC (program counter) automatically in the trace packet.
 */
TTraceContext::TTraceContext(const TClassification aClassification, const THasThreadIdentification aHasThreadIdentification, const THasProgramCounter aHasProgramCounter)
:iModuleUid(FW_DEFAULT_MODULEUID), iClassification(aClassification), iHasThreadIdentification(aHasThreadIdentification), iHasProgramCounter(aHasProgramCounter), iReserved1(0), iReserved2(0)
	{
	}


/**
 * Define the context of a trace packet by setting its attributes.
 *
 * @param aModuleUid 		@see TModuleUid
 * @param aClassification 	@see TClassification
 * @param aHasThreadIdentification	Set whether to add thread identification automatically in the trace packet.
 * @param aHasProgramCounter		Set whether to add PC (program counter) automatically in the trace packet.
 */
TTraceContext::TTraceContext(const TModuleUid aModuleUid, const TClassification aClassification, const THasThreadIdentification aHasThreadIdentification, const THasProgramCounter aHasProgramCounter)
:iModuleUid(aModuleUid), iClassification(aClassification), iHasThreadIdentification(aHasThreadIdentification), iHasProgramCounter(aHasProgramCounter), iReserved1(0), iReserved2(0)
	{
	}

//------------------ Trace -----------------------
/**
Outputs a trace packet containing variable length data.

If the specified data is too big to fit into a single
trace record a multipart trace is generated.

@param aContext 	Attributes of the trace point. 
@param aFormatId	A format identifier as specified by @see TFormatId
@param aData		Additional data to add to trace packet.
					Must be word aligned, i.e. a multiple of 4.

@return 			The trace packet was/was not logged.

@See BTrace::TMultipart
*/
template<typename T>
TBool Trace(const TTraceContext& aContext, TFormatId aFormatId, const T& aData)
	{
	return Trace(aContext, aFormatId, &aData, sizeof(aData));
	}

