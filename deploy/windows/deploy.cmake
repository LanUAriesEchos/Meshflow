# SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

# HACK This is set when the files is included so its the real path
# calling CMAKE_CURRENT_LIST_DIR after include would return the wrong scope var
set(MY_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${CMAKE_INSTALL_LIBDIR})
include(InstallRequiredSystemLibraries)

configure_file(${MY_DIR}/pre-cpack.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/pre-cpack.cmake @ONLY)
set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/pre-cpack.cmake)

configure_file(${MY_DIR}/cpack-options.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cpack-options.cmake @ONLY)
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/cpack-options.cmake)

set(OS_STRING "win-${BUILD_ARCHITECTURE}")

list(APPEND CPACK_GENERATOR "7Z")

# If Wix4+ is installed make a package
find_program(WIX_APP wix)
if (NOT "${WIX_APP}" STREQUAL "")
  set(CPACK_WIX_VERSION 4)
  set(CPACK_WIX_ARCHITECTURE ${BUILD_ARCHITECTURE})
  list(APPEND CPACK_GENERATOR "WIX")
endif()

set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_PROPER_NAME}")

# Menu Entry
set(CPACK_WIX_PROGRAM_MENU_FOLDER "${CMAKE_PROJECT_PROPER_NAME}")
set(CPACK_PACKAGE_EXECUTABLES "meshflow" "${CMAKE_PROJECT_PROPER_NAME}")

# Default Install Path
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_PROPER_NAME}")

# Wix Specific Values
set(CPACK_WIX_UPGRADE_GUID "027D1C8A-E7A5-4754-BB93-B2D45BFDBDC8")
set(CPACK_WIX_UI_BANNER "${MY_DIR}/wix-banner.png")
set(CPACK_WIX_UI_DIALOG "${MY_DIR}/wix-dialog.png")

# Required Extra Extenstions
list(APPEND CPACK_WIX_EXTENSIONS "WixToolset.Util.wixext" "WixToolset.Firewall.wixext")

# Make sure to also put the xmlns for the ext into the wix block on generated files
list(APPEND CPACK_WIX_CUSTOM_XMLNS "util=http://wixtoolset.org/schemas/v4/wxs/util" "firewall=http://wixtoolset.org/schemas/v4/wxs/firewall")

# The patch has to know the full path of our msm file
configure_file(
  ${MY_DIR}/wix-patch.xml.in
  ${CMAKE_CURRENT_BINARY_DIR}/wix-patch.xml @ONLY
)

# This patch set ups filewall rules, the service and msm module
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/wix-patch.xml")

# Build EXE bootstrapper (WiX Bundle) alongside MSI
if (NOT "${WIX_APP}" STREQUAL "")
  string(TOLOWER "${CMAKE_PROJECT_PROPER_NAME}" MESHFLOW_PKG_NAME)
  set(MESHFLOW_MSI_FILE "${MESHFLOW_PKG_NAME}-${PACKAGE_VERSION_LABEL}-${OS_STRING}.msi")
  set(MESHFLOW_EXE_FILE "${MESHFLOW_PKG_NAME}-${PACKAGE_VERSION_LABEL}-${OS_STRING}.exe")
  set(MESHFLOW_PKG_DIR "${CMAKE_BINARY_DIR}")

  configure_file(
    ${MY_DIR}/Bundle.wxs.in
    ${CMAKE_CURRENT_BINARY_DIR}/Bundle.wxs @ONLY
  )

  add_custom_command(OUTPUT "${MESHFLOW_PKG_DIR}/${MESHFLOW_EXE_FILE}"
    COMMAND ${WIX_APP} build
      ${CMAKE_CURRENT_BINARY_DIR}/Bundle.wxs
      -o "${MESHFLOW_PKG_DIR}/${MESHFLOW_EXE_FILE}"
      -ext "WixToolset.Util.wixext" -ext "WixToolset.Bal.wixext"
    DEPENDS
      "${CMAKE_CURRENT_BINARY_DIR}/Bundle.wxs"
      "${MESHFLOW_PKG_DIR}/${MESHFLOW_MSI_FILE}"
    COMMENT "Building Meshflow EXE bootstrapper"
    VERBATIM
  )

  add_custom_target(meshflow_exe_bootstrapper
    DEPENDS "${MESHFLOW_PKG_DIR}/${MESHFLOW_EXE_FILE}"
  )
endif()
