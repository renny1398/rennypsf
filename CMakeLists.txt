cmake_minimum_required(VERSION 2.8)

project(Rennypsf)
set(serial "0.0.1")

FIND_PACKAGE(wxWidgets REQUIRED core base)


FIND_PATH(CPPUNIT_INCLUDE_DIR cppunit/Test.h)
FIND_LIBRARY(CPPUNIT_LIBRARY NAMES cppunit)

IF (CPPUNIT_INCLUDE_DIR AND CPPUNIT_LIBRARY)
   SET(CPPUNIT_FOUND TRUE)
   INCLUDE_DIRECTORIES(${CPPUNIT_INCLUDE_DIR})
ENDIF (CPPUNIT_INCLUDE_DIR AND CPPUNIT_LIBRARY)

IF (CPPUNIT_FOUND)

   # show which CppUnit was found only if not quiet
   IF (NOT CppUnit_FIND_QUIETLY)
      MESSAGE(STATUS "Found CppUnit: ${CPPUNIT_LIBRARY}")
   ENDIF (NOT CppUnit_FIND_QUIETLY)

ELSE (CPPUNIT_FOUND)

   # fatal error if CppUnit is required but not found
   IF (CppUnit_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find CppUnit")
   ENDIF (CppUnit_FIND_REQUIRED)

ENDIF (CPPUNIT_FOUND)


SET(CMAKE_C_FLAGS_INIT                "-Wall -std=c99")
SET(CMAKE_C_FLAGS_DEBUG_INIT          "-g")
SET(CMAKE_C_FLAGS_MINSIZEREL_INIT     "-Os -DNDEBUG")
SET(CMAKE_C_FLAGS_RELEASE_INIT        "-O4 -DNDEBUG")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")

SET(CMAKE_CXX_FLAGS_INIT                "-Wall -Wno-potentially-evaluated-expression")
SET(CMAKE_CXX_FLAGS_DEBUG_INIT          "-g")
SET(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "-Os -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE_INIT        "-O4 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")

SET(CMAKE_C_FLAGS_DEBUG "-g -W -Wall -Wshadow -Wno-potentially-evaluated-expression")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -W -Wall -Wshadow -Wno-potentially-evaluated-expression")
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

CMAKE_POLICY(SET CMP0046 OLD)

IF(wxWidgets_FOUND)

    # use CUI
    # SET(USE_GUI)

    INCLUDE(${wxWidgets_USE_FILE})
    INCLUDE_DIRECTORIES(${wxWidgets_INCLUDE_DIRS})
    INCLUDE_DIRECTORIES(include)

    FILE(GLOB CC_COMMON src/common/*.cc)
    FILE(GLOB HDR_COMMON include/common/*.h)

    FILE(GLOB CPPFILES src/*.cpp)
    FILE(GLOB CCFILES src/*.cc)
    FILE(GLOB HEADERS include/*.h)

    FILE(GLOB VORBIS_SRC src/vorbis/*.cc)
    FILE(GLOB VORBIS_HDR include/vorbis/*.h)
    ADD_LIBRARY(rennyvorbis ${VORBIS_SRC} ${VORBIS_HDR})
    ADD_DEPENDENCIES(rennyvorbis ${VORBIS_HDR})

#    FILE(GLOB SNESAPU_SRC src/snesapu/*.cc)
#    FILE(GLOB SNESAPU_HDR include/snesapu/*.h)
#    ADD_LIBRARY(snesapu ${SNESAPU_SRC} ${SNESAPU_HDR})
#    ADD_DEPENDENCIES(snesapu ${SNESAPU_HDR})

    FILE(GLOB PSX_SRC src/psf/psx/*.cc)
    FILE(GLOB PSX_HDR include/psf/psx/*.h)
    ADD_LIBRARY(psx ${PSX_SRC} ${PSX_HDR})
    ADD_DEPENDENCIES(psx ${PSX_HDR})
    FILE(GLOB SPU_SRC src/psf/spu/*.cc)
    FILE(GLOB SPU_HDR include/psf/spu/*.h)
    ADD_LIBRARY(spu ${SPU_SRC} ${SPU_HDR} src/psf/spu/fftsg.c)
    ADD_DEPENDENCIES(spu ${SPU_HDR})   
    FILE(GLOB PSF_SRC src/psf/*.cc)
    FILE(GLOB PSF_HDR include/psf/*.h)
    ADD_LIBRARY(psf ${PSF_SRC} ${PSF_HDR})
    ADD_DEPENDENCIES(psf ${PSF_HDR})

#    IF(USE_GUI)
	FILE(GLOB GUI_SRC src/gui/*.cc)
    	FILE(GLOB GUI_HDR include/gui/*.h)
#        ADD_EXECUTABLE(rennypsf ${CPPFILES} ${CCFILES} ${GUI_SRC} ${GUI_HDR} ${HEADERS})
#    ELSE(USE_GUI)
        FILE(GLOB CUI_SRC src/cui/*.cc)
        FILE(GLOB CUI_HDR include/cui/*.h)
#    ENDIF(USE_GUI)

    ADD_LIBRARY(renny ${CC_COMMON} ${HDR_COMMON})
    ADD_DEPENDENCIES(renny ${HDR_COMMON})

    ADD_EXECUTABLE(rennypsf ${CPPFILES} ${CCFILES} ${GUI_SRC} ${GUI_HDR} ${CUI_SRC} ${CUI_HDR} ${HEADERS})
    ADD_DEPENDENCIES(rennypsf ${HEADERS})
    TARGET_LINK_LIBRARIES(rennypsf ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} vorbis vorbisfile rennyvorbis psx spu psf renny)

    enable_testing()
    add_executable(psx_test test/psx_test.cc)
    target_link_libraries(psx_test cppunit ${wxWidgets_LIBRARIES} ${OPENAL_LIBRARY} vorbis vorbisfile rennyvorbis psx spu psf renny)
    add_test(psx_test psx_test)

ENDIF(wxWidgets_FOUND)


MESSAGE(STATUS "CMake Build Type            : ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "wxWidgets Debug?            : ${wxWidgets_USE_DEBUG}")
