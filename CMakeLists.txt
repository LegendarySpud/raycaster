cmake_minimum_required(VERSION 3.16)

project(main LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/glad.c
)

# Headers live here
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw
        OpenGL::GL
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -Wall
        -Wextra
)
