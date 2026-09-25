; ============================================================================
;  Multi-Controller - NSIS Installer Script (Modern UI 2)
;  ----------------------------------------------------------------------------
;  This file is processed by CMake's configure_file() in deploy.cmake.
;  All @VARIABLES@ are substituted at CMake configure time.
;  To build the installer from a configured build tree, run:
;      makensis installer-configured.nsi
;  Or use the CMake custom target:  cmake --build . --target package_installer
; ============================================================================

Unicode true
ManifestDPIAware true

;--- Application metadata (substituted by CMake) ----------------------------
!define APP_NAME       "@APP_NAME@"
!define APP_VERSION    "@APP_VERSION@"
!define APP_PUBLISHER  "@APP_PUBLISHER@"
!define APP_EXE        "@APP_EXE@"
!define CLI_EXE        "@CLI_EXE@"
!define STAGING_DIR    "@STAGING_DIR@"      ; clean deploy folder built by windeployqt + vendor DLL copy
!define ICON_FILE      "@ICON_FILE@"         ; absolute path to app.ico (may be "unset")
!define ESTIMATED_SIZE @ESTIMATED_SIZE@     ; install size in KB (for ARP entry)

;--- Output / install location ---------------------------------------------
Name              "${APP_NAME} ${APP_VERSION}"
OutFile           "@OUT_FILE@"
InstallDir        "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey  HKLM "Software\${APP_NAME}" "InstallDir"
RequestExecutionLevel admin
ShowInstDetails  show
ShowUnInstDetails show
SetCompressor     /SOLID lzma
BrandingText     "${APP_NAME} ${APP_VERSION}"

;--- Modern UI 2 ------------------------------------------------------------
!include "MUI2.nsh"
!include "LogicLib.nsh"

!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING
; MUI icon block is injected by CMake only when an app.ico exists
@ICON_BLOCK@

;--- Installer pages --------------------------------------------------------
; License page is optional: CMake injects either the page macro or nothing.
@LICENSE_PAGE@

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

;--- Uninstaller pages ------------------------------------------------------
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--- Languages --------------------------------------------------------------
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"

; ============================================================================
;  Component declarations
; ============================================================================
Var /GLOBAL StartMenuFolder

InstType "Full"
InstType "Compact"

; ============================================================================
;  Sections
; ============================================================================

;--- Core application (required) -------------------------------------------
Section "${APP_NAME} (required)" SecCore
  SectionIn RO 1 2   ; required, belongs to all install types
  SetOutPath "$INSTDIR"

  ; Create runtime log directory (app writes install/uninstall logs here)
  CreateDirectory "$INSTDIR\logs"

  ; Main executables + CLI
  File "${STAGING_DIR}\${APP_EXE}"
  File "${STAGING_DIR}\${CLI_EXE}"

  ; Qt runtime tree collected by windeployqt (DLLs + platforms/styles/...) and
  ; vendor DLLs (Thorlabs + DVP) — everything else lives in the staging dir.
  ; app.ico (when present) is also carried over from staging, so shortcuts can
  ; reference $INSTDIR\app.ico below.
  File /nonfatal /r "${STAGING_DIR}\*.*"

  ; Remember install dir
  WriteRegStr HKLM "Software\${APP_NAME}" "InstallDir" "$INSTDIR"

  ; Uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Add/Remove Programs entry
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName"     "${APP_NAME}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon"     "$\"$INSTDIR\app.ico$\""
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion"  "${APP_VERSION}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher"       "${APP_PUBLISHER}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "InstallLocation" "$INSTDIR"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "EstimatedSize"   "${ESTIMATED_SIZE}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify"        1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair"        1
SectionEnd

;--- Desktop shortcut (optional) -------------------------------------------
Section "Desktop shortcut" SecDesktop
  SectionIn 1
  ${If} ${FileExists} "$INSTDIR\app.ico"
    CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\app.ico" 0
  ${Else}
    CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "" 0
  ${EndIf}
SectionEnd

;--- Start Menu shortcuts (optional) ---------------------------------------
Section "Start Menu shortcuts" SecStartMenu
  SectionIn 1 2
  StrCpy $StartMenuFolder "$SMPROGRAMS\${APP_NAME}"
  CreateDirectory "$StartMenuFolder"

  ${If} ${FileExists} "$INSTDIR\app.ico"
    CreateShortCut "$StartMenuFolder\${APP_NAME}.lnk"      "$INSTDIR\${APP_EXE}"  "" "$INSTDIR\app.ico" 0
    CreateShortCut "$StartMenuFolder\${APP_NAME} CLI.lnk" "$INSTDIR\${CLI_EXE}"  "" "$INSTDIR\app.ico" 0
  ${Else}
    CreateShortCut "$StartMenuFolder\${APP_NAME}.lnk"      "$INSTDIR\${APP_EXE}"  "" "" 0
    CreateShortCut "$StartMenuFolder\${APP_NAME} CLI.lnk" "$INSTDIR\${CLI_EXE}"  "" "" 0
  ${EndIf}
  CreateShortCut "$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
SectionEnd

;--- Component descriptions -------------------------------------------------
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore}      "Main application, Qt runtime and device vendor libraries (required)."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop}   "Create a shortcut to ${APP_NAME} on the desktop."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu}  "Create ${APP_NAME} shortcuts in the Start Menu."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ============================================================================
;  Callback functions
; ============================================================================

Function .onInit
  ; Pre-select install types for the components page
  SetShellVarContext all
FunctionEnd

Function un.onInit
  SetShellVarContext all
FunctionEnd

; ============================================================================
;  Uninstaller Section
; ============================================================================
Section "Uninstall"
  SetShellVarContext all

  ; Remove main executables
  Delete "$INSTDIR\${APP_EXE}"
  Delete "$INSTDIR\${CLI_EXE}"
  Delete "$INSTDIR\app.ico"
  Delete "$INSTDIR\Uninstall.exe"

  ; Remove runtime log directory created at install time
  Delete "$INSTDIR\logs\*.*"
  RMDir "$INSTDIR\logs"

  ; Remove Qt runtime tree + vendor DLLs (best-effort recursive cleanup).
  ; windeployqt places files under known subfolders; clean those plus any DLL
  ; sitting next to the executable.
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\styles"
  RMDir /r "$INSTDIR\imageformats"
  RMDir /r "$INSTDIR\iconengines"
  RMDir /r "$INSTDIR\translations"
  RMDir /r "$INSTDIR\resources"
  RMDir /r "$INSTDIR\mediaservice"
  RMDir /r "$INSTDIR\tls"
  RMDir /r "$INSTDIR\networkinformation"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\*.pdb"
  Delete "$INSTDIR\*.conf"

  ; Remove shortcuts
  Delete "$DESKTOP\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME} CLI.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
  RMDir  "$SMPROGRAMS\${APP_NAME}"

  ; Remove install dir if empty
  RMDir "$INSTDIR"

  ; Remove registry entries
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
  DeleteRegKey HKLM "Software\${APP_NAME}"
SectionEnd
