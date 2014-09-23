if (NOT BUILD_TESTING)
	return()
endif()

add_executable(test-transmatrix test-transmatrix.cpp)
target_link_libraries(test-transmatrix ${xoreos_LINK_TARGETS})
add_test(transmatrix ${CMAKE_BINARY_DIR}/bin/test-transmatrix)
