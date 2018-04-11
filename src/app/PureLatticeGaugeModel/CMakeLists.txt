set(project_name PureLatticeGaugeModel)

AddApplicationProject(${project_name})

# Properties->Linker->Input->Additional Dependencies
target_link_libraries (${project_name} etc)
target_link_libraries (${project_name} math)
target_link_libraries (${project_name} window)
target_link_libraries (${project_name} imgui)
target_link_libraries (${project_name} gpu)

if(MSVC)
	# Set the entry point
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /subsystem:console /ENTRY:mainCRTStartup")
endif(MSVC)
