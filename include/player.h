#ifndef PLAYER_H
#define PLAYER_H

#include <windows.h>

/*
 * Playback module interface.
 *
 * Wraps the Windows Multimedia Control Interface (MCI) to load, play
 * (looped), and stop a single MP3 file at a time. Loading a new file
 * automatically closes any previously loaded file, which is how the
 * caller switches songs.
 */

/* Loads (opens) the MP3 file at filePath, closing any previously loaded
 * file first. ownerWindow receives any MCI error message boxes.
 * Returns TRUE on success, FALSE on failure. */
BOOL Player_Load(HWND ownerWindow, LPCWSTR filePath);

/* Starts looped playback of the currently loaded file from the beginning.
 * Returns TRUE on success, FALSE if no file is loaded or MCI reports an error. */
BOOL Player_Play(HWND ownerWindow);

/* Stops playback of the currently loaded file and rewinds it to the start
 * so the next Player_Play call starts from the beginning.
 * Returns TRUE on success, FALSE if no file is loaded or MCI reports an error. */
BOOL Player_Stop(HWND ownerWindow);

/* Number of seconds Player_SkipBack/Player_SkipForward move the playback
 * position by on each call. */
#define PLAYER_SKIP_SECONDS 5

/* Moves the playback position back/forward by PLAYER_SKIP_SECONDS seconds,
 * clamped to the start/end of the file. If the song is currently playing,
 * playback resumes from the new position. Returns TRUE on success, FALSE
 * if no file is loaded or MCI reports an error. */
BOOL Player_SkipBack(HWND ownerWindow);
BOOL Player_SkipForward(HWND ownerWindow);

/* Enables or disables the Loop preference. This does not change how
 * Player_Play itself behaves (it already repeats the song indefinitely
 * via MCI's "repeat" option); the flag is tracked purely so callers can
 * display the current Loop on/off state (e.g. in a status bar). */
void Player_SetLoop(BOOL loop);

/* Returns the current Loop preference set via Player_SetLoop. */
BOOL Player_GetLoop(void);

/* Returns the total length of the currently loaded file, in milliseconds,
 * or 0 if no file is loaded or the query fails. */
DWORD Player_GetDurationMs(void);

/* Returns the current playback position of the currently loaded file, in
 * milliseconds, or 0 if no file is loaded or the query fails. */
DWORD Player_GetPositionMs(void);

/* Returns TRUE if a file is currently loaded (open in MCI). */
BOOL Player_IsLoaded(void);

/* Returns the file name (without path) of the currently loaded file, or
 * an empty string if no file is loaded. The returned pointer is valid
 * until the next call to Player_Load or Player_Cleanup. */
LPCWSTR Player_GetFileName(void);

/* Closes the MCI device, if open, and releases its resources. Safe to call
 * even if nothing is loaded. Should be called once before the application exits. */
void Player_Cleanup(void);

#endif /* PLAYER_H */
