/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-core - input_record.c                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define M64P_CORE_PROTOTYPES 1

#include "input_record.h"

#include "api/callbacks.h"
#include "api/m64p_types.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static FILE *s_record_file = NULL;
static FILE *s_play_file = NULL;
static uint32_t s_poll_index = 0;

#define REC_HEADER "# simple64-input-record v1\n# index\tcontroller\tbuttons_hex\n"

static void close_record(void)
{
    if (s_record_file) {
        fflush(s_record_file);
        fclose(s_record_file);
        s_record_file = NULL;
    }
}

static void close_play(void)
{
    if (s_play_file) {
        fclose(s_play_file);
        s_play_file = NULL;
    }
}

EXPORT m64p_error CALL InputRecordStart(const char *filename)
{
    if (!filename) return M64ERR_INPUT_ASSERT;
    close_record();
    close_play();
    s_record_file = fopen(filename, "w");
    if (!s_record_file) {
        DebugMessage(M64MSG_ERROR, "InputRecord: cannot open '%s' for writing", filename);
        return M64ERR_FILES;
    }
    fputs(REC_HEADER, s_record_file);
    s_poll_index = 0;
    DebugMessage(M64MSG_INFO, "InputRecord: recording to '%s'", filename);
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL InputRecordStop(void)
{
    close_record();
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL InputPlaybackStart(const char *filename)
{
    if (!filename) return M64ERR_INPUT_ASSERT;
    close_record();
    close_play();
    s_play_file = fopen(filename, "r");
    if (!s_play_file) {
        DebugMessage(M64MSG_ERROR, "InputRecord: cannot open '%s' for reading", filename);
        return M64ERR_FILES;
    }
    /* Validate magic on first non-comment line or skip comments entirely */
    char peek[128];
    long start = ftell(s_play_file);
    int looks_ok = 0;
    if (fgets(peek, sizeof(peek), s_play_file)) {
        if (strncmp(peek, "# simple64-input-record", 23) == 0)
            looks_ok = 1;
    }
    if (!looks_ok) {
        /* Allow headerless files — just rewind. */
        fseek(s_play_file, start, SEEK_SET);
    }
    s_poll_index = 0;
    DebugMessage(M64MSG_INFO, "InputRecord: playing back '%s'", filename);
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL InputPlaybackStop(void)
{
    close_play();
    return M64ERR_SUCCESS;
}

EXPORT int CALL InputRecordIsRecording(void)     { return s_record_file != NULL; }
EXPORT int CALL InputRecordIsPlayingBack(void)   { return s_play_file   != NULL; }
EXPORT uint32_t CALL InputRecordGetIndex(void)   { return s_poll_index; }

void input_record_on_stop(void)
{
    close_record();
    close_play();
    s_poll_index = 0;
}

/* Read the next data line from the playback file.
 * Returns 1 and fills *out_value on success, 0 on EOF/error.
 * Format: "INDEX<ws>CONTROLLER<ws>BUTTONS_HEX" — only buttons is used. */
static int play_read_next(uint32_t *out_value)
{
    char line[256];
    while (s_play_file && fgets(line, sizeof(line), s_play_file)) {
        /* skip comments / blanks */
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0')
            continue;
        unsigned idx = 0, ctrl = 0, value = 0;
        if (sscanf(line, "%u %u %x", &idx, &ctrl, &value) == 3) {
            *out_value = value;
            return 1;
        }
    }
    return 0;
}

uint32_t input_record_process(int controller_id, uint32_t live_input)
{
    uint32_t result = live_input;

    if (s_play_file) {
        uint32_t v;
        if (play_read_next(&v)) {
            result = v;
        } else {
            /* end of file — stop playback and fall through to live input */
            close_play();
            DebugMessage(M64MSG_INFO, "InputRecord: playback finished at index %u", s_poll_index);
            result = live_input;
        }
    }

    if (s_record_file) {
        fprintf(s_record_file, "%u\t%d\t%08X\n",
                s_poll_index, controller_id, (unsigned)result);
    }

    s_poll_index++;
    return result;
}
