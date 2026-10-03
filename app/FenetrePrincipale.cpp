// Prométhée : fenêtre principale. Licence GPL-3.0-only.
#include "FenetrePrincipale.hpp"

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

#include <filesystem>

#include "DialogueExport.hpp"
#include "Document.hpp"
#include "Panneau.hpp"
#include "Vue3D.hpp"
#include "VueCarte.hpp"
#include "promethee/geometrie.hpp"
#include "promethee/kicad.hpp"
#include "promethee/placement.hpp"

using namespace prom;

namespace {
QWidget* cadre(const QString& titre, QWidget* vue, QToolBar* outils) {
  auto* w = new QWidget;
  auto* l = new QVBoxLayout(w);
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(0);
  auto* entete = new QToolBar;
  auto* t = new QLabel(QStringLiteral("  ") + titre + QStringLiteral("  "));
  t->setStyleSheet(QStringLiteral("font-weight: 600;"));
  entete->addWidget(t);
  entete->addActions(outils->actions());
  l->addWidget(entete);
  l->addWidget(vue, 1);
  return w;
}
}  // namespace

FenetrePrincipale::FenetrePrincipale() : m_doc(new Document(this)) {
  m_carte = new VueCarte(m_doc);
  m_vue3d = new Vue3D(m_doc);
  m_panneau = new Panneau(m_doc);

  QToolBar outilsCarte, outilsBoitier;
  auto* groupeFace = new QActionGroup(this);
  for (const auto& [nom, dessous] : {std::pair{QStringLiteral("Dessus"), false}, std::pair{QStringLiteral("Dessous"), true}}) {
    QAction* a = outilsCarte.addAction(nom);
    a->setCheckable(true);
    a->setChecked(!dessous);
    a->setToolTip(dessous ? QStringLiteral("Montrer et modifier la face arrière de la carte (B)") : QStringLiteral("Montrer et modifier la face avant de la carte (B)"));
    groupeFace->addAction(a);
    const bool d = dessous;
    connect(a, &QAction::triggered, this, [this, d] { m_carte->setFaceArriere(d); });
  }
  auto* basculerFace = new QAction(QStringLiteral("Changer de face"), this);
  basculerFace->setShortcut(QKeySequence(Qt::Key_B));
  connect(basculerFace, &QAction::triggered, this, [this, groupeFace] {
    const bool d = !m_carte->faceArriere();
    m_carte->setFaceArriere(d);
    groupeFace->actions().at(d ? 1 : 0)->setChecked(true);
  });
  addAction(basculerFace);
  QAction* recadrerCarte = outilsCarte.addAction(QStringLiteral("Recadrer"));
  connect(recadrerCarte, &QAction::triggered, m_carte, &VueCarte::recadrer);
  auto* groupeCouvercle = new QActionGroup(this);
  const std::pair<QString, Vue3D::Couvercle> modes[] = {{QStringLiteral("Ouvert"), Vue3D::Couvercle::Masque}, {QStringLiteral("Fermé"), Vue3D::Couvercle::Pose},
                                                        {QStringLiteral("Éclaté"), Vue3D::Couvercle::Souleve}};
  for (const auto& [nom, mode] : modes) {
    QAction* a = outilsBoitier.addAction(nom);
    a->setCheckable(true);
    a->setChecked(mode == Vue3D::Couvercle::Souleve);
    groupeCouvercle->addAction(a);
    const Vue3D::Couvercle m = mode;
    connect(a, &QAction::triggered, this, [this, m] { m_vue3d->setCouvercle(m); });
  }
  QAction* transparent = outilsBoitier.addAction(QStringLiteral("Transparent"));
  transparent->setCheckable(true);
  transparent->setToolTip(QStringLiteral("Boîtier transparent : voir la face arrière de la carte à travers le fond (T)"));
  transparent->setShortcut(QKeySequence(Qt::Key_T));
  connect(transparent, &QAction::toggled, this, [this](bool on) { m_vue3d->setBoitierTransparent(on); });
  QAction* recadrer3d = outilsBoitier.addAction(QStringLiteral("Recadrer"));
  connect(recadrer3d, &QAction::triggered, m_vue3d, &Vue3D::recadrer);

  auto* separation = new QSplitter(Qt::Horizontal);
  separation->addWidget(cadre(QStringLiteral("Carte"), m_carte, &outilsCarte));
  separation->addWidget(cadre(QStringLiteral("Boîtier"), m_vue3d, &outilsBoitier));
  separation->setStretchFactor(0, 1);
  separation->setStretchFactor(1, 1);
  setCentralWidget(separation);

  auto* dock = new QDockWidget(QStringLiteral("Détails"), this);
  dock->setObjectName(QStringLiteral("details"));
  dock->setWidget(m_panneau);
  dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  addDockWidget(Qt::RightDockWidgetArea, dock);

  // Menus
  QMenu* fichier = menuBar()->addMenu(QStringLiteral("&Fichier"));
  fichier->addAction(QStringLiteral("&Nouveau projet"), QKeySequence::New, this, [this] { if (confirmerAbandon()) m_doc->nouveau(); });
  fichier->addAction(QStringLiteral("&Ouvrir…"), QKeySequence::Open, this, [this] {
    if (!confirmerAbandon()) return;
    const QString chemin = QFileDialog::getOpenFileName(this, QStringLiteral("Ouvrir un projet"), QString(), QStringLiteral("Projets Prométhée (*.json)"));
    if (!chemin.isEmpty()) ouvrirFichier(chemin);
  });
  fichier->addAction(QStringLiteral("&Importer une carte KiCad…"), QKeySequence(Qt::CTRL | Qt::Key_I), this, [this] { importerKicad(); });
  fichier->addAction(QStringLiteral("&Enregistrer"), QKeySequence::Save, this, [this] { enregistrer(false); });
  fichier->addAction(QStringLiteral("Enregistrer &sous…"), QKeySequence::SaveAs, this, [this] { enregistrer(true); });
  fichier->addSeparator();
  fichier->addAction(QStringLiteral("Recharger l’exemple"), this, [this] { if (confirmerAbandon()) m_doc->exemple(); });
  fichier->addAction(QStringLiteral("E&xporter le dossier de fabrication…"), QKeySequence(Qt::CTRL | Qt::Key_E), this, [this] { exporterDossier(); });
  fichier->addSeparator();
  fichier->addAction(QStringLiteral("&Quitter"), QKeySequence::Quit, this, &QWidget::close);

  QMenu* edition = menuBar()->addMenu(QStringLiteral("É&dition"));
  QAction* annuler = edition->addAction(QStringLiteral("&Annuler"), QKeySequence::Undo, m_doc, &Document::annuler);
  QAction* retablir = edition->addAction(QStringLiteral("&Rétablir"), QKeySequence::Redo, m_doc, &Document::retablir);

  QMenu* ajouter = menuBar()->addMenu(QStringLiteral("&Ajouter"));
  ajouter->addAction(QStringLiteral("Trou de fixation"), this, [this] {
    std::string id;
    m_doc->modifier([&](Projet& p) { id = ajouterTrou(p); });
    m_doc->selectionner({Cible::Trou, id});
  });
  ajouter->addSeparator();
  for (const auto& T : typesComposants()) {
    const std::string cle = T.cle;
    QString nom = QString::fromStdString(T.nom);
    if (T.bord) nom += QStringLiteral(" (perce la paroi)");
    else if (T.couvercle > 0) nom += QStringLiteral(" (perce le couvercle)");
    ajouter->addAction(nom, this, [this, cle] {
      std::string id;
      m_doc->modifier([&](Projet& p) { id = ajouterComposant(p, cle); });
      m_doc->selectionner({Cible::Composant, id});
    });
  }

  QMenu* affichage = menuBar()->addMenu(QStringLiteral("Affic&hage"));
  affichage->addAction(QStringLiteral("Recadrer les deux vues"), QKeySequence(Qt::Key_F), this, [this] { m_carte->recadrer(); m_vue3d->recadrer(); });
  affichage->addSection(QStringLiteral("Vues de la 3D"));
  const std::tuple<QString, V3d_TypeOfOrientation, QKeySequence> vues[] = {
      {QStringLiteral("Isométrique"), V3d_XnegYnegZpos, QKeySequence(Qt::Key_0)}, {QStringLiteral("Face"), V3d_Yneg, QKeySequence(Qt::Key_1)},
      {QStringLiteral("Dessus"), V3d_Zpos, QKeySequence(Qt::Key_7)}, {QStringLiteral("Droite"), V3d_Xpos, QKeySequence(Qt::Key_3)},
      {QStringLiteral("Gauche"), V3d_Xneg, QKeySequence()}, {QStringLiteral("Arrière"), V3d_Ypos, QKeySequence()}, {QStringLiteral("Dessous"), V3d_Zneg, QKeySequence()}};
  for (const auto& [nom, orientation, raccourci] : vues) {
    const V3d_TypeOfOrientation o = orientation;
    QAction* a = affichage->addAction(nom, this, [this, o] { m_vue3d->vueStandard(o); });
    a->setShortcut(raccourci);
  }
  affichage->addSection(QStringLiteral("Couvercle"));
  affichage->addActions(groupeCouvercle->actions());

  QMenu* aide = menuBar()->addMenu(QStringLiteral("Ai&de"));
  aide->addAction(QStringLiteral("À propos de Prométhée"), this, [this] {
    QMessageBox::about(this, QStringLiteral("À propos de Prométhée"),
                       QStringLiteral("<h3>Prométhée 0.5</h3><p>Plateforme libre d’ingénierie intégrée : la carte électronique et son boîtier forment un seul modèle.</p>"
                                      "<p>Vue Carte : glisser un composant, un trou ou une poignée du bord de la carte, molette pour zoomer, double-clic pour pivoter. "
                                      "Vue Boîtier : bouton gauche pour tourner, droit pour déplacer, molette pour zoomer.</p>"
                                      "<p>Licence GPL-3.0. Géométrie : Open CASCADE Technology. Interface : Qt.</p><pre>%1</pre>")
                           .arg(m_vue3d->infoGl().toHtmlEscaped()));
  });

  auto* barre = addToolBar(QStringLiteral("Principale"));
  barre->setObjectName(QStringLiteral("principale"));
  barre->addAction(annuler);
  barre->addAction(retablir);
  barre->addSeparator();
  QAction* verrouiller = barre->addAction(QStringLiteral("Verrouiller"));
  verrouiller->setCheckable(true);
  verrouiller->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
  verrouiller->setToolTip(QStringLiteral("Verrouiller la position du composant sélectionné (Ctrl+L)"));
  connect(verrouiller, &QAction::triggered, this, [this](bool on) {
    const Cible s = m_doc->selection();
    if (s.type != Cible::Composant) return;
    m_doc->modifier([&](Projet& p) { for (auto& k : p.composants) if (k.id == s.id) k.verrou = on; });
  });
  auto majVerrou = [this, verrouiller] {
    const Composant* k = m_doc->composantSelectionne();
    verrouiller->setEnabled(k != nullptr);
    verrouiller->setChecked(k && k->verrou);
    verrouiller->setText(k && k->verrou ? QStringLiteral("Verrouillé") : QStringLiteral("Verrouiller"));
  };
  connect(m_doc, &Document::selectionChangee, this, majVerrou);
  connect(m_doc, &Document::change, this, majVerrou);
  majVerrou();
  edition->addSeparator();
  edition->addAction(verrouiller);
  barre->addSeparator();
  QAction* exporter = barre->addAction(QStringLiteral("Dossier de fabrication"));
  connect(exporter, &QAction::triggered, this, [this] { exporterDossier(); });

  m_etat = new QLabel;
  statusBar()->addPermanentWidget(m_etat);
  auto majHistorique = [annuler, retablir, this] {
    annuler->setEnabled(m_doc->peutAnnuler());
    retablir->setEnabled(m_doc->peutRetablir());
    majEtat();
  };
  connect(m_doc, &Document::etatChange, this, majHistorique);
  connect(m_doc, &Document::change, this, [this] { majEtat(); });
  majHistorique();
  resize(1440, 860);
}

void FenetrePrincipale::majEtat() {
  const int erreurs = m_doc->nombreErreurs();
  const int alertes = static_cast<int>(m_doc->problemes().size()) - erreurs;
  if (erreurs) m_etat->setText(QStringLiteral("<span style='color:#C2402E'><b>%1 erreur%2</b></span>").arg(erreurs).arg(erreurs > 1 ? "s" : ""));
  else if (alertes) m_etat->setText(QStringLiteral("<span style='color:#A87410'><b>%1 alerte%2</b></span>").arg(alertes).arg(alertes > 1 ? "s" : ""));
  else m_etat->setText(QStringLiteral("<span style='color:#2F7D53'><b>Tout est cohérent</b></span>"));
  const QString nom = m_doc->chemin().isEmpty() ? QString::fromStdString(m_doc->projet().nom) : QFileInfo(m_doc->chemin()).fileName();
  setWindowTitle(nom + QStringLiteral("[*] – Prométhée"));
  setWindowModified(m_doc->modifie());
}

void FenetrePrincipale::ouvrirFichier(const QString& chemin) {
  try {
    m_doc->ouvrir(chemin);
    m_carte->recadrer();
    m_vue3d->recadrer();
  } catch (const std::exception& e) {
    QMessageBox::warning(this, QStringLiteral("Ouverture impossible"), QString::fromUtf8(e.what()));
  }
}

bool FenetrePrincipale::enregistrer(bool sousNouveauNom) {
  QString chemin = m_doc->chemin();
  if (sousNouveauNom || chemin.isEmpty()) {
    chemin = QFileDialog::getSaveFileName(this, QStringLiteral("Enregistrer le projet"), QString::fromStdString(m_doc->projet().nom) + QStringLiteral(".prom.json"),
                                          QStringLiteral("Projets Prométhée (*.json)"));
    if (chemin.isEmpty()) return false;
  }
  try {
    m_doc->enregistrer(chemin);
    statusBar()->showMessage(QStringLiteral("Projet enregistré."), 3000);
    return true;
  } catch (const std::exception& e) {
    QMessageBox::warning(this, QStringLiteral("Enregistrement impossible"), QString::fromUtf8(e.what()));
    return false;
  }
}

bool FenetrePrincipale::confirmerAbandon() {
  if (!m_doc->modifie()) return true;
  const auto r = QMessageBox::question(this, QStringLiteral("Modifications non enregistrées"), QStringLiteral("Enregistrer les modifications du projet ?"),
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
  if (r == QMessageBox::Save) return enregistrer(false);
  return r == QMessageBox::Discard;
}

void FenetrePrincipale::closeEvent(QCloseEvent* e) {
  if (confirmerAbandon()) e->accept();
  else e->ignore();
}

void FenetrePrincipale::importerKicad() {
  if (!confirmerAbandon()) return;
  const QString chemin = QFileDialog::getOpenFileName(this, QStringLiteral("Importer une carte KiCad"), QString(), QStringLiteral("Cartes KiCad (*.kicad_pcb)"));
  if (!chemin.isEmpty()) importerFichierKicad(chemin, true);
}

void FenetrePrincipale::importerFichierKicad(const QString& chemin, bool messages) {
  try {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    RapportImport r;
    const Projet p = projetDepuisKicad(lireFichierKicad(std::filesystem::path(chemin.toStdU16String()).string()), QFileInfo(chemin).completeBaseName().toStdString(), &r);
    m_doc->remplacer(p);
    m_doc->selectionner({});
    m_carte->recadrer();
    m_vue3d->recadrer();
    QApplication::restoreOverrideCursor();
    if (!messages) return;
    QString texte = QStringLiteral("Carte importée : contour réel, %1 trous de fixation, %2 composants dont %3 connecteurs de bord.").arg(r.trous).arg(r.composants).arg(r.connecteursBord);
    if (r.dessous) texte += QStringLiteral("\n%1 composants de la face arrière ne sont pas encore pris en compte.").arg(r.dessous);
    texte += QStringLiteral("\n\nLes hauteurs des composants sont estimées d’après le nom de leur empreinte : vérifie les plus hauts dans le panneau.");
    QMessageBox::information(this, QStringLiteral("Import KiCad"), texte);
  } catch (const std::exception& e) {
    QApplication::restoreOverrideCursor();
    QMessageBox::warning(this, QStringLiteral("Import impossible"), QString::fromUtf8(e.what()));
  }
}

void FenetrePrincipale::exporterDossier() {
  if (m_doc->nombreErreurs() > 0 &&
      QMessageBox::question(this, QStringLiteral("Le projet contient des erreurs"), QStringLiteral("Les fichiers seront produits tels quels. Continuer ?")) != QMessageBox::Yes)
    return;
  DialogueExport dialogue(m_doc, this);
  if (dialogue.exec() != QDialog::Accepted) return;
  try {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const QStringList ecrits = dialogue.exporter();
    QApplication::restoreOverrideCursor();
    QMessageBox::information(this, QStringLiteral("Dossier de fabrication"),
                             ecrits.isEmpty() ? QStringLiteral("Aucun format choisi.") : QStringLiteral("Fichiers écrits :\n") + ecrits.join(QStringLiteral("\n")));
  } catch (const std::exception& e) {
    QApplication::restoreOverrideCursor();
    QMessageBox::warning(this, QStringLiteral("Export impossible"), QString::fromUtf8(e.what()));
  }
}
