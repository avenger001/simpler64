#include "core_commands.h"

ptr_CoreStartup CoreStartup = nullptr;
ptr_CoreShutdown CoreShutdown = nullptr;
ptr_CoreDoCommand CoreDoCommand = nullptr;
ptr_CoreAttachPlugin CoreAttachPlugin = nullptr;
ptr_CoreDetachPlugin CoreDetachPlugin = nullptr;
ptr_CoreOverrideVidExt CoreOverrideVidExt = nullptr;

ptr_ConfigGetUserConfigPath ConfigGetUserConfigPath = nullptr;
ptr_ConfigSaveFile ConfigSaveFile = nullptr;
ptr_ConfigGetParameterHelp ConfigGetParameterHelp = nullptr;
ptr_ConfigGetParamInt ConfigGetParamInt = nullptr;
ptr_ConfigGetParamFloat ConfigGetParamFloat = nullptr;
ptr_ConfigGetParamBool ConfigGetParamBool = nullptr;
ptr_ConfigGetParamString ConfigGetParamString = nullptr;
ptr_ConfigSetParameter ConfigSetParameter = nullptr;
ptr_ConfigDeleteSection ConfigDeleteSection = nullptr;
ptr_ConfigOpenSection ConfigOpenSection = nullptr;
ptr_ConfigSaveSection ConfigSaveSection = nullptr;
ptr_ConfigListParameters ConfigListParameters = nullptr;
ptr_ConfigGetSharedDataFilepath ConfigGetSharedDataFilepath = nullptr;

ptr_CoreAddCheat CoreAddCheat = nullptr;

ptr_DebugSetCallbacks DebugSetCallbacks = nullptr;
ptr_DebugSetRunState DebugSetRunState = nullptr;
ptr_DebugGetState DebugGetState = nullptr;
ptr_DebugStep DebugStep = nullptr;
ptr_DebugDecodeOp DebugDecodeOp = nullptr;
ptr_DebugMemGetPointer DebugMemGetPointer = nullptr;
ptr_DebugMemRead64 DebugMemRead64 = nullptr;
ptr_DebugMemRead32 DebugMemRead32 = nullptr;
ptr_DebugMemRead16 DebugMemRead16 = nullptr;
ptr_DebugMemRead8 DebugMemRead8 = nullptr;
ptr_DebugMemWrite64 DebugMemWrite64 = nullptr;
ptr_DebugMemWrite32 DebugMemWrite32 = nullptr;
ptr_DebugMemWrite16 DebugMemWrite16 = nullptr;
ptr_DebugMemWrite8 DebugMemWrite8 = nullptr;
ptr_DebugGetCPUDataPtr DebugGetCPUDataPtr = nullptr;
ptr_DebugBreakpointLookup DebugBreakpointLookup = nullptr;
ptr_DebugBreakpointCommand DebugBreakpointCommand = nullptr;
ptr_DebugBreakpointTriggeredBy DebugBreakpointTriggeredBy = nullptr;
ptr_DebugVirtualToPhysical DebugVirtualToPhysical = nullptr;

ptr_InputRecordStart InputRecordStart = nullptr;
ptr_InputRecordStop InputRecordStop = nullptr;
ptr_InputPlaybackStart InputPlaybackStart = nullptr;
ptr_InputPlaybackStop InputPlaybackStop = nullptr;
ptr_InputRecordIsRecording InputRecordIsRecording = nullptr;
ptr_InputRecordIsPlayingBack InputRecordIsPlayingBack = nullptr;
ptr_InputRecordGetIndex InputRecordGetIndex = nullptr;
