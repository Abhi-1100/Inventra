cmake_minimum_required(VERSION 3.22)
project(Inventra VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
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
# Qt6
# ─────────────────────────────────────────────
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets Sql Charts Concurrent Network)
qt_standard_project_setup()

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
    src/core/ApiClient.cpp
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
    src/core/ApiClient.h
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
)

# ─────────────────────────────────────────────
# Executable
# ─────────────────────────────────────────────
qt_add_executable(Inventra WIN32 MACOSX_BUNDLE
    ${INVENTRA_SOURCES}
    ${INVENTRA_HEADERS}
    resources/inventra.qrc
)

set_target_properties(Inventra PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Inventra"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_IDENTIFIER "com.inventra.app"
)

target_include_directories(Inventra PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Inventra PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::Concurrent
    Qt6::Network
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
    # Copy to standard output dir (for non-bundle mode)
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/python"
        "$<TARGET_FILE_DIR:Inventra>/python"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/backend"
        "$<TARGET_FILE_DIR:Inventra>/backend"
    # Also copy into macOS .app bundle if it exists
    COMMAND ${CMAKE_COMMAND} -E $<IF:$<BOOL:$<TARGET_PROPERTY:Inventra,MACOSX_BUNDLE>>,copy_directory,true>
        "${CMAKE_SOURCE_DIR}/python"
        "$<TARGET_BUNDLE_CONTENT_DIR:Inventra>/MacOS/python"
    COMMAND ${CMAKE_COMMAND} -E $<IF:$<BOOL:$<TARGET_PROPERTY:Inventra,MACOSX_BUNDLE>>,copy_directory,true>
        "${CMAKE_SOURCE_DIR}/backend"
        "$<TARGET_BUNDLE_CONTENT_DIR:Inventra>/MacOS/backend"
    COMMENT "Copying Python ML modules and backend to output directory..."
)

# ─────────────────────────────────────────────
# Windows: run windeployqt automatically
# ─────────────────────────────────────────────
if(WIN32)
    get_target_property(_qmake_exec Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_exec}" DIRECTORY)

    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${_qt_bin_dir}"
    )

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
    BUNDLE DESTINATION .
)
install(DIRECTORY python/
    DESTINATION bin/python
)
install(DIRECTORY backend/
    DESTINATION bin/backend
)
qt_add_executable(test_pipeline test_pipeline.cpp)
target_link_libraries(test_pipeline PRIVATE Qt6::Core Qt6::Sql Qt6::Network sqlite3)
# Just a dirty hack for testing, we'll link against the same sources
target_sources(test_pipeline PRIVATE 
    src/python/PythonBridge.cpp 
    src/core/AppController.cpp 
    src/core/Database.cpp
    src/core/ApiClient.cpp
)
target_compile_definitions(test_pipeline PRIVATE KIRANA_PYTHON_ENABLED)
target_include_directories(test_pipeline PRIVATE src build/_deps/pybind11-src/include)
find_package(Python3 COMPONENTS Development REQUIRED)
target_link_libraries(test_pipeline PRIVATE Python3::Python)
