# État du projet

Mis à jour le 2 octobre 2026. Ce fichier est la mémoire du projet : chaque session commence par le lire et finit par le mettre à jour.

## Où on en est

Lot 0.B terminé le 2 octobre 2026 (socle natif au vert sur Linux, Windows et macOS). Lot 0.C en cours : la première version de l'application de bureau est écrite et vérifiée sous Linux, et la CI la construit sur les trois systèmes avec un paquet Windows prêt à lancer.

Le logiciel sait aujourd'hui lire un projet (même format que le prototype web), dériver tout le boîtier de la carte, vérifier 28 règles de conception avec les mêmes messages que le prototype, appliquer les corrections automatiques, construire le corps et le couvercle en solides exacts OpenCascade, et les exporter en STEP (noms et couleurs) et en STL posés sur le plateau. Tout est accessible par l'outil `promethee` en ligne de commande et par l'application de bureau `Promethee` : vue Carte (glisser les composants, les trous et les bords de la carte), vue Boîtier en 3D OpenCascade qui suit en direct (couvercle ouvert, fermé ou éclaté), panneau de propriétés, vérifications avec corrections en un clic, nomenclature, annuler et rétablir, ouverture et enregistrement des `.prom.json`, export du dossier de fabrication (STEP avec la carte, STL).

| Indicateur | Valeur |
| --- | --- |
| Tests | 21 sur 21, dont 6 comparaisons cas par cas avec le prototype (placement automatique compris) |
| Écart de volume avec le prototype | moins de 0,3 % (pièces exactes contre maillage fin) |
| Compilation automatique | Linux (OpenCascade 7.6), Windows (MSVC, OpenCascade 8 par vcpkg), macOS (OpenCascade 8 par Homebrew), toutes au vert |
| Avertissements du compilateur | aucun (-Wall -Wextra -Wpedantic) |

## Décisions prises

- [ADR 0001](docs/adr/0001-socle-technique.md) : C++20, CMake, OpenCascade, Qt 6, format `.prom.json` commun avec le prototype.
- [ADR 0002](docs/adr/0002-integration-kicad-freecad.md) : en phase 1, intégration avec KiCad et FreeCAD par les formats de fichiers, sans fusionner leurs codes.

## Rendez-vous en cours (lot 0.C, première version)

Envoyer le contenu du zip sur le dépôt (glisser-déposer sur la page d'envoi de GitHub), Claude met à jour le fichier de CI et le `.gitignore` dans l'éditeur de GitHub. Puis, dans l'onglet Actions, télécharger le paquet `Promethee-Windows` de la dernière compilation au vert, le décompresser, lancer `Promethee.exe`, et refaire le geste du jalon 0 : déplacer un trou, tirer un bord de la carte, ajouter une LED, agrandir un condensateur.

À rapporter : ce qui ne marche pas, ce qui surprend, ce qui manque par rapport à un logiciel de CAO habituel.

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
