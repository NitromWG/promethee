# Composants tiers

Prométhée est distribué sous GPL-3.0. Il s'appuie sur les composants suivants, chacun sous sa propre licence.

| Composant | Usage | Licence |
| --- | --- | --- |
| Open CASCADE Technology | Géométrie exacte, échanges STEP et STL, visualisation 3D | LGPL 2.1 avec exception |
| Qt 6 | Interface de l'application de bureau | LGPL 3 |
| nlohmann/json | Lecture et écriture des projets | MIT |
| Catch2 | Tests | Boost Software License 1.0 |
| three.js | Vue 3D du prototype web | MIT |
| occt-samples-qopenglwidget (Kirill Gavrilov) | Principe d'intégration d'OpenCascade dans Qt, repris et adapté dans `app/OcctGlTools.*` | MIT, notice reproduite dans ces fichiers |
