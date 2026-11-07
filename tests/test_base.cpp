#include "fixtures.hpp"
#include "test_harness.hpp"

#include "base/base.hpp"
#include "base/resources.hpp"
#include "base/timeManager.hpp"

#include <map>

void test_base() {
    using project::ResourceType;

    project::TimeManager clock;
    CHECK(clock.minute() == 0);
    CHECK(clock.day() == 0);
    CHECK(clock.minute_of_day() == 0);
    CHECK_THROWS_MSG("negative", project::TimeManager(-1));
    CHECK_THROWS_MSG("positive", clock.advance(0));
    CHECK(clock.minute() == 0);
    CHECK_THROWS_MSG("positive", clock.advance(-3));
    CHECK(clock.minute() == 0);
    clock.advance(3);
    CHECK(clock.minute() == 3);
    CHECK(clock.day() == 0);
    CHECK(clock.minute_of_day() == 3);
    clock.advance(1438);
    CHECK(clock.minute() == 1441);
    CHECK(clock.day() == 1);
    CHECK(clock.minute_of_day() == 1);

    project::Resources wallet;
    CHECK(wallet.amount(ResourceType::Money) == 0);
    CHECK_THROWS_MSG("negative", wallet.add(ResourceType::Money, -1));
    CHECK(wallet.amount(ResourceType::Money) == 0);
    wallet.add(ResourceType::Money, 0);
    CHECK(wallet.amount(ResourceType::Money) == 0);
    wallet.add(ResourceType::Money, 10);
    wallet.add(ResourceType::Alloys, 3);
    CHECK(wallet.amount(ResourceType::Money) == 10);
    CHECK(wallet.amount(ResourceType::Alloys) == 3);
    CHECK_THROWS_MSG("negative", wallet.spend(ResourceType::Money, -1));
    CHECK(wallet.amount(ResourceType::Money) == 10);
    wallet.spend(ResourceType::Chips, 0);
    CHECK(wallet.amount(ResourceType::Chips) == 0);
    CHECK_THROWS_MSG("Not enough", wallet.spend(ResourceType::Money, 11));
    CHECK(wallet.amount(ResourceType::Money) == 10);
    wallet.spend(ResourceType::Money, 4);
    CHECK(wallet.amount(ResourceType::Money) == 6);
    wallet.spend(ResourceType::Money, 6);
    CHECK(wallet.amount(ResourceType::Money) == 0);

    const std::map<ResourceType, int> too_much{
        {ResourceType::Money, 1},
        {ResourceType::Alloys, 1},
    };
    CHECK_THROWS_MSG("Not enough", wallet.spend(too_much));
    CHECK(wallet.amount(ResourceType::Alloys) == 3);
    CHECK(wallet.amount(ResourceType::Money) == 0);
    wallet.add(ResourceType::Money, 5);
    const std::map<ResourceType, int> over_alloys{
        {ResourceType::Money, 2},
        {ResourceType::Alloys, 4},
    };
    CHECK_THROWS_MSG("Not enough", wallet.spend(over_alloys));
    CHECK(wallet.amount(ResourceType::Money) == 5);
    CHECK(wallet.amount(ResourceType::Alloys) == 3);
    const std::map<ResourceType, int> price{
        {ResourceType::Money, 2},
        {ResourceType::Alloys, 1},
    };
    wallet.spend(price);
    CHECK(wallet.amount(ResourceType::Money) == 3);
    CHECK(wallet.amount(ResourceType::Alloys) == 2);

    const std::map<ResourceType, int> bad_grant{{ResourceType::Components, -2}};
    CHECK_THROWS_MSG("negative", wallet.add(bad_grant));
    CHECK(wallet.amount(ResourceType::Components) == 0);
    const std::map<ResourceType, int> bad_bill{
        {ResourceType::Money, 1},
        {ResourceType::Alloys, -1},
    };
    CHECK_THROWS_MSG("negative", wallet.spend(bad_bill));
    CHECK(wallet.amount(ResourceType::Money) == 3);
    CHECK(wallet.amount(ResourceType::Alloys) == 2);
    wallet.add(std::map<ResourceType, int>{
        {ResourceType::Components, 1},
        {ResourceType::Chips, 4},
    });
    CHECK(wallet.amount(ResourceType::Components) == 1);
    CHECK(wallet.amount(ResourceType::Chips) == 4);
    CHECK_THROWS_MSG(
        "negative",
        project::Resources({{ResourceType::EnergyBlock, -5}}));
    project::Resources seeded({{ResourceType::EnergyBlock, 7}});
    CHECK(seeded.amount(ResourceType::EnergyBlock) == 7);

    project::Base base;
    CHECK(base.day() == 0);
    CHECK(base.minute() == 0);
    CHECK(base.amount(ResourceType::Money) == 0);
    CHECK(base.soldiers().empty());
    CHECK(base.staff().empty());
    base.add(ResourceType::Money, 8);
    base.add(std::map<ResourceType, int>{{ResourceType::Alloys, 2}});
    base.tick(3 * project::kMinutesPerDay);
    CHECK(base.day() == 3);
    CHECK(base.minute() == 3 * project::kMinutesPerDay);
    CHECK(base.amount(ResourceType::Money) == 8);
    CHECK(base.amount(ResourceType::Alloys) == 2);
    CHECK(base.tick(0).empty());
    CHECK(base.day() == 3);
    CHECK(base.amount(ResourceType::Money) == 8);
    CHECK_THROWS_MSG("positive", base.tick(-2));
    CHECK(base.day() == 3);
    CHECK_THROWS_MSG("Not enough", base.spend(std::map<ResourceType, int>{
        {ResourceType::Money, 1},
        {ResourceType::Alloys, 9},
    }));
    CHECK(base.amount(ResourceType::Money) == 8);
    CHECK(base.amount(ResourceType::Alloys) == 2);
    base.spend(ResourceType::Money, 3);
    base.spend(std::map<ResourceType, int>{{ResourceType::Alloys, 2}});
    CHECK(base.amount(ResourceType::Money) == 5);
    CHECK(base.amount(ResourceType::Alloys) == 0);

    base.add_soldier(make_soldier());
    base.add_staff(make_staff());
    base.add_staff(make_staff("medic", project::StaffRole::Engineer, 1, 0, {}, "bob"));
    CHECK(base.soldiers().size() == 1);
    CHECK(base.staff().size() == 2);
    CHECK(base.find_soldier("soldier-1") != nullptr);
    CHECK(base.find_soldier("soldier-1")->name() == "Sam");
    CHECK(base.find_soldier("staff-1") == nullptr);
    CHECK(base.find_staff("staff-1") != nullptr);
    CHECK(base.find_staff("staff-1")->days_served() == 0);
    CHECK(base.find_staff("bob")->role() == project::StaffRole::Engineer);
    CHECK(base.find_staff("soldier-1") == nullptr);
    CHECK(base.find_soldier("missing") == nullptr);
    CHECK(base.find_staff("missing") == nullptr);
    const project::Base& listed = base;
    CHECK(listed.find_soldier("soldier-1")->kills() == 3);
    CHECK(listed.find_staff("staff-1")->rank_level() == 1);
    CHECK(listed.find_soldier("missing") == nullptr);
    CHECK(listed.find_staff("missing") == nullptr);

    CHECK_THROWS_MSG("empty", base.add_soldier(make_soldier(
        "rookie", 1, 10, 1, {}, empty_inventory(), true, "")));
    CHECK_THROWS_MSG("empty", base.add_staff(make_staff(
        "medic", project::StaffRole::Medic, 1, 0, {}, "")));
    CHECK(base.soldiers().size() == 1);
    CHECK(base.staff().size() == 2);
    CHECK_THROWS_MSG("already", base.add_soldier(make_soldier(
        "rookie", 1, 10, 1, {}, empty_inventory(), true, "soldier-1")));
    CHECK_THROWS_MSG("already", base.add_soldier(make_soldier(
        "rookie", 1, 10, 1, {}, empty_inventory(), true, "staff-1")));
    CHECK_THROWS_MSG("already", base.add_staff(make_staff(
        "medic", project::StaffRole::Medic, 1, 0, {}, "bob")));
    CHECK_THROWS_MSG("already", base.add_staff(make_staff(
        "medic", project::StaffRole::Medic, 1, 0, {}, "soldier-1")));
    CHECK(base.soldiers().size() == 1);
    CHECK(base.staff().size() == 2);

    const int experience = base.find_soldier("soldier-1")->experience();
    const int rank = base.find_soldier("soldier-1")->rank_level();
    const int kills = base.find_soldier("soldier-1")->kills();
    const int missions = base.find_soldier("soldier-1")->missions_completed();
    base.tick(10 * project::kMinutesPerDay);
    CHECK(base.day() == 13);
    CHECK(base.amount(ResourceType::Money) == 5);
    CHECK(base.find_staff("staff-1")->days_served() == 10);
    CHECK(base.find_staff("bob")->days_served() == 10);
    CHECK(base.find_staff("staff-1")->rank_level() == 1);
    CHECK(base.find_staff("staff-1")->skills_ids().empty());
    CHECK(base.find_soldier("soldier-1")->experience() == experience);
    CHECK(base.find_soldier("soldier-1")->rank_level() == rank);
    CHECK(base.find_soldier("soldier-1")->kills() == kills);
    CHECK(base.find_soldier("soldier-1")->missions_completed() == missions);

    project::Base nights;
    nights.add_staff(make_staff());
    nights.tick(100);
    CHECK(nights.day() == 0);
    CHECK(nights.minute_of_day() == 100);
    CHECK(nights.find_staff("staff-1")->days_served() == 0);
    nights.tick(1340);
    CHECK(nights.minute() == project::kMinutesPerDay);
    CHECK(nights.day() == 1);
    CHECK(nights.find_staff("staff-1")->days_served() == 1);
}
