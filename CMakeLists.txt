# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(bookstore)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(bookstore
        main.cpp
        command.cpp
        storage.cpp
        user.cpp
        book.cpp
        transaction.cpp
        utils.cpp
)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)