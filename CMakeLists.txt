cmake_minimum_required (VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".so" ".dll")

project ("funny_img")

file(GLOB img_inc_files 
    "src/img/pixel.hpp" 
    "src/img/bmp.hpp" 
    "src/img/bmp_error.hpp" 
    "src/img/png.hpp"
    "src/img/png_error.hpp"
    "src/img/deflate.hpp" 
    "src/img/generator.hpp" 
  )

add_executable (funny_img "src/main_img.cpp" ${img_inc_files})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_options(funny_img PRIVATE -static)
endif()

add_executable (funny_img_test "src/main_img_test.cpp" ${img_inc_files}  )


set(resource_dir "${CMAKE_SOURCE_DIR}/resource/")
set(resource_dest_dir "${CMAKE_CURRENT_BINARY_DIR}/") 

add_custom_command(
     TARGET funny_img POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_directory ${resource_dir} ${resource_dest_dir} 
     DEPENDS ${resource_dest_dir}
     COMMENT "copy_directory resources folder from ${resource_dir} => ${resource_dest_dir}"
 )

 add_custom_command(
     TARGET funny_img_test POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_directory ${resource_dir} ${resource_dest_dir} 
     DEPENDS ${resource_dest_dir}
     COMMENT "copy_directory resources folder from ${resource_dir} => ${resource_dest_dir}"
 )
