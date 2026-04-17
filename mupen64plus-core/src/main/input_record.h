/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-core - input_record.h                                      *
 *                                                                         *
 *   Lightweight input recording / playback. Records each call to the      *
 *   input plugin's getKeys() as a sequential index, making replay         *
 *   deterministic when paired with the same starting state.               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef M64P_MAIN_INPUT_RECORD_H
#define M64P_MAIN_INPUT_RECORD_H

#include "api/m64p_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exported API — called from the GUI via dynlib lookup */
EXPORT m64p_error CALL InputRecordStart(const char *filename);
EXPORT m64p_error CALL InputRecordStop(void);
EXPORT m64p_error CALL InputPlaybackStart(const char *filename);
EXPORT m64p_error CALL InputPlaybackStop(void);
EXPORT int         CALL InputRecordIsRecording(void);
EXPORT int         CALL InputRecordIsPlayingBack(void);
EXPORT uint32_t    CALL InputRecordGetIndex(void);

/* Internal API — called from the input plugin compat layer.
 * Given the live input value polled from the plugin, returns the
 * value that should actually be sent to the game. While recording
 * this is a pass-through (plus a file write); while playing back
 * it returns the stored value for this poll index. */
uint32_t input_record_process(int controller_id, uint32_t live_input);

/* Called from main when a ROM stops so we flush any open file. */
void input_record_on_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* M64P_MAIN_INPUT_RECORD_H */
