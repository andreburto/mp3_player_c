# MP3 Player

A minimal native Windows MP3 player with two rows of square buttons.

The first row:

- **L** — Load: opens a file picker to choose an MP3 file to play.
- **P** — Play: plays the currently loaded song in a loop until stopped.
- **S** — Stop: stops playback and rewinds the song to the beginning.

The second row:

- **<<** — Skip back 5 seconds.
- **8** — Loop: toggles the Loop preference, shown in the status bar.
- **>>** — Skip forward 5 seconds.

Loading a new file while one is already loaded (or playing) automatically
replaces it — that's how you change songs.

A status label above the buttons shows the last action and the currently
loaded file: `Loaded: <file>`, `Playing: <file>`, or `Stopped: <file>`. The
file name is shown without its path, and reads `None` when nothing is
loaded.

A status bar below the buttons shows two pieces of information:

- Left: the current playback position and total duration of the loaded
  song, e.g. `1:12 / 3:45`.
- Right: the current Loop preference, `Loop: on` or `Loop: off`.

## Project Structure

```
include/
  mp3player.h   Application window constants, AppState, and window procedure interface.
  player.h      Playback module interface (load/play/stop/skip/loop/cleanup), backed by MCI.
src/
  mp3player.c   WinMain, window/button creation, and UI event handling.
  player.c      MCI (winmm) based implementation of the playback module.
build.bat       Builds the project with the MSVC toolchain into bin\mp3player.exe.
```

## Building

Requires the MSVC build tools (Visual Studio Build Tools or Visual Studio
with the "Desktop development with C++" workload).

```
build.bat
```

This produces `bin\mp3player.exe`. If your Visual Studio installation path
differs from the default, edit the `VCVARS` path at the top of `build.bat`.

## Running

Double-click `bin\mp3player.exe`, or run it from a terminal. Click **L** to
choose an MP3 file, **P** to play it on a loop, and **S** to stop. Use
**<<** and **>>** to skip back/forward 5 seconds, and **L** on the second
row to toggle the Loop preference shown in the status bar.
