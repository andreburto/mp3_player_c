#include "../include/player.h"

#include <stdio.h>
#include <wchar.h>

/* MCI alias used to refer to the open MP3 device for every command. */
static const wchar_t PLAYER_ALIAS[] = L"mp3player_device";

/* Maximum length (in wchar_t) of an MCI command string, including the
 * quoted file path. */
#define PLAYER_MAX_COMMAND_LENGTH 600

/* Maximum length (in wchar_t) of an MCI error message string. */
#define PLAYER_MAX_ERROR_LENGTH 256

/* Maximum length (in wchar_t) of an MCI status query result string. */
#define PLAYER_MAX_STATUS_LENGTH 32

/* Number of milliseconds Player_SkipBack/Player_SkipForward move the
 * playback position by on each call. */
#define PLAYER_SKIP_MS (PLAYER_SKIP_SECONDS * 1000UL)

/* Tracks whether an MP3 file is currently open in MCI. */
static BOOL g_isLoaded = FALSE;

/* File name (without path) of the currently loaded file, valid only
 * while g_isLoaded is TRUE. */
static wchar_t g_fileName[MAX_PATH] = L"";

/* Current Loop preference, set via Player_SetLoop. See the comment on
 * Player_SetLoop in player.h for what this flag does (and does not)
 * affect. */
static BOOL g_isLooping = FALSE;

/* Path of the temporary ID3v2-tag-stripped copy created by
 * Player_StripId3v2Tag for the currently loaded file, or an empty string
 * if no temporary copy is in use. Deleted by Player_Cleanup. */
static wchar_t g_tempFilePath[MAX_PATH] = L"";

/* Returns a pointer into filePath just past its last path separator,
 * i.e. the file name component. */
static LPCWSTR Player_GetFileNameFromPath(LPCWSTR filePath)
{
    LPCWSTR lastBackslash = wcsrchr(filePath, L'\\');
    LPCWSTR lastSlash = wcsrchr(filePath, L'/');
    LPCWSTR lastSeparator = lastBackslash;

    if (lastSlash != NULL && (lastSeparator == NULL || lastSlash > lastSeparator))
    {
        lastSeparator = lastSlash;
    }

    return lastSeparator != NULL ? lastSeparator + 1 : filePath;
}

/* Reports an MCI error to the user via a message box, translating the
 * numeric error code into a human-readable message when possible. */
static void Player_ReportError(HWND ownerWindow, MCIERROR errorCode, LPCWSTR context)
{
    wchar_t errorText[PLAYER_MAX_ERROR_LENGTH];
    wchar_t message[PLAYER_MAX_ERROR_LENGTH + 128];

    if (!mciGetErrorStringW(errorCode, errorText, PLAYER_MAX_ERROR_LENGTH))
    {
        wcscpy_s(errorText, PLAYER_MAX_ERROR_LENGTH, L"Unknown MCI error.");
    }

    swprintf_s(message, ARRAYSIZE(message), L"%s: %s", context, errorText);
    MessageBoxW(ownerWindow, message, L"MP3 Player Error", MB_OK | MB_ICONERROR);
}

/* Sends "status PLAYER_ALIAS item" and returns the result parsed as an
 * unsigned integer, or 0 if no file is loaded or the query fails. Used for
 * the "position" and "length" status items, which are both reported in
 * milliseconds (see the "set ... time format milliseconds" command issued
 * by Player_Load). */
static DWORD Player_QueryStatusDWord(LPCWSTR item)
{
    wchar_t command[PLAYER_MAX_COMMAND_LENGTH];
    wchar_t resultText[PLAYER_MAX_STATUS_LENGTH] = L"0";

    if (!g_isLoaded)
    {
        return 0;
    }

    swprintf_s(command, ARRAYSIZE(command), L"status %s %s", PLAYER_ALIAS, item);
    mciSendStringW(command, resultText, ARRAYSIZE(resultText), NULL);

    return wcstoul(resultText, NULL, 10);
}

/* Returns TRUE if the currently loaded file is actively playing (as opposed
 * to stopped or paused). */
static BOOL Player_IsPlaying(void)
{
    wchar_t command[PLAYER_MAX_COMMAND_LENGTH];
    wchar_t modeText[PLAYER_MAX_STATUS_LENGTH] = L"";

    if (!g_isLoaded)
    {
        return FALSE;
    }

    swprintf_s(command, ARRAYSIZE(command), L"status %s mode", PLAYER_ALIAS);
    mciSendStringW(command, modeText, ARRAYSIZE(modeText), NULL);

    return wcscmp(modeText, L"playing") == 0;
}

/* Repositions playback to positionMs and, if resumePlaying is TRUE, resumes
 * looped playback from the new position. Returns TRUE on success, FALSE if
 * MCI reports an error. */
static BOOL Player_SeekToPosition(HWND ownerWindow, DWORD positionMs, BOOL resumePlaying)
{
    wchar_t command[PLAYER_MAX_COMMAND_LENGTH];
    MCIERROR errorCode;

    swprintf_s(command, ARRAYSIZE(command), L"seek %s to %lu", PLAYER_ALIAS, positionMs);
    errorCode = mciSendStringW(command, NULL, 0, NULL);
    if (errorCode != 0)
    {
        Player_ReportError(ownerWindow, errorCode, L"Failed to seek");
        return FALSE;
    }

    if (resumePlaying)
    {
        swprintf_s(command, ARRAYSIZE(command), L"play %s repeat", PLAYER_ALIAS);
        mciSendStringW(command, NULL, 0, NULL);
    }

    return TRUE;
}

/* Some MP3 files carry an unusually large ID3v2 tag (e.g. Amazon's UITS
 * rights-management block, which can pad the tag out to hundreds of KB).
 * The legacy MPEGVideo MCI driver can fail to scan past such a large tag
 * to find the start of the audio stream (MCI reports a generic "problem
 * occurred in initializing MCI" error). If filePath begins with an ID3v2
 * tag, this writes a temporary copy of the file's audio data (with the
 * tag removed) to tempFilePath and returns TRUE; returns FALSE (leaving
 * tempFilePath untouched) if there's no ID3v2 tag, or if the copy fails
 * for any reason. tempFilePath must have room for at least MAX_PATH
 * characters, per GetTempFileNameW's requirement. */
static BOOL Player_StripId3v2Tag(LPCWSTR filePath, LPWSTR tempFilePath)
{
    HANDLE sourceFile;
    BYTE header[10];
    DWORD bytesRead;
    DWORD tagSize;
    wchar_t tempDirectory[MAX_PATH];
    HANDLE destFile;
    BOOL succeeded = FALSE;

    sourceFile = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (sourceFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if (!ReadFile(sourceFile, header, sizeof(header), &bytesRead, NULL)
        || bytesRead < sizeof(header)
        || header[0] != 'I' || header[1] != 'D' || header[2] != '3'
        || (header[6] & 0x80) || (header[7] & 0x80) || (header[8] & 0x80) || (header[9] & 0x80))
    {
        CloseHandle(sourceFile);
        return FALSE;
    }

    /* The tag's size is a 28-bit "synchsafe" integer: 7 significant bits
     * per byte. It excludes the 10-byte header itself and the optional
     * 10-byte footer. */
    tagSize = ((DWORD)header[6] << 21) | ((DWORD)header[7] << 14)
        | ((DWORD)header[8] << 7) | (DWORD)header[9];
    tagSize += sizeof(header);
    if (header[5] & 0x10)
    {
        tagSize += 10;
    }

    if (SetFilePointer(sourceFile, (LONG)tagSize, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        CloseHandle(sourceFile);
        return FALSE;
    }

    if (GetTempPathW(ARRAYSIZE(tempDirectory), tempDirectory) == 0
        || GetTempFileNameW(tempDirectory, L"mp3", 0, tempFilePath) == 0)
    {
        CloseHandle(sourceFile);
        return FALSE;
    }

    destFile = CreateFileW(tempFilePath, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (destFile != INVALID_HANDLE_VALUE)
    {
        BYTE buffer[65536];
        BOOL copyFailed = FALSE;

        for (;;)
        {
            DWORD bytesWritten;

            if (!ReadFile(sourceFile, buffer, sizeof(buffer), &bytesRead, NULL))
            {
                copyFailed = TRUE;
                break;
            }
            if (bytesRead == 0)
            {
                break;
            }
            if (!WriteFile(destFile, buffer, bytesRead, &bytesWritten, NULL) || bytesWritten != bytesRead)
            {
                copyFailed = TRUE;
                break;
            }
        }

        CloseHandle(destFile);
        succeeded = !copyFailed;
    }

    CloseHandle(sourceFile);

    if (!succeeded)
    {
        DeleteFileW(tempFilePath);
    }

    return succeeded;
}

BOOL Player_Load(HWND ownerWindow, LPCWSTR filePath)
{
    wchar_t command[PLAYER_MAX_COMMAND_LENGTH];
    wchar_t tempFilePath[MAX_PATH] = L"";
    MCI_OPEN_PARMSW openParams = { 0 };
    MCIERROR errorCode;

    /* Closing any previously loaded file is how the user switches songs. */
    Player_Cleanup();

    /* mciSendStringW's "open" command string is parsed as space-delimited
     * tokens, and even a quoted file path can trip up that parser (e.g.
     * MCIERR_FILE_NOT_FOUND for some paths containing spaces). Using
     * mciSendCommandW's MCI_OPEN instead passes the path as a raw struct
     * field rather than part of a parsed string, avoiding the issue
     * entirely. The alias registered here still works with every other
     * mciSendStringW command in this file, since MCI aliases are shared
     * across both interfaces. */
    openParams.lpstrDeviceType = L"MPEGVideo";
    openParams.lpstrElementName = filePath;
    openParams.lpstrAlias = PLAYER_ALIAS;

    errorCode = mciSendCommandW(0, MCI_OPEN,
        MCI_OPEN_TYPE | MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS, (DWORD_PTR)&openParams);

    if (errorCode != 0 && Player_StripId3v2Tag(filePath, tempFilePath))
    {
        openParams.lpstrElementName = tempFilePath;
        errorCode = mciSendCommandW(0, MCI_OPEN,
            MCI_OPEN_TYPE | MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS, (DWORD_PTR)&openParams);
    }

    if (errorCode != 0)
    {
        if (tempFilePath[0] != L'\0')
        {
            DeleteFileW(tempFilePath);
        }
        Player_ReportError(ownerWindow, errorCode, L"Failed to load the selected file");
        return FALSE;
    }

    wcscpy_s(g_fileName, ARRAYSIZE(g_fileName), Player_GetFileNameFromPath(filePath));
    wcscpy_s(g_tempFilePath, ARRAYSIZE(g_tempFilePath), tempFilePath);
    g_isLoaded = TRUE;

    /* Query/seek results are parsed as plain integers, so force milliseconds
     * explicitly rather than relying on the device's default time format. */
    swprintf_s(command, ARRAYSIZE(command), L"set %s time format milliseconds", PLAYER_ALIAS);
    mciSendStringW(command, NULL, 0, NULL);

    return TRUE;
}

BOOL Player_Play(HWND ownerWindow)
{
    wchar_t command[PLAYER_MAX_COMMAND_LENGTH];
    MCIERROR errorCode;

    if (!g_isLoaded)
    {
        MessageBoxW(ownerWindow, L"Load a song before pressing Play.",
            L"MP3 Player", MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    /* "repeat" makes MCI loop the song indefinitely until a stop command
     * is issued. */
    swprintf_s(command, ARRAYSIZE(command), L"play %s repeat", PLAYER_ALIAS);
    errorCode = mciSendStringW(command, NULL, 0, NULL);
    if (errorCode != 0)
    {
        Player_ReportError(ownerWindow, errorCode, L"Failed to play the song");
        return FALSE;
    }

    return TRUE;
}

BOOL Player_Stop(HWND ownerWindow)
{
    wchar_t command[PLAYER_MAX_COMMAND_LENGTH];
    MCIERROR errorCode;

    if (!g_isLoaded)
    {
        return FALSE;
    }

    swprintf_s(command, ARRAYSIZE(command), L"stop %s", PLAYER_ALIAS);
    errorCode = mciSendStringW(command, NULL, 0, NULL);
    if (errorCode != 0)
    {
        Player_ReportError(ownerWindow, errorCode, L"Failed to stop the song");
        return FALSE;
    }

    /* Rewind so the next Play starts from the beginning of the song. */
    swprintf_s(command, ARRAYSIZE(command), L"seek %s to start", PLAYER_ALIAS);
    mciSendStringW(command, NULL, 0, NULL);

    return TRUE;
}

BOOL Player_SkipBack(HWND ownerWindow)
{
    DWORD position;

    if (!g_isLoaded)
    {
        return FALSE;
    }

    position = Player_QueryStatusDWord(L"position");

    return Player_SeekToPosition(ownerWindow,
        (position > PLAYER_SKIP_MS) ? position - PLAYER_SKIP_MS : 0,
        Player_IsPlaying());
}

BOOL Player_SkipForward(HWND ownerWindow)
{
    DWORD position;
    DWORD duration;
    DWORD newPosition;

    if (!g_isLoaded)
    {
        return FALSE;
    }

    position = Player_QueryStatusDWord(L"position");
    duration = Player_QueryStatusDWord(L"length");

    newPosition = position + PLAYER_SKIP_MS;
    if (duration > 0 && newPosition > duration)
    {
        newPosition = duration;
    }

    return Player_SeekToPosition(ownerWindow, newPosition, Player_IsPlaying());
}

void Player_SetLoop(BOOL loop)
{
    g_isLooping = loop;
}

BOOL Player_GetLoop(void)
{
    return g_isLooping;
}

DWORD Player_GetDurationMs(void)
{
    return Player_QueryStatusDWord(L"length");
}

DWORD Player_GetPositionMs(void)
{
    return Player_QueryStatusDWord(L"position");
}

BOOL Player_IsLoaded(void)
{
    return g_isLoaded;
}

LPCWSTR Player_GetFileName(void)
{
    return g_isLoaded ? g_fileName : L"";
}

void Player_Cleanup(void)
{
    wchar_t command[PLAYER_MAX_COMMAND_LENGTH];

    if (!g_isLoaded)
    {
        return;
    }

    swprintf_s(command, ARRAYSIZE(command), L"close %s", PLAYER_ALIAS);
    mciSendStringW(command, NULL, 0, NULL);
    g_isLoaded = FALSE;

    if (g_tempFilePath[0] != L'\0')
    {
        DeleteFileW(g_tempFilePath);
        g_tempFilePath[0] = L'\0';
    }
}
