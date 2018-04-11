function(AddLibraryProject	inProjectName)
	# Collect sources
	file (GLOB sources "*.cpp" "*.hpp")
	
	source_group("" FILES ${sources})
	
	# Properties->C/C++->General->Additional Include Directories
	include_directories (.)
	
	# Properties->General->Configuration Type
	if (build_shared_libs)
		add_definitions(-DBUILD_SHARED_LIBS)
		add_library(${inProjectName} SHARED ${sources})
	else (build_shared_libs)
		add_library(${inProjectName} STATIC ${sources})
	endif (build_shared_libs)
	
	# Creates a folder "lib" and adds target project under it
	set_property(TARGET ${inProjectName} PROPERTY FOLDER "lib")
	
	# Properties->General->Output Directory
	set_target_properties(${inProjectName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
	
	if (build_shared_libs)
		# Adds logic to INSTALL.vcproj to copy ${inProjectName}.dll to the destination directory
		install (TARGETS ${inProjectName}
		         RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/_install
		         LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/_install)
	endif (build_shared_libs)
endfunction()
