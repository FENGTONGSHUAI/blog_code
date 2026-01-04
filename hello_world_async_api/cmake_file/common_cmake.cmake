# 汇总当前文件夹下的源文件
file(GLOB_RECURSE CURRENT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# 可执行文件
add_executable(${PROJECT_NAME} ${CURRENT_SOURCE} ${SHARED_SOURCES})

# 添加include文件
foreach(include_dir ${ALL_INCLUDE_DIRS})
    target_include_directories("${PROJECT_NAME}" PRIVATE "${include_dir}")
    message(STATUS "已添加包含路径: ${include_dir}")
endforeach()

target_link_libraries(${PROJECT_NAME} 
    ${ALL_LIBS})