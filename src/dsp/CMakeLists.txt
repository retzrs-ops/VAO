add_library(
  vao-dsp
  STATIC
  biquad.cpp
  biquad.h
  compressor.cpp
  compressor.h
  denoiser.cpp
  denoiser.h
  deesser.cpp
  deesser.h
  eq.cpp
  eq.h
  gain.cpp
  gain.h
  gate.cpp
  gate.h
  highpass.cpp
  highpass.h
  limiter.cpp
  limiter.h
  loudness.cpp
  loudness.h
)
# 呼び出し側は #include "dsp/gain.h" と書く
target_include_directories(vao-dsp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
set_property(TARGET vao-dsp PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(vao-dsp PRIVATE "${VAO_RNNOISE_INCLUDE_DIR}")
target_link_libraries(vao-dsp PUBLIC "${VAO_RNNOISE_LIBRARY}")
