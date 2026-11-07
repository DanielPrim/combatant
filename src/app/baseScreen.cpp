#include "app/baseScreen.hpp"

#include <sstream>

namespace project {

BaseScreen::BaseScreen(const Base& base) : base_(base) {}

std::string BaseScreen::summary() const {
    std::ostringstream text;
    text << "day " << base_.day() << '\n';
    text << "minute " << base_.minute() << '\n';
    text << "money " << base_.amount(ResourceType::Money) << '\n';
    text << "soldiers " << base_.soldiers().size() << '\n';
    text << "staff " << base_.staff().size() << '\n';
    text << "barracks " << base_.barracks_capacity() << '\n';
    text << "warehouse " << base_.warehouse().weapon_count() << '\n';
    int stacks = 0;
    for (const auto& entry : base_.warehouse().items()) {
        stacks += entry.second;
    }
    text << "stacks " << stacks << '\n';
    text << "sites " << base_.sites().size() << '\n';
    return text.str();
}

}
