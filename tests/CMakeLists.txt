add_executable(Combatant_tests
    test_main.cpp
    test_shared.cpp
    test_items.cpp
    test_units.cpp
    test_base.cpp
    test_geoscape.cpp
    test_campaign.cpp
    test_expand.cpp
    test_ui.cpp
)

target_link_libraries(Combatant_tests PRIVATE combatant)
target_compile_definitions(Combatant_tests PRIVATE "COMBATANT_DATA_DIR=\"${CMAKE_SOURCE_DIR}/data\"")
target_include_directories(Combatant_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(COMBATANT_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Combatant_tests PRIVATE --coverage -O0 -g)
    target_link_options(Combatant_tests PRIVATE --coverage)
endif()
