cmake_minimum_required(VERSION 3.21)
project(qOmaedit VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Widgets)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(qomaedit
    src/main.cpp
    src/theme.cpp
    src/theme.h
    src/document.cpp
    src/document.h
    src/linenumbermodel.cpp
    src/linenumbermodel.h
    src/settings.cpp
    src/settings.h
    src/syntaxhighlighter.cpp
    src/syntaxhighlighter.h
)

qt_add_qml_module(qomaedit
    URI Qomaedit
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(qomaedit PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Widgets)
target_compile_definitions(qomaedit PRIVATE QOMAEDIT_VERSION="${PROJECT_VERSION}")

install(TARGETS qomaedit)

include(CTest)
if(BUILD_TESTING)
    add_executable(profile_memory EXCLUDE_FROM_ALL tests/profile_memory.cpp)
    target_link_libraries(profile_memory PRIVATE Qt6::Core)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    qt_add_executable(editor_tests
        tests/editor_tests.cpp
        src/document.cpp src/linenumbermodel.cpp src/settings.cpp
        src/syntaxhighlighter.cpp src/theme.cpp
    )
    target_include_directories(editor_tests PRIVATE src)
    target_link_libraries(editor_tests PRIVATE Qt6::Test Qt6::Quick Qt6::QuickControls2 Qt6::Widgets)
    target_compile_definitions(editor_tests PRIVATE MAIN_QML="${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml")
    add_test(NAME editor_tests COMMAND editor_tests)
    set_tests_properties(editor_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
