cmake_minimum_required(VERSION 3.21)
project(Qomaedit VERSION 0.1.0 LANGUAGES CXX)

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
