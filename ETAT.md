# État du projet

Mis à jour le 2 octobre 2026. Ce fichier est la mémoire du projet : chaque session commence par le lire et finit par le mettre à jour.

## Où on en est

Lot 0.B terminé côté code : le socle natif existe et il est vérifié. Prochain lot : 0.C, l'application de bureau.

Le logiciel sait aujourd'hui lire un projet (même format que le prototype web), dériver tout le boîtier de la carte, vérifier 28 règles de conception avec les mêmes messages que le prototype, appliquer les corrections automatiques, construire le corps et le couvercle en solides exacts OpenCascade, et les exporter en STEP (noms et couleurs) et en STL posés sur le plateau. Tout est accessible par l'outil `promethee` en ligne de commande.

| Indicateur | Valeur |
| --- | --- |
| Tests | 20 sur 20, dont 5 comparaisons cas par cas avec le prototype sur 9 projets |
| Écart de volume avec le prototype | moins de 0,3 % (pièces exactes contre maillage fin) |
| Compatibilité OpenCascade | 7.6 compilée et testée ; 8.0.1 vérifiée à la compilation sur ses en-têtes |
| Avertissements du compilateur | aucun (-Wall -Wextra -Wpedantic) |

## Décisions prises

- [ADR 0001](docs/adr/0001-socle-technique.md) : C++20, CMake, OpenCascade, Qt 6, format `.prom.json` commun avec le prototype.
- [ADR 0002](docs/adr/0002-integration-kicad-freecad.md) : en phase 1, intégration avec KiCad et FreeCAD par les formats de fichiers, sans fusionner leurs codes.

## Rendez-vous en cours

Le dépôt public existe : https://github.com/NitromWG/promethee. Il reste à y envoyer les fichiers, puis à vérifier que la compilation automatique passe au vert sur Linux, Windows et macOS.

Envoi sans ligne de commande : décompresser le zip, ouvrir le dossier `promethee`, tout sélectionner à l'intérieur (y compris `.github` et `.gitignore`), glisser la sélection sur https://github.com/NitromWG/promethee/upload, puis cliquer « Commit changes ». Il faut glisser le contenu du dossier et non le dossier lui-même, sinon le fichier de CI se retrouve au mauvais endroit.

Ensuite, onglet Actions du dépôt : les trois tâches doivent passer au vert. Le premier passage Windows compile OpenCascade et prend environ une heure, les suivants sont rapides grâce au cache. Si une tâche échoue, copier la fin de son journal dans la conversation.

## Prochain lot : 0.C

Application de bureau Qt 6, construite et empaquetée sur les trois systèmes dès la première version : fenêtre principale, vue 3D OpenCascade (AIS dans un QOpenGLWidget), vue carte 2D (portage de celle du prototype), panneau de propriétés, vérifications, nomenclature, annuler et rétablir, ouvrir et enregistrer, puis installeurs produits par la CI.

## Reprendre une session

Lire ce fichier, [FEUILLE_DE_ROUTE.md](FEUILLE_DE_ROUTE.md) et `docs/adr/`. Compiler et tester (voir le README). Le prototype web (`prototype-web/`) reste l'implémentation de référence tant que la parité n'est pas complète : après toute modification de son noyau, régénérer les cas de référence avec `npm install` puis `npm run reference` dans `prototype-web/`, et relancer les tests C++.

## Points de vigilance

- OpenCascade a renommé ses boîtes à outils d'échange en 7.8 ; `geometrie/CMakeLists.txt` choisit celles qui existent.
- La CI Windows et macOS n'a pas encore tourné : à confirmer au premier envoi.
