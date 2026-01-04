cmake_minimum_required(VERSION 3.16)
project(RouteGuide C CXX)

# 设置策略 CMP0144 为 NEW
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置gRPC路径
get_filename_component(GRPC_ROOT "${CMAKE_SOURCE_DIR}/../grpc" ABSOLUTE)
set(CMAKE_PREFIX_PATH "${GRPC_ROOT}")

# 查找依赖包
find_package(Threads REQUIRED)

option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
# set(_GRPC_CPP_PLUGIN_EXECUTABLE "${CMAKE_SOURCE_DIR}/../grpc/bin/grpc_cpp_plugin")

# 添加include路径
set(PROTO_FILE_DIR "${CMAKE_SOURCE_DIR}/proto")
set(COMMON_FILE_DIR "${CMAKE_SOURCE_DIR}/common")
set(ALL_INCLUDE_DIRS
  ${PROTO_FILE_DIR}
  ${COMMON_FILE_DIR}
)

# 设置proto lib 名
set(PROTO_LIB_NAME "hello_grpc_proto")

# 全部lib
set(ALL_LIBS
  absl::flags_parse
  absl::absl_log
  absl::log_initialize
  absl::log_globals
  ${PROTO_LIB_NAME}
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

# 设置公共cmake文件路径
set(COMMON_CMAKE_FILE "${CMAKE_SOURCE_DIR}/cmake_file/common_cmake.cmake")

# 包含子目录
add_subdirectory(proto)
add_subdirectory(server)
add_subdirectory(client)