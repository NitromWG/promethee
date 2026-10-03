// Prométhée : formats d'échange et de fabrication. Licence GPL-3.0-only.
#include "promethee/echanges.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <IGESControl_Controller.hxx>
#include <IGESControl_Writer.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Message_Printer.hxx>
#include <Poly_Triangulation.hxx>
#include <StlAPI_Writer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>

namespace prom {

namespace {

void silencer() {
  static const bool fait = [] {
    for (const auto& imprimante : Message::DefaultMessenger()->Printers()) imprimante->SetTraceLevel(Message_Fail);
    return true;
  }();
  (void)fait;
}

void ecrireFichier(const std::string& chemin, const std::string& contenu) {
  std::ofstream f(chemin, std::ios::binary);
  if (!f) throw std::runtime_error("Écriture impossible : " + chemin);
  f << contenu;
}

std::string nombre(double v, int dec = 4) {
  char t[48];
  std::snprintf(t, sizeof t, "%.*f", dec, std::abs(v) < 1e-12 ? 0.0 : v);
  std::string s = t;
  if (s.find('.') != std::string::npos) {
    while (s.back() == '0') s.pop_back();
    if (s.back() == '.') s.pop_back();
  }
  return s == "-0" ? "0" : s;
}

struct Maillage {
  std::vector<std::array<double, 3>> sommets;
  std::vector<std::array<uint32_t, 3>> triangles;
};

// Triangulation d'une pièce, sommets fusionnés pour obtenir un maillage fermé.
Maillage mailler(const TopoDS_Shape& s, double fleche) {
  BRepMesh_IncrementalMesh maillage(s, fleche, false, 0.25, true);
  Maillage m;
  std::map<std::array<long long, 3>, uint32_t> index;
  for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next()) {
    const TopoDS_Face& f = TopoDS::Face(ex.Current());
    TopLoc_Location loc;
    const Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(f, loc);
    if (tri.IsNull()) continue;
    const gp_Trsf tr = loc.Transformation();
    std::vector<uint32_t> num(static_cast<size_t>(tri->NbNodes()) + 1);
    for (int i = 1; i <= tri->NbNodes(); ++i) {
      const gp_Pnt q = tri->Node(i).Transformed(tr);
      const std::array<long long, 3> cle{std::llround(q.X() * 1e5), std::llround(q.Y() * 1e5), std::llround(q.Z() * 1e5)};
      auto it = index.find(cle);
      if (it == index.end()) {
        it = index.emplace(cle, static_cast<uint32_t>(m.sommets.size())).first;
        m.sommets.push_back({q.X(), q.Y(), q.Z()});
      }
      num[static_cast<size_t>(i)] = it->second;
    }
    const bool inverse = f.Orientation() == TopAbs_REVERSED;
    for (int i = 1; i <= tri->NbTriangles(); ++i) {
      int a = 0, b = 0, c = 0;
      tri->Triangle(i).Get(a, b, c);
      if (inverse) std::swap(b, c);
      const uint32_t A = num[static_cast<size_t>(a)], B = num[static_cast<size_t>(b)], C = num[static_cast<size_t>(c)];
      if (A == B || B == C || A == C) continue;
      m.triangles.push_back({A, B, C});
    }
  }
  return m;
}

std::string nomSimple(const std::string& nom) {
  std::string r;
  for (unsigned char c : nom) r += (std::isalnum(c) || c >= 0x80) ? static_cast<char>(c) : '_';
  return r.empty() ? "piece" : r;
}

uint32_t crc32(const std::string& s) {
  static const std::array<uint32_t, 256> table = [] {
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = i;
      for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
      t[i] = c;
    }
    return t;
  }();
  uint32_t c = 0xFFFFFFFFu;
  for (unsigned char o : s) c = table[(c ^ o) & 0xFF] ^ (c >> 8);
  return c ^ 0xFFFFFFFFu;
}

void le16(std::string& s, uint32_t v) { s += static_cast<char>(v & 0xFF); s += static_cast<char>((v >> 8) & 0xFF); }
void le32(std::string& s, uint32_t v) { le16(s, v & 0xFFFF); le16(s, v >> 16); }

}  // namespace

std::string zipStocke(const std::vector<std::pair<std::string, std::string>>& fichiers) {
  std::string sortie, central;
  for (const auto& [nom, contenu] : fichiers) {
    const uint32_t crc = crc32(contenu), taille = static_cast<uint32_t>(contenu.size()), decalage = static_cast<uint32_t>(sortie.size());
    std::string entete;
    le32(entete, 0x04034b50); le16(entete, 20); le16(entete, 0x0800); le16(entete, 0); le16(entete, 0); le16(entete, 0x21);
    le32(entete, crc); le32(entete, taille); le32(entete, taille); le16(entete, static_cast<uint32_t>(nom.size())); le16(entete, 0);
    sortie += entete + nom + contenu;
    le32(central, 0x02014b50); le16(central, 20); le16(central, 20); le16(central, 0x0800); le16(central, 0); le16(central, 0); le16(central, 0x21);
    le32(central, crc); le32(central, taille); le32(central, taille); le16(central, static_cast<uint32_t>(nom.size()));
    le16(central, 0); le16(central, 0); le16(central, 0); le16(central, 0); le32(central, 0); le32(central, decalage);
    central += nom;
  }
  const uint32_t debut = static_cast<uint32_t>(sortie.size());
  sortie += central;
  le32(sortie, 0x06054b50); le16(sortie, 0); le16(sortie, 0);
  le16(sortie, static_cast<uint32_t>(fichiers.size())); le16(sortie, static_cast<uint32_t>(fichiers.size()));
  le32(sortie, static_cast<uint32_t>(central.size())); le32(sortie, debut); le16(sortie, 0);
  return sortie;
}

void exporterIges(const std::vector<PieceNommee>& pieces, const std::string& chemin) {
  silencer();
  IGESControl_Controller::Init();
  IGESControl_Writer w("MM", 1);
  for (const auto& p : pieces) w.AddShape(p.forme);
  w.ComputeModel();
  if (!w.Write(chemin.c_str())) throw std::runtime_error("Écriture IGES impossible : " + chemin);
}

void exporterBrep(const std::vector<PieceNommee>& pieces, const std::string& chemin) {
  BRep_Builder b;
  TopoDS_Compound ensemble;
  b.MakeCompound(ensemble);
  for (const auto& p : pieces) b.Add(ensemble, p.forme);
  if (!BRepTools::Write(ensemble, chemin.c_str())) throw std::runtime_error("Écriture BREP impossible : " + chemin);
}

void exporterStlTexte(const TopoDS_Shape& s, const std::string& chemin, double fleche) {
  BRepMesh_IncrementalMesh maillage(s, fleche, false, 0.25, true);
  StlAPI_Writer w;
  w.ASCIIMode() = true;
  if (!w.Write(s, chemin.c_str())) throw std::runtime_error("Écriture STL impossible : " + chemin);
}

void exporterObj(const std::vector<PieceNommee>& pieces, const std::string& chemin, double fleche) {
  std::string base = chemin.substr(0, chemin.find_last_of('.')), mtl = base + ".mtl";
  const std::string nomMtl = mtl.substr(mtl.find_last_of("/\\") + 1);
  std::ostringstream obj, mat;
  obj << "# Prométhée\nmtllib " << nomMtl << "\n";
  size_t decalage = 1;
  for (const auto& p : pieces) {
    const Maillage m = mailler(p.forme, fleche);
    const std::string nom = nomSimple(p.nom);
    mat << "newmtl " << nom << "\nKd " << nombre(p.r, 3) << ' ' << nombre(p.g, 3) << ' ' << nombre(p.b, 3) << "\n\n";
    obj << "o " << nom << "\nusemtl " << nom << "\n";
    for (const auto& v : m.sommets) obj << "v " << nombre(v[0]) << ' ' << nombre(v[1]) << ' ' << nombre(v[2]) << '\n';
    for (const auto& t : m.triangles) obj << "f " << t[0] + decalage << ' ' << t[1] + decalage << ' ' << t[2] + decalage << '\n';
    decalage += m.sommets.size();
  }
  ecrireFichier(chemin, obj.str());
  ecrireFichier(mtl, mat.str());
}

void exporterPly(const std::vector<PieceNommee>& pieces, const std::string& chemin, double fleche) {
  std::vector<std::array<float, 3>> sommets;
  std::vector<std::array<unsigned char, 3>> couleurs;
  std::vector<std::array<uint32_t, 3>> faces;
  for (const auto& p : pieces) {
    const Maillage m = mailler(p.forme, fleche);
    const uint32_t base = static_cast<uint32_t>(sommets.size());
    const std::array<unsigned char, 3> c{static_cast<unsigned char>(p.r * 255 + 0.5), static_cast<unsigned char>(p.g * 255 + 0.5), static_cast<unsigned char>(p.b * 255 + 0.5)};
    for (const auto& v : m.sommets) { sommets.push_back({static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2])}); couleurs.push_back(c); }
    for (const auto& t : m.triangles) faces.push_back({t[0] + base, t[1] + base, t[2] + base});
  }
  std::string s = "ply\nformat binary_little_endian 1.0\ncomment Prométhée\nelement vertex " + std::to_string(sommets.size()) +
                  "\nproperty float x\nproperty float y\nproperty float z\nproperty uchar red\nproperty uchar green\nproperty uchar blue\nelement face " +
                  std::to_string(faces.size()) + "\nproperty list uchar int vertex_indices\nend_header\n";
  auto octets = [&s](const void* d, size_t n) { s.append(static_cast<const char*>(d), n); };
  for (size_t i = 0; i < sommets.size(); ++i) { octets(sommets[i].data(), 12); octets(couleurs[i].data(), 3); }
  for (const auto& f : faces) {
    const unsigned char trois = 3;
    octets(&trois, 1);
    for (uint32_t v : f) { const int32_t vi = static_cast<int32_t>(v); octets(&vi, 4); }
  }
  ecrireFichier(chemin, s);
}

void exporter3mf(const std::vector<PieceNommee>& pieces, const std::string& chemin, double fleche) {
  std::ostringstream modele;
  modele << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<model unit=\"millimeter\" xml:lang=\"fr-FR\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">\n"
            " <metadata name=\"Application\">Prométhée</metadata>\n <resources>\n";
  for (size_t i = 0; i < pieces.size(); ++i) {
    const Maillage m = mailler(pieces[i].forme, fleche);
    modele << "  <object id=\"" << i + 1 << "\" type=\"model\" name=\"" << pieces[i].nom << "\">\n   <mesh>\n    <vertices>\n";
    for (const auto& v : m.sommets) modele << "     <vertex x=\"" << nombre(v[0]) << "\" y=\"" << nombre(v[1]) << "\" z=\"" << nombre(v[2]) << "\"/>\n";
    modele << "    </vertices>\n    <triangles>\n";
    for (const auto& t : m.triangles) modele << "     <triangle v1=\"" << t[0] << "\" v2=\"" << t[1] << "\" v3=\"" << t[2] << "\"/>\n";
    modele << "    </triangles>\n   </mesh>\n  </object>\n";
  }
  modele << " </resources>\n <build>\n";
  for (size_t i = 0; i < pieces.size(); ++i) modele << "  <item objectid=\"" << i + 1 << "\"/>\n";
  modele << " </build>\n</model>\n";
  const std::string types =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
      "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
      "<Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/></Types>\n";
  const std::string liens =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
      "<Relationship Target=\"/3D/3dmodel.model\" Id=\"rel0\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/></Relationships>\n";
  ecrireFichier(chemin, zipStocke({{"[Content_Types].xml", types}, {"_rels/.rels", liens}, {"3D/3dmodel.model", modele.str()}}));
}

std::string dxfCarte(const Projet& p, const Derive& d) {
  const auto& c = p.carte;
  const double ox = c.x0 - c.L / 2, oy = c.y0 - c.W / 2;
  std::ostringstream s;
  auto g = [&s](int code, const std::string& v) { s << code << "\r\n" << v << "\r\n"; };
  auto ligne = [&](const char* couche, double x1, double y1, double x2, double y2) {
    g(0, "LINE"); g(8, couche); g(10, nombre(x1, 6)); g(20, nombre(y1, 6)); g(30, "0"); g(11, nombre(x2, 6)); g(21, nombre(y2, 6)); g(31, "0");
  };
  auto arc = [&](const char* couche, double x, double y, double r, double a1, double a2) {
    g(0, "ARC"); g(8, couche); g(10, nombre(x, 6)); g(20, nombre(y, 6)); g(30, "0"); g(40, nombre(r, 6)); g(50, nombre(a1, 6)); g(51, nombre(a2, 6));
  };
  g(0, "SECTION"); g(2, "HEADER"); g(9, "$ACADVER"); g(1, "AC1009"); g(9, "$INSUNITS"); g(70, "4"); g(0, "ENDSEC");
  g(0, "SECTION"); g(2, "TABLES");
  g(0, "TABLE"); g(2, "LTYPE"); g(70, "1"); g(0, "LTYPE"); g(2, "CONTINUOUS"); g(70, "0"); g(3, "Solid line"); g(72, "65"); g(73, "0"); g(40, "0"); g(0, "ENDTAB");
  g(0, "TABLE"); g(2, "LAYER"); g(70, "3");
  for (const auto& [nom, couleur] : {std::pair{"CONTOUR", "7"}, std::pair{"PERCAGES", "1"}, std::pair{"COMPOSANTS", "8"}}) {
    g(0, "LAYER"); g(2, nom); g(70, "0"); g(62, couleur); g(6, "CONTINUOUS");
  }
  g(0, "ENDTAB"); g(0, "ENDSEC");
  g(0, "SECTION"); g(2, "ENTITIES");
  const double r = c.r, L = c.L, W = c.W;
  if (r > 1e-6) {
    ligne("CONTOUR", r, 0, L - r, 0); arc("CONTOUR", L - r, r, r, 270, 360);
    ligne("CONTOUR", L, r, L, W - r); arc("CONTOUR", L - r, W - r, r, 0, 90);
    ligne("CONTOUR", L - r, W, r, W); arc("CONTOUR", r, W - r, r, 90, 180);
    ligne("CONTOUR", 0, W - r, 0, r); arc("CONTOUR", r, r, r, 180, 270);
  } else {
    ligne("CONTOUR", 0, 0, L, 0); ligne("CONTOUR", L, 0, L, W); ligne("CONTOUR", L, W, 0, W); ligne("CONTOUR", 0, W, 0, 0);
  }
  for (const auto& t : d.trous) { g(0, "CIRCLE"); g(8, "PERCAGES"); g(10, nombre(t.x - ox, 6)); g(20, nombre(t.y - oy, 6)); g(30, "0"); g(40, nombre(t.vis.trou / 2, 6)); }
  for (const auto& k : d.comps) {
    const double x1 = k.x1 - ox, x2 = k.x2 - ox, y1 = k.y1 - oy, y2 = k.y2 - oy;
    ligne("COMPOSANTS", x1, y1, x2, y1); ligne("COMPOSANTS", x2, y1, x2, y2); ligne("COMPOSANTS", x2, y2, x1, y2); ligne("COMPOSANTS", x1, y2, x1, y1);
  }
  g(0, "ENDSEC"); g(0, "EOF");
  return s.str();
}

std::string svgCarte(const Projet& p, const Derive& d) {
  const auto& c = p.carte;
  const double ox = c.x0 - c.L / 2, oy = c.y0 - c.W / 2, marge = 14, Lt = c.L + 2 * marge, Wt = c.W + 2 * marge + 12;
  auto X = [&](double x) { return nombre(x - ox + marge, 3); };
  auto Y = [&](double y) { return nombre(c.W - (y - oy) + marge, 3); };
  std::ostringstream s;
  s << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << nombre(Lt, 2) << "mm\" height=\"" << nombre(Wt, 2)
    << "mm\" viewBox=\"0 0 " << nombre(Lt, 2) << ' ' << nombre(Wt, 2) << "\" font-family=\"Arial, sans-serif\">\n"
    << " <title>" << p.nom << " : plan de la carte</title>\n <rect width=\"100%\" height=\"100%\" fill=\"#fff\"/>\n"
    << " <rect x=\"" << nombre(marge, 3) << "\" y=\"" << nombre(marge, 3) << "\" width=\"" << nombre(c.L, 3) << "\" height=\"" << nombre(c.W, 3) << "\" rx=\"" << nombre(c.r, 3)
    << "\" fill=\"#2e6a50\" fill-opacity=\"0.12\" stroke=\"#1e4c38\" stroke-width=\"0.25\"/>\n";
  for (const auto& k : d.comps)
    s << " <rect x=\"" << X(k.x1) << "\" y=\"" << Y(k.y2) << "\" width=\"" << nombre(k.x2 - k.x1, 3) << "\" height=\"" << nombre(k.y2 - k.y1, 3)
      << "\" fill=\"none\" stroke=\"#56645d\" stroke-width=\"0.18\"/>\n <text x=\"" << X(k.x) << "\" y=\"" << Y(k.y) << "\" font-size=\"1.6\" text-anchor=\"middle\" dominant-baseline=\"middle\" fill=\"#18211d\">" << k.ref << "</text>\n";
  for (const auto& t : d.trous)
    s << " <circle cx=\"" << X(t.x) << "\" cy=\"" << Y(t.y) << "\" r=\"" << nombre(t.vis.trou / 2, 3) << "\" fill=\"#fff\" stroke=\"#b4652b\" stroke-width=\"0.25\"/>\n"
      << " <text x=\"" << X(t.x) << "\" y=\"" << Y(t.y + t.vis.tete / 2 + 1) << "\" font-size=\"1.4\" text-anchor=\"middle\" fill=\"#b4652b\">" << t.ref << " Ø" << fmt(t.vis.trou, 2) << "</text>\n";
  const double yc = marge + c.W + 6;
  s << " <g stroke=\"#18211d\" stroke-width=\"0.18\"><line x1=\"" << nombre(marge, 3) << "\" y1=\"" << nombre(yc, 3) << "\" x2=\"" << nombre(marge + c.L, 3) << "\" y2=\"" << nombre(yc, 3) << "\"/></g>\n"
    << " <text x=\"" << nombre(marge + c.L / 2, 3) << "\" y=\"" << nombre(yc - 1, 3) << "\" font-size=\"2.2\" text-anchor=\"middle\">" << fmt(c.L, 2) << " mm</text>\n"
    << " <text x=\"" << nombre(marge - 3, 3) << "\" y=\"" << nombre(marge + c.W / 2, 3) << "\" font-size=\"2.2\" text-anchor=\"middle\" transform=\"rotate(-90 " << nombre(marge - 3, 3) << ' '
    << nombre(marge + c.W / 2, 3) << ")\">" << fmt(c.W, 2) << " mm</text>\n"
    << " <text x=\"" << nombre(marge, 3) << "\" y=\"" << nombre(Wt - 2, 3) << "\" font-size=\"2.4\" font-weight=\"bold\">" << p.nom << "</text>\n</svg>\n";
  return s.str();
}

}  // namespace prom
