# Исполняемый файл
set(SOURCES
    main.cpp
    MainWindow.cpp
    facade/TriangulationFacade.cpp
    facade/DebugFacade.cpp
    widgets/TriangulationWidget.cpp
    widgets/DebugControlsWidget.cpp
    rendering/QPainterRenderer.cpp
)
set(HEADERS
    MainWindow.h
    facade/TriangulationFacade.h
    facade/DebugFacade.h
    widgets/TriangulationWidget.h
    widgets/DebugControlsWidget.h
    rendering/IRenderer.h
    rendering/QPainterRenderer.h
)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Подключение библиотек
target_link_libraries(${PROJECT_NAME} PRIVATE
    core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
)

# Включаемые директории
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Флаги компиляции
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Для Windows: консоль в Debug
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE OFF
    )
endif()

# Для Linux/Mac: экспорт символов
if(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        ENABLE_EXPORTS ON
    )
endif()