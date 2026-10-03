find_package(Catch2 3 REQUIRED)
add_executable(promethee_tests test_noyau.cpp test_reference.cpp test_geometrie.cpp test_fixations_ancrages.cpp)
target_link_libraries(promethee_tests PRIVATE Promethee::noyau Promethee::geometrie Catch2::Catch2WithMain)
target_compile_definitions(promethee_tests PRIVATE PROMETHEE_DONNEES="${CMAKE_CURRENT_SOURCE_DIR}/donnees")
if(WIN32)
  target_sources(promethee_tests PRIVATE ${PROJECT_SOURCE_DIR}/cmake/utf8.manifest)
endif()
include(Catch)
catch_discover_tests(promethee_tests DISCOVERY_MODE PRE_TEST)
add_test(NAME outil_exemple_puis_export
  COMMAND ${CMAKE_COMMAND} -DOUTIL=$<TARGET_FILE:promethee> -DDOSSIER=${CMAKE_CURRENT_BINARY_DIR}/sortie_outil
          -P ${CMAKE_CURRENT_SOURCE_DIR}/outil.cmake)
