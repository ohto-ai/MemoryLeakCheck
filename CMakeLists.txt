cmake_minimum_required (VERSION 3.12)

project (mlc)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MEMORY_LEAK_CHECK_IS_MAIN_PROJECT TRUE)
else()
    set(MEMORY_LEAK_CHECK_IS_MAIN_PROJECT FALSE)
endif()

add_library(${PROJECT_NAME} INTERFACE)
add_library(mlc::mlc ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

if(MEMORY_LEAK_CHECK_IS_MAIN_PROJECT)
  add_executable (${PROJECT_NAME}_demo "test/main.cpp")
  target_link_libraries(${PROJECT_NAME}_demo PRIVATE mlc::mlc)
endif()
