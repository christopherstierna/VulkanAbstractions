@echo off
setlocal EnableExtensions

set "SHADER_DIR=%~dp0"
set "OUTPUT_DIR=%SHADER_DIR%"
set "FAILED=0"

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

for /r "%SHADER_DIR%" %%F in (*.slang) do (
    echo %%F | findstr /I /C:"\Include\" >nul
    if not errorlevel 1 (
        echo Skipping %%F
    ) else (
        echo Compiling %%F...

        slangc ^
            "%%F" ^
            -target spirv ^
            -profile spirv_1_6 ^
            -fvk-use-entrypoint-name ^
            -O3 ^
            -o "%OUTPUT_DIR%%%~nF.spv"

        if errorlevel 1 (
            echo     FAILED: %%~nxF
            set "FAILED=1"
        ) else (
            echo     OK: %OUTPUT_DIR%%%~nF.spv
        )
    )
)

exit /b %FAILED%