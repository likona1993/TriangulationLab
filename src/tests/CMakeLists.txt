# Находим Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Тесты для ядра
add_executable(test_core
    test_types.cpp
    test_ear_clipping.cpp
)

target_link_libraries(test_core core ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)

target_include_directories(test_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_options(test_core PRIVATE
    -Wall -Wextra -Wpedantic
)

add_test(NAME test_core COMMAND test_core)

# Тесты для GUI (опционально, с Qt Test)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test)
if(Qt${QT_VERSION_MAJOR}_Test_FOUND)
    add_executable(test_gui
        test_triangulation_widget.cpp
    )
    
    target_link_libraries(test_gui
        ${PROJECT_NAME}
        Qt${QT_VERSION_MAJOR}::Test
    )
    
    add_test(NAME test_gui COMMAND test_gui)
endif()