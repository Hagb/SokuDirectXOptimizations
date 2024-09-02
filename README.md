# SokuDirectXOptimizations for Hisoutensoku

This mod introduces some optimizations about DirectX into Hisoutensoku as follows:

- It makes Soku's rendering parallel with the processing of the game, i.e., rendering will no longer block the main thread of the game. As a result, the possibility of frame drops will be reduced, and even if the rendering inevitably becomes stammering because of performance issues, your game will not be slowed down if your CPU is not too bad. This will improve the gaming experience for those who sometimes get frame drops. Moreover, I also recommend that all players, even those with good hardware, install and enable this mod as insurance.
- Speeds up texture loading a little.
- Supports DirectX 9Ex, which may reduce the memory and CPU usage a bit, according to Microsoft. (It is enabled by default, and you can toggle it off by setting `use_d3d9ex=0` in `SokuDirectXOptimizations.ini`. The current version of SokuLobbies will mass up its text rendering with this option enabled.)
- Supports VSync (Vertical Synchronization). (It is disabled by default, and you can enable it by setting `vsync=0` in `SokuDirectXOptimizations.ini`.)

# Build
Requires CMake, git and the VisualStudio compiler (MSVC).
Both git and cmake needs to be in the PATH environment variable.

All the following commands are to be run inside the visual studio 32bits compiler
command prompt (called `x86 Native Tools Command Prompt for VS 20XX` in the start menu), unless stated otherwise.

## Initialization
First go inside the folder you want the repository to be in.
In this example it will be C:\Users\PinkySmile\SokuProjects but remember to replace this
with the path for your machine. If you don't want to type the full path, you can drag and
drop the folder onto the console.

`cd C:\Users\PinkySmile\SokuProjects`

Now let's download the repository and initialize it for the first time
```
git clone https://github.com/SokuDev/ModTemplate
cd ModTemplate
git submodule init
git submodule update
mkdir build
cd build
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
```
Note that if you want to build in Release, you should replace `-DCMAKE_BUILD_TYPE=Debug` with `-DCMAKE_BUILD_TYPE=Release`.

## Compiling
Now, to build the mod, go to the build directory (if you did the previous step you already are)
`cd C:\Users\PinkySmile\SokuProjects\ModTemplate\build` and invoke the compiler by running `cmake --build . --target ModTemplate`. If you change the name of the mod (in the add_library statement in CMakeLists.txt), you will need to replace 'ModTemplate' by the name of your mod in the previous command.

You should find the resulting ModTemplate.dll mod inside the build folder that can be to SWRSToys.ini.
In my case, I would add this line to it `ModTemplate=C:/Users/PinkySmile/SokuProjects/ModTemplate/build/ModTemplate.dll`.
