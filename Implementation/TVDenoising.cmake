set(TVDEN_NAME TVDenoising)

file(GLOB TVDEN_SOURCES   ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB TVDEN_INCS      ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
set(TVDEN_PLIST           ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/AppIcon.plist)

file(GLOB TVDEN_INC_TD    ${NATID_SDK_INC}/td/*.h)
file(GLOB TVDEN_INC_GUI   ${NATID_SDK_INC}/gui/*.h)
file(GLOB TVDEN_INC_SPARSE        ${NATID_SDK_INC}/sparse/*.h)
file(GLOB TVDEN_INC_SPARSE_PRIV   ${NATID_SDK_INC}/sparse/priv/*.h)
file(GLOB TVDEN_INC_DENSE         ${NATID_SDK_INC}/dense/*.h)
file(GLOB TVDEN_INC_FO    ${NATID_SDK_INC}/fo/*.h)
file(GLOB TVDEN_INC_DP    ${NATID_SDK_INC}/dp/*.h)

if(WIN32)
    set(TVDEN_WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.rc)
else()
    set(TVDEN_WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.cpp)
endif()

add_executable(${TVDEN_NAME}
    ${TVDEN_INCS}
    ${TVDEN_SOURCES}
    ${TVDEN_INC_TD}
    ${TVDEN_INC_GUI}
    ${TVDEN_INC_SPARSE}
    ${TVDEN_INC_SPARSE_PRIV}
    ${TVDEN_INC_DENSE}
    ${TVDEN_INC_FO}
    ${TVDEN_INC_DP}
    ${TVDEN_WINAPP_ICON})

source_group("inc"               FILES ${TVDEN_INCS})
source_group("inc\\td"           FILES ${TVDEN_INC_TD})
source_group("inc\\gui"          FILES ${TVDEN_INC_GUI})
source_group("inc\\sparse"       FILES ${TVDEN_INC_SPARSE})
source_group("inc\\sparse\\priv" FILES ${TVDEN_INC_SPARSE_PRIV})
source_group("inc\\dense"        FILES ${TVDEN_INC_DENSE})
source_group("inc\\fo"           FILES ${TVDEN_INC_FO})
source_group("inc\\dp"           FILES ${TVDEN_INC_DP})
source_group("src"               FILES ${TVDEN_SOURCES})

# Image paths are resolved at runtime via gui::getResFileName()
# (registered in res/main.xml) — no compile-time path macros needed.
target_compile_definitions(${TVDEN_NAME} PUBLIC
    SER_RESULTS
    MU_USETIMER)

target_link_libraries(${TVDEN_NAME}
    debug   ${MU_LIB_DEBUG}     optimized ${MU_LIB_RELEASE}
    debug   ${NATGUI_LIB_DEBUG} optimized ${NATGUI_LIB_RELEASE}
    debug   ${MATRIX_LIB_DEBUG} optimized ${MATRIX_LIB_RELEASE}
    debug   ${DP_LIB_DEBUG}     optimized ${DP_LIB_RELEASE})

setTargetPropertiesForGUIApp(${TVDEN_NAME} ${TVDEN_PLIST})
setAppIcon(${TVDEN_NAME} ${CMAKE_CURRENT_LIST_DIR})
setIDEPropertiesForGUIExecutable(${TVDEN_NAME} ${CMAKE_CURRENT_LIST_DIR})
setPlatformDLLPath(${TVDEN_NAME})
