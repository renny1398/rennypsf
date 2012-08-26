cmake_minimum_required(VERSION 2.6)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_DEBUG "-g")

set(CMAKE_CXX_FLAGS_RELEASE "-W -Wall -O3")
set(CMAKE_C_FLAGS_RELEASE "-W -Wall -O3")

FIND_PACKAGE(wxWidgets REQUIRED core base)
#INCLUDE($(wxWidgets_USE_FILE))
#TARGET_LINK_LIBRARIES(rennypsf $(wxWidgets_LIBRARIES))

add_executable(rennypsf main.cpp)
