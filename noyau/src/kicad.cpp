// Prométhée : lecture des cartes KiCad (.kicad_pcb). Licence GPL-3.0-only.
#include "promethee/kicad.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <numbers>
#include <sstream>
#include <stdexcept>

namespace prom {

namespace {

// ---------- Expressions S ----------
struct Expr {
  bool liste = false;
  std::string atome;
  std::vector<Expr> elements;
  const std::string& nom() const { static const std::string vide; return liste && !elements.empty() && !elements[0].liste ? elements[0].atome : vide; }
  const Expr* enfant(const std::string& n) const {
    for (const auto& e : elements) if (e.nom() == n) return &e;
    return nullptr;
  }
  std::string valeur(size_t i) const { return i < elements.size() && !elements[i].liste ? elements[i].atome : std::string(); }
  double nombre(size_t i, double def = 0) const {
    const std::string v = valeur(i);
    if (v.empty()) return def;
    char* fin = nullptr;
    const double x = std::strtod(v.c_str(), &fin);
    return fin == v.c_str() ? def : x;
  }
};

class Lecteur {
public:
  explicit Lecteur(const std::string& t) : m_t(t) {}
  Expr lire() {
    blancs();
    if (m_i >= m_t.size()) throw std::runtime_error("Fichier KiCad vide.");
    return expression();
  }

private:
  void blancs() { while (m_i < m_t.size() && std::isspace(static_cast<unsigned char>(m_t[m_i]))) ++m_i; }
  Expr expression() {
    blancs();
    Expr e;
    if (m_t[m_i] == '(') {
      e.liste = true;
      ++m_i;
      for (;;) {
        blancs();
        if (m_i >= m_t.size()) throw std::runtime_error("Fichier KiCad tronqué : parenthèse non fermée.");
        if (m_t[m_i] == ')') { ++m_i; break; }
        e.elements.push_back(expression());
      }
    } else if (m_t[m_i] == '"') {
      ++m_i;
      while (m_i < m_t.size() && m_t[m_i] != '"') {
        if (m_t[m_i] == '\\' && m_i + 1 < m_t.size()) ++m_i;
        e.atome += m_t[m_i++];
      }
      ++m_i;
    } else {
      while (m_i < m_t.size() && !std::isspace(static_cast<unsigned char>(m_t[m_i])) && m_t[m_i] != '(' && m_t[m_i] != ')') e.atome += m_t[m_i++];
    }
    return e;
  }
  const std::string& m_t;
  size_t m_i = 0;
};

// KiCad : Y vers le bas ; Prométhée : Y vers le haut.
Point pt(const Expr& e) { return {e.nombre(1), -e.nombre(2)}; }

std::string couche(const Expr& e) {
  const Expr* c = e.enfant("layer");
  return c ? c->valeur(1) : std::string();
}

bool proches(const Point& a, const Point& b) { return std::hypot(a.x - b.x, a.y - b.y) < 1e-3; }

// Centre et rayon du cercle passant par trois points.
bool cercle(const Point& a, const Point& b, const Point& c, Point& centre, double& r) {
  const double d = 2 * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
  if (std::abs(d) < 1e-12) return false;
  const double a2 = a.x * a.x + a.y * a.y, b2 = b.x * b.x + b.y * b.y, c2 = c.x * c.x + c.y * c.y;
  centre = {(a2 * (b.y - c.y) + b2 * (c.y - a.y) + c2 * (a.y - b.y)) / d, (a2 * (c.x - b.x) + b2 * (a.x - c.x) + c2 * (b.x - a.x)) / d};
  r = std::hypot(a.x - centre.x, a.y - centre.y);
  return true;
}

ElementContour inverse(const ElementContour& e) { return {e.fin, e.debut, e.milieu}; }

// Reconstitue des boucles à partir d'éléments donnés dans le désordre.
std::vector<Contour> chainer(std::vector<ElementContour> morceaux) {
  std::vector<Contour> res;
  while (!morceaux.empty()) {
    Contour c;
    c.elements.push_back(morceaux.back());
    morceaux.pop_back();
    bool progres = true;
    while (progres && !proches(c.elements.front().debut, c.elements.back().fin)) {
      progres = false;
      for (size_t i = 0; i < morceaux.size(); ++i) {
        if (proches(morceaux[i].debut, c.elements.back().fin)) c.elements.push_back(morceaux[i]);
        else if (proches(morceaux[i].fin, c.elements.back().fin)) c.elements.push_back(inverse(morceaux[i]));
        else continue;
        morceaux.erase(morceaux.begin() + static_cast<long>(i));
        progres = true;
        break;
      }
    }
    c.ferme = proches(c.elements.front().debut, c.elements.back().fin);
    res.push_back(c);
  }
  // Le contour extérieur, le plus grand, en premier ; tous orientés dans le sens direct.
  for (auto& c : res)
    if (c.aire() < 0) {
      std::reverse(c.elements.begin(), c.elements.end());
      for (auto& e : c.elements) e = inverse(e);
    }
  std::sort(res.begin(), res.end(), [](const Contour& a, const Contour& b) { return std::abs(a.aire()) > std::abs(b.aire()); });
  return res;
}

void etendre(EmpreinteKicad& f, const Point& p) {
  f.xmin = std::min(f.xmin, p.x); f.xmax = std::max(f.xmax, p.x);
  f.ymin = std::min(f.ymin, p.y); f.ymax = std::max(f.ymax, p.y);
}

}  // namespace

CarteKicad lireKicad(const std::string& texte) {
  const Expr racine = Lecteur(texte).lire();
  if (racine.nom() != "kicad_pcb") throw std::runtime_error("Ce fichier n’est pas une carte KiCad (.kicad_pcb).");
  CarteKicad carte;
  if (const Expr* g = racine.enfant("general"))
    if (const Expr* t = g->enfant("thickness")) carte.epaisseur = t->nombre(1, 1.6);
  if (const Expr* tb = racine.enfant("title_block"))
    if (const Expr* t = tb->enfant("title")) carte.titre = t->valeur(1);
  std::vector<ElementContour> morceaux;
  for (const auto& e : racine.elements) {
    const std::string& n = e.nom();
    if (n.rfind("gr_", 0) != 0 || couche(e) != "Edge.Cuts") continue;
    const Expr* debut = e.enfant("start");
    const Expr* fin = e.enfant("end");
    if (n == "gr_line" && debut && fin) {
      morceaux.push_back({pt(*debut), pt(*fin), std::nullopt});
    } else if (n == "gr_arc" && debut && fin && e.enfant("mid")) {
      morceaux.push_back({pt(*debut), pt(*fin), pt(*e.enfant("mid"))});
    } else if (n == "gr_rect" && debut && fin) {
      const Point a = pt(*debut), b = pt(*fin);
      morceaux.push_back({{a.x, a.y}, {b.x, a.y}, std::nullopt});
      morceaux.push_back({{b.x, a.y}, {b.x, b.y}, std::nullopt});
      morceaux.push_back({{b.x, b.y}, {a.x, b.y}, std::nullopt});
      morceaux.push_back({{a.x, b.y}, {a.x, a.y}, std::nullopt});
    } else if (n == "gr_circle" && e.enfant("center") && fin) {
      const Point c = pt(*e.enfant("center")), f2 = pt(*fin);
      const double r = std::hypot(f2.x - c.x, f2.y - c.y);
      morceaux.push_back({{c.x + r, c.y}, {c.x - r, c.y}, Point{c.x, c.y + r}});
      morceaux.push_back({{c.x - r, c.y}, {c.x + r, c.y}, Point{c.x, c.y - r}});
    } else if (n == "gr_poly") {
      if (const Expr* pts = e.enfant("pts")) {
        std::vector<Point> p;
        for (const auto& xy : pts->elements) if (xy.nom() == "xy") p.push_back(pt(xy));
        for (size_t i = 0; i + 1 < p.size() + 1 && p.size() > 1; ++i) morceaux.push_back({p[i], p[(i + 1) % p.size()], std::nullopt});
      }
    }
  }
  carte.contours = chainer(morceaux);
  if (!carte.contours.empty()) {
    const auto poly = carte.contours.front().polygone();
    carte.xmin = carte.ymin = std::numeric_limits<double>::max();
    carte.xmax = carte.ymax = std::numeric_limits<double>::lowest();
    for (const auto& q : poly) {
      carte.xmin = std::min(carte.xmin, q.x); carte.xmax = std::max(carte.xmax, q.x);
      carte.ymin = std::min(carte.ymin, q.y); carte.ymax = std::max(carte.ymax, q.y);
    }
  }
  for (const auto& e : racine.elements) {
    if (e.nom() != "footprint" && e.nom() != "module") continue;
    EmpreinteKicad f;
    f.nom = e.valeur(1);
    f.dessous = couche(e) == "B.Cu";
    if (const Expr* at = e.enfant("at")) { f.position = pt(*at); f.rotation = at->nombre(3, 0); }
    for (const auto& s : e.elements) {
      if (s.nom() == "property") {
        if (s.valeur(1) == "Reference") f.ref = s.valeur(2);
        else if (s.valeur(1) == "Value") f.valeur = s.valeur(2);
      } else if (s.nom() == "fp_text") {
        if (s.valeur(1) == "reference") f.ref = s.valeur(2);
        else if (s.valeur(1) == "value") f.valeur = s.valeur(2);
      }
    }
    // Repère local de l'empreinte (Y vers le haut) tourné de sa rotation, puis placé.
    const double a = f.rotation * std::numbers::pi / 180;
    auto monde = [&](const Expr& x) {
      const double lx = x.nombre(1), ly = -x.nombre(2);
      return Point{f.position.x + lx * std::cos(a) - ly * std::sin(a), f.position.y + lx * std::sin(a) + ly * std::cos(a)};
    };
    f.xmin = f.ymin = std::numeric_limits<double>::max();
    f.xmax = f.ymax = std::numeric_limits<double>::lowest();
    bool zone = false;
    for (const auto& s : e.elements) {
      const std::string c = couche(s);
      if ((c == "F.CrtYd" || c == "B.CrtYd") && (s.nom() == "fp_line" || s.nom() == "fp_rect" || s.nom() == "fp_circle" || s.nom() == "fp_poly" || s.nom() == "fp_arc")) {
        zone = true;
        if (s.nom() == "fp_circle" && s.enfant("center") && s.enfant("end")) {
          // Cercle : son encombrement est un carré de côté 2r autour du centre, quelle que soit la rotation.
          const Expr& c = *s.enfant("center");
          const double r = std::hypot(s.enfant("end")->nombre(1) - c.nombre(1), s.enfant("end")->nombre(2) - c.nombre(2));
          const Point m = monde(c);
          etendre(f, {m.x - r, m.y - r});
          etendre(f, {m.x + r, m.y + r});
          continue;
        }
        for (const char* cle : {"start", "end", "mid", "center"})
          if (const Expr* q = s.enfant(cle)) etendre(f, monde(*q));
        if (const Expr* pts = s.enfant("pts"))
          for (const auto& xy : pts->elements) if (xy.nom() == "xy") etendre(f, monde(xy));
      }
    }
    for (const auto& s : e.elements) {
      if (s.nom() != "pad") continue;
      const Expr* at = s.enfant("at");
      const Expr* taille = s.enfant("size");
      if (const Expr* dr = s.enfant("drill")) f.percage = std::max(f.percage, dr->nombre(1, 0));
      if (zone || !at || !taille) continue;
      const Point c = monde(*at);
      const double r = std::max(taille->nombre(1), taille->nombre(2)) / 2;
      etendre(f, {c.x - r, c.y - r});
      etendre(f, {c.x + r, c.y + r});
    }
    if (f.xmin > f.xmax) { f.xmin = f.xmax = f.position.x; f.ymin = f.ymax = f.position.y; }
    f.fixation = f.nom.find("MountingHole") != std::string::npos || (f.ref.rfind("H", 0) == 0 && f.percage >= 2.0);
    carte.empreintes.push_back(f);
  }
  return carte;
}

namespace {
std::string minuscules(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}
bool contient(const std::string& s, std::initializer_list<const char*> mots) {
  for (const char* m : mots) if (s.find(m) != std::string::npos) return true;
  return false;
}
// Diamètre lu dans un nom d'empreinte du type « CP_Radial_D10.0mm ».
double diametreDansNom(const std::string& n) {
  const auto i = n.find("_d");
  if (i == std::string::npos) return 0;
  return std::strtod(n.c_str() + i + 2, nullptr);
}
std::string visPourPercage(double d) {
  if (d <= 2.45) return "M2";
  if (d <= 2.95) return "M2.5";
  if (d <= 3.7) return "M3";
  if (d <= 4.7) return "M4";
  return "M5";
}
}  // namespace

double hauteurEstimee(const std::string& nomEmpreinte) {
  const std::string n = minuscules(nomEmpreinte);
  if (contient(n, {"cp_radial", "c_radial", "cp_elec"})) {
    const double d = diametreDansNom(n);
    return d <= 6.3 ? 11 : d <= 8 ? 12 : d <= 10 ? 16 : 20;
  }
  // std::vector et non std::initializer_list : la liste stockée dans une paire ne survivrait pas à l'initialisation.
  static const std::vector<std::pair<std::vector<const char*>, double>> table = {
      {{"usb_c", "usb-c", "type-c", "typec"}, 3.3}, {{"barreljack", "barrel_jack", "dc_jack"}, 11}, {{"esp32", "esp-", "rf_module"}, 3.2},
      {{"sw_push", "tactile", "button"}, 5}, {{"led_d3", "led_d5"}, 8.6}, {{"pinheader", "pin_header", "pinsocket", "pin_socket"}, 8.5},
      {{"screwterminal", "terminalblock", "terminal_block"}, 10}, {{"relay"}, 15}, {{"to-220", "to220"}, 16}, {{"to-92", "to92"}, 5},
      {{"jst_sh"}, 4.3}, {{"jst_ph"}, 6}, {{"jst_xh"}, 7}, {{"dip-", "dip_"}, 4.5}, {{"crystal", "hc49", "hc-49"}, 4},
      {{"r_axial", "d_axial", "do-41", "do41"}, 3}, {{"c_disc"}, 6}, {{"c_rect"}, 10}, {{"sot-223", "sot223"}, 1.8}, {{"sot-23", "sot23"}, 1.1},
      {{"soic", "so-"}, 1.75}, {{"tssop", "msop", "ssop"}, 1.2}, {{"lqfp", "tqfp", "qfp"}, 1.6}, {{"qfn", "dfn", "lga"}, 1.0}, {{"bga"}, 1.5},
      {{"0201", "0402"}, 0.5}, {{"0603", "1608"}, 0.8}, {{"0805", "2012"}, 1.0}, {{"1206", "3216"}, 1.2}};
  for (const auto& [mots, h] : table)
    for (const char* m : mots)
      if (n.find(m) != std::string::npos) return h;
  return 2.0;
}

Projet projetDepuisKicad(const CarteKicad& k, const std::string& nom, RapportImport* rapport) {
  if (k.contours.empty() || !k.contours.front().ferme) throw std::runtime_error("La carte KiCad n’a pas de contour Edge.Cuts fermé.");
  RapportImport r;
  const double dx = -(k.xmin + k.xmax) / 2, dy = -(k.ymin + k.ymax) / 2;
  auto decale = [&](Point q) { return Point{q.x + dx, q.y + dy}; };
  Json j = Json::object();
  j["nom"] = nom.empty() ? (k.titre.empty() ? std::string("Carte KiCad") : k.titre) : nom;
  double t = 1.6, ecartMin = 1e9;
  for (double e : EPAISSEURS_PCB) if (std::abs(e - k.epaisseur) < ecartMin) { ecartMin = std::abs(e - k.epaisseur); t = e; }
  Json contours = Json::array();
  for (const auto& c : k.contours) {
    if (!c.ferme) continue;
    Json elements = Json::array();
    for (const auto& e : c.elements) {
      const Point a = decale(e.debut), b = decale(e.fin);
      Json je = {{"debut", {a.x, a.y}}, {"fin", {b.x, b.y}}};
      if (e.milieu) { const Point m = decale(*e.milieu); je["milieu"] = {m.x, m.y}; }
      elements.push_back(je);
    }
    contours.push_back({{"elements", elements}});
  }
  j["carte"] = {{"t", t}, {"contours", contours}};
  j["boitier"] = {{"jeu", 1}, {"paroi", 2}, {"fond", 2}, {"entretoise", 5}, {"couvercle", 2}, {"vis", "M3"}, {"hauteurAuto", true}};
  j["trous"] = Json::array();
  j["composants"] = Json::array();
  const double L = k.xmax - k.xmin, W = k.ymax - k.ymin;
  for (const auto& f : k.empreintes) {
    const Point c = decale(f.position);
    if (f.fixation) {
      Json jt = {{"ref", f.ref}, {"x", c.x}, {"y", c.y}, {"ancrage", "libre"}};
      if (f.percage > 0) { jt["vis"] = visPourPercage(f.percage); jt["dimensions"] = {{"trou", f.percage}}; }
      j["trous"].push_back(jt);
      ++r.trous;
      continue;
    }
    if (f.dessous) { ++r.dessous; continue; }
    if (f.ref.empty() || f.ref[0] == '#' || f.xmax - f.xmin < 0.05) continue;
    const std::string n = minuscules(f.nom);
    const double x1 = f.xmin + dx, x2 = f.xmax + dx, y1 = f.ymin + dy, y2 = f.ymax + dy;
    const double cx = (x1 + x2) / 2, cy = (y1 + y2) / 2, w = x2 - x1, d = y2 - y1;
    Json jk = {{"ref", f.ref}, {"valeur", f.valeur}, {"h", hauteurEstimee(f.nom)}, {"origine", "kicad"}};
    const bool usbc = contient(n, {"usb_c", "usb-c", "type-c", "typec"});
    const bool jack = contient(n, {"barreljack", "barrel_jack", "dc_jack"});
    if (usbc || jack) {
      // Connecteur de bord : rattaché au côté le plus proche de l'encombrement de la carte.
      const double dE = L / 2 - x2, dW = x1 + L / 2, dN = W / 2 - y2, dS = y1 + W / 2;
      const double m = std::min({dE, dW, dN, dS});
      const char* bord = m == dE ? "E" : m == dW ? "W" : m == dN ? "N" : "S";
      const bool hz = m == dN || m == dS;
      jk["type"] = usbc ? "usbc" : "jack";
      jk["bord"] = bord;
      jk["le_long"] = hz ? cx : cy;
      jk["w"] = hz ? w : d;
      jk["d"] = hz ? d : w;
      ++r.connecteursBord;
    } else {
      std::string type = "generique";
      if (contient(n, {"cp_radial", "c_radial", "cp_elec"})) type = "condo";
      else if (contient(n, {"led_d3", "led_d5"})) type = "led";
      else if (contient(n, {"sw_push_6mm", "sw_tactile_6", "tactile_6"})) type = "bouton";
      else if (contient(n, {"esp32", "esp-", "rf_module"})) type = "module";
      else if (contient(n, {"jst"})) type = "jst";
      jk["type"] = type;
      jk["x"] = cx; jk["y"] = cy; jk["rot"] = 0;
      jk["w"] = w; jk["d"] = d;
    }
    j["composants"].push_back(jk);
    ++r.composants;
  }
  if (rapport) *rapport = r;
  return normaliser(j);
}

CarteKicad lireFichierKicad(const std::string& chemin) {
  std::ifstream f(chemin, std::ios::binary);
  if (!f) throw std::runtime_error("Impossible d’ouvrir " + chemin + ".");
  std::stringstream s;
  s << f.rdbuf();
  return lireKicad(s.str());
}

}  // namespace prom
