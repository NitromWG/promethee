// Prométhée : fenêtre principale de l'application de bureau. Licence GPL-3.0-only.
#pragma once

#include <QMainWindow>

class Document;
class Panneau;
class QLabel;
class Vue3D;
class VueCarte;

class FenetrePrincipale : public QMainWindow {
  Q_OBJECT
public:
  FenetrePrincipale();
  void ouvrirFichier(const QString& chemin);
  void importerFichierKicad(const QString& chemin, bool messages);
  Document* document() const { return m_doc; }
  VueCarte* vueCarte() const { return m_carte; }
  Vue3D* vue3D() const { return m_vue3d; }

protected:
  void closeEvent(QCloseEvent* e) override;

private:
  bool confirmerAbandon();
  bool enregistrer(bool sousNouveauNom);
  void exporterDossier();
  void importerKicad();
  void majEtat();

  Document* m_doc;
  VueCarte* m_carte;
  Vue3D* m_vue3d;
  Panneau* m_panneau;
  QLabel* m_etat;
};
