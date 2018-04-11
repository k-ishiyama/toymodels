function(AddApplicationProject inProjectName)
	# Collect sources
	file (GLOB sources "*.h" "*.cpp")
	
	source_group("" FILES ${sources})
	
	# Properties->C/C++->General->Additional Include Directories
	include_directories ("${PROJECT_SOURCE_DIR}")
	
	# Set Properties->General->Configuration Type
	add_executable (${inProjectName} ${sources})
	
	# Creates a folder "app" and adds target project (${inProjectName}.vcproj) under it
	set_property(TARGET ${inProjectName} PROPERTY FOLDER "app")
	
	# Properties->General->Output Directory
	set_target_properties(${inProjectName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
	
	# Properties -> Debugging -> Working Directorty
	set_target_properties(${inProjectName} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	
	# Adds logic to INSTALL.vcproj to copy node.exe to destination directory
	install (TARGETS ${inProjectName} RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/_install)
endfunction()
