cmake_minimum_required(VERSION 3.22)
project(Promethee VERSION 0.3.0 LANGUAGES CXX
        DESCRIPTION "Plateforme libre d'ingénierie intégrée : carte et boîtier dans un seul modèle")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Type de compilation" FORCE)
endif()

option(PROMETHEE_TESTS "Compiler les tests" ON)
option(PROMETHEE_APP "Compiler l'application de bureau (Qt 6)" ON)

if(MSVC)
  add_compile_options(/W4 /utf-8 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(nlohmann_json 3.10 REQUIRED)
find_package(OpenCASCADE REQUIRED)

add_subdirectory(noyau)
add_subdirectory(geometrie)
add_subdirectory(outils)
if(PROMETHEE_APP)
  add_subdirectory(app)
endif()

if(PROMETHEE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
