// Prométhée : pièces mécaniques exactes (B-rep OpenCascade). Licence GPL-3.0-only.
#include "promethee/geometrie.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <Bnd_Box.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GProp_GProps.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Message_Printer.hxx>
#include <Quantity_Color.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <STEPControl_Reader.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <StlAPI_Writer.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopExp_Explorer.hxx>
#include <NCollection_List.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

namespace prom {

namespace {

constexpr double TOL = 1e-7;

// Repère plan : un point d'origine et deux directions, pour tracer un profil dans n'importe quel plan.
struct Repere {
  gp_Pnt o;
  gp_Vec u, v;
  gp_Pnt point(double a, double b) const { return o.Translated(u * a + v * b); }
};

TopoDS_Wire filRectArrondi(const Repere& R, double cu, double cv, double hu, double hv, double r) {
  r = std::clamp(r, 0.0, std::min(hu, hv));
  BRepBuilderAPI_MakeWire fil;
  auto ligne = [&](double a1, double b1, double a2, double b2) {
    const gp_Pnt P = R.point(a1, b1), Q = R.point(a2, b2);
    if (P.Distance(Q) > TOL) fil.Add(BRepBuilderAPI_MakeEdge(P, Q).Edge());
  };
  auto arc = [&](double ca, double cb, double a0) {
    if (r < TOL) return;
    const double a1 = a0 + std::numbers::pi / 4, a2 = a0 + std::numbers::pi / 2;
    const gp_Pnt P = R.point(ca + r * std::cos(a0), cb + r * std::sin(a0));
    const gp_Pnt M = R.point(ca + r * std::cos(a1), cb + r * std::sin(a1));
    const gp_Pnt Q = R.point(ca + r * std::cos(a2), cb + r * std::sin(a2));
    fil.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(P, M, Q).Value()).Edge());
  };
  ligne(cu - hu + r, cv - hv, cu + hu - r, cv - hv);
  arc(cu + hu - r, cv - hv + r, -std::numbers::pi / 2);
  ligne(cu + hu, cv - hv + r, cu + hu, cv + hv - r);
  arc(cu + hu - r, cv + hv - r, 0);
  ligne(cu + hu - r, cv + hv, cu - hu + r, cv + hv);
  arc(cu - hu + r, cv + hv - r, std::numbers::pi / 2);
  ligne(cu - hu, cv + hv - r, cu - hu, cv - hv + r);
  arc(cu - hu + r, cv - hv + r, std::numbers::pi);
  if (!fil.IsDone()) throw std::runtime_error("Contour arrondi impossible à construire.");
  return fil.Wire();
}

TopoDS_Shape prisme(const Repere& R, double cu, double cv, double hu, double hv, double r, const gp_Vec& profondeur) {
  const TopoDS_Face f = BRepBuilderAPI_MakeFace(filRectArrondi(R, cu, cv, hu, hv, r), true).Face();
  return BRepPrimAPI_MakePrism(f, profondeur).Shape();
}

// Prisme vertical à base de rectangle arrondi, de z0 à z0 + h.
TopoDS_Shape prismeRR(double cx, double cy, double hx, double hy, double r, double z0, double h) {
  const Repere R{gp_Pnt(0, 0, z0), gp_Vec(1, 0, 0), gp_Vec(0, 1, 0)};
  return prisme(R, cx, cy, hx, hy, r, gp_Vec(0, 0, h));
}

TopoDS_Shape cylindre(double x, double y, double z0, double r, double h) {
  return BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(x, y, z0), gp_Dir(0, 0, 1)), r, h).Shape();
}

TopoDS_Shape booleen(bool fusion, const TopoDS_Shape& a, const std::vector<TopoDS_Shape>& outils) {
  if (outils.empty()) return a;
  NCollection_List<TopoDS_Shape> args, tools;  // TopTools_ListOfShape, obsolète depuis OpenCascade 8
  args.Append(a);
  for (const auto& o : outils) tools.Append(o);
  if (fusion) {
    BRepAlgoAPI_Fuse op;
    op.SetArguments(args); op.SetTools(tools); op.SetRunParallel(true); op.Build();
    if (!op.IsDone() || op.HasErrors()) throw std::runtime_error("Union de solides impossible.");
    return op.Shape();
  }
  BRepAlgoAPI_Cut op;
  op.SetArguments(args); op.SetTools(tools); op.SetRunParallel(true); op.Build();
  if (!op.IsDone() || op.HasErrors()) throw std::runtime_error("Soustraction de solides impossible.");
  return op.Shape();
}

// Fusionne les faces coplanaires et arêtes alignées laissées par les opérations booléennes.
TopoDS_Shape unifier(const TopoDS_Shape& s) {
  ShapeUpgrade_UnifySameDomain u(s, true, true, false);
  u.Build();
  return u.Shape();
}

// Volume qui traverse une paroi à l'emplacement d'une découpe (dépasse de 1 mm de chaque côté).
TopoDS_Shape prismeDecoupe(const Derive& d, double paroi, Bord mur, const DecoupeValide& o) {
  Repere R;
  gp_Vec sens;
  switch (mur) {
    case Bord::N: R = {gp_Pnt(0, d.cy + d.Wi / 2 - 1, 0), gp_Vec(1, 0, 0), gp_Vec(0, 0, 1)}; sens = gp_Vec(0, paroi + 2, 0); break;
    case Bord::S: R = {gp_Pnt(0, d.cy - d.Wi / 2 + 1, 0), gp_Vec(1, 0, 0), gp_Vec(0, 0, 1)}; sens = gp_Vec(0, -(paroi + 2), 0); break;
    case Bord::E: R = {gp_Pnt(d.cx + d.Li / 2 - 1, 0, 0), gp_Vec(0, 1, 0), gp_Vec(0, 0, 1)}; sens = gp_Vec(paroi + 2, 0, 0); break;
    default:      R = {gp_Pnt(d.cx - d.Li / 2 + 1, 0, 0), gp_Vec(0, 1, 0), gp_Vec(0, 0, 1)}; sens = gp_Vec(-(paroi + 2), 0, 0); break;
  }
  return prisme(R, o.s, o.t, o.w / 2, o.h / 2, o.r, sens);
}

// Les échanges STEP d'OpenCascade sont bavards : on ne garde que les échecs.
void silencer() {
  static const bool fait = [] {
    for (const auto& imprimante : Message::DefaultMessenger()->Printers()) imprimante->SetTraceLevel(Message_Fail);
    return true;
  }();
  (void)fait;
}

}  // namespace

TopoDS_Shape construireCorps(const Projet& p, const Derive& d) {
  const TopoDS_Shape exterieur = prismeRR(d.cx, d.cy, d.Lo / 2, d.Wo / 2, d.ro, 0, d.zt);
  const TopoDS_Shape cavite = prismeRR(d.cx, d.cy, d.Li / 2, d.Wi / 2, d.ri, d.zf, d.zt - d.zf + 1);
  TopoDS_Shape corps = booleen(false, exterieur, {cavite});

  std::vector<TopoDS_Shape> futs, retraits;
  for (const auto& q : piliersValides(d)) {
    futs.push_back(cylindre(q.x, q.y, d.zf - 0.5, q.Rb, d.zpb - d.zf + 0.5));   // ancré dans le fond
    retraits.push_back(cylindre(q.x, q.y, d.zf, q.rp, d.zpb - d.zf + 1));         // avant-trou ou logement d'insert
  }
  corps = booleen(true, corps, futs);
  for (const auto& [mur, liste] : decoupesValides(d))
    for (const auto& o : liste) retraits.push_back(prismeDecoupe(d, p.boitier.paroi, mur, o));
  corps = booleen(false, corps, retraits);
  return unifier(corps);
}

TopoDS_Shape construireCouvercle(const Projet& p, const Derive& d) {
  const double e = p.boitier.couvercle;
  const TopoDS_Shape plaque = prismeRR(d.cx, d.cy, d.Lo / 2, d.Wo / 2, d.ro, d.zt, e);
  // La lèvre monte de 0,5 mm dans la plaque pour une union franche, sans face commune.
  const TopoDS_Shape levreExt = prismeRR(d.cx, d.cy, d.lo.hx, d.lo.hy, d.lo.r, d.zLevre, Levre::h + 0.5);
  const TopoDS_Shape levreInt = prismeRR(d.cx, d.cy, d.li.hx, d.li.hy, d.li.r, d.zLevre - 1, Levre::h + 1.5);
  TopoDS_Shape couvercle = booleen(true, plaque, {booleen(false, levreExt, {levreInt})});
  std::vector<TopoDS_Shape> percages;
  for (const auto& o : percagesValides(d)) percages.push_back(cylindre(o.x, o.y, d.zt - 1, o.d / 2, e + 2));
  couvercle = booleen(false, couvercle, percages);
  return unifier(couvercle);
}

TopoDS_Shape construireCarte(const Projet& p, const Derive& d) {
  const auto& c = p.carte;
  const TopoDS_Shape plaque = prismeRR(c.x0, c.y0, c.L / 2, c.W / 2, c.r, d.zpb, c.t);
  std::vector<TopoDS_Shape> trous;
  for (const auto& t : d.trous)
    if (sdRR(t.x, t.y, c.x0, c.y0, c.L / 2, c.W / 2, c.r) < -(t.vis.trou / 2 + 0.05)) trous.push_back(cylindre(t.x, t.y, d.zpb - 1, t.vis.trou / 2, c.t + 2));
  return booleen(false, plaque, trous);
}

TopoDS_Shape construireComposant(const Derive& d, const GeoComp& g) {
  const TypeComposant* T = typeComposant(g.type);
  const double h = std::max(0.2, g.h);
  if (T && T->rond) return cylindre(g.x, g.y, d.zpt, std::max(0.1, std::min(g.hx, g.hy)), h);
  return BRepPrimAPI_MakeBox(gp_Pnt(g.x - g.hx, g.y - g.hy, d.zpt), std::max(0.1, 2 * g.hx), std::max(0.1, 2 * g.hy), h).Shape();
}

double volume(const TopoDS_Shape& s) {
  GProp_GProps props;
  BRepGProp::VolumeProperties(s, props);
  return props.Mass();
}

bool estValide(const TopoDS_Shape& s) { return !s.IsNull() && BRepCheck_Analyzer(s).IsValid(); }

int nombreSolides(const TopoDS_Shape& s) {
  int n = 0;
  for (TopExp_Explorer ex(s, TopAbs_SOLID); ex.More(); ex.Next()) ++n;
  return n;
}

Boite encombrement(const TopoDS_Shape& s) {
  Bnd_Box b;
  BRepBndLib::AddOptimal(s, b, false, false);
  Boite r;
  b.Get(r.xmin, r.ymin, r.zmin, r.xmax, r.ymax, r.zmax);
  return r;
}

TopoDS_Shape pourImpression(const TopoDS_Shape& s, bool retourner, double cx, double cy) {
  gp_Trsf t;
  t.SetTranslation(gp_Vec(-cx, -cy, 0));
  TopoDS_Shape r = BRepBuilderAPI_Transform(s, t, true).Shape();
  if (retourner) {
    gp_Trsf rot;
    rot.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)), std::numbers::pi);
    r = BRepBuilderAPI_Transform(r, rot, true).Shape();
  }
  const Boite b = encombrement(r);
  gp_Trsf pose;
  pose.SetTranslation(gp_Vec(0, 0, -b.zmin));
  return BRepBuilderAPI_Transform(r, pose, true).Shape();
}

void exporterStep(const std::vector<PieceNommee>& pieces, const std::string& chemin) {
  silencer();
  Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
  Handle(TDocStd_Document) doc;
  app->NewDocument("MDTV-XCAF", doc);
  Handle(XCAFDoc_ShapeTool) formes = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
  Handle(XCAFDoc_ColorTool) couleurs = XCAFDoc_DocumentTool::ColorTool(doc->Main());
  for (const auto& piece : pieces) {
    const TDF_Label l = formes->AddShape(piece.forme, false);
    TDataStd_Name::Set(l, TCollection_ExtendedString(piece.nom.c_str(), true));
    couleurs->SetColor(l, Quantity_Color(piece.r, piece.g, piece.b, Quantity_TOC_RGB), XCAFDoc_ColorGen);
  }
  STEPCAFControl_Writer w;
  w.SetNameMode(true);
  w.SetColorMode(true);
  const bool ok = w.Transfer(doc, STEPControl_AsIs) && w.Write(chemin.c_str()) == IFSelect_RetDone;
  app->Close(doc);
  if (!ok) throw std::runtime_error("Écriture STEP impossible : " + chemin);
}

TopoDS_Shape lireStep(const std::string& chemin) {
  silencer();
  STEPControl_Reader r;
  if (r.ReadFile(chemin.c_str()) != IFSelect_RetDone) throw std::runtime_error("Lecture STEP impossible : " + chemin);
  r.TransferRoots();
  return r.OneShape();
}

void exporterStl(const TopoDS_Shape& s, const std::string& chemin, double fleche) {
  BRepMesh_IncrementalMesh maillage(s, fleche, false, 0.25, true);
  StlAPI_Writer w;
  w.ASCIIMode() = false;
  if (!w.Write(s, chemin.c_str())) throw std::runtime_error("Écriture STL impossible : " + chemin);
}

}  // namespace prom
