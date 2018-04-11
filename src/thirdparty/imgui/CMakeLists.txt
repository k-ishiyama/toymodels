# Define the project name
set(project_name imgui)

# Collect sources into the variable SOURCES  
file (GLOB MAIN_SOURCES "imgui/*.h" "imgui/*.cpp")
file (GLOB DX11_SOURCES "imgui/examples/directx11_example/*.h" "imgui/examples/directx11_example/*.cpp")

# Create named folders for the sources within the .vcproj
source_group("" FILES ${MAIN_SOURCES})
source_group("examples\\directx11_example" FILES ${DX11_SOURCES})

# Properties->C/C++->General->Additional Include Directories
include_directories(./imgui)

# Set Properties->General->Configuration Type
if (build_shared_libs)
	add_definitions(-DBUILD_SHARED_LIBS)
	add_library(${project_name} SHARED ${MAIN_SOURCES} ${DX11_SOURCES})
else (build_shared_libs)
	add_library(${project_name} STATIC ${MAIN_SOURCES} ${DX11_SOURCES})
endif (build_shared_libs)

# Creates a folder and adds target project under it
set_property(TARGET ${project_name} PROPERTY FOLDER "thirdparty")

# Properties->General->Output Directory
set_target_properties(${project_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

if (build_shared_libs)
	# Adds logic to INSTALL.vcproj to copy ${project_name}.dll to the destination directory
	install (TARGETS ${project_name}
	         RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/_install
	         LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/_install)
endif (build_shared_libs)

# Properties->Linker->Input->Additional Dependencies
target_link_libraries(${project_name} d3d11)
target_link_libraries(${project_name} d3dcompiler)
target_link_libraries(${project_name} dxguid)
