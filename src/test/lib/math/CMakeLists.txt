# Define the target project for test
set(test_target math)

# Properties->C/C++->General->Additional Include Directories
include_directories ("${PROJECT_SOURCE_DIR}")

# Collect sources
file(GLOB sources "*.hpp" "*.cpp")

# Create named folders for the sources within the .vcproj
# Empty name lists them directly under the .vcproj
source_group("" FILES ${sources})

add_executable(${test_target}_test ${sources})

target_link_libraries(${test_target}_test ${test_target})
set_property(TARGET ${test_target}_test PROPERTY FOLDER "test")

add_test(NAME ${test_target}_test COMMAND ${test_target}_test)
