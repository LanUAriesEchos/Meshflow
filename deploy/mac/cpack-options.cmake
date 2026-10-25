# SPDX-FileCopyrightText: (C) 2026 Rimora Studio
# SPDX-License-Identifier: MIT

# Evaluated by CPack once per generator
if(CPACK_GENERATOR STREQUAL "productbuild")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/Applications")
  set(CPACK_PRODUCTBUILD_IDENTIFIER "org.meshflow.meshflow")
  # The component name of a non-component install differs between CMake versions, so cover the candidates
  foreach(_comp UNSPECIFIED ALL_IN_ONE MESHFLOW)
    set(CPACK_POSTFLIGHT_${_comp}_SCRIPT "${CPACK_MESHFLOW_POSTFLIGHT}")
  endforeach()
endif()
