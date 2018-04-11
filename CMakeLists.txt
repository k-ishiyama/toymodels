# References
#	* The Ultimate Guide to Modern CMake, https://rix0r.nl/blog/2015/08/13/cmake-guide/
#	* CMAKE AND VISUAL STUDIO, https://cognitivewaves.wordpress.com/cmake-and-visual-studio/
cmake_minimum_required (VERSION 3.9)

# test
include(./AddApplicationProject.cmake)
include(./AddLibraryProject.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

# Select if dynamic or static libraries are built
option (build_shared_libs "Build Shared Libraries" OFF)

# Maps to a solution file (ToyModels.sln)
project (ToyModels)

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options. 
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

# Command to output information to the console. Useful for displaying errors, warnings, and debugging
message ("cxx Flags: " ${CMAKE_CXX_FLAGS})

# Sub-directories where more CMakeLists.txt exist
add_subdirectory(src/app/helloworld)
add_subdirectory(src/app/PrecomputedAtmosphericScattering)
add_subdirectory(src/app/PureLatticeGaugeModel)
add_subdirectory(src/lib/etc)
add_subdirectory(src/lib/math)
add_subdirectory(src/lib/window)
add_subdirectory(src/lib/gpu)
add_subdirectory(src/thirdparty/imgui)

if(MSVC)
	# Set the startup project
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT helloworld)
endif()

# Turn on CMake testing capabilities
enable_testing()
add_subdirectory(src/test/lib/math)
