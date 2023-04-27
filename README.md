**P.S.** is a music track written from scratch using the C programming language.
All sounds and notes were entered manually without using any music programs.
One single source file contains synth code, patterns and player.
No third party libraries are used except SDL2 to output the audio stream.

---
Fork info:
* Added support cross-platform CMake build system.  
* Refactored Code.
* Other minor fixes/improvements.

### Linux: 
Dependencies:  
*ubuntu/debian:
```
sudo apt install libsdl2-dev git cmake make
```
Arch/Manjaro:  
```
pacman -S git cmake sdl2 make
```
### macOS:
```
brew install git sdl2 cmake
```
### Build:
```
mkdir build && cd build
cmake -G "Unix Makefiles" ..
make
```  
MinGW32 or MXE CrossCompiling: 
```
mkdir build && cd build
cmake -G "Unix Makefiles" .. -DCMAKE_TOOLCHAIN_FILE=Your-CrossCompiling-Toolchain-file.cmake
make
```

### Windows build (soon):  

[Visual Studio](https://visualstudio.microsoft.com/)  
[CMake](https://cmake.org/download/)  
[git](https://github.com/git-for-windows/git/releases/latest)  
[SDL2](https://www.libsdl.org/)
```
cmake -G "Visual Studio 16 2019" -A x64 -B "build"
cd build
cmake --build . --config Release
```
Or  
Open generated .sln project file in Visual Studio... select sdl2 libraries... etc...  

---
Usage:
* just play the song: `./ps`
* export to WAV: `./ps -o filename.wav`

Press CTRL+C to exit

by NightRadio  
https://warmplace.ru  
nightradio@gmail.com  
2005 - 2021
