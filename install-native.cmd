@echo off
setlocal
set "FLOWBLUR_SRC=%~dp0native\build\FlowBlur.aex"
set "FLOWBLUR_DEST=C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\FlowBlur"
if not exist "%FLOWBLUR_SRC%" exit /b 2
if not exist "%FLOWBLUR_DEST%" mkdir "%FLOWBLUR_DEST%"
copy /y "%FLOWBLUR_SRC%" "%FLOWBLUR_DEST%\FlowBlur.aex" > "%~dp0native\build\install-result.txt" 2>&1
exit /b %errorlevel%
