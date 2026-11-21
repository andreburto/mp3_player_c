#include "../include/mp3player.h"
#include "../include/player.h"

#include <commdlg.h>
#include <wchar.h>

/* Creates the status label, the two rows of square buttons, the bottom
 * status bar, and the fonts used to render their text, and stores the
 * resulting handles in the window's AppState. */
static void App_CreateControls(HWND window, AppState* state)
{
    const int labelTop = BUTTON_MARGIN;
    const int labelWidth = BUTTON_MARGIN * 4 + BUTTON_SIZE * 3 - BUTTON_MARGIN * 2;
    const int buttonsTop = labelTop + STATUS_LABEL_HEIGHT + BUTTON_MARGIN;
    const int buttonsRow2Top = buttonsTop + BUTTON_SIZE + BUTTON_MARGIN;
    const int statusBarTop = buttonsRow2Top + BUTTON_SIZE + BUTTON_MARGIN;
    const int statusBarLeftWidth = labelWidth / 2;
    HDC deviceContext = GetDC(window);
    const int statusFontHeight = -MulDiv(STATUS_FONT_POINT_SIZE, GetDeviceCaps(deviceContext, LOGPIXELSY), 72);
    ReleaseDC(window, deviceContext);

    state->buttonFont = CreateFontW(
        32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    state->statusFont = CreateFontW(
        statusFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    /* cEscapement/cOrientation of 900 (90.0 degrees, in tenths of a degree)
     * rotates the Loop button's "8" character 90 degrees counterclockwise. */
    state->loopButtonFont = CreateFontW(
        32, 0, 900, 900, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    state->statusLabel = CreateWindowExW(0, L"STATIC", L"Stopped: None",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        BUTTON_MARGIN, labelTop, labelWidth, STATUS_LABEL_HEIGHT,
        window, (HMENU)(UINT_PTR)ID_STATUS_LABEL, NULL, NULL);

    state->loadButton = CreateWindowExW(0, L"BUTTON", L"L",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        BUTTON_MARGIN, buttonsTop, BUTTON_SIZE, BUTTON_SIZE,
        window, (HMENU)(UINT_PTR)ID_BUTTON_LOAD, NULL, NULL);

    state->playButton = CreateWindowExW(0, L"BUTTON", L"P",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        BUTTON_MARGIN * 2 + BUTTON_SIZE, buttonsTop, BUTTON_SIZE, BUTTON_SIZE,
        window, (HMENU)(UINT_PTR)ID_BUTTON_PLAY, NULL, NULL);

    state->stopButton = CreateWindowExW(0, L"BUTTON", L"S",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        BUTTON_MARGIN * 3 + BUTTON_SIZE * 2, buttonsTop, BUTTON_SIZE, BUTTON_SIZE,
        window, (HMENU)(UINT_PTR)ID_BUTTON_STOP, NULL, NULL);

    state->skipBackButton = CreateWindowExW(0, L"BUTTON", L"<<",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        BUTTON_MARGIN, buttonsRow2Top, BUTTON_SIZE, BUTTON_SIZE,
        window, (HMENU)(UINT_PTR)ID_BUTTON_SKIP_BACK, NULL, NULL);

    /* Owner-drawn so its "8" character can be rendered rotated 90 degrees
     * (see App_DrawLoopButton), which a standard BS_PUSHBUTTON cannot do. */
    state->loopButton = CreateWindowExW(0, L"BUTTON", L"8",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        BUTTON_MARGIN * 2 + BUTTON_SIZE, buttonsRow2Top, BUTTON_SIZE, BUTTON_SIZE,
        window, (HMENU)(UINT_PTR)ID_BUTTON_LOOP, NULL, NULL);

    state->skipForwardButton = CreateWindowExW(0, L"BUTTON", L">>",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        BUTTON_MARGIN * 3 + BUTTON_SIZE * 2, buttonsRow2Top, BUTTON_SIZE, BUTTON_SIZE,
        window, (HMENU)(UINT_PTR)ID_BUTTON_SKIP_FORWARD, NULL, NULL);

    state->statusBarLeft = CreateWindowExW(0, L"STATIC", L"0:00 / 0:00",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        BUTTON_MARGIN, statusBarTop, statusBarLeftWidth, STATUS_BAR_HEIGHT,
        window, (HMENU)(UINT_PTR)ID_STATUS_BAR_LEFT, NULL, NULL);

    state->statusBarRight = CreateWindowExW(0, L"STATIC", L"Loop: off",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        BUTTON_MARGIN + statusBarLeftWidth, statusBarTop, labelWidth - statusBarLeftWidth, STATUS_BAR_HEIGHT,
        window, (HMENU)(UINT_PTR)ID_STATUS_BAR_RIGHT, NULL, NULL);

    SendMessageW(state->statusLabel, WM_SETFONT, (WPARAM)state->statusFont, TRUE);
    SendMessageW(state->loadButton, WM_SETFONT, (WPARAM)state->buttonFont, TRUE);
    SendMessageW(state->playButton, WM_SETFONT, (WPARAM)state->buttonFont, TRUE);
    SendMessageW(state->stopButton, WM_SETFONT, (WPARAM)state->buttonFont, TRUE);
    SendMessageW(state->skipBackButton, WM_SETFONT, (WPARAM)state->buttonFont, TRUE);
    SendMessageW(state->skipForwardButton, WM_SETFONT, (WPARAM)state->buttonFont, TRUE);
    SendMessageW(state->statusBarLeft, WM_SETFONT, (WPARAM)state->statusFont, TRUE);
    SendMessageW(state->statusBarRight, WM_SETFONT, (WPARAM)state->statusFont, TRUE);

    /* Play, Stop, and the skip buttons are disabled until a song is loaded. */
    EnableWindow(state->playButton, FALSE);
    EnableWindow(state->stopButton, FALSE);
    EnableWindow(state->skipBackButton, FALSE);
    EnableWindow(state->skipForwardButton, FALSE);
}

/* Updates the status label to show verb (e.g. "Loaded", "Playing",
 * "Stopped") followed by the currently loaded file name, or "None" if
 * no file is loaded. */
static void App_SetStatus(AppState* state, LPCWSTR verb)
{
    wchar_t statusText[MAX_PATH + 32];
    LPCWSTR fileName = Player_GetFileName();

    swprintf_s(statusText, ARRAYSIZE(statusText), L"%s: %s",
        verb, (fileName != NULL && fileName[0] != L'\0') ? fileName : L"None");
    SetWindowTextW(state->statusLabel, statusText);
}

/* Formats milliseconds as "M:SS" into buffer. */
static void App_FormatDuration(DWORD milliseconds, wchar_t* buffer, size_t bufferCount)
{
    const DWORD totalSeconds = milliseconds / 1000;
    const DWORD minutes = totalSeconds / 60;
    const DWORD seconds = totalSeconds % 60;

    swprintf_s(buffer, bufferCount, L"%lu:%02lu", minutes, seconds);
}

/* Updates the bottom status bar: the left pane shows the current playback
 * position and total duration of the loaded song, and the right pane
 * shows the current Loop preference. */
static void App_UpdateStatusBar(AppState* state)
{
    wchar_t positionBuffer[16];
    wchar_t durationBuffer[16];
    wchar_t positionText[32];
    const BOOL isLoaded = Player_IsLoaded();

    App_FormatDuration(isLoaded ? Player_GetPositionMs() : 0, positionBuffer, ARRAYSIZE(positionBuffer));
    App_FormatDuration(isLoaded ? Player_GetDurationMs() : 0, durationBuffer, ARRAYSIZE(durationBuffer));
    swprintf_s(positionText, ARRAYSIZE(positionText), L"%s / %s", positionBuffer, durationBuffer);
    SetWindowTextW(state->statusBarLeft, positionText);

    SetWindowTextW(state->statusBarRight, Player_GetLoop() ? L"Loop: on" : L"Loop: off");
}

/* Prompts the user to pick an MP3 file and, if one is chosen, loads it
 * into the playback module. */
static void App_HandleLoad(HWND window, AppState* state)
{
    wchar_t filePath[MAX_PATH] = L"";
    OPENFILENAMEW openFileName = { 0 };

    openFileName.lStructSize = sizeof(openFileName);
    openFileName.hwndOwner = window;
    openFileName.lpstrFilter = L"MP3 Files (*.mp3)\0*.mp3\0All Files (*.*)\0*.*\0";
    openFileName.lpstrFile = filePath;
    openFileName.nMaxFile = ARRAYSIZE(filePath);
    openFileName.lpstrTitle = L"Select an MP3 File";
    openFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameW(&openFileName))
    {
        return;
    }

    if (Player_Load(window, filePath))
    {
        /* A newly loaded song is ready to play but not yet playing. */
        EnableWindow(state->playButton, TRUE);
        EnableWindow(state->stopButton, FALSE);
        EnableWindow(state->skipBackButton, TRUE);
        EnableWindow(state->skipForwardButton, TRUE);
        KillTimer(window, ID_TIMER_POSITION);
        App_SetStatus(state, L"Loaded");
        App_UpdateStatusBar(state);
    }
}

/* Starts looped playback of the currently loaded song. */
static void App_HandlePlay(HWND window, AppState* state)
{
    if (Player_Play(window))
    {
        EnableWindow(state->stopButton, TRUE);
        SetTimer(window, ID_TIMER_POSITION, POSITION_TIMER_INTERVAL_MS, NULL);
        App_SetStatus(state, L"Playing");
        App_UpdateStatusBar(state);
    }
}

/* Stops playback of the currently loaded song. */
static void App_HandleStop(HWND window, AppState* state)
{
    if (Player_Stop(window))
    {
        EnableWindow(state->stopButton, FALSE);
        KillTimer(window, ID_TIMER_POSITION);
        App_SetStatus(state, L"Stopped");
        App_UpdateStatusBar(state);
    }
}

/* Skips playback back/forward and immediately refreshes the status bar to
 * reflect the new position. */
static void App_HandleSkipBack(HWND window, AppState* state)
{
    Player_SkipBack(window);
    App_UpdateStatusBar(state);
}

static void App_HandleSkipForward(HWND window, AppState* state)
{
    Player_SkipForward(window);
    App_UpdateStatusBar(state);
}

/* Toggles the Loop preference and refreshes the status bar to reflect it. */
static void App_HandleLoop(AppState* state)
{
    Player_SetLoop(!Player_GetLoop());
    App_UpdateStatusBar(state);
}

/* Draws the owner-drawn Loop button in response to WM_DRAWITEM: a standard
 * push-button frame with its character drawn using loopButtonFont, whose
 * 90 degree escapement/orientation rotates it to read sideways. */
static void App_DrawLoopButton(AppState* state, LPDRAWITEMSTRUCT drawItem)
{
    RECT rect = drawItem->rcItem;
    const int centerX = (rect.left + rect.right) / 2;
    const int centerY = (rect.top + rect.bottom) / 2;
    const BOOL isPressed = (drawItem->itemState & ODS_SELECTED) != 0;
    const BOOL isDisabled = (drawItem->itemState & ODS_DISABLED) != 0;
    wchar_t text[8];
    const int textLength = GetWindowTextW(drawItem->hwndItem, text, ARRAYSIZE(text));
    HFONT previousFont;
    TEXTMETRICW metrics;
    SIZE textSize;

    DrawFrameControl(drawItem->hDC, &rect, DFC_BUTTON,
        DFCS_BUTTONPUSH | (isPressed ? DFCS_PUSHED : 0) | (isDisabled ? DFCS_INACTIVE : 0));

    previousFont = (HFONT)SelectObject(drawItem->hDC, state->loopButtonFont);
    SetBkMode(drawItem->hDC, TRANSPARENT);
    SetTextAlign(drawItem->hDC, TA_LEFT | TA_TOP);
    GetTextMetricsW(drawItem->hDC, &metrics);
    GetTextExtentPoint32W(drawItem->hDC, text, textLength, &textSize);

    /* loopButtonFont's escapement/orientation is 900 (90 degrees), so
     * cos(90)=0 and sin(90)=1; the standard text-rotation formula for
     * positioning a TA_LEFT|TA_TOP reference point reduces to swapping the
     * un-rotated bounding box's half-width/half-height so the rotated
     * glyph ends up centered on (centerX, centerY). */
    TextOutW(drawItem->hDC,
        centerX - (metrics.tmAscent + metrics.tmDescent) / 2,
        centerY + textSize.cx / 2,
        text, textLength);

    SelectObject(drawItem->hDC, previousFont);

    if (drawItem->itemState & ODS_FOCUS)
    {
        DrawFocusRect(drawItem->hDC, &rect);
    }
}

LRESULT CALLBACK App_WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    AppState* state = (AppState*)GetWindowLongPtrW(window, GWLP_USERDATA);

    switch (message)
    {
    case WM_CREATE:
    {
        state = (AppState*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AppState));
        if (state == NULL)
        {
            return -1;
        }
        SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)state);
        App_CreateControls(window, state);
        return 0;
    }

    case WM_COMMAND:
    {
        if (state == NULL)
        {
            break;
        }
        switch (LOWORD(wParam))
        {
        case ID_BUTTON_LOAD:
            App_HandleLoad(window, state);
            return 0;
        case ID_BUTTON_PLAY:
            App_HandlePlay(window, state);
            return 0;
        case ID_BUTTON_STOP:
            App_HandleStop(window, state);
            return 0;
        case ID_BUTTON_SKIP_BACK:
            App_HandleSkipBack(window, state);
            return 0;
        case ID_BUTTON_LOOP:
            App_HandleLoop(state);
            return 0;
        case ID_BUTTON_SKIP_FORWARD:
            App_HandleSkipForward(window, state);
            return 0;
        default:
            break;
        }
        break;
    }

    case WM_TIMER:
    {
        if (state == NULL)
        {
            break;
        }
        if (wParam == ID_TIMER_POSITION)
        {
            App_UpdateStatusBar(state);
            return 0;
        }
        break;
    }

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;

        if (state == NULL || drawItem->CtlID != ID_BUTTON_LOOP)
        {
            break;
        }
        App_DrawLoopButton(state, drawItem);
        return TRUE;
    }

    case WM_DESTROY:
        KillTimer(window, ID_TIMER_POSITION);
        Player_Cleanup();
        if (state != NULL)
        {
            if (state->buttonFont != NULL)
            {
                DeleteObject(state->buttonFont);
            }
            if (state->statusFont != NULL)
            {
                DeleteObject(state->statusFont);
            }
            if (state->loopButtonFont != NULL)
            {
                DeleteObject(state->loopButtonFont);
            }
            HeapFree(GetProcessHeap(), 0, state);
            SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)NULL);
        }
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

BOOL App_RegisterWindowClass(HINSTANCE instance)
{
    WNDCLASSEXW windowClass = { 0 };

    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = App_WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    windowClass.lpszClassName = APP_WINDOW_CLASS_NAME;
    windowClass.hIcon = LoadIconW(NULL, IDI_APPLICATION);

    return RegisterClassExW(&windowClass) != 0;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, LPWSTR commandLine, int showCommand)
{
    /* Client area sized to fit the status label, two rows of square
     * buttons, and the status bar, with margins on all sides and between
     * each row. */
    const int clientWidth = BUTTON_MARGIN * 4 + BUTTON_SIZE * 3;
    const int clientHeight = BUTTON_MARGIN * 2 + STATUS_LABEL_HEIGHT
        + BUTTON_MARGIN + BUTTON_SIZE
        + BUTTON_MARGIN + BUTTON_SIZE
        + BUTTON_MARGIN + STATUS_BAR_HEIGHT;
    const DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT windowRect = { 0, 0, clientWidth, clientHeight };
    HWND window;
    MSG message;

    UNREFERENCED_PARAMETER(previousInstance);
    UNREFERENCED_PARAMETER(commandLine);

    if (!App_RegisterWindowClass(instance))
    {
        MessageBoxW(NULL, L"Failed to register the window class.", APP_WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }

    AdjustWindowRect(&windowRect, windowStyle, FALSE);

    window = CreateWindowExW(0, APP_WINDOW_CLASS_NAME, APP_WINDOW_TITLE, windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        NULL, NULL, instance, NULL);

    if (window == NULL)
    {
        MessageBoxW(NULL, L"Failed to create the main window.", APP_WINDOW_TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    while (GetMessageW(&message, NULL, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return (int)message.wParam;
}
