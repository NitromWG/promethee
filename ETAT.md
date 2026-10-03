# État du projet

Mis à jour le 3 octobre 2026. Ce fichier est la mémoire du projet : chaque session commence par le lire et finit par le mettre à jour.

## Où on en est

Version 0.4, lot 1.A bien avancé : Prométhée importe une vraie carte KiCad et dessine un boîtier qui épouse son contour. Le menu Fichier, Importer une carte KiCad (Ctrl+I), lit le .kicad_pcb, reprend le contour Edge.Cuts (segments, arcs, cercles, découpes intérieures), les trous de fixation avec leur perçage, et les composants de la face avant avec leur encombrement ; les hauteurs sont estimées d'après le nom des empreintes. Les parois, la lèvre et le couvercle sont des décalages exacts du contour (OpenCascade), les découpes de connecteurs se posent au droit du bord réel, et les exports DXF et SVG reprennent le contour avec ses arcs. Les composants placés par KiCad peuvent se chevaucher et déborder de la carte sans fausse erreur ; un débordement qui touche la paroi propose d'augmenter le jeu. Les noms des pièces sont écrits sans accents dans le STEP. Vérifié sur deux cartes de démonstration de KiCad (préampli à lampe ECC83, contour rectangulaire avec quatre trous M3 ; StickHub, contour arrondi à encoche).

Version 0.3.1 : rotation 3D libre sans butée, point du pavé numérique, verrou visible (panneau, barre d'outils, clic droit), dimensions des trous personnalisables, vis M5.

Le logiciel sait aujourd'hui lire un projet (même format que le prototype web), dériver tout le boîtier de la carte, vérifier 28 règles de conception avec les mêmes messages que le prototype, appliquer les corrections automatiques, construire le corps et le couvercle en solides exacts OpenCascade, et les exporter en STEP (noms et couleurs) et en STL posés sur le plateau. Tout est accessible par l'outil `promethee` en ligne de commande et par l'application de bureau `Promethee` : vue Carte (glisser les composants, les trous et les bords de la carte), vue Boîtier en 3D OpenCascade qui suit en direct (couvercle ouvert, fermé ou éclaté), panneau de propriétés, vérifications avec corrections en un clic, nomenclature, annuler et rétablir, ouverture et enregistrement des `.prom.json`, export du dossier de fabrication (STEP avec la carte, STL).

| Indicateur | Valeur |
| --- | --- |
| Tests | 36 sur 36, dont 6 comparaisons cas par cas avec le prototype, 9 essais des fonctions de la version 0.3 et 6 essais sur des cartes KiCad réelles |
| Écart de volume avec le prototype | moins de 0,3 % (pièces exactes contre maillage fin) |
| Compilation automatique | Linux (OpenCascade 7.6), Windows (MSVC, OpenCascade 8 par vcpkg), macOS (OpenCascade 8 par Homebrew), toutes au vert |
| Avertissements du compilateur | aucun (-Wall -Wextra -Wpedantic) |

## Décisions prises

- [ADR 0001](docs/adr/0001-socle-technique.md) : C++20, CMake, OpenCascade, Qt 6, format `.prom.json` commun avec le prototype.
- [ADR 0002](docs/adr/0002-integration-kicad-freecad.md) : en phase 1, intégration avec KiCad et FreeCAD par les formats de fichiers, sans fusionner leurs codes.

## Version 0.3.1 (retours du deuxième essai)

La rotation 3D est réécrite : glisser au bouton gauche fait tourner le modèle autour de son centre dans les axes de l'écran, sans aucune butée (un tour complet par-dessus le modèle est vérifié automatiquement). Les champs numériques acceptent le point du pavé numérique. Le verrou est visible en tête du panneau, dans la barre d'outils (Ctrl+L) et dans le menu du clic droit sur un composant, qui propose aussi pivoter, ancrer et supprimer. Chaque trou a des dimensions personnalisables (trou, pastille, pilier, logement, longueur d'insert) et la vis M5 s'ajoute.

## Retours du deuxième essai (version 0.3, 2 octobre 2026)

Inserts et vis faciles à régler, mais choix trop limité et sans personnalisation (traité en 0.3.1). Rotation de la caméra bloquée à certains angles, point essentiel (traité). Point du pavé numérique refusé (traité). Verrou introuvable (traité). Contraintes jugées sommaires : les cartes réelles ont des formes complexes, pas des rectangles ; c'est l'objet du lot suivant. Pas de logiciel de CAO payant pour vérifier les exports : utiliser FreeCAD, un trancheur gratuit, KiCad ou un visualiseur en ligne.

## Rendez-vous en cours (version 0.4)

Envoyer le contenu du zip sur le dépôt, puis dans le paquet Windows : importer une carte KiCad (des cartes libres se trouvent sur GitHub, par exemple les démonstrations de KiCad), regarder le boîtier épouser le contour, corriger les hauteurs estimées des grands composants, exporter et vérifier le STL dans un trancheur et le STEP dans FreeCAD.

## Suite du lot 1.A

Composants de la face arrière (le boîtier doit leur laisser la place sous la carte), connecteurs de bord reconnus plus largement, hauteurs tirées des modèles 3D de KiCad quand ils existent, ancrages relatifs aux éléments du contour plutôt qu'aux coins de l'encombrement, export vers KiCad du contour et des trous modifiés dans Prométhée, format de projet versionné. Puis les vraies contraintes entre éléments (lot 1.B).

## Retours du premier essai (version 0.2, 2 octobre 2026)

Ce qui marche : ajouter un trou, agrandir la carte par les poignées ou les valeurs avec le reste qui suit, changer la hauteur d'un composant, naviguer en 3D, exporter. Ce qui manquait et a été traité en 0.3 : réglages des trous, contraintes pour choisir ce qui suit la carte et ce qui reste fixe, rotation 3D et surfaces moins lisses que SolidWorks, choix des formats d'export, interface peu soignée. Ce qui reste demandé : de vraies contraintes entre éléments (lot 1.B), beaucoup plus de fonctions de CAO.

## Dernier rendez-vous (lot 0.B)

Dépôt créé et rempli, compilation automatique au vert sur les trois systèmes. Corrections en route : fichiers commençant par un point ajoutés à la main, paquets X11 pour Linux, cache Windows conservé même en cas d'échec (OpenCascade se compile une fois en 88 minutes, ensuite en 30 secondes), manifeste UTF-8 sous Windows, test qui effaçait un fichier encore ouvert.

## Reste du lot 0.C

Glisser directement dans la vue 3D (parois, composants, piliers) ; nomenclature avec masses et volumes ; exports DXF et plan coté repris du prototype ; préférences mémorisées ; installeurs signés pour Windows et macOS au lieu du paquet à décompresser ; paquet Linux (AppImage).

## Reprendre une session

Lire ce fichier, [FEUILLE_DE_ROUTE.md](FEUILLE_DE_ROUTE.md) et `docs/adr/`. Compiler et tester (voir le README). Le prototype web (`prototype-web/`) reste l'implémentation de référence tant que la parité n'est pas complète : après toute modification de son noyau, régénérer les cas de référence avec `npm install` puis `npm run reference` dans `prototype-web/`, et relancer les tests C++.

## Points de vigilance

- OpenCascade a renommé ses boîtes à outils d'échange en 7.8 ; `geometrie/CMakeLists.txt` choisit celles qui existent.
- L'extension Chrome de Claude ne sait pas téléverser de fichiers : Claude modifie le dépôt fichier par fichier dans l'éditeur de GitHub, et les gros envois se font par glisser-déposer du contenu d'un dossier (sans les fichiers commençant par un point, à ajouter à part).
- Chaque envoi relance la compilation sur les trois systèmes ; un nouvel envoi annule la compilation en cours.
