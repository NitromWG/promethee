// Prométhée : formats d'échange et de fabrication. Licence GPL-3.0-only.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "promethee/geometrie.hpp"

namespace prom {

// Pièces exactes (B-rep)
void exporterIges(const std::vector<PieceNommee>& pieces, const std::string& chemin);
void exporterBrep(const std::vector<PieceNommee>& pieces, const std::string& chemin);  // format natif OpenCascade
// Maillages (impression 3D, rendu, visualiseurs)
void exporterStlTexte(const TopoDS_Shape& s, const std::string& chemin, double fleche = 0.02);
void exporterObj(const std::vector<PieceNommee>& pieces, const std::string& chemin, double fleche = 0.02);  // avec son .mtl
void exporterPly(const std::vector<PieceNommee>& pieces, const std::string& chemin, double fleche = 0.02);
void exporter3mf(const std::vector<PieceNommee>& pieces, const std::string& chemin, double fleche = 0.02);
// Plans 2D de la carte (millimètres, origine au coin inférieur gauche)
std::string dxfCarte(const Projet& p, const Derive& d);  // DXF R12 : CONTOUR, PERCAGES, COMPOSANTS
std::string svgCarte(const Projet& p, const Derive& d);  // plan coté à l'échelle 1
// Archive zip sans compression
std::string zipStocke(const std::vector<std::pair<std::string, std::string>>& fichiers);

}  // namespace prom
