cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(wxWidgets REQUIRED core base)
INCLUDE(${wxWidgets_USE_FILE})

FILE(GLOB CPPFILES *.cpp)
FILE(GLOB HEADERS *.h)

ADD_EXECUTABLE(rennypsf ${CPPFILES} ${HEADERS})
TARGET_LINK_LIBRARIES(rennypsf ${wxWidgets_LIBRARIES})
