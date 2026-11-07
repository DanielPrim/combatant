#pragma once

#include <string>

#include "base/base.hpp"

namespace project {

class BaseScreen {
public:
    explicit BaseScreen(const Base& base);

    std::string summary() const;

private:
    const Base& base_;
};

}
