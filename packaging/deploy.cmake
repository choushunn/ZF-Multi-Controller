# ============================================================================
#  deploy.cmake - Qt windeployqt + vendor DLL deployment helpers
#  ----------------------------------------------------------------------------
#  Include from the top-level CMakeLists.txt after the executable targets are
#  defined:
#
#      include("${CMAKE_SOURCE_DIR}/packaging/deploy.cmake")
#
#  What it does:
#    1. Locates windeployqt (Qt6) and runs it after build to collect the Qt
#       runtime dependencies (DLLs + plugins subfolders) of Multi-Controller
#       and Multi-ControllerCLI into a clean staging directory.
#    2. Copies the main executables, the vendor DLLs (Thorlabs KDC101 + DVP2)
#       and (if present) the app icon into the same staging directory.
#    3. Configures packaging/installer.nsi (via configure_file) so it can be
#       compiled by makensis to produce the NSIS installer.
#    4. Adds a `package_installer` custom target that runs makensis, and a
#       basic CPack NSIS integration so `cpack -G NSIS` also works.
# ============================================================================

# --- Guard: this fragment is Windows + Qt6 only ----------------------------
if(NOT WIN32)
  message(STATUS "deploy.cmake: non-Windows platform, skipping windeployqt/NSIS setup.")
  return()
endif()

if(NOT TARGET ${PROJECT_NAME})
  message(FATAL_ERROR "deploy.cmake: ${PROJECT_NAME} target must be defined before inclusion.")
endif()

# --- Locate windeployqt -----------------------------------------------------
# Prefer the windeployqt that ships with the detected Qt (qt_add_executable
# already required Qt, so QT_PREFIX_PATH / Qt bin dir is available). The
# Qt::qmake imported target may not exist on minimal installs, so guard it.
set(_qt_bin_dir "")
if(TARGET Qt${QT_VERSION_MAJOR}::qmake)
    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    if(_qmake_executable)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" PATH)
    endif()
endif()

find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt6
    HINTS "${_qt_bin_dir}"
          "$ENV{QT_PREFIX_PATH}/bin"
          "$ENV{QTDIR}/bin"
          ENV PATH
    REQUIRED
)
message(STATUS "deploy.cmake: windeployqt -> ${WINDEPLOYQT_EXECUTABLE}")

# --- Staging directory (clean deploy folder consumed by installer.nsi) -----
set(MC_STAGING_DIR "${CMAKE_BINARY_DIR}/installer-staging")
set(MC_CONFIGURED_NSI "${CMAKE_BINARY_DIR}/installer-configured.nsi")

# Installer version string. The CMake project is declared VERSION 0.1 (no patch
# component), but the NSIS installer must ship a semver x.y.z. Fall back to a
# ".0" patch when the project omits one, staying in sync with project(VERSION).
if(PROJECT_VERSION_PATCH STREQUAL "" OR NOT DEFINED PROJECT_VERSION_PATCH)
    set(MC_APP_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.0")
else()
    set(MC_APP_VERSION "${PROJECT_VERSION}")
endif()
set(MC_INSTALLER_OUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${MC_APP_VERSION}-win64-setup.exe")

# --- windeployqt flags ------------------------------------------------------
# --release/--debug: 按配置收集对应 Qt 库（Debug 构建必须 --debug，否则部署 Release DLL）
# --no-translations / --no-system-d3d-compiler / --no-opengl-sw keep size down
# --no-quick-import: this is a Widgets app, skip QML bits
set(MC_WINDEPLOYQT_ARGS
    "$<IF:$<CONFIG:Debug>,--debug,--release>"
    --no-translations
    --no-system-d3d-compiler
    --no-opengl-sw
    --no-quick-import
)

# --- Vendor DLL paths (match the ones copied in CMakeLists.txt) -------------
set(MC_VENDOR_DLLS
    "${CMAKE_SOURCE_DIR}/3rdparty/KDC101/Thorlabs.MotionControl.DeviceManager.dll"
    "${CMAKE_SOURCE_DIR}/3rdparty/KDC101/Thorlabs.MotionControl.KCube.DCServo.dll"
    "${CMAKE_SOURCE_DIR}/3rdparty/DVP2/bin/x64/DVPCamera64.dll"
    "${CMAKE_SOURCE_DIR}/3rdparty/ToupTek/bin/x64/toupcam.dll"
)

# --- App icon (optional, the file may not exist yet) -----------------------
set(MC_APP_ICON "${CMAKE_SOURCE_DIR}/src/resources/icons/app.ico")
if(EXISTS "${MC_APP_ICON}")
  set(MC_ICON_FILE "${MC_APP_ICON}")
  set(MC_ICON_BLOCK "!define MUI_ICON \"${MC_APP_ICON}\"\n!define MUI_UNICON \"${MC_APP_ICON}\"")
  message(STATUS "deploy.cmake: app icon -> ${MC_APP_ICON}")
else()
  set(MC_ICON_FILE "")
  set(MC_ICON_BLOCK "; no app.ico found - using NSIS default icon")
  message(STATUS "deploy.cmake: app.ico not found at ${MC_APP_ICON}, using default NSIS icon")
endif()

# --- License page (optional) -----------------------------------------------
# Provide MC_LICENSE_FILE to enable the license page. When not set, a short
# placeholder license text is generated so the page is shown (set
# MC_ENABLE_LICENSE=OFF to skip the page entirely).
option(MC_ENABLE_LICENSE "Show a license page in the NSIS installer" ON)
set(MC_LICENSE_FILE "" CACHE FILEPATH "Optional license text file for the installer")

if(MC_ENABLE_LICENSE)
  if(NOT MC_LICENSE_FILE OR NOT EXISTS "${MC_LICENSE_FILE}")
    set(MC_GENERATED_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")
    file(WRITE "${MC_GENERATED_LICENSE}"
"${PROJECT_NAME} ${MC_APP_VERSION}

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose.
")
    set(MC_LICENSE_FILE "${MC_GENERATED_LICENSE}")
    message(STATUS "deploy.cmake: generated placeholder license at ${MC_GENERATED_LICENSE}")
  endif()
  set(MC_LICENSE_PAGE "!insertmacro MUI_PAGE_LICENSE \"${MC_LICENSE_FILE}\"")
else()
  set(MC_LICENSE_PAGE "; license page disabled (MC_ENABLE_LICENSE=OFF)")
endif()

# ============================================================================
#  POST_BUILD commands: stage everything into MC_STAGING_DIR
#  ----------------------------------------------------------------------------
#  The staging command is attached to the main GUI target. To guarantee the
#  CLI executable exists when staging copies it (the two targets have no link
#  dependency between them), make Multi-Controller depend on Multi-ControllerCLI.
#  windeployqt only needs to run on the GUI exe: it links Qt::Widgets which
#  transitively pulls Qt::Core, so the CLI's (Core-only) runtime is covered too.
# ============================================================================
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}CLI)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # Wipe + recreate a clean staging tree (remove_directory is CMake 3.16-safe
    # and a no-op when the path does not exist).
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${MC_STAGING_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MC_STAGING_DIR}"

    # windeployqt deploys Qt runtime (DLLs + platforms/styles/... plugins) into
    # the staging dir via --dir, so we never copy the build tree wholesale (and
    # thus never carry .pdb/.ilk build junk into the installer). Running on the
    # GUI exe covers the CLI's Qt::Core runtime as well.
    COMMAND ${WINDEPLOYQT_EXECUTABLE}
            ${MC_WINDEPLOYQT_ARGS}
            --dir "${MC_STAGING_DIR}"
            "$<TARGET_FILE:${PROJECT_NAME}>"

    # Main executables (windeployqt --dir does not copy the binary itself).
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"    "${MC_STAGING_DIR}/${PROJECT_NAME}.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}CLI>" "${MC_STAGING_DIR}/${PROJECT_NAME}CLI.exe"

    # Vendor DLLs (Thorlabs KDC101 + DVP2).
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MC_VENDOR_DLLS} "${MC_STAGING_DIR}/"

    COMMENT "deploy.cmake: windeployqt + vendor DLL staging -> ${MC_STAGING_DIR}"
    VERBATIM
)

# Copy app icon (if present) so shortcuts + ARP entry can reference it.
# Done as a separate conditional POST_BUILD so an absent icon never breaks the
# main staging command (an empty generator-expression COMMAND is invalid).
if(MC_ICON_FILE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MC_ICON_FILE}" "${MC_STAGING_DIR}/app.ico"
        COMMENT "deploy.cmake: staging app.ico"
        VERBATIM
    )
endif()

# Convenience alias so `cmake --build . --target deploy` restages without a
# full rebuild. Both executables are guaranteed built via DEPENDS; windeployqt
# only needs the GUI exe (covers the CLI's Qt::Core runtime too).
add_custom_target(deploy
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${MC_STAGING_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MC_STAGING_DIR}"
    COMMAND ${WINDEPLOYQT_EXECUTABLE}
            ${MC_WINDEPLOYQT_ARGS}
            --dir "${MC_STAGING_DIR}"
            "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"    "${MC_STAGING_DIR}/${PROJECT_NAME}.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}CLI>" "${MC_STAGING_DIR}/${PROJECT_NAME}CLI.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MC_VENDOR_DLLS} "${MC_STAGING_DIR}/"
    DEPENDS ${PROJECT_NAME} ${PROJECT_NAME}CLI
    COMMENT "deploy: restage Qt + vendor DLLs -> ${MC_STAGING_DIR}"
    VERBATIM
)

# ============================================================================
#  Configure the NSIS script (substitutes @VARIABLES@ in installer.nsi)
# ============================================================================
# Rough install size estimate (KB) for the ARP entry. windeployqt release tree
# for a Widgets app + 3 vendor DLLs is typically ~60-90 MB.
set(MC_ESTIMATED_SIZE 81920)

# --- Map internal MC_/PROJECT_ values to the @VARIABLE@ names used in installer.nsi
set(APP_NAME       "${PROJECT_NAME}")
set(APP_VERSION    "${MC_APP_VERSION}")
set(APP_PUBLISHER  "${PROJECT_NAME}")
set(APP_EXE        "${PROJECT_NAME}.exe")
set(CLI_EXE        "${PROJECT_NAME}CLI.exe")
set(STAGING_DIR    "${MC_STAGING_DIR}")
set(ICON_FILE      "${MC_ICON_FILE}")
set(ICON_BLOCK     "${MC_ICON_BLOCK}")
set(LICENSE_PAGE   "${MC_LICENSE_PAGE}")
set(ESTIMATED_SIZE "${MC_ESTIMATED_SIZE}")
set(OUT_FILE       "${MC_INSTALLER_OUT}")

configure_file(
    "${CMAKE_SOURCE_DIR}/packaging/installer.nsi"
    "${MC_CONFIGURED_NSI}"
    @ONLY
    NEWLINE_STYLE CRLF
)

# Locate makensis (NSIS). Allow override via cache variable.
find_program(MAKENSIS_EXECUTABLE
    NAMES makensis
    HINTS "C:/Program Files (x86)/NSIS"
          "C:/Program Files/NSIS"
          ENV PATH
)
if(MAKENSIS_EXECUTABLE)
  message(STATUS "deploy.cmake: makensis -> ${MAKENSIS_EXECUTABLE}")

  add_custom_target(package_installer
      COMMAND ${MAKENSIS_EXECUTABLE} "${MC_CONFIGURED_NSI}"
      DEPENDS ${PROJECT_NAME} ${PROJECT_NAME}CLI deploy
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      COMMENT "package_installer: building NSIS installer -> ${MC_INSTALLER_OUT}"
      VERBATIM
  )
else()
  message(WARNING "deploy.cmake: makensis not found. Install NSIS (e.g. to C:\\Program Files (x86)\\NSIS) to enable the package_installer target.")
endif()

# ============================================================================
#  CPack integration (basic NSIS generator fallback)
#  Note: CPack's built-in NSIS template is separate from installer.nsi above.
#  The custom `package_installer` target produces the branded installer using
#  packaging/installer.nsi; the CPack block below provides a standard fallback
#  so `cpack -G NSIS` also yields a working (plain) installer.
# ============================================================================
# NOTE: the main ${PROJECT_NAME} executable is already installed by the
# existing install(TARGETS ${PROJECT_NAME}) in CMakeLists.txt, so we only
# register the CLI target here (installing the same target twice is a CMake
# error). vendor DLLs + the Qt runtime tree go next to the binaries in bin/.
install(TARGETS ${PROJECT_NAME}CLI
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${MC_VENDOR_DLLS} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY "${MC_STAGING_DIR}/" DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILES_MATCHING PATTERN "*.dll"
                    PATTERN "platforms/*"
                    PATTERN "styles/*"
                    PATTERN "imageformats/*"
                    PATTERN "iconengines/*")

set(CPACK_GENERATOR NSIS)
set(CPACK_PACKAGE_NAME              "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION           "${MC_APP_VERSION}")
set(CPACK_PACKAGE_VENDOR            "${PROJECT_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${PROJECT_NAME}")
set(CPACK_NSIS_INSTALL_ROOT         "$PROGRAMFILES64")
set(CPACK_NSIS_PACKAGE_NAME         "${PROJECT_NAME}")
set(CPACK_NSIS_DISPLAY_NAME        "${PROJECT_NAME} ${PROJECT_VERSION}")
set(CPACK_NSIS_CONTACT             "${PROJECT_NAME}")
set(CPACK_NSIS_MODIFY_UNINSTALL    ON)
set(CPACK_NSIS_URL_INFO_ABOUT      "")
set(CPACK_NSIS_HELP_LINK           "")
set(CPACK_NSIS_MENU_LINKS
    "${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}.exe;${PROJECT_NAME}"
    "${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}CLI.exe;${PROJECT_NAME} CLI")
if(MC_ICON_FILE)
  set(CPACK_NSIS_INSTALLER_ICON   "${MC_ICON_FILE}")
  set(CPACK_NSIS_UNINSTALLER_ICON "${MC_ICON_FILE}")
endif()
# CPack's NSIS template already sets RequestExecutionLevel (admin when installing
# under $PROGRAMFILES64) and the compressor, so no extra CPACK_NSIS_DEFINES here.
include(CPack)
