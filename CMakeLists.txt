cmake_minimum_required(VERSION 3.21)
project(qOmaedit VERSION 0.2.2 LANGUAGES CXX)
include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup(REQUIRES 6.5)

add_library(editor_core STATIC
    src/codeeditor.cpp src/codeeditor.h
    src/mainwindow.cpp src/mainwindow.h
    src/theme.cpp src/theme.h
    src/settings.cpp src/settings.h
    src/syntaxhighlighter.cpp src/syntaxhighlighter.h
)
target_include_directories(editor_core PUBLIC src)
target_link_libraries(editor_core PUBLIC Qt6::Widgets)
qt_add_resources(editor_core "qomaedit_license" PREFIX "/" FILES LICENSE)
qt_add_executable(qomaedit src/main.cpp)
qt_add_resources(qomaedit "qomaedit_resources"
    PREFIX "/"
    FILES resources/qomaedit.png
)
target_link_libraries(qomaedit PRIVATE editor_core)
target_compile_definitions(qomaedit PRIVATE QOMAEDIT_VERSION="${PROJECT_VERSION}")
install(TARGETS qomaedit RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/qomaedit.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/qomaedit.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES resources/qomaedit.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/qomaedit)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    add_executable(profile_memory EXCLUDE_FROM_ALL tests/profile_memory.cpp)
    target_link_libraries(profile_memory PRIVATE Qt6::Core)
    qt_add_executable(editor_tests tests/editor_tests.cpp)
    target_link_libraries(editor_tests PRIVATE editor_core Qt6::Test)
    add_test(NAME editor_tests COMMAND editor_tests)
    set_tests_properties(editor_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
