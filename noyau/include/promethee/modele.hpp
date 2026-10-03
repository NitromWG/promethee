// Prométhée : modèle unique carte + boîtier. Licence GPL-3.0-only.
// Port fidèle du noyau du prototype web (jalon 0) : mêmes règles, mêmes valeurs, même format de fichier.
#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace prom {

using Json = nlohmann::ordered_json;

// ---------- Données de référence ----------
struct Vis { double d, trou, avant, tete; };
const Vis& vis(const std::string& nom);              // "M2", "M2.5", "M3"
bool visConnue(const std::string& nom);
const std::vector<std::string>& nomsVis();

// Insert laiton posé à chaud dans un pilier imprimé : diamètre de perçage et longueur.
struct Insert { double trou, longueur; };
const Insert& insert(const std::string& nomVis);

struct Levre {
  static constexpr double jeu = 0.25;  // jeu entre lèvre du couvercle et paroi
  static constexpr double ep = 1.2;    // épaisseur de la lèvre
  static constexpr double h = 2.0;     // hauteur de la lèvre
};
inline constexpr double PAROI_PILIER = 2.4;
inline constexpr std::array<double, 5> EPAISSEURS_PCB{0.8, 1.0, 1.2, 1.6, 2.0};
inline constexpr std::array<int, 10> LONGUEURS_VIS{4, 5, 6, 8, 10, 12, 16, 20, 25, 30};

struct Decoupe { double w = 0, h = 0, r = 0; };

struct TypeComposant {
  std::string cle, nom, prefixe;
  double w = 1, d = 1, h = 1;
  bool bord = false;        // connecteur de bord : perce une paroi
  Decoupe decoupe{};
  double zc = 0;            // hauteur de l'axe au-dessus de la carte
  bool rond = false;
  double couvercle = 0;     // diamètre du perçage dans le couvercle (0 = aucun)
  bool bouton = false;      // doit affleurer le couvercle
  std::string valeur;
};
const TypeComposant* typeComposant(const std::string& cle);   // nullptr si inconnu
const std::vector<TypeComposant>& typesComposants();

enum class Bord { N, S, E, W };
char lettre(Bord b);
std::optional<Bord> bordDe(const std::string& s);
inline bool horizontal(Bord b) { return b == Bord::N || b == Bord::S; }

// ---------- Projet ----------
struct Carte { double x0 = 0, y0 = 0, L = 60, W = 40, r = 2, t = 1.6; };

struct Boitier {
  double jeu = 1, paroi = 2, fond = 2, entretoise = 5, hauteur = 20, couvercle = 2;
  std::string vis = "M3";
  bool hauteurAuto = true;  // la hauteur suit le composant le plus haut
};

struct Trou {
  std::string id, ref;
  std::optional<std::string> coin;  // "NE", "NW", "SE", "SW" : suit ce coin de la carte
  double ox = 4, oy = 4;            // distances au coin (trou ancré)
  double x = 0, y = 0;              // position libre (trou non ancré)
  std::string ancrage = "auto";     // "auto" (coin le plus proche à moins de 15 mm), "libre", ou un coin imposé
  std::optional<std::string> vis;   // vis propre à ce trou ; sinon celle du projet
  std::string fixation = "autotaraudeuse";  // ou "insert" (insert laiton posé à chaud)
};

struct Composant {
  std::string id, ref, type, valeur;
  double w = 1, d = 1, h = 1;
  double x = 0, y = 0;              // composant libre : centre
  int rot = 0;                      // 0, 90, 180, 270
  std::optional<Bord> bord;         // connecteur de bord
  double le_long = 0, ecart = 0.4, zc = 0;
  Decoupe decoupe{};
  std::optional<double> trou_couvercle;
  // Contrainte de position quand la carte change de taille.
  // Composant libre : "libre", "NE", "NW", "SE", "SW" (suit ce coin), "centre" (suit le centre).
  // Connecteur de bord : "libre", "debut", "fin" (suit une extrémité du bord), "milieu".
  std::string ancrage = "libre";
  double ax = 0, ay = 0;            // décalages d'ancrage
  bool verrou = false;              // ne se déplace pas à la souris ni au clavier
};

struct Projet {
  std::string nom = "Projet sans nom";
  Carte carte;
  Boitier boitier;
  std::vector<Trou> trous;
  std::vector<Composant> composants;
};

// Lecture tolérante : toute valeur absente, illisible ou hors bornes est remplacée ou bornée.
Projet normaliser(const Json& source);
Json versJson(const Projet& p);
Projet lireProjet(const std::string& chemin);                 // std::runtime_error si illisible
void ecrireProjet(const Projet& p, const std::string& chemin);
Projet exemple();
Projet projetVide();

// ---------- Outils géométriques ----------
struct Point { double x, y; };
Point posTrou(const Projet& p, const Trou& t);
void ancrerTrou(const Projet& p, Trou& t, double x, double y);  // ancre au coin le plus proche si à moins de 15 mm
void placerSurBord(const Projet& p, Composant& k, double x, double y);
void changerAncrageTrou(const Projet& p, Trou& t, const std::string& ancrage);  // garde la position actuelle
// Position d'un composant selon son ancrage, et placement qui respecte cet ancrage.
Point posComposant(const Projet& p, const Composant& k);
double leLongComposant(const Projet& p, const Composant& k);
void placerLeLong(const Projet& p, Composant& k, double u);  // position le long du bord, selon l'ancrage
void placerComposant(const Projet& p, Composant& k, double x, double y);
void changerAncrage(const Projet& p, Composant& k, const std::string& ancrage);  // garde la position actuelle
void contraindreBords(Projet& p);  // ramène les connecteurs de bord sur la longueur du bord
bool ancrageValide(const Composant& k, const std::string& ancrage);
int longueurVis(double epaisseurCarte, const Vis& v);
// Distance signée d'un point à un rectangle arrondi (négative à l'intérieur).
double sdRR(double px, double py, double cx, double cy, double hx, double hy, double r);
std::string prochaineRef(const Projet& p, const std::string& prefixe, const void* exclure = nullptr);
std::string nouvelId(const std::string& prefixe, const std::vector<std::string>& existants);

// ---------- Texte ----------
// Nombre au format français : virgule décimale, zéros inutiles retirés, signe moins typographique.
std::string fmt(double v, int decimales = 1);
double arrondiJs(double x);  // arrondi identique à Math.round

}  // namespace prom
