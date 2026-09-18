cmake_minimum_required(VERSION 3.17)

# Console program that reproduces every number, plot and image in the paper.
# It compiles the solver headers of the application unchanged
# (../../../Implementation/src) and is kept out of the application build,
# so the release pipeline is not affected.

set(SOLUTION_NAME TVPaperExperiments)

project(${SOLUTION_NAME})
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})

if (WIN32)
    set(HOME_ROOT $ENV{USERPROFILE})
    string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
else()
    set(HOME_ROOT $ENV{HOME})
endif()

set(NATID_SDK_ROOT ${HOME_ROOT}/natID.SDK)
include(${NATID_SDK_ROOT}/DevEnv/Common.cmake)
include(${NATID_SDK_ROOT}/DevEnv/natGUI.cmake)
include(${NATID_SDK_ROOT}/DevEnv/MatrixLib.cmake)

set(TV_APP_SRC ${CMAKE_CURRENT_LIST_DIR}/../../../Implementation/src)

add_executable(${SOLUTION_NAME} ${CMAKE_CURRENT_LIST_DIR}/PaperExperiments.cpp)
target_include_directories(${SOLUTION_NAME} PRIVATE ${TV_APP_SRC})

target_link_libraries(${SOLUTION_NAME}
    debug   ${MU_LIB_DEBUG}     optimized ${MU_LIB_RELEASE}
    debug   ${NATGUI_LIB_DEBUG} optimized ${NATGUI_LIB_RELEASE}
    debug   ${MATRIX_LIB_DEBUG} optimized ${MATRIX_LIB_RELEASE})

setIDEPropertiesForExecutable(${SOLUTION_NAME})
