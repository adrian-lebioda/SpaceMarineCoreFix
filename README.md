# Space Marine Core Fix

Project inspired by similar fix for Dawn of War 2 ([maximumgame/DOW2CoreFix](https://github.com/maximumgame/DOW2CoreFix)).

This one however impersonates DirectInput8 DLL to install detours on Windows API functions for querying number of cores and forwards call to original DLL.

## Download

Download **DINPUT8.dll** from [releases](https://github.com/adrian-lebioda/SpaceMarineCoreFix/releases).

## Installation

1. Download and install [x86 Visual Studio 2022 Redistributable](https://aka.ms/vs/17/release/vc_redist.x86.exe)
2. Copy **DINPUT8.dll** to the install directory of Space Marine (next to **SpaceMarine.exe**)
3. Launch game

## Limitations

- This will work only for standard Windows installation in C:\Windows path.
- Tested only on 64bit Windows altough in theory it should work on 32bit too
- Tested only with Space Marine Aniversary Edition from Steam

## Building

Open SpaceMarineCoreFix.sln in Visual Studio 2022 Community and build Release version. Built DLL should be in Release folder in root project directory.
