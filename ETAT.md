# État du projet

Mis à jour le 2 octobre 2026. Ce fichier est la mémoire du projet : chaque session commence par le lire et finit par le mettre à jour.

## Où on en est

Lot 0.C, version 0.3 : l'application de bureau intègre les retours du premier essai sous Windows. Chaque trou a sa vis (M2 à M4) et son montage (vis autotaraudeuse ou insert laiton posé à chaud), et peut suivre le coin le plus proche, un coin imposé ou rester à sa place. Chaque composant a un ancrage (rester à sa place, suivre un coin, le centre, ou une extrémité du bord pour les connecteurs) et peut être verrouillé ; la vue Carte dessine en pointillés cuivre la contrainte de l'élément sélectionné. La vue 3D a un cube de vue cliquable, des vues normalisées animées et des surfaces plus lisses. L'export propose douze formats (STEP, IGES, BREP, STL binaire et texte, 3MF, OBJ, PLY, DXF, SVG, CSV, projet). L'interface a un thème clair cohérent, quel que soit le thème de Windows.

Le logiciel sait aujourd'hui lire un projet (même format que le prototype web), dériver tout le boîtier de la carte, vérifier 28 règles de conception avec les mêmes messages que le prototype, appliquer les corrections automatiques, construire le corps et le couvercle en solides exacts OpenCascade, et les exporter en STEP (noms et couleurs) et en STL posés sur le plateau. Tout est accessible par l'outil `promethee` en ligne de commande et par l'application de bureau `Promethee` : vue Carte (glisser les composants, les trous et les bords de la carte), vue Boîtier en 3D OpenCascade qui suit en direct (couvercle ouvert, fermé ou éclaté), panneau de propriétés, vérifications avec corrections en un clic, nomenclature, annuler et rétablir, ouverture et enregistrement des `.prom.json`, export du dossier de fabrication (STEP avec la carte, STL).

| Indicateur | Valeur |
| --- | --- |
| Tests | 29 sur 29, dont 6 comparaisons cas par cas avec le prototype et 8 essais des fonctions de la version 0.3 |
| Écart de volume avec le prototype | moins de 0,3 % (pièces exactes contre maillage fin) |
| Compilation automatique | Linux (OpenCascade 7.6), Windows (MSVC, OpenCascade 8 par vcpkg), macOS (OpenCascade 8 par Homebrew), toutes au vert |
| Avertissements du compilateur | aucun (-Wall -Wextra -Wpedantic) |

## Décisions prises

- [ADR 0001](docs/adr/0001-socle-technique.md) : C++20, CMake, OpenCascade, Qt 6, format `.prom.json` commun avec le prototype.
- [ADR 0002](docs/adr/0002-integration-kicad-freecad.md) : en phase 1, intégration avec KiCad et FreeCAD par les formats de fichiers, sans fusionner leurs codes.

## Rendez-vous en cours (version 0.3)

Envoyer le contenu du zip sur le dépôt, puis essayer le nouveau paquet Windows : régler la vis et l'insert d'un trou, ancrer un composant à un coin puis agrandir la carte, verrouiller un composant, utiliser le cube de vue, exporter dans plusieurs formats et ouvrir le STEP dans Inventor et le 3MF dans un trancheur.

## Retours du premier essai (version 0.2, 2 octobre 2026)

Ce qui marche : ajouter un trou, agrandir la carte par les poignées ou les valeurs avec le reste qui suit, changer la hauteur d'un composant, naviguer en 3D, exporter. Ce qui manquait et a été traité en 0.3 : réglages des trous, contraintes pour choisir ce qui suit la carte et ce qui reste fixe, rotation 3D et surfaces moins lisses que SolidWorks, choix des formats d'export, interface peu soignée. Ce qui reste demandé : de vraies contraintes entre éléments (lot 1.B), beaucoup plus de fonctions de CAO.

## Dernier rendez-vous (lot 0.B)

Dépôt créé et rempli, compilation automatique au vert sur les trois systèmes. Corrections en route : fichiers commençant par un point ajoutés à la main, paquets X11 pour Linux, cache Windows conservé même en cas d'échec (OpenCascade se compile une fois en 88 minutes, ensuite en 30 secondes), manifeste UTF-8 sous Windows, test qui effaçait un fichier encore ouvert.

## Suite du lot 0.C

Glisser directement dans la vue 3D (parois, composants, piliers) ; nomenclature avec masses et volumes ; exports DXF et plan coté repris du prototype ; préférences mémorisées ; installeurs signés pour Windows et macOS au lieu du paquet à décompresser ; paquet Linux (AppImage).

## Reprendre une session

Lire ce fichier, [FEUILLE_DE_ROUTE.md](FEUILLE_DE_ROUTE.md) et `docs/adr/`. Compiler et tester (voir le README). Le prototype web (`prototype-web/`) reste l'implémentation de référence tant que la parité n'est pas complète : après toute modification de son noyau, régénérer les cas de référence avec `npm install` puis `npm run reference` dans `prototype-web/`, et relancer les tests C++.

## Points de vigilance

- OpenCascade a renommé ses boîtes à outils d'échange en 7.8 ; `geometrie/CMakeLists.txt` choisit celles qui existent.
- L'extension Chrome de Claude ne sait pas téléverser de fichiers : Claude modifie le dépôt fichier par fichier dans l'éditeur de GitHub, et les gros envois se font par glisser-déposer du contenu d'un dossier (sans les fichiers commençant par un point, à ajouter à part).
- Chaque envoi relance la compilation sur les trois systèmes ; un nouvel envoi annule la compilation en cours.
