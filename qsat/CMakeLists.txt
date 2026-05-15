# add pybind11 library
if(NOT TARGET pybind11::pybind11)
  add_subdirectory("${PROJECT_SOURCE_DIR}/extern/pybind11" "extern/pybind11" EXCLUDE_FROM_ALL)
endif()

# add pybind11_json library
if(NOT TARGET pybind11_json)
  add_subdirectory("${PROJECT_SOURCE_DIR}/extern/pybind11_json" "extern/pybind11_json" EXCLUDE_FROM_ALL)
endif()

# add Python interface library
add_library(${PROJECT_NAME}_python INTERFACE)

# set include directories
target_include_directories(${PROJECT_NAME}_python INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# link with main project library and pybind11 libraries
target_link_libraries(${PROJECT_NAME}_python INTERFACE ${PROJECT_NAME} pybind11::pybind11 pybind11_json)

# add pyqsat module
pybind11_add_module(py${PROJECT_NAME} Bindings.cpp)
target_link_libraries(py${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_python ${PROJECT_NAME})
target_compile_definitions(py${PROJECT_NAME} PRIVATE VERSION_INFO=${QSAT_VERSION_INFO})