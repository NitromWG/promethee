// Prométhée : pièces mécaniques exactes (B-rep OpenCascade) tirées du modèle unique.
// Licence GPL-3.0-only.
#pragma once

#include <string>
#include <vector>

#include <TopoDS_Shape.hxx>

#include "promethee/derive.hpp"
#include "promethee/modele.hpp"

namespace prom {

// Corps du boîtier et couvercle, en position assemblée (coordonnées du modèle, en mm).
TopoDS_Shape construireCorps(const Projet& p, const Derive& d);
TopoDS_Shape construireCouvercle(const Projet& p, const Derive& d);

// Carte électronique percée de ses trous de fixation, et volume simplifié d'un composant (pour l'affichage 3D).
TopoDS_Shape construireCarte(const Projet& p, const Derive& d);
TopoDS_Shape construireComposant(const Derive& d, const GeoComp& g);

double volume(const TopoDS_Shape& s);           // mm³
bool estValide(const TopoDS_Shape& s);          // topologie et géométrie saines
int nombreSolides(const TopoDS_Shape& s);

struct Boite { double xmin = 0, ymin = 0, zmin = 0, xmax = 0, ymax = 0, zmax = 0; };
Boite encombrement(const TopoDS_Shape& s);

// Pièce posée sur le plateau d'impression : centrée, face d'appui à z = 0, couvercle retourné.
TopoDS_Shape pourImpression(const TopoDS_Shape& s, bool retourner, double cx, double cy);

struct PieceNommee {
  std::string nom;
  TopoDS_Shape forme;
  double r = 0.8, g = 0.8, b = 0.8;   // couleur, composantes entre 0 et 1
};
// STEP AP214 avec noms et couleurs, lisible par FreeCAD, KiCad, Fusion, Inventor, SolidWorks.
void exporterStep(const std::vector<PieceNommee>& pieces, const std::string& chemin);
TopoDS_Shape lireStep(const std::string& chemin);
void exporterStl(const TopoDS_Shape& s, const std::string& chemin, double fleche = 0.02);

}  // namespace prom
