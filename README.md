# BatteryLine
BatteryLine shows system's battery status as line in screen.

Written in Qt5, for Windows and Linux.

BatteryLine 2.0 is still in develop. For production version, please use [BatteryLine-Legacy](https://github.com/ied206/BatteryLine-Legacy).

# Usage
Compile and launch BatteryLine.exe.

Or, you can download precompiled binary from [project homepage](https://ied206.github.io/BatteryLine).

# Compile
This program is built under this environment:
- CodeBlocks
- TDM-GCC-64 (based on MinGW-w64)

You can compile BatteryLine yourself using MinGW-w64.

## Using Code::Blocks
1. Open BatteryLine-dist.cbp using CodeBlocks
2. Open 'Projects' tab in Management panel (usually at left of screen)
3. Right-click project 'BatteryLine' and select 'Build Options'
4. Choose your favorite compiler, usually 'GNU GCC Compiler', then press OK button
5. Set Build targer to 'Release', then build project.
6. Binary will be generated in bin\Release\BatteryLine.exe.

## Using MinGW-w64 and make
1\. Add MinGW-w64 toolchain to PATH. Make sure `gcc, g++, ar, windres, mingw32-make` can be called in console.  
2\. Build project with mingw32-make
```
Type this in source directory's root:
> mingw32-make
```
3\. 32bit and 64bit binaries are compiled under `bin` folder.

## Microsoft Visual Studio
This code can be compiled under C99 compliance compiler.  
If your version of Visual Studio support C99, this code would be compiled without trouble.  
Any contribution related to add official MSVC support is welcome.

# License
This project is licensed under MIT License.  
Binary distribution of this project is compiled with TDM-GCC-64, which links [winpthreads](http://mingw-w64.org) statically.  
