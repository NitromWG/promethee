// Prométhée : modèle unique carte + boîtier. Licence GPL-3.0-only.
#include "promethee/modele.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>

namespace prom {

// ---------- Données de référence ----------
namespace {
const std::vector<std::pair<std::string, Vis>>& tableVis() {
  static const std::vector<std::pair<std::string, Vis>> t = {
      {"M2", {2.0, 2.2, 1.6, 3.8}}, {"M2.5", {2.5, 2.7, 2.0, 4.5}}, {"M3", {3.0, 3.2, 2.5, 5.5}}, {"M4", {4.0, 4.3, 3.3, 7.0}}, {"M5", {5.0, 5.3, 4.2, 8.5}}};
  return t;
}
}  // namespace

bool visConnue(const std::string& nom) {
  for (const auto& [n, v] : tableVis()) if (n == nom) return true;
  return false;
}
const Vis& vis(const std::string& nom) {
  for (const auto& [n, v] : tableVis()) if (n == nom) return v;
  return tableVis()[2].second;  // M3 par défaut
}
const std::vector<std::string>& nomsVis() {
  static const std::vector<std::string> n = {"M2", "M2.5", "M3", "M4", "M5"};
  return n;
}

const Insert& insert(const std::string& nomVis) {
  static const std::vector<std::pair<std::string, Insert>> t = {{"M2", {3.2, 4.0}}, {"M2.5", {3.6, 5.7}}, {"M3", {4.0, 5.7}}, {"M4", {5.6, 8.1}}, {"M5", {6.4, 9.5}}};
  for (const auto& [n, i] : t) if (n == nomVis) return i;
  return t[2].second;
}

const std::vector<TypeComposant>& typesComposants() {
  static const std::vector<TypeComposant> t = [] {
    std::vector<TypeComposant> v;
    auto ajout = [&v](TypeComposant x) { v.push_back(std::move(x)); };
    TypeComposant x;
    x = {}; x.cle = "module"; x.nom = "Module radio"; x.prefixe = "U"; x.w = 18; x.d = 20; x.h = 3.2; x.valeur = "ESP32-C3-WROOM-02"; ajout(x);
    x = {}; x.cle = "ic"; x.nom = "Circuit intégré"; x.prefixe = "U"; x.w = 6.5; x.d = 7; x.h = 1.8; x.valeur = "Régulateur 3,3 V"; ajout(x);
    x = {}; x.cle = "capteur"; x.nom = "Capteur"; x.prefixe = "U"; x.w = 2.5; x.d = 2.5; x.h = 0.93; x.valeur = "BME280"; ajout(x);
    x = {}; x.cle = "usbc"; x.nom = "Connecteur USB-C"; x.prefixe = "J"; x.w = 8.94; x.d = 7.35; x.h = 3.26; x.bord = true;
    x.decoupe = {12.5, 7, 2}; x.zc = 1.63; x.valeur = "USB-C 16 broches"; ajout(x);
    x = {}; x.cle = "jack"; x.nom = "Jack d\u2019alimentation"; x.prefixe = "J"; x.w = 9; x.d = 14; x.h = 11; x.bord = true;
    x.decoupe = {8, 8, 4}; x.zc = 6.3; x.valeur = "5,5 × 2,1 mm"; ajout(x);
    x = {}; x.cle = "jst"; x.nom = "Connecteur JST-PH"; x.prefixe = "J"; x.w = 6; x.d = 4.5; x.h = 6; x.valeur = "2 broches"; ajout(x);
    x = {}; x.cle = "led"; x.nom = "LED 3 mm"; x.prefixe = "D"; x.w = 3.2; x.d = 3.2; x.h = 5.3; x.rond = true; x.couvercle = 3.4; x.valeur = "Rouge"; ajout(x);
    x = {}; x.cle = "bouton"; x.nom = "Bouton poussoir"; x.prefixe = "SW"; x.w = 6; x.d = 6; x.h = 13; x.couvercle = 4.2; x.bouton = true;
    x.valeur = "6 × 6 mm, tige de 13 mm"; ajout(x);
    x = {}; x.cle = "condo"; x.nom = "Condensateur"; x.prefixe = "C"; x.w = 6.3; x.d = 6.3; x.h = 7.7; x.rond = true; x.valeur = "100 µF 16 V"; ajout(x);
    x = {}; x.cle = "generique"; x.nom = "Composant"; x.prefixe = "U"; x.w = 5; x.d = 5; x.h = 2; x.valeur = ""; ajout(x);
    return v;
  }();
  return t;
}
const TypeComposant* typeComposant(const std::string& cle) {
  for (const auto& t : typesComposants()) if (t.cle == cle) return &t;
  return nullptr;
}

char lettre(Bord b) {
  switch (b) { case Bord::N: return 'N'; case Bord::S: return 'S'; case Bord::E: return 'E'; default: return 'W'; }
}
std::optional<Bord> bordDe(const std::string& s) {
  if (s == "N") return Bord::N;
  if (s == "S") return Bord::S;
  if (s == "E") return Bord::E;
  if (s == "W") return Bord::W;
  return std::nullopt;
}

// ---------- Outils ----------
double arrondiJs(double x) {
  const double f = std::floor(x);
  return (x - f) >= 0.5 ? f + 1 : f;
}

std::string fmt(double v, int decimales) {
  if (!std::isfinite(v)) return "\u2013";
  const double k = std::pow(10.0, decimales);
  char tampon[64];
  std::snprintf(tampon, sizeof tampon, "%.*f", decimales, arrondiJs(v * k) / k);
  std::string s = tampon;
  if (s.find('.') != std::string::npos) {
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
  }
  if (s == "-0") s = "0";
  const auto point = s.find('.');
  if (point != std::string::npos) s.replace(point, 1, ",");
  const auto moins = s.find('-');
  if (moins != std::string::npos) s.replace(moins, 1, "\u2212");
  return s;
}

double sdRR(double px, double py, double cx, double cy, double hx, double hy, double r) {
  const double qx = std::abs(px - cx) - (hx - r), qy = std::abs(py - cy) - (hy - r);
  return std::hypot(std::max(qx, 0.0), std::max(qy, 0.0)) + std::min(std::max(qx, qy), 0.0) - r;
}

std::string nouvelId(const std::string& prefixe, const std::vector<std::string>& existants) {
  static std::mt19937 gen{std::random_device{}()};
  static const char* chiffres = "0123456789abcdefghijklmnopqrstuvwxyz";
  std::uniform_int_distribution<int> tirage(0, 35);
  std::string id;
  do {
    id = prefixe;
    for (int i = 0; i < 6; ++i) id += chiffres[tirage(gen)];
  } while (std::find(existants.begin(), existants.end(), id) != existants.end());
  return id;
}

std::string prochaineRef(const Projet& p, const std::string& prefixe, const void* exclure) {
  std::set<std::string> pris;
  for (const auto& k : p.composants) if (&k != exclure) pris.insert(k.ref);
  for (const auto& t : p.trous) if (&t != exclure) pris.insert(t.ref);
  int i = 1;
  while (pris.count(prefixe + std::to_string(i))) ++i;
  return prefixe + std::to_string(i);
}

namespace {
const Json& champ(const Json& o, const char* cle) {
  static const Json nul;
  if (!o.is_object()) return nul;
  const auto it = o.find(cle);
  return it == o.end() ? nul : *it;
}
double nombre(const Json& v, double def) {
  if (v.is_number()) { const double n = v.get<double>(); return std::isfinite(n) ? n : def; }
  if (v.is_string()) {
    const std::string s = v.get<std::string>();
    char* fin = nullptr;
    const double n = std::strtod(s.c_str(), &fin);
    if (fin != s.c_str() && std::isfinite(n)) return n;
  }
  return def;
}
double borne(double v, double a, double b) { return std::min(b, std::max(a, v)); }
bool espace(unsigned char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; }
std::string rogner(const std::string& s) {
  size_t a = 0, b = s.size();
  for (;;) {
    if (a < b && espace(static_cast<unsigned char>(s[a]))) { ++a; continue; }
    if (a + 1 < b && static_cast<unsigned char>(s[a]) == 0xC2 && static_cast<unsigned char>(s[a + 1]) == 0xA0) { a += 2; continue; }
    break;
  }
  for (;;) {
    if (b > a && espace(static_cast<unsigned char>(s[b - 1]))) { --b; continue; }
    if (b >= a + 2 && static_cast<unsigned char>(s[b - 2]) == 0xC2 && static_cast<unsigned char>(s[b - 1]) == 0xA0) { b -= 2; continue; }
    break;
  }
  return s.substr(a, b - a);
}
// Tronque à n unités UTF-16, comme String.prototype.slice.
std::string tronquer(const std::string& s, size_t n) {
  size_t i = 0, unites = 0;
  while (i < s.size()) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    const size_t len = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : 4;
    const size_t u = len == 4 ? 2 : 1;
    if (unites + u > n) break;
    unites += u; i += len;
  }
  return s.substr(0, i);
}
std::string chaine(const Json& v, size_t n) {
  if (!v.is_string()) return "";
  const std::string t = rogner(v.get<std::string>());
  return t.empty() ? "" : tronquer(t, n);
}
bool parmi(double v, std::initializer_list<double> l) {
  for (double x : l) if (v == x) return true;
  return false;
}
}  // namespace

Projet normaliser(const Json& s) {
  Projet p;
  const Json& c = champ(s, "carte");
  const Json& b = champ(s, "boitier");
  const std::string nom = chaine(champ(s, "nom"), 80);
  p.nom = nom.empty() ? "Projet sans nom" : nom;
  const double t0 = nombre(champ(c, "t"), 1.6);
  p.carte.x0 = nombre(champ(c, "x0"), 0);
  p.carte.y0 = nombre(champ(c, "y0"), 0);
  p.carte.L = borne(nombre(champ(c, "L"), 60), 10, 300);
  p.carte.W = borne(nombre(champ(c, "W"), 40), 10, 300);
  p.carte.t = std::find(EPAISSEURS_PCB.begin(), EPAISSEURS_PCB.end(), t0) != EPAISSEURS_PCB.end() ? t0 : 1.6;
  p.carte.r = borne(nombre(champ(c, "r"), 2), 0, std::min(p.carte.L, p.carte.W) / 2 - 0.5);
  const Json& jcontours = champ(c, "contours");
  if (jcontours.is_array()) {
    auto point = [](const Json& v, Point& q) {
      if (!v.is_array() || v.size() != 2 || !v[0].is_number() || !v[1].is_number()) return false;
      q = {v[0].get<double>(), v[1].get<double>()};
      return std::isfinite(q.x) && std::isfinite(q.y);
    };
    for (const auto& jc : jcontours) {
      Contour k;
      bool valide = champ(jc, "elements").is_array();
      if (valide)
        for (const auto& je : champ(jc, "elements")) {
          ElementContour e;
          if (!point(champ(je, "debut"), e.debut) || !point(champ(je, "fin"), e.fin)) { valide = false; break; }
          Point mil{};
          if (point(champ(je, "milieu"), mil)) e.milieu = mil;
          k.elements.push_back(e);
        }
      if (!valide || k.elements.size() < 2) continue;
      k.ferme = std::hypot(k.elements.front().debut.x - k.elements.back().fin.x, k.elements.front().debut.y - k.elements.back().fin.y) < 1e-3;
      if (k.ferme && std::abs(k.aire()) > 1) p.carte.contours.push_back(k);
    }
    if (!p.carte.contours.empty()) {
      orienterContours(p.carte.contours);
      recalerEncombrement(p.carte);
    }
  }

  auto& bo = p.boitier;
  bo.jeu = borne(nombre(champ(b, "jeu"), 1), 0.2, 10);
  bo.paroi = borne(nombre(champ(b, "paroi"), 2), 0.8, 8);
  bo.fond = borne(nombre(champ(b, "fond"), 2), 0.8, 8);
  bo.entretoise = borne(nombre(champ(b, "entretoise"), 5), 1, 30);
  bo.couvercle = borne(nombre(champ(b, "couvercle"), 2), 0.8, 8);
  const Json& jv = champ(b, "vis");
  bo.vis = jv.is_string() && visConnue(jv.get<std::string>()) ? jv.get<std::string>() : "M3";
  const Json& ja = champ(b, "hauteurAuto");
  bo.hauteurAuto = ja.is_boolean() ? ja.get<bool>() : true;
  bo.hauteur = borne(nombre(champ(b, "hauteur"), 20), bo.entretoise + p.carte.t + 1, 200);

  std::vector<std::string> ids;
  auto prendreId = [&ids](const Json& v, const char* prefixe) {
    std::string id;
    if (v.is_string() && !v.get<std::string>().empty() &&
        std::find(ids.begin(), ids.end(), v.get<std::string>()) == ids.end())
      id = v.get<std::string>();
    else
      id = nouvelId(prefixe, ids);
    ids.push_back(id);
    return id;
  };

  const Json& jt = champ(s, "trous");
  if (jt.is_array()) {
    size_t n = 0;
    for (const auto& t : jt) {
      if (n++ >= 40) break;
      if (!(t.is_object() || t.is_array())) continue;
      Trou o;
      o.id = prendreId(champ(t, "id"), "t");
      o.ref = chaine(champ(t, "ref"), 12);
      const Json& coin = champ(t, "coin");
      const std::string cs = coin.is_string() ? coin.get<std::string>() : "";
      if (cs == "NE" || cs == "NW" || cs == "SE" || cs == "SW") {
        o.coin = cs;
        o.ox = borne(nombre(champ(t, "ox"), 4), -300, 300);
        o.oy = borne(nombre(champ(t, "oy"), 4), -300, 300);
      } else {
        o.x = nombre(champ(t, "x"), p.carte.x0);
        o.y = nombre(champ(t, "y"), p.carte.y0);
      }
      const Json& ja = champ(t, "ancrage");
      const std::string as = ja.is_string() ? ja.get<std::string>() : "";
      if (as == "libre" || as == "NE" || as == "NW" || as == "SE" || as == "SW") o.ancrage = as;
      const Json& jv = champ(t, "vis");
      if (jv.is_string() && visConnue(jv.get<std::string>())) o.vis = jv.get<std::string>();
      const Json& jf = champ(t, "fixation");
      if (jf.is_string() && jf.get<std::string>() == "insert") o.fixation = "insert";
      const Json& dims = champ(t, "dimensions");
      auto dimension = [&dims](const char* cle, double a, double b) -> std::optional<double> {
        const Json& v = champ(dims, cle);
        if (!v.is_number()) return std::nullopt;
        return borne(v.get<double>(), a, b);
      };
      o.diamTrou = dimension("trou", 0.5, 20);
      o.diamPastille = dimension("pastille", 1, 30);
      o.diamPilier = dimension("pilier", 2, 40);
      o.diamLogement = dimension("logement", 0.5, 30);
      o.longueurInsert = dimension("insert", 1, 30);
      p.trous.push_back(o);
    }
  }
  const Json& jc = champ(s, "composants");
  if (jc.is_array()) {
    size_t n = 0;
    for (const auto& k : jc) {
      if (n++ >= 200) break;
      if (!(k.is_object() || k.is_array())) continue;
      const Json& jtype = champ(k, "type");
      const TypeComposant* T = jtype.is_string() ? typeComposant(jtype.get<std::string>()) : nullptr;
      if (!T) continue;
      Composant o;
      o.id = prendreId(champ(k, "id"), "c");
      o.ref = chaine(champ(k, "ref"), 12);
      o.type = T->cle;
      const Json& jval = champ(k, "valeur");
      o.valeur = jval.is_string() ? tronquer(jval.get<std::string>(), 60) : T->valeur;
      o.w = borne(nombre(champ(k, "w"), T->w), 0.5, 150);
      o.d = borne(nombre(champ(k, "d"), T->d), 0.5, 150);
      o.h = borne(nombre(champ(k, "h"), T->h), 0.2, 100);
      if (T->bord) {
        const Json& jb = champ(k, "bord");
        const auto bd = jb.is_string() ? bordDe(jb.get<std::string>()) : std::nullopt;
        o.bord = bd ? *bd : Bord::W;
        o.le_long = nombre(champ(k, "le_long"), horizontal(*o.bord) ? p.carte.x0 : p.carte.y0);
        o.ecart = borne(nombre(champ(k, "ecart"), 0.4), 0, 10);
        const Json& jd = champ(k, "decoupe");
        const Json dc = jd.is_object() ? jd : Json::object();
        o.decoupe.w = borne(nombre(champ(dc, "w"), T->decoupe.w), 1, 100);
        o.decoupe.h = borne(nombre(champ(dc, "h"), T->decoupe.h), 1, 100);
        o.decoupe.r = borne(nombre(champ(dc, "r"), T->decoupe.r), 0, std::min(o.decoupe.w, o.decoupe.h) / 2);
        o.zc = borne(nombre(champ(k, "zc"), T->zc), 0, 100);
      } else {
        o.x = nombre(champ(k, "x"), p.carte.x0);
        o.y = nombre(champ(k, "y"), p.carte.y0);
        const double r = nombre(champ(k, "rot"), 0);
        o.rot = parmi(r, {0, 90, 180, 270}) ? static_cast<int>(r) : 0;
      }
      if (T->couvercle > 0) o.trou_couvercle = borne(nombre(champ(k, "trou_couvercle"), T->couvercle), 0.5, 30);
      const Json& ja = champ(k, "ancrage");
      if (ja.is_string() && ancrageValide(o, ja.get<std::string>()) && ja.get<std::string>() != "libre") {
        // Sans décalages enregistrés, on les déduit de la position absolue.
        const bool decalages = champ(k, "ax").is_number();
        const std::string a = ja.get<std::string>();
        if (decalages) {
          o.ancrage = a;
          o.ax = borne(nombre(champ(k, "ax"), 0), -300, 300);
          o.ay = borne(nombre(champ(k, "ay"), 0), -300, 300);
        } else {
          changerAncrage(p, o, a);
        }
      }
      const Json& jverrou = champ(k, "verrou");
      o.verrou = jverrou.is_boolean() && jverrou.get<bool>();
      const Json& jorigine = champ(k, "origine");
      if (jorigine.is_string() && jorigine.get<std::string>() == "kicad") o.origine = "kicad";
      p.composants.push_back(o);
    }
  }
  for (auto& t : p.trous) if (t.ref.empty()) t.ref = prochaineRef(p, "T", &t);
  for (auto& k : p.composants) if (k.ref.empty()) k.ref = prochaineRef(p, typeComposant(k.type)->prefixe, &k);
  return p;
}

Json versJson(const Projet& p) {
  Json j;
  j["format"] = "promethee-projet";
  j["version"] = 1;
  j["nom"] = p.nom;
  j["carte"] = {{"x0", p.carte.x0}, {"y0", p.carte.y0}, {"L", p.carte.L}, {"W", p.carte.W}, {"r", p.carte.r}, {"t", p.carte.t}};
  if (p.carte.libre()) {
    Json contours = Json::array();
    for (const auto& k : p.carte.contours) {
      Json elements = Json::array();
      for (const auto& e : k.elements) {
        Json je = {{"debut", {e.debut.x, e.debut.y}}, {"fin", {e.fin.x, e.fin.y}}};
        if (e.milieu) je["milieu"] = {e.milieu->x, e.milieu->y};
        elements.push_back(je);
      }
      contours.push_back({{"elements", elements}});
    }
    j["carte"]["contours"] = contours;
  }
  const auto& b = p.boitier;
  j["boitier"] = {{"jeu", b.jeu}, {"paroi", b.paroi}, {"fond", b.fond}, {"entretoise", b.entretoise},
                  {"hauteur", b.hauteur}, {"couvercle", b.couvercle}, {"vis", b.vis}, {"hauteurAuto", b.hauteurAuto}};
  j["trous"] = Json::array();
  for (const auto& t : p.trous) {
    Json o = {{"id", t.id}, {"ref", t.ref}};
    if (t.coin) { o["coin"] = *t.coin; o["ox"] = t.ox; o["oy"] = t.oy; }
    else { o["x"] = t.x; o["y"] = t.y; }
    if (t.ancrage != "auto") o["ancrage"] = t.ancrage;
    if (t.vis) o["vis"] = *t.vis;
    if (t.fixation != "autotaraudeuse") o["fixation"] = t.fixation;
    Json dims = Json::object();
    if (t.diamTrou) dims["trou"] = *t.diamTrou;
    if (t.diamPastille) dims["pastille"] = *t.diamPastille;
    if (t.diamPilier) dims["pilier"] = *t.diamPilier;
    if (t.diamLogement) dims["logement"] = *t.diamLogement;
    if (t.longueurInsert) dims["insert"] = *t.longueurInsert;
    if (!dims.empty()) o["dimensions"] = dims;
    j["trous"].push_back(o);
  }
  j["composants"] = Json::array();
  for (const auto& k : p.composants) {
    Json o = {{"id", k.id}, {"ref", k.ref}, {"type", k.type}, {"valeur", k.valeur}, {"w", k.w}, {"d", k.d}, {"h", k.h}};
    if (k.bord) {
      o["bord"] = std::string(1, lettre(*k.bord));
      o["le_long"] = leLongComposant(p, k);
      o["ecart"] = k.ecart;
      o["decoupe"] = {{"w", k.decoupe.w}, {"h", k.decoupe.h}, {"r", k.decoupe.r}};
      o["zc"] = k.zc;
    } else {
      const Point q = posComposant(p, k);
      o["x"] = q.x; o["y"] = q.y; o["rot"] = k.rot;
    }
    if (k.trou_couvercle) o["trou_couvercle"] = *k.trou_couvercle;
    if (k.ancrage != "libre") {
      o["ancrage"] = k.ancrage;
      o["ax"] = k.ax;
      if (!k.bord) o["ay"] = k.ay;
    }
    if (k.verrou) o["verrou"] = true;
    if (!k.origine.empty()) o["origine"] = k.origine;
    j["composants"].push_back(o);
  }
  return j;
}

Projet lireProjet(const std::string& chemin) {
  std::ifstream f(chemin, std::ios::binary);
  if (!f) throw std::runtime_error("Impossible d\u2019ouvrir " + chemin + ".");
  std::stringstream tampon;
  tampon << f.rdbuf();
  Json j;
  try {
    j = Json::parse(tampon.str());
  } catch (const std::exception&) {
    throw std::runtime_error(chemin + " n\u2019est pas un fichier JSON valide.");
  }
  if (!j.is_object() || (champ(j, "format") != Json("promethee-projet") && !j.contains("carte")))
    throw std::runtime_error(chemin + " n\u2019est pas un projet Prométhée.");
  return normaliser(j);
}

void ecrireProjet(const Projet& p, const std::string& chemin) {
  std::ofstream f(chemin, std::ios::binary);
  if (!f) throw std::runtime_error("Impossible d\u2019écrire " + chemin + ".");
  f << versJson(p).dump(2) << '\n';
}

Projet exemple() {
  return normaliser(Json::parse(R"({
    "nom": "Station météo ESP32",
    "carte": {"x0": 0, "y0": 0, "L": 60, "W": 40, "r": 2, "t": 1.6},
    "boitier": {"jeu": 1, "paroi": 2, "fond": 2, "entretoise": 5, "hauteur": 18, "couvercle": 2, "vis": "M3", "hauteurAuto": true},
    "trous": [
      {"id": "t1", "ref": "T1", "coin": "SW", "ox": 4.5, "oy": 4.5},
      {"id": "t2", "ref": "T2", "coin": "SE", "ox": 4.5, "oy": 4.5},
      {"id": "t3", "ref": "T3", "coin": "NE", "ox": 4.5, "oy": 4.5},
      {"id": "t4", "ref": "T4", "coin": "NW", "ox": 4.5, "oy": 4.5}
    ],
    "composants": [
      {"id": "c1", "ref": "J1", "type": "usbc", "bord": "W", "le_long": 0, "ecart": 0.4},
      {"id": "c2", "ref": "U1", "type": "module", "x": 16, "y": 0, "rot": 90},
      {"id": "c3", "ref": "U2", "type": "ic", "valeur": "AMS1117-3.3", "x": -16.5, "y": 0, "rot": 0},
      {"id": "c4", "ref": "U3", "type": "capteur", "x": -3, "y": -5, "rot": 0},
      {"id": "c5", "ref": "C1", "type": "condo", "x": -3, "y": 6, "rot": 0},
      {"id": "c6", "ref": "D1", "type": "led", "valeur": "Verte", "x": -14, "y": 13, "rot": 0},
      {"id": "c7", "ref": "D2", "type": "led", "valeur": "Rouge", "x": -8.5, "y": 13, "rot": 0},
      {"id": "c8", "ref": "SW1", "type": "bouton", "x": -14, "y": -12, "rot": 0},
      {"id": "c9", "ref": "J2", "type": "jst", "valeur": "Batterie, 2 broches", "x": 2, "y": -15, "rot": 0}
    ]})"));
}

Projet projetVide() {
  return normaliser(Json::parse(R"({
    "nom": "Nouveau projet",
    "carte": {"x0": 0, "y0": 0, "L": 50, "W": 30, "r": 1.5, "t": 1.6},
    "boitier": {"jeu": 1, "paroi": 2, "fond": 2, "entretoise": 5, "hauteur": 12, "couvercle": 2, "vis": "M3", "hauteurAuto": true},
    "trous": [
      {"ref": "T1", "coin": "SW", "ox": 4, "oy": 4}, {"ref": "T2", "coin": "SE", "ox": 4, "oy": 4},
      {"ref": "T3", "coin": "NE", "ox": 4, "oy": 4}, {"ref": "T4", "coin": "NW", "ox": 4, "oy": 4}
    ],
    "composants": []})"));
}

Point posTrou(const Projet& p, const Trou& t) {
  const auto& c = p.carte;
  if (t.coin) {
    const double sx = t.coin->find('E') != std::string::npos ? 1 : -1;
    const double sy = t.coin->find('N') != std::string::npos ? 1 : -1;
    return {c.x0 + sx * (c.L / 2 - t.ox), c.y0 + sy * (c.W / 2 - t.oy)};
  }
  return {t.x, t.y};
}

void ancrerTrou(const Projet& p, Trou& t, double x, double y) {
  const auto& c = p.carte;
  if (t.ancrage == "libre") {
    t.coin.reset(); t.ox = 4; t.oy = 4; t.x = x; t.y = y;
    return;
  }
  if (t.ancrage.size() == 2) {
    const double sx = t.ancrage[1] == 'E' ? 1 : -1, sy = t.ancrage[0] == 'N' ? 1 : -1;
    t.coin = t.ancrage;
    t.ox = c.L / 2 - sx * (x - c.x0); t.oy = c.W / 2 - sy * (y - c.y0); t.x = 0; t.y = 0;
    return;
  }
  const double sx = x >= c.x0 ? 1 : -1, sy = y >= c.y0 ? 1 : -1;
  const double ox = c.L / 2 - sx * (x - c.x0), oy = c.W / 2 - sy * (y - c.y0);
  if (ox <= 15 && oy <= 15) {
    t.coin = std::string(1, sy > 0 ? 'N' : 'S') + (sx > 0 ? 'E' : 'W');
    t.ox = ox; t.oy = oy; t.x = 0; t.y = 0;
  } else {
    t.coin.reset(); t.ox = 4; t.oy = 4; t.x = x; t.y = y;
  }
}

void placerSurBord(const Projet& p, Composant& k, double x, double y) {
  const auto& c = p.carte;
  const double dE = std::abs(c.x0 + c.L / 2 - x), dW = std::abs(x - (c.x0 - c.L / 2));
  const double dN = std::abs(c.y0 + c.W / 2 - y), dS = std::abs(y - (c.y0 - c.W / 2));
  Bord b = Bord::E; double m = dE;
  if (dW < m) { b = Bord::W; m = dW; }
  if (dN < m) { b = Bord::N; m = dN; }
  if (dS < m) { b = Bord::S; m = dS; }
  k.bord = b;
  const bool hz = horizontal(b);
  const double centre = hz ? c.x0 : c.y0, demi = (hz ? c.L : c.W) / 2;
  const double lim = std::max(0.0, demi - k.w / 2 - c.r);
  k.le_long = borne(hz ? x : y, centre - lim, centre + lim);
}

void changerAncrageTrou(const Projet& p, Trou& t, const std::string& ancrage) {
  const Point q = posTrou(p, t);
  t.ancrage = ancrage;
  ancrerTrou(p, t, q.x, q.y);
}

bool ancrageValide(const Composant& k, const std::string& a) {
  if (k.bord) return a == "libre" || a == "debut" || a == "fin" || a == "milieu";
  return a == "libre" || a == "centre" || a == "NE" || a == "NW" || a == "SE" || a == "SW";
}

Point posComposant(const Projet& p, const Composant& k) {
  const auto& c = p.carte;
  if (k.ancrage == "centre") return {c.x0 + k.ax, c.y0 + k.ay};
  if (k.ancrage.size() == 2) {
    const double sx = k.ancrage[1] == 'E' ? 1 : -1, sy = k.ancrage[0] == 'N' ? 1 : -1;
    return {c.x0 + sx * (c.L / 2 - k.ax), c.y0 + sy * (c.W / 2 - k.ay)};
  }
  return {k.x, k.y};
}

namespace {
void bornesBord(const Projet& p, const Composant& k, double& centre, double& demi) {
  const bool hz = k.bord && horizontal(*k.bord);
  centre = hz ? p.carte.x0 : p.carte.y0;
  demi = (hz ? p.carte.L : p.carte.W) / 2;
}
void fixerLeLong(const Projet& p, Composant& k, double u) {
  double centre, demi;
  bornesBord(p, k, centre, demi);
  if (k.ancrage == "debut") k.ax = u - (centre - demi);
  else if (k.ancrage == "fin") k.ax = (centre + demi) - u;
  else if (k.ancrage == "milieu") k.ax = u - centre;
  k.le_long = u;
}
}  // namespace

double leLongComposant(const Projet& p, const Composant& k) {
  if (!k.bord) return 0;
  double centre, demi;
  bornesBord(p, k, centre, demi);
  if (k.ancrage == "debut") return centre - demi + k.ax;
  if (k.ancrage == "fin") return centre + demi - k.ax;
  if (k.ancrage == "milieu") return centre + k.ax;
  return k.le_long;
}

void placerLeLong(const Projet& p, Composant& k, double u) { fixerLeLong(p, k, u); }

void placerComposant(const Projet& p, Composant& k, double x, double y) {
  if (k.bord) {
    placerSurBord(p, k, x, y);
    fixerLeLong(p, k, k.le_long);
    return;
  }
  const auto& c = p.carte;
  if (k.ancrage == "centre") {
    k.ax = x - c.x0; k.ay = y - c.y0;
  } else if (k.ancrage.size() == 2) {
    const double sx = k.ancrage[1] == 'E' ? 1 : -1, sy = k.ancrage[0] == 'N' ? 1 : -1;
    k.ax = c.L / 2 - sx * (x - c.x0); k.ay = c.W / 2 - sy * (y - c.y0);
  }
  k.x = x; k.y = y;
}

void changerAncrage(const Projet& p, Composant& k, const std::string& ancrage) {
  if (!ancrageValide(k, ancrage)) return;
  if (k.bord) {
    const double u = leLongComposant(p, k);
    k.ancrage = ancrage;
    fixerLeLong(p, k, u);
  } else {
    const Point q = posComposant(p, k);
    k.ancrage = ancrage;
    placerComposant(p, k, q.x, q.y);
  }
}

void contraindreBords(Projet& p) {
  for (auto& k : p.composants) {
    if (!k.bord) continue;
    double centre, demi;
    bornesBord(p, k, centre, demi);
    const double lim = std::max(0.0, demi - k.w / 2 - p.carte.r);
    fixerLeLong(p, k, borne(leLongComposant(p, k), centre - lim, centre + lim));
  }
}

int longueurVis(double t, const Vis& v) {
  for (int L : LONGUEURS_VIS) if (L - t >= 1.5 * v.d - 0.2) return L;
  return LONGUEURS_VIS.back();
}

}  // namespace prom
