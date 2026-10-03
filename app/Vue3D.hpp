// Prométhée : vue 3D du boîtier, de la carte et des composants (OpenCascade). Licence GPL-3.0-only.
// Le principe d'intégration (contexte OpenGL de Qt réutilisé par OpenCascade) vient de
// « occt-samples-qopenglwidget » de Kirill Gavrilov, sous licence MIT (voir OcctGlTools.hpp).
#pragma once

#include <map>
#include <string>
#include <thread>

#include <QOpenGLWidget>
#include <QTimer>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <AIS_ViewCube.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <V3d_View.hxx>
#include <gp_Pnt.hxx>

#include "Document.hpp"

class Vue3D : public QOpenGLWidget, public AIS_ViewController {
  Q_OBJECT
public:
  enum class Couvercle { Masque, Pose, Souleve };
  explicit Vue3D(Document* doc, QWidget* parent = nullptr);
  ~Vue3D() override;

  void setCouvercle(Couvercle c);
  void recadrer();
  void setBoitierTransparent(bool transparent);
  void vueStandard(V3d_TypeOfOrientation orientation);  // transition animée vers une vue normalisée
  const QString& infoGl() const { return m_infoGl; }
  gp_Dir directionCamera() const { return m_vue->Camera()->Direction(); }
  QSize minimumSizeHint() const override { return {240, 200}; }
  QSize sizeHint() const override { return {640, 480}; }

protected:
  void initializeGL() override;
  void paintGL() override;
  void mousePressEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void wheelEvent(QWheelEvent* e) override;
  void OnSelectionChanged(const Handle(AIS_InteractiveContext)& ctx, const Handle(V3d_View)& vue) override;
  void handleViewRedraw(const Handle(AIS_InteractiveContext)& ctx, const Handle(V3d_View)& vue) override;

private:
  void planifier();
  void reconstruire();
  void afficherSelection();
  void placerCouvercle();
  bool transmettreSouris(QMouseEvent* e);
  void tourner(double dx, double dy);
  void cliquer(const QPointF& position, Qt::KeyboardModifiers modificateurs);
  // Rotation au bouton gauche, gérée ici plutôt que par OpenCascade pour tourner sans butée.
  struct Rotation { bool actif = false, bouge = false; QPointF depart, dernier; };
  Rotation m_rotation;
  gp_Pnt m_pivot;

  Document* m_doc;
  Handle(V3d_Viewer) m_visu;
  Handle(V3d_View) m_vue;
  Handle(AIS_InteractiveContext) m_ctx;
  Handle(AIS_Shape) m_corps, m_couvercle, m_carte;
  Handle(AIS_ViewCube) m_cube;
  std::map<std::string, Handle(AIS_Shape)> m_composants;
  std::map<std::string, std::string> m_sigComposants;
  std::string m_sigCorps, m_sigCouvercle, m_sigCarte;
  // Le boîtier et le couvercle se construisent et se maillent dans un fil à part : l'interface reste fluide.
  void appliquer(const TopoDS_Shape& corps, const TopoDS_Shape& couvercle, const std::string& sc, const std::string& sl, const QString& erreur);
  std::thread m_ouvrier;
  bool m_enCours = false, m_relancer = false, m_cadreCorps = false, m_transparent = false;
  std::string m_sigCorpsDemande, m_sigCouvercleDemande;
  QTimer m_minuteur;
  Couvercle m_modeCouvercle = Couvercle::Souleve;
  QString m_infoGl;
  bool m_pret = false, m_cadre = false, m_synchro = false;
};
