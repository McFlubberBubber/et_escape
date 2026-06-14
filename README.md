# Extraterrestrial Escape (et_escape)

## Introduction

This is an invaders-inspired game I made during my first year in university as a web-game. Since the assets I made were still available, I decided to port the game into a raylib application with C++!

As a result, raylib is statically linked within this project, therefore you should not need to download any dependencies.

## Installation

- Either clone the repository by downloading the .zip here on GitHub, or use the following command within a terminal:

```
git clone https://github.com/McFlubberBubber/et_escape.git
```

- Once you have the repository saved on your machine, I like to navigate to the root and make a 'build' directory. You can do the following on Windows:

```
mkdir build
```

- Since I am on Windows and use Visual Studio to compile and debug, I run the following commands within the build directory to generate solutions files and compile the proram.

```
# I run this command first to generate .sln files.
cmake .. -G <insert whatever VS version you use here> -A x64

# Then, I compile the program.
cmake --build . --config <Debug / Release>
```

- I have not tested this application on different operating systems, but I have used CMake as a build system to **hopefully** make it accessible, therefore you can just
use the CMake commands like how you usually would.

- Once you run those commands, the binaries should all be built within the build/game path. From there, you can directly run the program from your file explorer or a terminal instance!
