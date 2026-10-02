// Prométhée : grandeurs dérivées. Licence GPL-3.0-only.
#include "promethee/derive.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace prom {

namespace {
double borne(double v, double a, double b) { return std::min(b, std::max(a, v)); }

GeoComp geoComp(const Projet& p, const Derive& d, const Composant& k) {
  const auto& c = p.carte;
  GeoComp g;
  g.id = k.id; g.ref = k.ref; g.type = k.type; g.h = k.h; g.top = d.zpt + k.h; g.bord = k.bord; g.w = k.w; g.d = k.d;
  if (k.bord) {
    const double jeu = p.boitier.jeu;
    if (*k.bord == Bord::E || *k.bord == Bord::W) {
      const double s = *k.bord == Bord::E ? 1 : -1;
      g.face = c.x0 + s * (c.L / 2 + jeu - k.ecart);
      g.x = g.face - s * k.d / 2; g.y = k.le_long; g.hx = k.d / 2; g.hy = k.w / 2;
    } else {
      const double s = *k.bord == Bord::N ? 1 : -1;
      g.face = c.y0 + s * (c.W / 2 + jeu - k.ecart);
      g.y = g.face - s * k.d / 2; g.x = k.le_long; g.hx = k.w / 2; g.hy = k.d / 2;
    }
    g.decoupe = DecoupeMur{*k.bord, k.le_long, d.zpt + k.zc, k.decoupe.w, k.decoupe.h, k.decoupe.r};
    switch (*k.bord) { case Bord::E: g.rot = 0; break; case Bord::N: g.rot = 90; break; case Bord::W: g.rot = 180; break; default: g.rot = 270; }
  } else {
    const bool echange = k.rot == 90 || k.rot == 270;
    g.x = k.x; g.y = k.y;
    g.hx = (echange ? k.d : k.w) / 2; g.hy = (echange ? k.w : k.d) / 2; g.rot = k.rot;
  }
  g.x1 = g.x - g.hx; g.x2 = g.x + g.hx; g.y1 = g.y - g.hy; g.y2 = g.y + g.hy;
  if (k.trou_couvercle) g.trouCouvercle = Percage{g.x, g.y, *k.trou_couvercle};
  return g;
}
}  // namespace

bool dansLevre(const Derive& d, const GeoComp& g) {
  double m = -std::numeric_limits<double>::infinity();
  const std::array<Point, 4> coins{{{g.x1, g.y1}, {g.x2, g.y1}, {g.x2, g.y2}, {g.x1, g.y2}}};
  for (const auto& q : coins) m = std::max(m, sdRR(q.x, q.y, d.cx, d.cy, d.li.hx, d.li.hy, d.li.r));
  return m > -0.2;
}

double hauteurIdeale(const Projet& p, const Derive& d) {
  const auto& b = p.boitier;
  const double base = b.entretoise + p.carte.t;
  double H = base + 1;
  for (const auto& g : d.comps) {
    if (typeComposant(g.type)->bouton) continue;
    H = std::max(H, base + g.h + (dansLevre(d, g) ? Levre::h + 0.3 : 0.5));
    if (g.decoupe) H = std::max(H, g.decoupe->zc + g.decoupe->h / 2 - d.zf + Levre::h + 0.2);
  }
  for (const auto& g : d.comps) if (typeComposant(g.type)->bouton) H = std::max(H, base + g.h - b.couvercle);
  return std::min(200.0, std::ceil(H * 2 - 1e-9) / 2);
}

Derive deriver(const Projet& p) {
  const auto& c = p.carte;
  const auto& b = p.boitier;
  const Vis& v = vis(b.vis);
  Derive d;
  d.cx = c.x0; d.cy = c.y0;
  d.Li = c.L + 2 * b.jeu; d.Wi = c.W + 2 * b.jeu;
  d.ri = std::max(c.r + b.jeu, 0.5);
  d.Lo = d.Li + 2 * b.paroi; d.Wo = d.Wi + 2 * b.paroi; d.ro = d.ri + b.paroi;
  d.zf = b.fond; d.zpb = d.zf + b.entretoise; d.zpt = d.zpb + c.t;
  d.vis = v; d.Rb = v.avant / 2 + PAROI_PILIER; d.rp = v.avant / 2; d.rTete = v.tete / 2 + 0.6;
  d.lo = {d.Li / 2 - Levre::jeu, d.Wi / 2 - Levre::jeu, std::max(d.ri - Levre::jeu, 0.3)};
  d.li = {d.lo.hx - Levre::ep, d.lo.hy - Levre::ep, std::max(d.lo.r - Levre::ep, 0.3)};
  d.sb = d.Li / 2 - d.ri; d.sa = d.Wi / 2 - d.ri;
  for (const auto& t : p.trous) { const Point q = posTrou(p, t); d.trous.push_back({t.id, t.ref, q.x, q.y}); }
  for (const auto& k : p.composants) d.comps.push_back(geoComp(p, d, k));
  d.longVis = longueurVis(c.t, v);
  d.H = b.hauteurAuto ? borne(hauteurIdeale(p, d), b.entretoise + c.t + 1, 200) : b.hauteur;
  d.zt = d.zf + d.H; d.ztop = d.zt + b.couvercle; d.zLevre = d.zt - Levre::h;
  return d;
}

std::map<Bord, std::vector<DecoupeValide>> decoupesValides(const Derive& d) {
  std::map<Bord, std::vector<DecoupeValide>> res{{Bord::N, {}}, {Bord::S, {}}, {Bord::E, {}}, {Bord::W, {}}};
  for (const auto& g : d.comps) {
    if (!g.decoupe) continue;
    const auto& o = *g.decoupe;
    const bool hz = horizontal(o.mur);
    const double centre = hz ? d.cx : d.cy, demi = hz ? d.sb : d.sa;
    if (std::abs(o.u - centre) + o.w / 2 > demi - 0.3) continue;
    if (o.zc - o.h / 2 < d.zf + 0.3 || o.zc + o.h / 2 > d.zt - 0.3) continue;
    auto& lst = res[o.mur];
    const bool conflit = std::any_of(lst.begin(), lst.end(), [&](const DecoupeValide& q) {
      return std::abs(q.s - o.u) < (q.w + o.w) / 2 + 0.3 && std::abs(q.t - o.zc) < (q.h + o.h) / 2 + 0.3;
    });
    if (conflit) continue;
    lst.push_back({o.u, o.zc, o.w, o.h, o.r, g.id});
  }
  return res;
}

std::vector<PilierValide> piliersValides(const Derive& d) {
  std::vector<PilierValide> res;
  for (const auto& t : d.trous) {
    if (sdRR(t.x, t.y, d.cx, d.cy, d.Li / 2, d.Wi / 2, d.ri) > -(d.Rb + 0.25)) continue;
    const bool conflit = std::any_of(res.begin(), res.end(), [&](const PilierValide& q) { return std::hypot(q.x - t.x, q.y - t.y) < 2 * d.Rb + 0.25; });
    if (conflit) continue;
    res.push_back({t.x, t.y, t.id});
  }
  return res;
}

std::vector<PercageValide> percagesValides(const Derive& d) {
  std::vector<PercageValide> res;
  for (const auto& g : d.comps) {
    if (!g.trouCouvercle) continue;
    const auto& o = *g.trouCouvercle;
    if (sdRR(o.x, o.y, d.cx, d.cy, d.li.hx, d.li.hy, d.li.r) > -(o.d / 2 + 0.3)) continue;
    const bool conflit = std::any_of(res.begin(), res.end(), [&](const PercageValide& q) { return std::hypot(q.x - o.x, q.y - o.y) < (q.d + o.d) / 2 + 0.4; });
    if (conflit) continue;
    res.push_back({o.x, o.y, o.d, g.id});
  }
  return res;
}

}  // namespace prom
