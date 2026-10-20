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
1. Install the [Microsoft C/C++ Extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools).
2. Install the [Code Runner Extension by Jun Han](https://marketplace.visualstudio.com/items?itemName=formulahendry.code-runner).
3. Install a modern version of mingw i.e. [winlibs](https://winlibs.com/) or just have gpp installed. *NOTE: If you're using a less common gpp install, see [Manually Setting Your GPP Path](#manually-setting-your-gpp-path)*
4. The custom `settings.json` file is present in the .vscode folder (if you cloned this repository correctly, it should be there).

## How To Run
1. Open any cpp file. This will cause the run button to appear.

<img width="1622" height="544" alt="image" src="https://github.com/user-attachments/assets/24a0fc6d-4f92-4828-846b-24f39189d4e8" />
<br /> <br />

2. Click the dropdown on the run button and select `Run Code`. This will invoke the code runner configured in the `.vscode/settings.json` file. For subsequent runs in the same session, you should be able to just click the button without having to open the dropdown anymore. If you close and re-open vscode, you will have to do the dropdown thing again.

<img width="248" height="118" alt="image" src="https://github.com/user-attachments/assets/e4614184-e1f0-41e9-9fd5-3018ce9d714e" />
<br /> <br />

3. Program should now build and then run. You will know that it worked properly if the command invokes the build.bat file.

<img width="1617" height="625" alt="image" src="https://github.com/user-attachments/assets/423c95b6-4908-4640-b186-eb2ceb52d49d" />

<img width="1612" height="962" alt="image" src="https://github.com/user-attachments/assets/f66e6b8e-0a7d-452b-adf7-b345c5b63464" />


## Manually Setting Your GPP Path

On the off chance you are using a gpp installation that the build script cannot find, you might need to do some extra setup.

First, open up a powershell window and run the following command:

```powershell
Get-Command g++

# Output might be something like this

CommandType     Name                                               Version    Source
-----------     ----                                               -------    ------
Application     g++.exe                                            0.0.0.0    C:\mingw64\bin\g++.exe

```

If you do have gpp installed, then you will see where your g++ is located. Copy that path file and just add it to the `build.bat file`. You will see a list of g++ paths. Just add it there.

<img width="1601" height="791" alt="image" src="https://github.com/user-attachments/assets/14685d9f-0d1b-4e6d-a9a2-a13e46d2c4af" />

## Manual Build

OR Manual building and running with g++ build script.

```powershell
.\build.bat
.\main.exe
```
