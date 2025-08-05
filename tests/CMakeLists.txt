# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Define test sources
set(TEST_SOURCES
    DataUnitTests.cpp
    DataFileTests.cpp
    DataProviderTests.cpp
    DataAcceptorTests.cpp
    DataUnitConverterTests.cpp
)

# Create test executables in a loop
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    
    add_executable(${test_name} ${test_source})
    
    target_include_directories(${test_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
    )
    
    target_link_libraries(${test_name}
        PRIVATE
            core
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
            GTest::gmock_main
    )
    
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
 