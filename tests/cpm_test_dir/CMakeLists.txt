cmake_minimum_required(VERSION 3.10)
project(�[�f�� VERSION �@���� LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Include directories
include_directories(include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB_RECURSE HEADERS "include/*.h")

# Create library
add_library(�[�f�� ${SOURCES} ${HEADERS})

# Create example executable (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/main.c")
    add_executable(�[�f��_example examples/main.c)
    target_link_libraries(�[�f��_example �[�f��)
endif()

# Install targets
install(TARGETS �[�f��
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
