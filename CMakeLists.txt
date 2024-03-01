cmake_minimum_required(VERSION 3.18)
project(LDipDect VERSION 1.0 DESCRIPTION "Light Dip Detector" LANGUAGES C)

# Compiler Options
set(CMAKE_C_STANDARD 11)
# set(CXX_STANDARD 17)
add_compile_options(-Wall -Werror -Wpedantic -Wextra)
# add_compile_options(-g -Wall -Werror -Wpedantic -Wextra)
add_compile_options(-fdiagnostics-color)

# Enable address sanitizer
# (Comment this out to make your code faster)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_subdirectory(hal)
add_subdirectory(app)

# Comment out Lines 8, 14, and 15
# Uncomment Line 7