cmake_minimum_required(VERSION 3.16)
project(week4_deadlock_lab LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(banker_core src/banker.c src/scenario.c)
target_include_directories(banker_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(banker_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(deadlock_lab src/main.c)
target_link_libraries(deadlock_lab PRIVATE banker_core)
target_compile_options(deadlock_lab PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_executable(test_banker test/test_banker.c)
target_link_libraries(test_banker PRIVATE banker_core)
target_compile_options(test_banker PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME banker_core COMMAND test_banker)
