set(project_name gpu)

AddLibraryProject(${project_name})

# Properties->Linker->Input->Additional Dependencies
target_link_libraries(${project_name} d3d11)
target_link_libraries(${project_name} d3dcompiler)
target_link_libraries(${project_name} dxguid)
