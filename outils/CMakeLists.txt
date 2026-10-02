add_executable(promethee promethee.cpp)
target_link_libraries(promethee PRIVATE Promethee::noyau Promethee::geometrie)
install(TARGETS promethee RUNTIME DESTINATION bin)
