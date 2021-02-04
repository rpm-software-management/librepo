CONFIGURE_FILE("run_unittests.sh.in"  "run_unittests.sh")
ADD_TEST(test_python run_unittests.sh -s ${CMAKE_CURRENT_SOURCE_DIR})
