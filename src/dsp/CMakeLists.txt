add_library(vao-dsp STATIC gain.cpp gain.h)
# 呼び出し側は #include "dsp/gain.h" と書く
target_include_directories(vao-dsp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
set_property(TARGET vao-dsp PROPERTY POSITION_INDEPENDENT_CODE ON)
