// Prométhée : application de bureau. Licence GPL-3.0-only.
#include <cstdlib>

#include <QApplication>
#include <QLocale>
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
  QCoreApplication::setApplicationVersion(QStringLiteral("0.2.0"));
  QLocale::setDefault(QLocale(QLocale::French, QLocale::France));

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
      fenetre.vue3D()->setCouvercle(Vue3D::Couvercle::Souleve);
    });
  if (!image.isEmpty())
    QTimer::singleShot(3000, &fenetre, [&fenetre, image] {
      fenetre.grab().save(image);
      QCoreApplication::exit(0);  // sans passer par la question « enregistrer les modifications ? »
    });
  return app.exec();
}
