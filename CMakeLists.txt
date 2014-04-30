project(dreplace)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
ADD_DEFINITIONS(-std=c++0x)

set(dreplace_SRCS main.cpp)
add_executable(dreplace ${dreplace_SRCS})
target_link_libraries(dreplace ${QT_QTCORE_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})

