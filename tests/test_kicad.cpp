// Lecture de cartes KiCad réelles (démos de KiCad, GPL). Licence GPL-3.0-only.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>

#include "promethee/kicad.hpp"

using namespace prom;
using Catch::Approx;

namespace {
std::string donnee(const char* nom) { return std::string(PROMETHEE_DONNEES) + "/kicad/" + nom; }
const EmpreinteKicad* empreinte(const CarteKicad& c, const std::string& ref) {
  for (const auto& f : c.empreintes) if (f.ref == ref) return &f;
  return nullptr;
}
}  // namespace

TEST_CASE("Une carte KiCad rectangulaire est lue avec ses trous de fixation") {
  const CarteKicad c = lireFichierKicad(donnee("ecc83-pp.kicad_pcb"));
  CHECK(c.epaisseur == Approx(1.6));
  REQUIRE_FALSE(c.contours.empty());
  CHECK(c.contours.front().ferme);
  CHECK(c.contours.front().elements.size() == 4);
  CHECK(c.xmax - c.xmin == Approx(52.07).margin(1e-6));
  CHECK(c.ymax - c.ymin == Approx(46.355).margin(1e-6));
  CHECK(c.contours.front().aire() == Approx(52.07 * 46.355).epsilon(1e-6));
  CHECK(c.empreintes.size() == 15);
  const auto fixations = std::count_if(c.empreintes.begin(), c.empreintes.end(), [](const EmpreinteKicad& f) { return f.fixation; });
  CHECK(fixations == 4);
  for (const auto& f : c.empreintes) if (f.fixation) CHECK(f.percage == Approx(3.2));
  const EmpreinteKicad* c1 = empreinte(c, "C1");
  REQUIRE(c1 != nullptr);
  CHECK(c1->position.x == Approx(141.605));
  CHECK(c1->position.y == Approx(-99.695));  // axe Y retourné
  CHECK(c1->rotation == Approx(90));
  CHECK(c1->xmin < c1->position.x);
  CHECK(c1->xmax > c1->position.x);
  // Zone de placement circulaire de 10,5 mm de diamètre (condensateur radial de 10 mm).
  CHECK(c1->xmax - c1->xmin == Approx(10.5).margin(1e-6));
  CHECK(c1->ymax - c1->ymin == Approx(10.5).margin(1e-6));
}

TEST_CASE("Une carte KiCad au contour arrondi est reconstituée en une boucle fermée") {
  const CarteKicad c = lireFichierKicad(donnee("StickHub.kicad_pcb"));
  REQUIRE_FALSE(c.contours.empty());
  const Contour& ext = c.contours.front();
  CHECK(ext.ferme);
  CHECK(std::any_of(ext.elements.begin(), ext.elements.end(), [](const ElementContour& e) { return e.milieu.has_value(); }));
  CHECK(c.xmax - c.xmin == Approx(16.5).margin(1e-3));
  CHECK(c.ymax - c.ymin == Approx(40).margin(1e-3));
  CHECK(ext.aire() > 0);
  CHECK(ext.aire() < 16.5 * 40);  // les angles arrondis retirent un peu de surface
  CHECK(ext.aire() > 16.5 * 40 * 0.85);  // encoches et arrondis
  CHECK(c.empreintes.size() == 94);
}

TEST_CASE("Un fichier qui n'est pas une carte KiCad est refusé clairement") {
  CHECK_THROWS_WITH(lireKicad("(kicad_sch (version 1))"), Catch::Matchers::ContainsSubstring("pas une carte KiCad"));
  CHECK_THROWS_WITH(lireKicad("(kicad_pcb (general"), Catch::Matchers::ContainsSubstring("tronqué"));
}

#include "promethee/derive.hpp"
#include "promethee/echanges.hpp"
#include "promethee/geometrie.hpp"
#include "promethee/verifs.hpp"

TEST_CASE("Une carte KiCad devient un projet dont le boîtier épouse le contour") {
  RapportImport r;
  const Projet p = projetDepuisKicad(lireFichierKicad(donnee("ecc83-pp.kicad_pcb")), "Préampli ECC83", &r);
  REQUIRE(p.carte.libre());
  CHECK(p.carte.L == Approx(52.07).margin(1e-3));
  CHECK(p.carte.W == Approx(46.355).margin(1e-3));
  CHECK(p.carte.x0 == Approx(0).margin(1e-6));
  CHECK(r.trous == 4);
  CHECK(r.composants == 11);
  CHECK(p.trous.size() == 4);
  for (const auto& t : p.trous) {
    CHECK(t.vis == std::optional<std::string>("M3"));
    CHECK(t.diamTrou == std::optional<double>(3.2));
    CHECK(t.ancrage == "libre");
  }
  const Derive d = deriver(p);
  CHECK(d.libre);
  CHECK(d.sdCarte(0, 0) < 0);
  CHECK(d.sdCarte(100, 0) > 0);
  (void)verifier(p, d);
  const TopoDS_Shape corps = construireCorps(p, d), couvercle = construireCouvercle(p, d), carte = construireCarte(p, d);
  CHECK(estValide(corps));
  CHECK(nombreSolides(corps) == 1);
  CHECK(estValide(couvercle));
  CHECK(estValide(carte));
  const Boite b = encombrement(corps);
  CHECK(b.xmax - b.xmin == Approx(52.07 + 2 * (p.boitier.jeu + p.boitier.paroi)).margin(0.01));
  // En Y, le connecteur P4 dépasse du bord de la carte : le boîtier s'élargit d'autant à cet endroit.
  double ymin = -23.1775, ymax = 23.1775;
  for (const auto& r : d.debords) { ymin = std::min(ymin, r.y1); ymax = std::max(ymax, r.y2); }
  CHECK(b.ymax - b.ymin == Approx(ymax - ymin + 2 * (p.boitier.jeu + p.boitier.paroi)).margin(0.02));
  CHECK(volume(carte) == Approx(52.07 * 46.355 * 1.6 - 4 * 3.14159265 * 1.6 * 1.6 * 1.6).epsilon(0.002));
  // Le projet se relit à l'identique, contour compris.
  const Projet q = normaliser(versJson(p));
  CHECK(versJson(q) == versJson(p));
}

TEST_CASE("Un contour arrondi donne un boîtier arrondi valide et des plans exacts") {
  const Projet p = projetDepuisKicad(lireFichierKicad(donnee("StickHub.kicad_pcb")), "");
  const Derive d = deriver(p);
  const TopoDS_Shape corps = construireCorps(p, d), couvercle = construireCouvercle(p, d);
  CHECK(estValide(corps));
  CHECK(nombreSolides(corps) == 1);
  CHECK(estValide(couvercle));
  CHECK(contourDecale(p, d, p.boitier.jeu).size() > 50);
  const std::string dxf = dxfCarte(p, d);
  CHECK(dxf.find("\r\nARC\r\n") != std::string::npos);
  CHECK(svgCarte(p, d).find("<path") != std::string::npos);
}

TEST_CASE("Les hauteurs sont estimées d'après le nom des empreintes") {
  CHECK(hauteurEstimee("Capacitor_SMD:C_0603_1608Metric") == Approx(0.8));
  CHECK(hauteurEstimee("Package_SO:SOIC-8_3.9x4.9mm_P1.27mm") == Approx(1.75));
  CHECK(hauteurEstimee("Capacitor_THT:CP_Radial_D10.0mm_P5.00mm") == Approx(16));
  CHECK(hauteurEstimee("Connector_USB:USB_C_Receptacle_GCT_USB4085") == Approx(3.3));
  CHECK(hauteurEstimee("Truc:Inconnu") == Approx(2));
}

TEST_CASE("Les composants qui débordent élargissent le boîtier autour d'eux, sans toucher au jeu") {
  const Projet p = projetDepuisKicad(lireFichierKicad(donnee("StickHub.kicad_pcb")), "");
  const Derive d = deriver(p);
  REQUIRE_FALSE(d.debords.empty());  // J5 dépasse du contour dans KiCad
  CHECK(p.boitier.jeu == Approx(1));
  // Plus aucune erreur de débordement : la cavité contourne chaque composant avec le jeu.
  for (const auto& q : verifier(p, d)) CHECK(q.code != "touche_paroi");
  for (const auto& r : d.debords) {
    CHECK(d.sdCavite(r.x1, r.y1) <= -p.boitier.jeu + 1e-6);
    CHECK(d.sdCavite(r.x2, r.y2) <= -p.boitier.jeu + 1e-6);
  }
  const TopoDS_Shape corps = construireCorps(p, d);
  CHECK(estValide(corps));
  CHECK(nombreSolides(corps) == 1);
  CHECK(estValide(construireCouvercle(p, d)));
  // La paroi dessinée en 2D suit aussi l'élargissement.
  const auto paroi = contourDecale(p, d, p.boitier.jeu + p.boitier.paroi);
  double xmax = -1e9, ymax = -1e9;
  for (const auto& q : paroi) { xmax = std::max(xmax, q.x); ymax = std::max(ymax, q.y); }
  double rx = -1e9, ry = -1e9;
  for (const auto& r : d.debords) { rx = std::max(rx, r.x2); ry = std::max(ry, r.y2); }
  CHECK(std::max(xmax - rx, ymax - ry) >= p.boitier.jeu + p.boitier.paroi - 0.05);
}

TEST_CASE("La face arrière est importée et a ses propres vérifications") {
  RapportImport r;
  Projet p = projetDepuisKicad(lireFichierKicad(donnee("StickHub.kicad_pcb")), "", &r);
  CHECK(r.dessous > 0);
  const auto nDessous = std::count_if(p.composants.begin(), p.composants.end(), [](const Composant& k) { return k.dessous; });
  CHECK(nDessous == r.dessous);
  // Un composant de la face arrière plus haut que les entretoises touche le fond : erreur et correction.
  for (auto& k : p.composants) if (k.dessous) { k.h = 7; break; }
  auto pb = verifier(p, deriver(p));
  REQUIRE(std::any_of(pb.begin(), pb.end(), [](const Probleme& q) { return q.code == "dessous_fond"; }));
  double plusHaut = 0;
  for (const auto& k : p.composants) if (k.dessous) plusHaut = std::max(plusHaut, k.h);
  corrigerTout(p);
  CHECK(p.boitier.entretoise == Approx(std::ceil((plusHaut + 0.5) * 2) / 2));
  pb = verifier(p, deriver(p));
  CHECK_FALSE(std::any_of(pb.begin(), pb.end(), [](const Probleme& q) { return q.code == "dessous_fond"; }));
  // La face est enregistrée avec le projet.
  const Projet q = normaliser(versJson(p));
  CHECK(std::count_if(q.composants.begin(), q.composants.end(), [](const Composant& k) { return k.dessous; }) == nDessous);
  // Les composants de la face arrière pendent sous la carte en 3D.
  const Derive d = deriver(p);
  for (const auto& g : d.comps)
    if (g.dessous) { CHECK(encombrement(construireComposant(d, g)).zmax == Approx(d.zpb).margin(1e-6)); break; }
}
