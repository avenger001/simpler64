#ifndef __CORE_COMMANDS_H__
#define __CORE_COMMANDS_H__

#include "m64p_frontend.h"
#include "m64p_config.h"
#include "m64p_debugger.h"
#include <stdint.h>

extern ptr_CoreStartup CoreStartup;
extern ptr_CoreShutdown CoreShutdown;
extern ptr_CoreDoCommand CoreDoCommand;
extern ptr_CoreAttachPlugin CoreAttachPlugin;
extern ptr_CoreDetachPlugin CoreDetachPlugin;
extern ptr_CoreOverrideVidExt CoreOverrideVidExt;

extern ptr_ConfigGetUserConfigPath ConfigGetUserConfigPath;
extern ptr_ConfigSaveFile ConfigSaveFile;
extern ptr_ConfigGetParameterHelp ConfigGetParameterHelp;
extern ptr_ConfigGetParamInt ConfigGetParamInt;
extern ptr_ConfigGetParamFloat ConfigGetParamFloat;
extern ptr_ConfigGetParamBool ConfigGetParamBool;
extern ptr_ConfigGetParamString ConfigGetParamString;
extern ptr_ConfigSetParameter ConfigSetParameter;
extern ptr_ConfigDeleteSection ConfigDeleteSection;
extern ptr_ConfigOpenSection ConfigOpenSection;
extern ptr_ConfigSaveSection ConfigSaveSection;
extern ptr_ConfigListParameters ConfigListParameters;
extern ptr_ConfigGetSharedDataFilepath ConfigGetSharedDataFilepath;

extern ptr_CoreAddCheat CoreAddCheat;

extern ptr_DebugSetCallbacks DebugSetCallbacks;
extern ptr_DebugSetRunState DebugSetRunState;
extern ptr_DebugGetState DebugGetState;
extern ptr_DebugStep DebugStep;
extern ptr_DebugDecodeOp DebugDecodeOp;
extern ptr_DebugMemGetPointer DebugMemGetPointer;
extern ptr_DebugMemRead64 DebugMemRead64;
extern ptr_DebugMemRead32 DebugMemRead32;
extern ptr_DebugMemRead16 DebugMemRead16;
extern ptr_DebugMemRead8 DebugMemRead8;
extern ptr_DebugMemWrite64 DebugMemWrite64;
extern ptr_DebugMemWrite32 DebugMemWrite32;
extern ptr_DebugMemWrite16 DebugMemWrite16;
extern ptr_DebugMemWrite8 DebugMemWrite8;
extern ptr_DebugGetCPUDataPtr DebugGetCPUDataPtr;
extern ptr_DebugBreakpointLookup DebugBreakpointLookup;
extern ptr_DebugBreakpointCommand DebugBreakpointCommand;
extern ptr_DebugBreakpointTriggeredBy DebugBreakpointTriggeredBy;
extern ptr_DebugVirtualToPhysical DebugVirtualToPhysical;

/* simple64 input recording / playback — defined in the core, looked up by name. */
typedef m64p_error (*ptr_InputRecordStart)(const char *);
typedef m64p_error (*ptr_InputRecordStop)(void);
typedef m64p_error (*ptr_InputPlaybackStart)(const char *);
typedef m64p_error (*ptr_InputPlaybackStop)(void);
typedef int        (*ptr_InputRecordIsRecording)(void);
typedef int        (*ptr_InputRecordIsPlayingBack)(void);
typedef uint32_t   (*ptr_InputRecordGetIndex)(void);

extern ptr_InputRecordStart InputRecordStart;
extern ptr_InputRecordStop InputRecordStop;
extern ptr_InputPlaybackStart InputPlaybackStart;
extern ptr_InputPlaybackStop InputPlaybackStop;
extern ptr_InputRecordIsRecording InputRecordIsRecording;
extern ptr_InputRecordIsPlayingBack InputRecordIsPlayingBack;
extern ptr_InputRecordGetIndex InputRecordGetIndex;
#endif
