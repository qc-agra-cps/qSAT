#[[
# cmake-lint: disable=C0103
set(JSON_BuildTests
    OFF
    CACHE INTERNAL "")
# cmake-lint: disable=C0103
set(JSON_MultipleHeaders
    OFF
    CACHE INTERNAL "")
# add nlohmann::json library
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/json" "extern/json" EXCLUDE_FROM_ALL)
]]
#[[
# main project library
add_library(
  ${PROJECT_NAME} SHARED
  ${PROJECT_SOURCE_DIR}/include/Literal.hpp
  ${PROJECT_SOURCE_DIR}/include/Monomial.hpp
  ${PROJECT_SOURCE_DIR}/include/Clause.hpp
  ${PROJECT_SOURCE_DIR}/include/BooleanFormula.hpp
  ${PROJECT_SOURCE_DIR}/include/Definition.hpp
  parser/Parse.cpp
  BooleanFormula.cpp
  MiterGenerator.cpp
  QsatException.cpp
  ReversibleGate.cpp
  UtilityFunction.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# set include directories
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include
                                                  ${PROJECT_BINARY_DIR}/include)

target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
]]

set(QSAT_SOURCES
    BooleanFormula.cpp
    Clause.cpp
    Literal.cpp
    Monomial.cpp
    Decomposer.cpp
    parser/Parse.cpp
    QCoreBridge.cpp
    SAQC.cpp)

add_library(qsat ${QSAT_SOURCES})

set_target_properties(qsat PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(qsat
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Link against qcore from the required submodule
target_link_libraries(qsat PUBLIC qcore clipperplus)

target_compile_features(qsat PUBLIC cxx_std_20)
