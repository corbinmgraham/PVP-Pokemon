#ifndef IO_H
# define IO_H

#include <string>

class Pokemon;
class PVP_C;
class Character;
typedef int16_t pair_t[2];

void io_init_terminal(void);
void io_reset_terminal(void);
void io_display(void);
void io_handle_input(pair_t dest);
void io_queue_message(const char *format, ...);
void io_battle(Character *aggressor, Character *defender);
std::string io_get_move(Pokemon *enemy);
void io_display_pvp_battle(std::string message);
void io_display_pvp_battle(PVP_C *pvp_enemy);
void io_encounter_pokemon(void);

void io_display_msg(std::string msg);

void set_global_reset(int gr);
int get_global_reset();

#endif