
file(GLOB shipExplorerSources *.cpp)
file(GLOB shipExplorerIncludes *.h )

cuda_add_executable(ship_explorer ${shipExplorerSources})
target_link_libraries(ship_explorer jetson-inference)
install(TARGETS ship_explorer DESTINATION bin)
