cmake_minimum_required(VERSION 3.16)

# 1. 查找所有Proto文件
# 使用GLOB_RECURSE获取当前目录及其子目录下所有.proto文件
file(GLOB_RECURSE PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

# 初始化存储生成文件路径的列表
set(PROTO_ALL_SRCS "")
set(PROTO_ALL_HDRS "")
set(GRPC_ALL_SRCS "")
set(GRPC_ALL_HDRS "")

# 2. 遍历每个Proto文件并生成对应的代码
foreach(PROTO_FILE ${PROTO_FILES})
    # 获取proto文件的绝对路径和文件名（不含扩展名）
    get_filename_component(PROTO_ABS_PATH "${PROTO_FILE}" ABSOLUTE)
    get_filename_component(PROTO_NAME_WE "${PROTO_FILE}" NAME_WE)

    # 设置当前proto文件对应的生成文件路径
    set(PROTO_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_NAME_WE}.pb.cc")
    set(PROTO_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_NAME_WE}.pb.h")
    set(GRPC_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_NAME_WE}.grpc.pb.cc")
    set(GRPC_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_NAME_WE}.grpc.pb.h")

    # 将当前文件的生成路径添加到总列表中
    list(APPEND PROTO_ALL_SRCS ${PROTO_SRCS})
    list(APPEND PROTO_ALL_HDRS ${PROTO_HDRS})
    list(APPEND GRPC_ALL_SRCS ${GRPC_SRCS})
    list(APPEND GRPC_ALL_HDRS ${GRPC_HDRS})


    # 为当前proto文件添加生成规则
    add_custom_command(
        OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}"
             --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}"
             -I "${CMAKE_CURRENT_SOURCE_DIR}"        # 使用源目录作为proto导入根目录
             --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
             "${PROTO_ABS_PATH}"
        DEPENDS "${PROTO_ABS_PATH}"
    )
endforeach()

# 3. 确保生成目录存在，并包含头文件路径
# 包含构建目录，以便编译器找到生成的头文件 (.pb.h, .grpc.pb.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 4. 创建静态库，链接所有生成的源文件
add_library(${PROTO_LIB_NAME} STATIC
    ${PROTO_ALL_SRCS}
    ${PROTO_ALL_HDRS}
    ${GRPC_ALL_SRCS}
    ${GRPC_ALL_HDRS}
)

# 5. 链接必要的gRPC和Protobuf库
target_link_libraries(${PROTO_LIB_NAME}
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)