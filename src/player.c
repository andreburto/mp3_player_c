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

/* Tracks whether an MP3 file is currently open in MCI. */
static BOOL g_isLoaded = FALSE;

/* File name (without path) of the currently loaded file, valid only
 * while g_isLoaded is TRUE. */
static wchar_t g_fileName[MAX_PATH] = L"";

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

BOOL Player_Load(HWND ownerWindow, LPCWSTR filePath)
{
    wchar_t command[PLAYER_MAX_COMMAND_LENGTH];
    MCIERROR errorCode;
    int formatResult;

    /* Closing any previously loaded file is how the user switches songs. */
    Player_Cleanup();

    formatResult = swprintf_s(command, ARRAYSIZE(command),
        L"open \"%s\" type MPEGVideo alias %s", filePath, PLAYER_ALIAS);
    if (formatResult <= 0)
    {
        MessageBoxW(ownerWindow, L"The selected file path is too long.",
            L"MP3 Player Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    errorCode = mciSendStringW(command, NULL, 0, NULL);
    if (errorCode != 0)
    {
        Player_ReportError(ownerWindow, errorCode, L"Failed to load the selected file");
        return FALSE;
    }

    wcscpy_s(g_fileName, ARRAYSIZE(g_fileName), Player_GetFileNameFromPath(filePath));
    g_isLoaded = TRUE;
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
}
