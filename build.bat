@echo off

if "%Platform%" neq "x64" (
    echo ERROR: Platform is not "x64" - please run this from the MSVC x64 native tools command prompt.
    exit /b 1
)

setlocal

cd %~dp0

if not exist build mkdir build
cd build

if "%1" neq "" (
	echo Invalid number of arguments^. Usage^: build.bat
	goto end
)

set "ignored_warnings= -Wno-unused-parameter -Wno-unused-function -Wno-missing-prototypes -Wno-cast-qual -Wno-comma -Wno-cast-align -Wno-incompatible-pointer-types-discards-qualifiers -Wno-unused-macros -Wno-unused-variable"

set "compile_options= -Wall -Wextra -Wshadow -Wconversion -Wdouble-promotion %ignored_warnings% -fsanitize=undefined -Od -Zo -Z7 -I..\vendor"
set "link_options= -incremental:no -opt:ref -subsystem:windows libvcruntime.lib libvcruntimed.lib Shell32.lib"

clang-cl %compile_options% ..\src\illum.c /link %link_options% user32.lib gdi32.lib opengl32.lib /pdb:illum_base.pdb /out:illum.exe

:end
endlocal
