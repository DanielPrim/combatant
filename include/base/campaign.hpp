#pragma once

#include <string>

#include "base/base.hpp"
#include "catalog/catalog.hpp"

namespace project {

std::string save_campaign(const Base& base);
Base load_campaign(const std::string& text, const Catalog& catalog);

}
