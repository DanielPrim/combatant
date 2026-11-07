add_library(combatant STATIC
    items/ammotype.cpp
    items/armor.cpp
    items/attachment.cpp
    items/augment.cpp
    items/consumable.cpp
    items/grenade.cpp
    items/item.cpp
    items/medkit.cpp
    items/weapon.cpp
    shared/damagetype.cpp
    shared/effect.cpp
    shared/inventory.cpp
    base/timeManager.cpp
    base/resources.cpp
    base/warehouse.cpp
    base/base.cpp
    base/battle.cpp
    base/command.cpp
    base/campaign.cpp
    base/content.cpp
    catalog/names.cpp
    catalog/catalog.cpp
    app/baseScreen.cpp
    app/timeControl.cpp
    app/geoscapeFrame.cpp
    app/shell/notify.cpp
    app/shell/desktop.cpp
    app/shell/scene.cpp
    units/body/body.cpp
    units/body/limb.cpp
    units/progression/rank.cpp
    units/progression/skill.cpp
    units/progression/specialization.cpp
    units/soldier.cpp
    units/staff.cpp
    units/stats.cpp
    units/unit.cpp
)

target_include_directories(combatant PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/include
)

target_compile_definitions(combatant PUBLIC SPDLOG_HEADER_ONLY)

if(MSVC)
    target_compile_options(combatant PUBLIC /utf-8)
endif()

if(COMBATANT_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(combatant PUBLIC --coverage -O0 -g)
    target_link_options(combatant PUBLIC --coverage)
endif()
