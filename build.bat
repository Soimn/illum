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

set "compile_options= /nologo /W3 /Od /Zo /Z7 /RTC1 /MTd /I..\vendor"
set "link_options= /incremental:no /opt:ref /subsystem:windows libvcruntime.lib libvcruntimed.lib Shell32.lib"

cl %compile_options% ..\src\i_win32.c /link %link_options% user32.lib gdi32.lib opengl32.lib /pdb:illum_base.pdb /out:illum.exe
cl %compile_options% ..\src\illum.c /LD /link %link_options% /pdb:illum.pdb /out:illum.dll

:end
endlocal
