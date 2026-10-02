// Prométhée : le document ouvert, partagé par toutes les vues. Licence GPL-3.0-only.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include <QObject>
#include <QString>

#include "promethee/derive.hpp"
#include "promethee/modele.hpp"
#include "promethee/verifs.hpp"

// Élément désigné dans les vues (rien, la carte, un composant ou un trou).
struct Cible {
  enum Type { Aucune, Carte, Composant, Trou } type = Aucune;
  std::string id;
  bool operator==(const Cible& o) const { return type == o.type && id == o.id; }
  bool operator!=(const Cible& o) const { return !(*this == o); }
};

class Document : public QObject {
  Q_OBJECT
public:
  explicit Document(QObject* parent = nullptr);

  const prom::Projet& projet() const { return m_projet; }
  const prom::Derive& derive() const { return m_derive; }
  const std::vector<prom::Probleme>& problemes() const { return m_problemes; }
  int nombreErreurs() const;

  // Modification en un pas d'historique.
  void modifier(const std::function<void(prom::Projet&)>& f);
  void remplacer(const prom::Projet& p);
  // Geste continu (glisser) : toutes les étapes comptent pour un seul pas d'historique.
  void debutGeste();
  void pendantGeste(const std::function<void(prom::Projet&)>& f);
  void finGeste();
  bool enGeste() const { return m_geste; }

  bool peutAnnuler() const { return !m_annuler.empty(); }
  bool peutRetablir() const { return !m_retablir.empty(); }
  void annuler();
  void retablir();

  const Cible& selection() const { return m_selection; }
  void selectionner(const Cible& c);
  const prom::Composant* composantSelectionne() const;
  const prom::Trou* trouSelectionne() const;

  const QString& chemin() const { return m_chemin; }
  bool modifie() const;
  void ouvrir(const QString& chemin);        // lève std::runtime_error
  void enregistrer(const QString& chemin);   // lève std::runtime_error
  void nouveau();
  void exemple();

signals:
  void change();             // le modèle a changé, tout est recalculé
  void selectionChangee();
  void etatChange();         // historique, chemin ou état « modifié »

private:
  std::string instantane() const;
  void restaurer(const std::string& s);
  void recalculer();
  void validerSelection();

  prom::Projet m_projet;
  prom::Derive m_derive;
  std::vector<prom::Probleme> m_problemes;
  std::vector<std::string> m_annuler, m_retablir;
  std::string m_avantGeste, m_enregistre;
  bool m_geste = false;
  Cible m_selection;
  QString m_chemin;
};
