cmake_minimum_required(VERSION 2.6)

project(Rennypsf)

FIND_PACKAGE(wxWidgets REQUIRED core base)

SET(CMAKE_C_FLAGS_INIT                "-Wall -std=c99")
SET(CMAKE_C_FLAGS_DEBUG_INIT          "-g")
SET(CMAKE_C_FLAGS_MINSIZEREL_INIT     "-Os -DNDEBUG")
SET(CMAKE_C_FLAGS_RELEASE_INIT        "-O4 -DNDEBUG")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")

SET(CMAKE_CXX_FLAGS_INIT                "-Wall")
SET(CMAKE_CXX_FLAGS_DEBUG_INIT          "-g")
SET(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "-Os -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE_INIT        "-O4 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")

#SET(CMAKE_C_FLAGS "-O3 -DNDEBUG")
#SET(CMAKE_CXX_FLAGS "-O3 -DNDEBUG")
#SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -W -Wall -finline-functions -ffast-math -DwxDEBUG_LEVEL=0")

SET(CMAKE_C_FLAGS_DEBUG "-g -W -Wall")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -W -Wall")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -W -Wall -finline-functions -ffast-math -DwxDEBUG_LEVEL=0")

#SET(CMAKE_BUILD_TYPE "Release")
SET(CMAKE_BUILD_TYPE "Debug")
SET(wxWidgets_USE_DEBUG ON)
SET(wxWidgets_USE_REL_AND_DBG ON)


FIND_LIBRARY(OPENAL_LIBRARY
  NAMES OpenAL al openal OpenAL32
  HINTS
  $ENV{OPENALDIR}
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  [HKEY_LOCAL_MACHINE\\SOFTWARE\\Creative\ Labs\\OpenAL\ 1.1\ Software\ Development\ Kit\\1.00.0000;InstallDir]
)

INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})

IF(wxWidgets_FOUND)
    INCLUDE(${wxWidgets_USE_FILE})
    INCLUDE_DIRECTORIES(${wxWidgets_INCLUDE_DIRS})
    INCLUDE_DIRECTORIES(include)

    FILE(GLOB CPPFILES src/*.cpp)
    FILE(GLOB CCFILES src/*.cc)
    FILE(GLOB HEADERS include/*.h)

    FILE(GLOB PSX_SRC_CPP src/psx/*.cpp)
    FILE(GLOB PSX_SRC_CC src/psx/*.cc)
    FILE(GLOB PSX_HDR include/psx/*.h)
    ADD_LIBRARY(psx ${PSX_SRC_CPP} ${PSX_SRC_CC} ${PSX_HDR})
    FILE(GLOB SPU_SRC src/spu/*.cpp)
    FILE(GLOB SPU_HDR include/spu/*.h)
    ADD_LIBRARY(spu ${SPU_SRC} src/spu/fftsg.c ${SPU_HDR})

    IF(wxUSE_GUI)
	FILE(GLOB GUI_SRC src/gui/*.cc)
    	FILE(GLOB GUI_HDR include/gui/*.h)
    ENDIF(wxUSE_GUI)

    ADD_EXECUTABLE(rennypsf ${CPPFILES} ${CCFILES})
    ADD_DEPENDENCIES(rennypsf ${HEADERS})
    TARGET_LINK_LIBRARIES(rennypsf ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} psx spu)
ENDIF(wxWidgets_FOUND)


MESSAGE(STATUS "CMake Build Type            : ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "wxWidgets Debug?            : ${wxWidgets_USE_DEBUG}")
