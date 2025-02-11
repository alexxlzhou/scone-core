cmake_minimum_required(VERSION 3.4...3.18)

# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

pybind11_add_module(sconepy src/sconepy.cpp)

set_target_properties(sconepy PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_link_libraries(sconepy PRIVATE sconelib)

if (SCONE_OPENSIM_3)
	target_link_libraries(sconepy PRIVATE sconeopensim3)
	target_compile_definitions(sconepy PRIVATE SCONE_OPENSIM_3)
endif()

if (SCONE_OPENSIM_4)
	target_link_libraries(sconepy PRIVATE sconeopensim4)
	target_compile_definitions(sconepy PRIVATE SCONE_OPENSIM_4)
endif()

if (SCONE_HYFYDY)
	target_link_libraries(sconepy PRIVATE sconehfd)
	target_compile_definitions(sconepy PRIVATE SCONE_HYFYDY)
endif()

if (SCONE_LUA)
	target_link_libraries(sconepy PRIVATE sconelua)
	target_compile_definitions(sconepy PRIVATE SCONE_LUA)
endif()
