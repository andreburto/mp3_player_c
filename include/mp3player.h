#ifndef MP3PLAYER_H
#define MP3PLAYER_H

#include <windows.h>

/*
 * Main application window interface: window class registration, control
 * identifiers, and the per-window state used to enable/disable the
 * Load/Play/Stop buttons.
 */

#define APP_WINDOW_CLASS_NAME L"MP3PlayerWindowClass"
#define APP_WINDOW_TITLE L"MP3 Player"

/* Square button side length, in pixels. */
#define BUTTON_SIZE 80
/* Spacing between buttons and from the window edges, in pixels. */
#define BUTTON_MARGIN 20

/* Height of the status label shown above the buttons, in pixels. */
#define STATUS_LABEL_HEIGHT 24
/* Point size of the status label font. */
#define STATUS_FONT_POINT_SIZE 14

/* Height of the status bar shown below the buttons, in pixels. */
#define STATUS_BAR_HEIGHT 24

/* Control identifiers reported through WM_COMMAND. */
#define ID_BUTTON_LOAD 101
#define ID_BUTTON_PLAY 102
#define ID_BUTTON_STOP 103
#define ID_STATUS_LABEL 104
#define ID_BUTTON_SKIP_BACK 105
#define ID_BUTTON_LOOP 106
#define ID_BUTTON_SKIP_FORWARD 107
#define ID_STATUS_BAR_LEFT 108
#define ID_STATUS_BAR_RIGHT 109

/* Timer identifier used to periodically refresh the status bar's
 * position/duration text while a song is playing. */
#define ID_TIMER_POSITION 1
/* Interval, in milliseconds, between status bar position refreshes. */
#define POSITION_TIMER_INTERVAL_MS 500

/* Per-window state: handles to the buttons, status label, status bar
 * panes, and the fonts used to render their text. Stored in
 * GWLP_USERDATA. */
typedef struct AppState
{
    HWND loadButton;
    HWND playButton;
    HWND stopButton;
    HWND statusLabel;
    HWND skipBackButton;
    HWND loopButton;
    HWND skipForwardButton;
    HWND statusBarLeft;
    HWND statusBarRight;
    HFONT buttonFont;
    HFONT statusFont;
    /* Rotated (90 degree escapement) font used to draw the Loop button's
     * "8" character sideways; see App_DrawLoopButton in mp3player.c. */
    HFONT loopButtonFont;
} AppState;

/* Registers the application window class. Returns TRUE on success. */
BOOL App_RegisterWindowClass(HINSTANCE instance);

/* Main window procedure. */
LRESULT CALLBACK App_WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

#endif /* MP3PLAYER_H */
