cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(wxWidgets REQUIRED core base)
INCLUDE(${wxWidgets_USE_FILE})

#FILE(GLOB CFILES *.c)
FILE(GLOB CPPFILES *.cpp)
FILE(GLOB HEADERS *.h)
#FILE(GLOB LAZYUSF lazyusf/*.c)

#SET(LAZYUSF lazyusf/recompiler_cpu.c lazyusf/memory.c lazyusf/rsp.c lazyusf/rsp_interpreter_cpu.c lazyusf/rsp_recompiler_cpu.c)

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
add_executable(rennypsf main.cpp psx.cpp)
=======
add_executable(rennypsf main.cpp)
>>>>>>> 8b71071... created
=======
#ADD_EXECUTABLE(rennypsf ${LAZYUSF} ${CFILES} ${CPPFILES})
ADD_EXECUTABLE(rennypsf ${CPPFILES} ${HEADERS})
TARGET_LINK_LIBRARIES(rennypsf ${wxWidgets_LIBRARIES})
>>>>>>> 1428e97... updated CMakeLists.txt
=======
add_executable(rennypsf main.cpp psx.cpp)
>>>>>>> c4d0ba8... added cpp file
