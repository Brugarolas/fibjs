cmake_minimum_required(VERSION 3.5)

set(name "fibjs")
project(${name})

add_definitions(-DOPENSSL_DEV_NO_ATOMICS)

include(../vender/build_tools/cmake/option.cmake)

add_library(${name} ${src_list} ${src_platform_list})

include_directories("${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/../vender"
	"${PROJECT_SOURCE_DIR}/../vender/uv"
	"${PROJECT_SOURCE_DIR}/../vender/uv/include"
	"${PROJECT_SOURCE_DIR}/../vender/zlib/include"
	"${PROJECT_SOURCE_DIR}/../vender/msgpack/include"
	"${PROJECT_SOURCE_DIR}/../vender/openssl/include"
	"${CMAKE_CURRENT_BINARY_DIR}")

include(../vender/v8/cmake/options.cmake)

setup_result_library(${name})