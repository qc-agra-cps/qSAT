cmake_minimum_required(VERSION 3.16)

project(
    qsat
    VERSION 1.0
    DESCRIPTION "QSAT: A library for quantum circuit representation of miter circuit for formal verification"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(QSAT_BUILD_TESTS "Build the qSAT test suite" ON)
option(QSAT_ENABLE_INSTALL "Generate install/export rules" OFF)
option(QSAT_BUILD_BINDINGS "Build the qSAT Python bindings" OFF)

if(NOT DEFINED QSAT_VERSION_INFO)
    set(QSAT_VERSION_INFO ${PROJECT_VERSION})
endif()

# check whether the submodule ``modulename`` is correctly cloned in the ``/extern`` directory.
macro(CHECK_SUBMODULE_PRESENT modulename)
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/${modulename}/CMakeLists.txt")
    message(
      FATAL_ERROR
        "${modulename} submodule not cloned properly. \
        Please run `git submodule update --init --recursive` \
        from the main project directory")
  endif()
endmacro()

# Require qcore submodule
CHECK_SUBMODULE_PRESENT(qcore)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/qcore extern/qcore)

# add main library code
add_subdirectory(src)

# add Python binding code
if(QSAT_BUILD_BINDINGS)
    CHECK_SUBMODULE_PRESENT(json)
    CHECK_SUBMODULE_PRESENT(pybind11)
    CHECK_SUBMODULE_PRESENT(pybind11_json)
    add_subdirectory(qsat)
endif()

if(QSAT_ENABLE_INSTALL)
    install(
        TARGETS qsat qcore
        EXPORT qsatTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    install(
        DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(
        EXPORT qsatTargets
        NAMESPACE qsat::
        FILE qsatTargets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qsat)

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/qsatConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    set(qsat_config_in ${CMAKE_CURRENT_BINARY_DIR}/qsatConfig.cmake.in)
    file(WRITE ${qsat_config_in} "@PACKAGE_INIT@\n\ninclude(\"\\${CMAKE_CURRENT_LIST_DIR}/qsatTargets.cmake\")\n\ncheck_required_components(qsat)\n")

    configure_package_config_file(
        ${qsat_config_in}
        ${CMAKE_CURRENT_BINARY_DIR}/qsatConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qsat)

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/qsatConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/qsatConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qsat)
endif()
