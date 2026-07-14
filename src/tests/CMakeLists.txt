# Подключаем GoogleTest через FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

# Тесты для ядра
add_executable(test_core
    test_types.cpp
    test_ear_clipping.cpp
    test_geometry_predicates.cpp
    test_monotone_triangulation.cpp
)

# Линкуем с целями GTest
target_link_libraries(test_core 
    core                     # ваша библиотека ядра
    gtest_main               # предоставляет main() и линкует gtest
    pthread
)

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