# Prométhée

Logiciel libre d'ingénierie intégrée : CAO mécanique, mise en plan, CAO électronique et électrique dans un seul modèle. Le cahier des charges complet décrit le produit visé ; ce dépôt en est l'implémentation, construite étape par étape.

Le principe fondateur, déjà démontré : **la carte électronique et son boîtier forment un seul modèle**. Les piliers suivent les trous de fixation, les découpes de paroi suivent les connecteurs, les perçages du couvercle suivent les LED et les boutons, la hauteur du boîtier suit le composant le plus haut, et les vérifications tournent à chaque modification.

## État

Phase 0 (cadrage), voir [ETAT.md](ETAT.md) et [FEUILLE_DE_ROUTE.md](FEUILLE_DE_ROUTE.md).

| Partie | Contenu |
| --- | --- |
| `noyau/` | Modèle unique carte + boîtier, grandeurs dérivées, vérifications et corrections (C++20) |
| `geometrie/` | Pièces exactes en B-rep OpenCascade, export STEP (noms, couleurs) et STL |
| `outils/` | `promethee`, l'outil en ligne de commande (mode sans écran) |
| `tests/` | Tests unitaires, géométriques, et comparaison cas par cas avec le prototype |
| `prototype-web/` | Prototype du jalon 0, implémentation de référence en JavaScript |
| `docs/adr/` | Décisions d'architecture |

## Compiler sous Linux (Ubuntu 24.04)

```sh
sudo apt install cmake ninja-build g++ libtbb-dev catch2 nlohmann-json3-dev \
  libocct-modeling-algorithms-dev libocct-modeling-data-dev \
  libocct-data-exchange-dev libocct-foundation-dev
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows et macOS arrivent avec l'application de bureau (étape 0.5) : la CI produira des paquets prêts à installer.

## Utiliser l'outil

```sh
promethee exemple station.prom.json          # écrit le projet d'exemple
promethee verifier station.prom.json         # vérifie la carte et le boîtier
promethee corriger station.prom.json         # applique les corrections automatiques
promethee exporter station.prom.json sortie  # boitier.step, boitier.stl, couvercle.stl
```

Les fichiers `.prom.json` sont communs au prototype web et au logiciel natif.

## Licence et contributions

GPL-3.0 seule (voir [LICENSE](LICENSE)), sans double licence. Les contributions sont acceptées sous certificat d'origine du développeur (DCO) : chaque commit porte une ligne `Signed-off-by`. Détails dans [CONTRIBUER.md](CONTRIBUER.md).
