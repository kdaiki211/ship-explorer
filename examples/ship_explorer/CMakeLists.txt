
file(GLOB shipExplorerSources *.cpp)
file(GLOB shipExplorerIncludes *.h )
find_package(yaml-cpp REQUIRED)

cuda_add_executable(ship_explorer ${shipExplorerSources})
target_link_libraries(ship_explorer jetson-inference)
target_link_libraries(ship_explorer PRIVATE yaml-cpp)
install(TARGETS ship_explorer DESTINATION bin)
