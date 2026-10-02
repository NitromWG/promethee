add_library(promethee_noyau
  src/modele.cpp
  src/derive.cpp
  src/verifs.cpp
  src/placement.cpp)
add_library(Promethee::noyau ALIAS promethee_noyau)
target_include_directories(promethee_noyau PUBLIC include)
target_link_libraries(promethee_noyau PUBLIC nlohmann_json::nlohmann_json)
target_compile_features(promethee_noyau PUBLIC cxx_std_20)
