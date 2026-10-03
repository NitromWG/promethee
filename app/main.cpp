// Prométhée : application de bureau. Licence GPL-3.0-only.
#include <cstdlib>

#include <QApplication>
#include <QLocale>
#include <QPalette>
#include <QStyleFactory>
#include <QSurfaceFormat>
#include <QTimer>

#include "Document.hpp"
#include "FenetrePrincipale.hpp"
#include "Vue3D.hpp"
#include "promethee/placement.hpp"

int main(int argc, char** argv) {
#if defined(_WIN32)
  QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#elif !defined(__APPLE__)
  // La vue 3D d'OpenCascade passe par X11 : on évite Wayland natif tant qu'il n'est pas pris en charge.
  if (!std::getenv("QT_QPA_PLATFORM")) setenv("QT_QPA_PLATFORM", "xcb", 0);
#endif
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
#ifdef __APPLE__
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setVersion(4, 1);
#else
  format.setProfile(QSurfaceFormat::CompatibilityProfile);
#endif
  QSurfaceFormat::setDefaultFormat(format);

  QApplication app(argc, argv);
  QCoreApplication::setApplicationName(QStringLiteral("Prométhée"));
  QCoreApplication::setOrganizationName(QStringLiteral("Prométhée"));
  QCoreApplication::setApplicationVersion(QStringLiteral("0.3.0"));
  QLocale::setDefault(QLocale(QLocale::French, QLocale::France));

  // Thème clair et cohérent, quel que soit le thème du système : papier, encre et cuivre, comme le prototype.
  QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
  QPalette palette;
  const QColor papier(0xE9, 0xED, 0xEA), surface(0xFB, 0xFC, 0xFB), encre(0x18, 0x21, 0x1D), cuivre(0xB4, 0x65, 0x2B), filet(0xCC, 0xD5, 0xD0);
  palette.setColor(QPalette::Window, papier);
  palette.setColor(QPalette::WindowText, encre);
  palette.setColor(QPalette::Base, Qt::white);
  palette.setColor(QPalette::AlternateBase, QColor(0xF1, 0xF4, 0xF2));
  palette.setColor(QPalette::Text, encre);
  palette.setColor(QPalette::Button, surface);
  palette.setColor(QPalette::ButtonText, encre);
  palette.setColor(QPalette::Highlight, cuivre);
  palette.setColor(QPalette::HighlightedText, Qt::white);
  palette.setColor(QPalette::ToolTipBase, encre);
  palette.setColor(QPalette::ToolTipText, surface);
  palette.setColor(QPalette::PlaceholderText, QColor(0x7D, 0x8A, 0x84));
  palette.setColor(QPalette::Mid, filet);
  palette.setColor(QPalette::Light, Qt::white);
  palette.setColor(QPalette::Dark, QColor(0xA9, 0xB4, 0xAE));
  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(0x9A, 0xA5, 0xA0));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x9A, 0xA5, 0xA0));
  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x9A, 0xA5, 0xA0));
  app.setPalette(palette);
  app.setStyleSheet(QStringLiteral(R"(
    QMenuBar { background: #FBFCFB; border-bottom: 1px solid #CCD5D0; padding: 2px; }
    QMenuBar::item { padding: 5px 10px; border-radius: 5px; }
    QMenuBar::item:selected { background: #EEF1EF; }
    QMenu { background: #FBFCFB; border: 1px solid #CCD5D0; padding: 4px; }
    QMenu::item { padding: 6px 26px 6px 14px; border-radius: 4px; }
    QMenu::item:selected { background: #EEF1EF; color: #18211D; }
    QMenu::separator { height: 1px; background: #CCD5D0; margin: 4px 6px; }
    QToolBar { background: #FBFCFB; border: none; border-bottom: 1px solid #CCD5D0; spacing: 4px; padding: 3px 6px; }
    QToolButton { padding: 4px 10px; border-radius: 5px; color: #18211D; }
    QToolButton:hover { background: #EEF1EF; }
    QToolButton:checked { background: #F2E3D7; color: #18211D; }
    QToolButton:disabled { color: #9AA5A0; }
    QDockWidget::title { padding: 7px 10px; background: #FBFCFB; border-bottom: 1px solid #CCD5D0; font-weight: 600; }
    QTabWidget::pane { border: none; border-top: 1px solid #CCD5D0; background: #FBFCFB; }
    QTabBar::tab { padding: 8px 12px; border: none; border-bottom: 2px solid transparent; color: #56645D; font-weight: 600; background: transparent; }
    QTabBar::tab:selected { color: #18211D; border-bottom: 2px solid #B4652B; }
    QScrollArea, QListWidget, QTableWidget { background: #FBFCFB; border: none; }
    QGroupBox { font-weight: 600; border: none; border-top: 1px solid #CCD5D0; margin-top: 16px; padding-top: 10px; }
    QGroupBox::title { subcontrol-origin: margin; left: 0px; padding: 0 6px 0 0; color: #18211D; }
    QPushButton { padding: 6px 14px; border: 1px solid #CCD5D0; border-radius: 6px; background: #FBFCFB; }
    QPushButton:hover { background: #F1F4F2; }
    QPushButton#principal { background: #B4652B; color: white; border-color: #B4652B; }
    QLineEdit { padding: 4px 6px; border: 1px solid #CCD5D0; border-radius: 5px; background: white; selection-background-color: #B4652B; }
    QLineEdit:focus { border-color: #B4652B; }
    QStatusBar { background: #FBFCFB; border-top: 1px solid #CCD5D0; }
    QSplitter::handle { background: #CCD5D0; }
    QHeaderView::section { background: #F1F4F2; border: none; border-bottom: 1px solid #CCD5D0; padding: 5px; font-weight: 600; }
  )"));

  FenetrePrincipale fenetre;
  QStringList args = app.arguments();
  // --capture image.png : enregistre une capture de la fenêtre puis quitte (essais automatiques, documentation).
  const int capture = static_cast<int>(args.indexOf(QStringLiteral("--capture")));
  QString image;
  if (capture > 0 && capture + 1 < args.size()) {
    image = args.at(capture + 1);
    args.remove(capture, 2);
  }
  // --essai : déroule un scénario d'utilisation (déplacer un trou, agrandir la carte, ajouter une LED,
  // grandir un condensateur, annuler, rétablir) pour vérifier que toutes les vues suivent sans planter.
  const bool essai = args.removeAll(QStringLiteral("--essai")) > 0;
  if (args.size() > 1) fenetre.ouvrirFichier(args.at(1));
  fenetre.show();
  if (essai)
    QTimer::singleShot(800, &fenetre, [&fenetre] {
      Document* doc = fenetre.document();
      doc->modifier([](prom::Projet& p) { for (auto& t : p.trous) if (t.ref == "T1") prom::ancrerTrou(p, t, -20.5, -15.5); });
      doc->debutGeste();
      for (int i = 1; i <= 12; ++i) doc->pendantGeste([i](prom::Projet& p) { p.carte.L = 60 + i; p.carte.x0 = i / 2.0; });
      doc->finGeste();
      std::string led;
      doc->modifier([&led](prom::Projet& p) { led = prom::ajouterComposant(p, "led"); });
      doc->selectionner({Cible::Composant, led});
      doc->modifier([](prom::Projet& p) { for (auto& k : p.composants) if (k.ref == "C1") k.h = 16; });
      doc->annuler();
      doc->retablir();
      // Version 0.3 : insert laiton sur T1, U1 suit le coin haut droit, SW1 verrouillé, la carte s'élargit encore.
      doc->modifier([](prom::Projet& p) {
        for (auto& t : p.trous) if (t.ref == "T1") { t.fixation = "insert"; t.vis = "M2.5"; }
        for (auto& k : p.composants) {
          if (k.ref == "U1") prom::changerAncrage(p, k, "NE");
          if (k.ref == "SW1") k.verrou = true;
        }
        p.carte.L += 8;
        p.carte.x0 += 4;
      });
      for (const auto& k : doc->projet().composants) if (k.ref == "U1") doc->selectionner({Cible::Composant, k.id});
      fenetre.vue3D()->setCouvercle(Vue3D::Couvercle::Souleve);
    });
  if (!image.isEmpty())
    QTimer::singleShot(3000, &fenetre, [&fenetre, image] {
      fenetre.grab().save(image);
      QCoreApplication::exit(0);  // sans passer par la question « enregistrer les modifications ? »
    });
  return app.exec();
}
