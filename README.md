**P.S.** is a music track written from scratch using the C programming language.
All sounds and notes were entered manually without using any music programs.
One single source file contains synth code, patterns and player.
No third party libraries are used except SDL2 to output the audio stream.

---
Added support cmake build system.  

Linux build: 
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

Windows build:  
```
cmake -G "Visual Studio 16 2019" -A x64 -B "build"
```
Open .sln in Visual Studio... etc...  

---
Usage:
* just play the song: `./ps`
* export to WAV: `./ps -o filename.wav`

Press CTRL+C to exit

How to make: just run one of the MAKE_* scripts (currently only Linux, but scripts for other systems will be added later too)

by NightRadio  
https://warmplace.ru  
nightradio@gmail.com  
2005 - 2021
