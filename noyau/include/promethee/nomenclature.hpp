// Prométhée : nomenclature unique (électronique, pièces fabriquées, visserie). Licence GPL-3.0-only.
#pragma once

#include <string>
#include <vector>

#include "promethee/derive.hpp"
#include "promethee/modele.hpp"

namespace prom {

struct LigneNomenclature {
  std::string groupe, ref, designation, appro;
  int qte = 1;
};

std::vector<LigneNomenclature> nomenclature(const Projet& p, const Derive& d);
// CSV au séparateur point-virgule, en UTF-8 avec marque d'ordre : s'ouvre tel quel dans Excel en français.
std::string versCsv(const std::vector<LigneNomenclature>& lignes);

}  // namespace prom
