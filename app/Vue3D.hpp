// Prométhée : vue 3D du boîtier, de la carte et des composants (OpenCascade). Licence GPL-3.0-only.
// Le principe d'intégration (contexte OpenGL de Qt réutilisé par OpenCascade) vient de
// « occt-samples-qopenglwidget » de Kirill Gavrilov, sous licence MIT (voir OcctGlTools.hpp).
#pragma once

#include <map>
#include <string>

#include <QOpenGLWidget>
#include <QTimer>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <AIS_ViewCube.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <V3d_View.hxx>

#include "Document.hpp"

class Vue3D : public QOpenGLWidget, public AIS_ViewController {
  Q_OBJECT
public:
  enum class Couvercle { Masque, Pose, Souleve };
  explicit Vue3D(Document* doc, QWidget* parent = nullptr);
  ~Vue3D() override;

  void setCouvercle(Couvercle c);
  void recadrer();
  void vueStandard(V3d_TypeOfOrientation orientation);  // transition animée vers une vue normalisée
  const QString& infoGl() const { return m_infoGl; }
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

  Document* m_doc;
  Handle(V3d_Viewer) m_visu;
  Handle(V3d_View) m_vue;
  Handle(AIS_InteractiveContext) m_ctx;
  Handle(AIS_Shape) m_corps, m_couvercle, m_carte;
  Handle(AIS_ViewCube) m_cube;
  std::map<std::string, Handle(AIS_Shape)> m_composants;
  std::string m_sigCorps, m_sigCouvercle, m_sigCarte;
  QTimer m_minuteur;
  Couvercle m_modeCouvercle = Couvercle::Souleve;
  QString m_infoGl;
  bool m_pret = false, m_cadre = false, m_synchro = false;
};
