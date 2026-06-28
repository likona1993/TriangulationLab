# Простой поиск Eigen3
find_path(EIGEN3_INCLUDE_DIR
    NAMES Eigen/Core
    PATHS
    /usr/include/eigen3
    /usr/local/include/eigen3
    /opt/local/include/eigen3
    ${CMAKE_INSTALL_PREFIX}/include/eigen3
)

mark_as_advanced(EIGEN3_INCLUDE_DIR)

if(EIGEN3_INCLUDE_DIR)
    set(Eigen3_FOUND TRUE)
    add_library(Eigen3::Eigen INTERFACE IMPORTED)
    set_target_properties(Eigen3::Eigen PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${EIGEN3_INCLUDE_DIR}
    )
else()
    set(Eigen3_FOUND FALSE)
endif()