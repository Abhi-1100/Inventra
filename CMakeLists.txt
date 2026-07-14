cmake_minimum_required(VERSION 3.22)
project(Inventra VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt auto-tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)  # Not using .ui files

# ─────────────────────────────────────────────
# Options
# ─────────────────────────────────────────────
option(INVENTRA_ENABLE_PYTHON
    "Enable embedded Python ML pipeline via pybind11 (Phase 2)"
    OFF)

# ─────────────────────────────────────────────
# Qt (Qt6 with fallback to Qt5)
# ─────────────────────────────────────────────
find_package(Qt6 6.4 COMPONENTS Core Widgets Sql Charts)
if(Qt6_FOUND)
    qt_standard_project_setup()
    set(QT_LIBRARIES Qt6::Core Qt6::Widgets Qt6::Sql Qt6::Charts)
else()
    message(STATUS "Qt6 not found. Falling back to Qt5...")
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Sql Charts)
    set(QT_LIBRARIES Qt5::Core Qt5::Widgets Qt5::Sql Qt5::Charts)
endif()

# ─────────────────────────────────────────────
# Python + pybind11  (Phase 2 — optional)
# ─────────────────────────────────────────────
if(INVENTRA_ENABLE_PYTHON)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter Development)

    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG        v2.13.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(pybind11)
    message(STATUS "Python ML pipeline: ENABLED (Python ${Python3_VERSION})")
else()
    message(STATUS "Python ML pipeline: DISABLED (Phase 1 dummy-data mode)")
endif()

# ─────────────────────────────────────────────
# Source files
# ─────────────────────────────────────────────
set(INVENTRA_SOURCES
    src/main.cpp
    src/core/ProductModel.cpp
    src/core/AppController.cpp
    src/core/PipelineWorker.cpp
    src/core/Database.cpp
    src/core/AuthController.cpp
    src/core/ThemeManager.cpp
    src/python/PythonBridge.cpp
    src/ui/MainWindow.cpp
    src/ui/Sidebar.cpp
    src/ui/StatusBar.cpp
    src/ui/AuthWidget.cpp
    src/ui/DashboardWidget.cpp
    src/ui/DailyEntryWidget.cpp
    src/ui/StockWidget.cpp
    src/ui/ProductDetailPanel.cpp
    src/ui/AnalyticsWidget.cpp
    src/ui/ImportWidget.cpp
    src/ui/SettingsWidget.cpp
    src/ui/ProductCatalogWidget.cpp
    src/widgets/MetricCard.cpp
    src/widgets/BadgeDelegate.cpp
    src/widgets/QtAwesome.cpp
)

set(INVENTRA_HEADERS
    src/core/ProductData.h
    src/core/ProductModel.h
    src/core/AppController.h
    src/core/PipelineWorker.h
    src/core/Database.h
    src/core/AuthData.h
    src/core/AuthController.h
    src/core/ThemeManager.h
    src/python/PythonBridge.h
    src/python/PipelineResult.h
    src/ui/MainWindow.h
    src/ui/Sidebar.h
    src/ui/StatusBar.h
    src/ui/AuthWidget.h
    src/ui/DashboardWidget.h
    src/ui/DailyEntryWidget.h
    src/ui/StockWidget.h
    src/ui/ProductDetailPanel.h
    src/ui/AnalyticsWidget.h
    src/ui/ImportWidget.h
    src/ui/SettingsWidget.h
    src/ui/ProductCatalogWidget.h
    src/widgets/MetricCard.h
    src/widgets/BadgeDelegate.h
    src/widgets/QtAwesome.h
)

# ─────────────────────────────────────────────
# Executable
# ─────────────────────────────────────────────
if(Qt6_FOUND)
    qt_add_executable(Inventra WIN32
        ${INVENTRA_SOURCES}
        ${INVENTRA_HEADERS}
        resources/inventra.qrc
    )
else()
    add_executable(Inventra WIN32
        ${INVENTRA_SOURCES}
        ${INVENTRA_HEADERS}
        resources/inventra.qrc
    )
endif()

target_include_directories(Inventra PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Inventra PRIVATE
    ${QT_LIBRARIES}
)

# Phase 2: link Python
if(INVENTRA_ENABLE_PYTHON)
    target_link_libraries(Inventra PRIVATE
        pybind11::embed
        Python3::Python
    )
    target_include_directories(Inventra PRIVATE
        ${Python3_INCLUDE_DIRS}
    )
    target_compile_definitions(Inventra PRIVATE INVENTRA_PYTHON_ENABLED KIRANA_PYTHON_ENABLED)
endif()

# ─────────────────────────────────────────────
# Post-build: copy Python scripts to output dir
# ─────────────────────────────────────────────
add_custom_command(TARGET Inventra POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/python"
        "$<TARGET_FILE_DIR:Inventra>/python"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/backend"
        "$<TARGET_FILE_DIR:Inventra>/backend"
    COMMENT "Copying Python ML modules and backend to output directory..."
)

# ─────────────────────────────────────────────
# Windows: run windeployqt automatically
# ─────────────────────────────────────────────
if(WIN32)
    if(TARGET Qt6::qmake)
        get_target_property(_qmake_exec Qt6::qmake IMPORTED_LOCATION)
    elseif(TARGET Qt5::qmake)
        get_target_property(_qmake_exec Qt5::qmake IMPORTED_LOCATION)
    else()
        set(_qmake_exec "")
    endif()

    if(_qmake_exec)
        get_filename_component(_qt_bin_dir "${_qmake_exec}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt
            HINTS "${_qt_bin_dir}"
        )
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET Inventra POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                --no-system-d3d-compiler
                "$<TARGET_FILE:Inventra>"
            COMMENT "Running windeployqt..."
        )
    else()
        message(WARNING "windeployqt not found — you may need to deploy Qt DLLs manually.")
    endif()
endif()

# ─────────────────────────────────────────────
# Install (optional)
# ─────────────────────────────────────────────
install(TARGETS Inventra
    RUNTIME DESTINATION bin
)
install(DIRECTORY python/
    DESTINATION bin/python
)
install(DIRECTORY backend/
    DESTINATION bin/backend
)
