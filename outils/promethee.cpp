// Prométhée : outil en ligne de commande (mode sans écran prévu par le CDC). Licence GPL-3.0-only.
#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "promethee/derive.hpp"
#include "promethee/geometrie.hpp"
#include "promethee/kicad.hpp"
#include "promethee/modele.hpp"
#include "promethee/verifs.hpp"

namespace fs = std::filesystem;
using namespace prom;

namespace {

void aide() {
  std::cout <<
      "Prométhée 0.4, outil en ligne de commande\n\n"
      "Utilisation :\n"
      "  promethee verifier <projet.prom.json>             vérifie la carte et le boîtier\n"
      "  promethee corriger <projet.prom.json> [sortie]    applique les corrections automatiques\n"
      "  promethee exporter <projet.prom.json> <dossier>   produit les pièces STEP et STL\n"
      "  promethee exemple <fichier.prom.json>             écrit le projet d’exemple\n"
      "  promethee kicad <carte.kicad_pcb>                 résume une carte KiCad (contour, trous, empreintes)\n"
      "  promethee importer-kicad <carte.kicad_pcb> <projet.prom.json>   crée le projet Prométhée de cette carte\n\n"
      "Les fichiers .prom.json sont ceux du prototype web : un projet passe de l’un à l’autre.\n";
}

int afficherProblemes(const std::vector<Probleme>& pb) {
  int erreurs = 0;
  if (pb.empty()) { std::cout << "Tout est cohérent.\n"; return 0; }
  for (const auto& q : pb) {
    if (q.sev == Severite::Erreur) ++erreurs;
    std::cout << (q.sev == Severite::Erreur ? "  erreur  " : "  alerte  ") << q.msg;
    if (q.fix) std::cout << "  [correction : " << q.fix->libelle << "]";
    std::cout << '\n';
  }
  std::cout << erreurs << (erreurs > 1 ? " erreurs, " : " erreur, ") << (pb.size() - erreurs)
            << (pb.size() - erreurs > 1 ? " alertes.\n" : " alerte.\n");
  return erreurs;
}

void resume(const Projet& p, const Derive& d) {
  std::cout << p.nom << " : carte de " << fmt(p.carte.L) << " × " << fmt(p.carte.W) << " mm, boîtier de "
            << fmt(d.Lo) << " × " << fmt(d.Wo) << " × " << fmt(d.ztop) << " mm couvercle compris, "
            << p.composants.size() << " composants, " << p.trous.size() << " trous.\n";
}

int verifierCmd(const std::string& chemin) {
  const Projet p = lireProjet(chemin);
  const Derive d = deriver(p);
  resume(p, d);
  return afficherProblemes(verifier(p, d)) > 0 ? 1 : 0;
}

int corrigerCmd(const std::string& chemin, const std::string& sortie) {
  Projet p = lireProjet(chemin);
  const auto faites = corrigerTout(p);
  for (const auto& f : faites) std::cout << "  " << f << '\n';
  std::cout << faites.size() << (faites.size() > 1 ? " corrections appliquées.\n" : " correction appliquée.\n");
  ecrireProjet(p, sortie);
  std::cout << "Écrit : " << sortie << '\n';
  return afficherProblemes(verifier(p, deriver(p))) > 0 ? 1 : 0;
}

int exporterCmd(const std::string& chemin, const std::string& dossier) {
  const Projet p = lireProjet(chemin);
  const Derive d = deriver(p);
  resume(p, d);
  const int erreurs = afficherProblemes(verifier(p, d));
  fs::create_directories(dossier);
  const TopoDS_Shape corps = construireCorps(p, d), couvercle = construireCouvercle(p, d);
  for (const auto& [nom, forme] : {std::pair<const char*, const TopoDS_Shape&>{"Boîtier", corps}, {"Couvercle", couvercle}}) {
    if (!estValide(forme) || nombreSolides(forme) != 1) {
      std::cerr << nom << " : la pièce générée n’est pas un solide sain.\n";
      return 2;
    }
  }
  const fs::path base(dossier);
  exporterStep({{"Boîtier", corps, 0.83, 0.85, 0.83}, {"Couvercle", couvercle, 0.90, 0.91, 0.90}}, (base / "boitier.step").string());
  exporterStl(pourImpression(corps, false, d.cx, d.cy), (base / "boitier.stl").string());
  exporterStl(pourImpression(couvercle, true, d.cx, d.cy), (base / "couvercle.stl").string());
  ecrireProjet(p, (base / "projet.prom.json").string());
  std::cout << "Boîtier : " << fmt(volume(corps) / 1000, 2) << " cm³, couvercle : " << fmt(volume(couvercle) / 1000, 2) << " cm³.\n"
            << "Écrit dans " << dossier << " : boitier.step (assemblage, deux pièces), boitier.stl, couvercle.stl, projet.prom.json.\n";
  return erreurs > 0 ? 1 : 0;
}

int kicadCmd(const std::string& chemin) {
  const CarteKicad c = lireFichierKicad(chemin);
  std::cout << (c.titre.empty() ? std::string("Carte KiCad") : c.titre) << " : " << fmt(c.xmax - c.xmin, 2) << " × " << fmt(c.ymax - c.ymin, 2)
            << " mm, épaisseur " << fmt(c.epaisseur, 2) << " mm.\n";
  if (c.contours.empty()) std::cout << "Aucun contour Edge.Cuts.\n";
  for (size_t i = 0; i < c.contours.size(); ++i) {
    const auto& k = c.contours[i];
    const auto arcs = std::count_if(k.elements.begin(), k.elements.end(), [](const ElementContour& e) { return e.milieu.has_value(); });
    std::cout << (i == 0 ? "  contour extérieur : " : "  découpe intérieure : ") << k.elements.size() << " éléments dont " << arcs << " arcs, "
              << (k.ferme ? "fermé" : "OUVERT") << ", " << fmt(std::abs(k.aire()), 1) << " mm².\n";
  }
  int fixations = 0;
  for (const auto& f : c.empreintes)
    if (f.fixation) {
      ++fixations;
      std::cout << "  trou de fixation " << f.ref << " : Ø " << fmt(f.percage, 2) << " mm en (" << fmt(f.position.x - c.xmin, 2) << " ; " << fmt(f.position.y - c.ymin, 2) << ")\n";
    }
  std::cout << c.empreintes.size() << " empreintes, dont " << fixations << " trous de fixation.\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
#endif
  const std::vector<std::string> a(argv + 1, argv + argc);
  try {
    if (a.size() == 2 && a[0] == "verifier") return verifierCmd(a[1]);
    if ((a.size() == 2 || a.size() == 3) && a[0] == "corriger") return corrigerCmd(a[1], a.size() == 3 ? a[2] : a[1]);
    if (a.size() == 3 && a[0] == "exporter") return exporterCmd(a[1], a[2]);
    if (a.size() == 2 && a[0] == "kicad") return kicadCmd(a[1]);
    if (a.size() == 3 && a[0] == "importer-kicad") {
      RapportImport r;
      const Projet p = projetDepuisKicad(lireFichierKicad(a[1]), "", &r);
      ecrireProjet(p, a[2]);
      std::cout << "Projet écrit : contour réel, " << r.trous << " trous de fixation, " << r.composants << " composants (dont " << r.connecteursBord
                << " connecteurs de bord), " << r.dessous << " composants de la face arrière ignorés.\n";
      return 0;
    }
    if (a.size() == 2 && a[0] == "exemple") { ecrireProjet(exemple(), a[1]); std::cout << "Écrit : " << a[1] << '\n'; return 0; }
    aide();
    return a.empty() || a[0] == "aide" || a[0] == "--help" ? 0 : 64;
  } catch (const std::exception& e) {
    std::cerr << "Erreur : " << e.what() << '\n';
    return 2;
  }
}
