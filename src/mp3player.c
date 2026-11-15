#include "../include/mp3player.h"
#include "../include/player.h"

#include <commdlg.h>
#include <wchar.h>

/* Creates the status label, the Load/Play/Stop square buttons, and the
 * fonts used to render their text, and stores the resulting handles in
 * the window's AppState. */
static void App_CreateControls(HWND window, AppState* state)
{
    const int labelTop = BUTTON_MARGIN;
    const int labelWidth = BUTTON_MARGIN * 4 + BUTTON_SIZE * 3 - BUTTON_MARGIN * 2;
    const int buttonsTop = labelTop + STATUS_LABEL_HEIGHT + BUTTON_MARGIN;
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

    SendMessageW(state->statusLabel, WM_SETFONT, (WPARAM)state->statusFont, TRUE);
    SendMessageW(state->loadButton, WM_SETFONT, (WPARAM)state->buttonFont, TRUE);
    SendMessageW(state->playButton, WM_SETFONT, (WPARAM)state->buttonFont, TRUE);
    SendMessageW(state->stopButton, WM_SETFONT, (WPARAM)state->buttonFont, TRUE);

    /* Play and Stop are disabled until a song is loaded. */
    EnableWindow(state->playButton, FALSE);
    EnableWindow(state->stopButton, FALSE);
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
        App_SetStatus(state, L"Loaded");
    }
}

/* Starts looped playback of the currently loaded song. */
static void App_HandlePlay(HWND window, AppState* state)
{
    if (Player_Play(window))
    {
        EnableWindow(state->stopButton, TRUE);
        App_SetStatus(state, L"Playing");
    }
}

/* Stops playback of the currently loaded song. */
static void App_HandleStop(HWND window, AppState* state)
{
    if (Player_Stop(window))
    {
        EnableWindow(state->stopButton, FALSE);
        App_SetStatus(state, L"Stopped");
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
        default:
            break;
        }
        break;
    }

    case WM_DESTROY:
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
    /* Client area sized to fit the status label and three square buttons
     * with margins on all sides. */
    const int clientWidth = BUTTON_MARGIN * 4 + BUTTON_SIZE * 3;
    const int clientHeight = BUTTON_MARGIN * 2 + STATUS_LABEL_HEIGHT + BUTTON_MARGIN + BUTTON_SIZE;
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
