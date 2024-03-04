cmake_minimum_required (VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".so" ".dll")

project ("funny_img")

function(Add_copy_asset for)
    set(resource_dir "${CMAKE_SOURCE_DIR}/resource/")
    set(resource_dest_dir "${CMAKE_CURRENT_BINARY_DIR}/") 
    add_custom_command(
         TARGET ${for} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_directory ${resource_dir} ${resource_dest_dir} 
         DEPENDS ${resource_dest_dir}
         COMMENT "copy_directory resources folder from ${resource_dir} => ${resource_dest_dir}"
     )
endfunction()

function(Add_dist for)
    set(tname "$<TARGET_FILE_NAME:${for}>")
    set(dir "${CMAKE_SOURCE_DIR}/dist")
    string(TOLOWER "${for}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" dname)
    set(dest_dir "${dir}/${dname}")
    set(dest_path "${dest_dir}/${tname}")
    set(checksum "${dest_dir}/sha1sum.txt")

    add_custom_command(
        TARGET ${for} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${tname}" ${dest_path}
        COMMAND ${CMAKE_COMMAND} -E sha1sum "${tname}" > ${checksum}
        DEPENDS ${dest_path} ${checksum}
     )
endfunction()

file(GLOB img_inc_files 
    "src/img/pixel.hpp" 
    "src/img/bmp.hpp" 
    "src/img/bmp_error.hpp" 
    "src/img/png.hpp"
    "src/img/png_error.hpp"
    "src/img/deflate.hpp"
    "src/img/deflate_error.hpp"
    "src/img/deflate_generator.hpp" 
    "src/img/generator.hpp" 
  )

add_executable (funny_img "src/main_img.cpp" ${img_inc_files})
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_options(funny_img PRIVATE -static)
    Add_dist(funny_img)
endif()


add_executable (funny_img_test "src/main_img_test.cpp" ${img_inc_files}  )
Add_copy_asset(funny_img_test)
