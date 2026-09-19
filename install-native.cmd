@echo off
setlocal
set "FLOWBLUR_UPDATER=%~dp0updater\build\FlowBlurUpdater.exe"
if not exist "%FLOWBLUR_UPDATER%" (
  echo Build with build_release.py first, or download FlowBlurSetup.exe from GitHub Releases.
  exit /b 2
)
start "" "%FLOWBLUR_UPDATER%"
exit /b 0
