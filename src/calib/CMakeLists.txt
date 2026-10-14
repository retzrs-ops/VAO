add_library(vao-calib STATIC measurer.cpp measurer.h auto_settings.cpp auto_settings.h)
target_include_directories(vao-calib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
set_property(TARGET vao-calib PROPERTY POSITION_INDEPENDENT_CODE ON)
