---
layout: default
title: Build MG
tags: [help]
---


MG can be built successfully using either Linux or Windows.
In both cases you can use gyp to build mg. In linux you can also use autotools for building.
For building mg in linux you need at least gcc 5 or a later version since mg uses c++ 11.

# Get mg
First of all you need to get the code. Start cloning mg and all sub-modules.

execute `git clone --recursive https://github.com/mediagoom/mg.git` to get all 
or execute `git clone https://github.com/mediagoom/mg.git` and then `git submodule update --init --recursive`.


# Building using gyp
If you are in windows the following instruction will assume you are using **powershell** as your shell.
First check gyp is available. In windows type `get-command gyp` and in bash `which gyp`. If gyp is not available you have to install it. *TODO*: point to gyp installation info.

Change folder to `cd ./mg/test`.

# Linux Build
In *linux* in the mg/test folder run :
```bash
gyp mgtest.gyp --depth=. -Duv_library=static_library \
"-Dtarget_arch=x64" -I../deps/libuv/common.gypi
```
If gyp finish without error run:
```bash
make
```
When make end you could run some validation test with:
```bash
env srcdir="$(pwd)" out/Release/mgtest
```
# Window Build 32 bit
In *window* in the mg\test folder run :
```powershell
gyp mgtest.gyp --depth 0 -Duv_library=static_library `
"-Dtarget_arch=ia32" -I../deps/libuv/common.gypi`
#then
msbuild /p:Configuration=Release /p:Platform=Win32
```
When msbuild finish you could run some validation test with: 
`.\mgtest.ps1`

# Window Build 64 bit
In *window* in the mg\test folder run :
```powershell
gyp mgtest.gyp --depth 0 -Duv_library=static_library `
"-Dtarget_arch=x64" -I../deps/libuv/common.gypi`
#then
msbuild /p:Configuration=Release /p:Platform=x64
```
When msbuild finish you could run some validation test with: 
`.\mgtest.ps1`

# Build Using Autotools

First of all build libuv:

```bash
cd deps/libuv/
./autogen.sh
./configure
make
sudo make install
make install
```
Then build AES:

```bash
cd deps/AES/
./configure
make
sudo make install
```

Then build flavor:

```bash
cd deps/flavor/
./configure
make
sudo make install
```

Finally you can build mg:

```bash
./bootstrap
./configure
make
make check
```
