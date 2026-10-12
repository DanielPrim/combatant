add_executable(Combatant
)

target_include_directories(Combatant PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Прилинковать библиотеки
target_link_libraries(Combatant PRIVATE
)