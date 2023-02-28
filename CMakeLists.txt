cmake_minimum_required(VERSION 3.1)
project(asio-1240)

find_package(Boost 1.81.0 REQUIRED COMPONENTS system)

if(NOT Boost_USE_STATIC_LIBS)
    list(APPEND compile_definitions "BOOST_ALL_DYN_LINK")
endif()
if(WIN32)
    list(APPEND compile_definitions
        WIN32
        WIN32_LEAN_AND_MEAN
        WINVER=0x0501
        _WIN32_WINNT=0x0501
        _WIN32_WINDOWS=0x0501
        _WIN32_IE=0x0600
        _UNICODE
        UNICODE
        _WINSOCK_DEPRECATED_NO_WARNINGS)
endif()

set(include_directories ${Boost_INCLUDE_DIRS})
set(libraries ${Boost_LIBRARIES})

if(UNIX AND NOT APPLE)
    list(APPEND libraries "rt")
endif()
if(WIN32)
    list(APPEND libraries "ws2_32" "mswsock")
endif()

add_executable(asio-1240 main.cpp)
target_compile_features(asio-1240 PRIVATE cxx_std_17)
target_compile_definitions(asio-1240 PRIVATE ${compile_definitions})
target_include_directories(asio-1240 PRIVATE ${include_directories})
target_link_libraries(asio-1240 PRIVATE ${libraries})
