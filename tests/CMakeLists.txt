add_executable(vao-dsp-tests test_gain.cpp)
target_link_libraries(vao-dsp-tests PRIVATE vao-dsp)
add_test(NAME dsp_gain COMMAND vao-dsp-tests)
