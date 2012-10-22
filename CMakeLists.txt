cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(wxWidgets REQUIRED core base)

SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)

SET(CMAKE_BUILD_TYPE Debug)
SET(wxWidgets_USE_DEBUG ON)
SET(wxWidgets_USE_REL_AND_DBG ON)

SET(CMAKE_C_FLAGS_DEBUG "-g -W -Wall")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -W -Wall")


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


IF(wxWidgets_FOUND)
    INCLUDE(${wxWidgets_USE_FILE})

    FILE(GLOB CPPFILES *.cpp)
    FILE(GLOB HEADERS *.h)

    #IF(wxWidgets_DEFINITIONS_DEBUG)
    #    SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG ${wxWidgets_DEFINITIONS_DEBUG})
    #ENDIF(wxWidgets_DEFINITIONS_DEBUG)

    FILE(GLOB PSX_SRC psx/*.cpp)
    FILE(GLOB PSX_HDR psx/*.h)
    ADD_LIBRARY(psx ${PSX_SRC} ${PSX_HDR})
    FILE(GLOB SPU_SRC spu/*.cpp)
    FILE(GLOB SPU_HDR spu/*.h)
    ADD_LIBRARY(spu ${SPU_SRC} ${SPU_HDR})

    ADD_EXECUTABLE(rennypsf ${CPPFILES} ${HEADERS})
    TARGET_LINK_LIBRARIES(rennypsf ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} psx spu)
ENDIF(wxWidgets_FOUND)
