# MP3 Player

A minimal native Windows MP3 player with three square buttons:

- **L** — Load: opens a file picker to choose an MP3 file to play.
- **P** — Play: plays the currently loaded song in a loop until stopped.
- **S** — Stop: stops playback and rewinds the song to the beginning.

Loading a new file while one is already loaded (or playing) automatically
replaces it — that's how you change songs.

A status label above the buttons shows the last action and the currently
loaded file: `Loaded: <file>`, `Playing: <file>`, or `Stopped: <file>`. The
file name is shown without its path, and reads `None` when nothing is
loaded.

## Project Structure

```
include/
  mp3player.h   Application window constants, AppState, and window procedure interface.
  player.h      Playback module interface (load/play/stop/cleanup), backed by MCI.
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
choose an MP3 file, **P** to play it on a loop, and **S** to stop.
