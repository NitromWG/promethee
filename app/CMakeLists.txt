# Application de bureau. Facultative : sans Qt 6, le reste du projet se compile quand même.
find_package(Qt6 6.2 QUIET COMPONENTS Widgets OpenGLWidgets)
if(NOT Qt6_FOUND)
  message(STATUS "Qt 6 introuvable : l'application de bureau n'est pas compilée.")
  return()
endif()
set(CMAKE_AUTOMOC ON)
find_package(OpenGL REQUIRED)

set(PROMETHEE_OCCT_VISU TKService TKV3d TKOpenGl)
add_executable(promethee_app WIN32 MACOSX_BUNDLE
  main.cpp Document.cpp VueCarte.cpp Vue3D.cpp OcctGlTools.cpp Panneau.cpp FenetrePrincipale.cpp DialogueExport.cpp)
set_target_properties(promethee_app PROPERTIES OUTPUT_NAME Promethee)
target_link_libraries(promethee_app PRIVATE Promethee::noyau Promethee::geometrie Qt6::Widgets Qt6::OpenGLWidgets OpenGL::GL ${PROMETHEE_OCCT_VISU})
if(WIN32)
  target_sources(promethee_app PRIVATE ${PROJECT_SOURCE_DIR}/cmake/utf8.manifest)
endif()
install(TARGETS promethee_app RUNTIME DESTINATION bin BUNDLE DESTINATION .)
