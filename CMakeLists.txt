cmake_minimum_required(VERSION 3.0)

project(service)

find_package(Boost REQUIRED COMPONENTS system thread log program_options
    chrono filesystem)
find_package(Threads REQUIRED)
find_package(OpenSSL 1.0.1 REQUIRED)
find_package(libmongocxx REQUIRED)

set(OPENSSL_VERSION "1.0.1")
set(OPENSSL_LIBS "${OPENSSL_LIBRARIES}")
set(LIBRARIES_SEARCH_PATHS ${CPPREST_LIB} ${OPENSSL_LIBS}
    ${Boost_LIBRARIES})
message(BOOST_LIBS " ${Boost_LIBRARIES}")
message(OPENSSL_LIBS " ${OPENSSL_LIBRARIES}")
message(LIBRARIES_SEARCH_PATHS " ${LIBRARIES_SEARCH_PATHS}")


find_library(CPPREST_LIB cpprest)
find_library(TWITCURL_LIB twitcurl)

message("-- cpprest: ${CPPREST_LIB}")
message("-- configuring gcc options")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17 -DBOOST_LOG_DYN_LINK")

# source files ...
set(MICRO_CONTROL ./micro-service/microsvc_controller.cpp)
set(NETWORK_UTILS ./micro-service/foundation/network_utils.cpp)
set(BASIC_CONTROL ./micro-service/foundation/basic_controller.cpp)
set(MICRO_CACHE ./micro-service/cache.cpp)
set(FS_UTIL_SRC ./fs_utils.cpp)

set(MICRO_SRC ${MICRO_CONTROL} ${NETWORK_UTILS} ${BASIC_CONTROL}
    ${MICRO_CACHE})
set(COMMON_SRC ${MICRO_SRC} ${FS_UTIL_SRC})

# server settings
add_executable(server ./server.cpp ${COMMON_SRC})
target_include_directories(server PRIVATE ${LIBMONGOCXX_INCLUDE_DIRS})
target_link_libraries(server ${LIBMONGOCXX_LIBRARIES}
    ${LIBRARIES_SEARCH_PATHS})
target_compile_definitions(server PRIVATE ${LIBMONGOCXX_DEFINITIONS})

# headers search paths ...
set(MICROSERVICE_INCLUDE_DIR "./micro-service/foundation/include")
message(MICROSERVICE " ${MICROSERVICE_INCLUDE_DIR}")
set(HEADER_SEARCH_PATHS ${MICROSERVICE_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})

# library search paths ...

include_directories(${HEADER_SEARCH_PATHS})
target_link_libraries(server ${LIBRARIES_SEARCH_PATHS})

# client settings
add_executable(client ./client.cpp ${FS_UTIL_SRC})
include_directories(${HEADER_SEARCH_PATHS})
target_include_directories(client PRIVATE ${LIBMONGOCXX_INCLUDE_DIRS})
target_link_libraries(client ${LIBMONGOCXX_LIBRARIES}
    ${LIBRARIES_SEARCH_PATHS})
target_compile_definitions(client PRIVATE ${LIBMONGOCXX_DEFINITIONS})

# test command
add_custom_target(run COMMAND ./client DEPENDS client)
