@echo off
setlocal enabledelayedexpansion

:: -------------------------------------------------------------------------
:: compile-shaders.bat
:: Compiles all .vert and .frag files in DeferredRender/src/shader/ to SPIR-V
:: using glslc from the Vulkan SDK.  Run from anywhere.
:: -------------------------------------------------------------------------

if "%VULKAN_SDK%"=="" (
    echo ERROR: VULKAN_SDK is not set. Install the Vulkan SDK and try again.
    exit /b 1
)

set GLSLC=%VULKAN_SDK%\Bin\glslc.exe
set SHADER_DIR=%~dp0..\DeferredRender\src\shader

if not exist "%GLSLC%" (
    echo ERROR: glslc not found at %GLSLC%
    exit /b 1
)

echo Compiling shaders in %SHADER_DIR%...
pushd "%SHADER_DIR%"

set HAD_ERROR=0
for %%f in (*.vert *.frag) do (
    echo   %%f -^> %%f.spv
    "%GLSLC%" "%%f" -o "%%f.spv"
    if errorlevel 1 (
        echo   FAILED: %%f
        set HAD_ERROR=1
    )
)

popd

if !HAD_ERROR! == 1 (
    echo.
    echo One or more shaders failed to compile.
    exit /b 1
)

echo All shaders compiled successfully.
