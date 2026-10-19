# CSOPESYMULATOR
An OS Emulator made for CSOPESY

Group Members:
- GIAN LORENZO ORTHA
- GIANELA KIM AGSALON
- MARC JARED SEAN ERCIA
- THEON SCHUYLER GARCIA

Entry class file: main.cpp

# Running and Building with VSCode

**Pre Requisites:**
1. Install the Microsoft C/C++ Extension.
2. Install a modern version of mingw i.e. [winlibs](https://winlibs.com/).
3. The custom `tasks.json` file is present in the .vscode folder (if you cloned this repository correctly, it should be there).

## How To Run
1. Open any cpp file. This will cause the run button to appear.
2. Click the dropdown on the run button and select `Run C/C++ File`.
3. If prompted which build configuration to use, select the one with the label **"Build OS Emulator."**
4. Program should now build and then run.

*NOTE: There was an issue that existed, causing the program to run within the `src` directory. As of last testing it, it now runs from the workspace root. I cannot say what caused this behavior to change, but if it does run from the src directory, ggs idk what to do about that...*

## Manual Build

OR Manual building and running with g++ build script. Note that this script may not have 100% parity with the build in the `tasks.json` file if ever we forget to upload it.

```bash
.\build.bat
.\main.exe
```