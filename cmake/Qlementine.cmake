# SPDX-FileCopyrightText: (C) 2026 Rimora Studio
# SPDX-License-Identifier: MIT

# Qlementine: modern QStyle with light/dark themes (MIT)
# Built from source so it links against the same Qt as the rest of the app
# (vcpkg's qlementine port would build against vcpkg Qt, which we don't use).

include(FetchContent)

set(QLEMENTINE_SANDBOX OFF CACHE BOOL "" FORCE)
set(QLEMENTINE_SHOWCASE OFF CACHE BOOL "" FORCE)

# Qlementine sources may use Qt's signals/slots/emit keywords; our tree builds with QT_NO_KEYWORDS.
remove_definitions(-DQT_NO_KEYWORDS)

FetchContent_Declare(
  Qlementine
  GIT_REPOSITORY https://github.com/oclero/qlementine.git
  GIT_TAG v1.4.2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Qlementine)

add_definitions(-DQT_NO_KEYWORDS)
