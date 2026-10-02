// Prométhée : le document ouvert. Licence GPL-3.0-only.
#include "Document.hpp"

#include <algorithm>

Document::Document(QObject* parent) : QObject(parent) {
  m_projet = prom::exemple();
  m_enregistre = instantane();
  recalculer();
}

int Document::nombreErreurs() const {
  return static_cast<int>(std::count_if(m_problemes.begin(), m_problemes.end(), [](const prom::Probleme& q) { return q.sev == prom::Severite::Erreur; }));
}

std::string Document::instantane() const { return prom::versJson(m_projet).dump(); }

void Document::restaurer(const std::string& s) { m_projet = prom::normaliser(prom::Json::parse(s)); }

void Document::recalculer() {
  m_derive = prom::deriver(m_projet);
  m_problemes = prom::verifier(m_projet, m_derive);
  validerSelection();
  emit change();
}

void Document::validerSelection() {
  if ((m_selection.type == Cible::Composant && !composantSelectionne()) || (m_selection.type == Cible::Trou && !trouSelectionne())) {
    m_selection = {};
    emit selectionChangee();
  }
}

void Document::modifier(const std::function<void(prom::Projet&)>& f) {
  const std::string avant = instantane();
  f(m_projet);
  if (instantane() == avant) return;
  m_annuler.push_back(avant);
  if (m_annuler.size() > 300) m_annuler.erase(m_annuler.begin());
  m_retablir.clear();
  recalculer();
  emit etatChange();
}

void Document::remplacer(const prom::Projet& p) {
  modifier([&p](prom::Projet& q) { q = p; });
}

void Document::debutGeste() {
  m_avantGeste = instantane();
  m_geste = true;
}

void Document::pendantGeste(const std::function<void(prom::Projet&)>& f) {
  f(m_projet);
  recalculer();
}

void Document::finGeste() {
  if (!m_geste) return;
  m_geste = false;
  if (instantane() != m_avantGeste) {
    m_annuler.push_back(m_avantGeste);
    m_retablir.clear();
    emit etatChange();
  }
  recalculer();
}

void Document::annuler() {
  if (m_annuler.empty()) return;
  m_retablir.push_back(instantane());
  restaurer(m_annuler.back());
  m_annuler.pop_back();
  recalculer();
  emit etatChange();
}

void Document::retablir() {
  if (m_retablir.empty()) return;
  m_annuler.push_back(instantane());
  restaurer(m_retablir.back());
  m_retablir.pop_back();
  recalculer();
  emit etatChange();
}

void Document::selectionner(const Cible& c) {
  if (c == m_selection) return;
  m_selection = c;
  emit selectionChangee();
}

const prom::Composant* Document::composantSelectionne() const {
  if (m_selection.type != Cible::Composant) return nullptr;
  for (const auto& k : m_projet.composants) if (k.id == m_selection.id) return &k;
  return nullptr;
}

const prom::Trou* Document::trouSelectionne() const {
  if (m_selection.type != Cible::Trou) return nullptr;
  for (const auto& t : m_projet.trous) if (t.id == m_selection.id) return &t;
  return nullptr;
}

bool Document::modifie() const { return instantane() != m_enregistre; }

void Document::ouvrir(const QString& chemin) {
  m_projet = prom::lireProjet(chemin.toStdString());
  m_chemin = chemin;
  m_enregistre = instantane();
  m_annuler.clear();
  m_retablir.clear();
  m_selection = {};
  emit selectionChangee();
  recalculer();
  emit etatChange();
}

void Document::enregistrer(const QString& chemin) {
  prom::ecrireProjet(m_projet, chemin.toStdString());
  m_chemin = chemin;
  m_enregistre = instantane();
  emit etatChange();
}

void Document::nouveau() {
  remplacer(prom::projetVide());
  m_chemin.clear();
  emit etatChange();
}

void Document::exemple() {
  remplacer(prom::exemple());
  m_chemin.clear();
  emit etatChange();
}
