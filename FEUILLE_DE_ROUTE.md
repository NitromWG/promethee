# Feuille de route

Le logiciel se construit par lots. Un lot regroupe les étapes qui dépendent les unes des autres : on les mène de front, parce que les faire l'une après l'autre obligerait à défaire puis refaire. Chaque lot se termine par un rendez-vous où Evan essaie le résultat en vrai, sur sa machine ou en atelier, avant qu'on attaque le suivant. Les phases et les jalons sont ceux du cahier des charges (§22).

## Phase 0 : cadrage, jusqu'au jalon 0

| Lot | Étapes menées de front | Pourquoi ensemble | Rendez-vous | État |
| --- | --- | --- | --- | --- |
| 0.A | Cahier des charges ; prototype web du lien carte-boîtier | Le prototype valide la promesse centrale du CDC | Essai du prototype sur téléphone | Fait |
| 0.B | Socle natif C++20 et OpenCascade ; portage vérifié du modèle, des vérifications et des corrections ; pièces exactes, STEP et STL ; outil en ligne de commande ; CI sur Linux, Windows et macOS | Un socle qui ne se compile que sur une plateforme ferait tourner en rond dès l'interface | Pousser le dépôt sur GitHub, la CI passe au vert sur les trois systèmes | Code fait, CI Windows et macOS à valider au premier envoi |
| 0.C | Application de bureau Qt 6 : vue 3D OpenCascade, vue carte 2D, panneau, annuler et rétablir, ouvrir et enregistrer ; installeurs Windows, macOS et Linux produits par la CI | L'interface et ses dépendances (Qt, OpenGL, visualisation OpenCascade) doivent s'installer sur Windows dès la première version | Installer sur son PC et refaire le geste du jalon 0 : déplacer un trou, tirer une paroi | À faire |
| 0.D | Décisions d'architecture sur les points ouverts du CDC (§26), publiées en ADR | Elles conditionnent les lots de la phase 1 | Relecture et arbitrage des ADR | En cours (ADR 0001 et 0002) |

**Jalon 0 natif** : carte et boîtier se modifient l'un l'autre en direct dans le vrai logiciel, installé sous Windows.

## Phase 1 : MVP, jusqu'au jalon 1

| Lot | Étapes menées de front | Pourquoi ensemble | Rendez-vous |
| --- | --- | --- | --- |
| 1.A | Import KiCad (.kicad_pcb) ; export vers KiCad ; format de projet versionné avec migrations | L'aller-retour avec KiCad dicte ce que le format doit stocker : empreintes, modèles 3D, hauteurs, connecteurs de bord | Une vraie carte KiCad importée, son boîtier généré puis imprimé |
| 1.B | Esquisses paramétriques (solveur) ; fonctions volumiques ; arbre de construction ; nommage topologique ; boîtier généré rendu modifiable | Le nommage topologique se conçoit avec l'arbre dès le départ, sinon tout est à reprendre (FreeCAD y a laissé des années) | Modéliser une pièce réelle et ajouter ouvertures et nervures au boîtier |
| 1.C | Mise en plan (vues, coupes, cotation, cartouche) ; exports de fabrication (PDF, DXF, SVG, STEP, STL, 3MF, dossier complet) | Ils partagent la projection à lignes cachées et le modèle de calques | Un plan coté conforme aux habitudes d'un bureau d'études |
| 1.D | Nomenclature unique ; structure du répertoire fournisseurs ; scripts Python et mode sans écran | Mêmes accès au modèle, même interface de programmation | Un script qui génère une variante de produit |
| 1.E | Sauvegarde automatique, récupération après plantage, performances ; documentation ; projet témoin | La robustesse se mesure sur le projet témoin | Le projet témoin du jalon 1 de bout en bout |

**Jalon 1** : une carte et son boîtier imprimé conçus, mis en plan et exportés.

## Phase 2 : v1.0 et parité, jusqu'au jalon 2

| Lot | Étapes menées de front | Pourquoi ensemble |
| --- | --- | --- |
| 2.A | Assemblages, contraintes, détection d'interférences | Les contraintes et les interférences portent sur les mêmes liaisons |
| 2.B | Schéma électronique ; routage du circuit imprimé ; co-conception avancée (pont KiCad) | Schéma, circuit et mécanique doivent partager les mêmes composants |
| 2.C | CAO électrique : schémas, borniers, câblage, armoires, faisceaux | Liée au lot 2.B par les composants et les connecteurs |
| 2.D | Simulation mécanique et thermique (CalculiX), électronique (ngspice) | Maillage, matériaux et conditions aux limites communs |
| 2.E | Fabrication (usinage, impression, découpe) ; interopérabilité étendue (IGES, DWG, 3MF, imports Altium et EAGLE) | Les sorties de fabrication sont aussi des formats d'échange |
| 2.F | Versions et collaboration ; accessibilité ; traductions ; gros projets ; projet témoin complet | La parité se mesure sur le projet témoin complet (§24) |

**Jalon 2** : projet témoin complet, parité d'au moins 90 %.

## Phase 3 : v2, dépassement, jusqu'au jalon 3

| Lot | Étapes menées de front |
| --- | --- |
| 3.A | Assistant IA local branché sur le modèle unique ; répertoire fournisseurs rempli (procédés, coûts, délais) |
| 3.B | Conception générative, multiphysique couplée, fonctions sans équivalent |

**Jalon 3** : fonctions sans équivalent validées par des utilisateurs.

## Phase 4 : vision

En continu, selon les retours de la communauté.
