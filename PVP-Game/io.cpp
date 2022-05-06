#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <string>
#include <locale.h>
#include <vector>

#include "io.h"
#include "character.h"
#include "poke327.h"
#include "pokemon.h"
#include "db_parse.h"

#include "pvp.h"

// Colors
#define COLOR_BOULDER      51
#define COLOR_TREE         52
#define COLOR_PATH         53
#define COLOR_MART         54
#define COLOR_CENTER       55
#define COLOR_GRASS        56
#define COLOR_TALLGRASS    57
#define COLOR_MOUNTAIN     58

// Color Pairs
#define BOULDER_PAIR       10
#define TREE_PAIR          11
#define PATH_PAIR          12
#define MART_PAIR          13
#define CENTER_PAIR        14
#define GRASS_PAIR         15
#define TALLGRASS_PAIR     16
#define MOUNTAIN_PAIR      17
#define MOUNTAIN_WALKER    18
#define GRASS_WALKER       19

int GLOBAL_RESET = -1;

void set_global_reset(int gr) {
  GLOBAL_RESET = gr;
}


// This is just horrible and I'm sorry it exists.
std::string item_type_str[] = {
  "Pokeball",
  "Potion",
  "Revive"
};

typedef struct {
  int x, y;
} coord_t;

coord_t newc(int x, int y)
{
  coord_t coord;
  coord.x = x;
  coord.y = y;
  return coord;
}

typedef struct io_message {
  /* Will print " --more-- " at end of line when another message follows. *
   * Leave 10 extra spaces for that.                                      */
  char msg[71];
  struct io_message *next;
} io_message_t;

static io_message_t *io_head, *io_tail;

void io_init_terminal(void)
{
  setlocale(LC_ALL, "");
  initscr();
  raw();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();
  
  // Std Pair
  init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
  init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);

  init_color(COLOR_WHITE, 255 / .255, 255 / .255, 255 / .255);
  init_pair(COLOR_WHITE, COLOR_BLACK, COLOR_WHITE);

  // Custom Pair
  init_color(COLOR_TREE, 93 / .255, 141 / .255, 57 / .255);  // 93,141,57
  init_color(COLOR_BOULDER, 140 / .255, 123 / .255, 94 / .255);  // 110,91,77
  init_color(COLOR_MOUNTAIN, 162 / .255, 142 / .255, 111 / .255); // 162,142,111
  init_color(COLOR_PATH, 229 / .255, 215 / .255, 155 / .255); // 229,215,155
  init_color(COLOR_MART, 33 / .255, 129 / .255, 255 / .255);
  init_color(COLOR_CENTER, 255 / .255, 33 / .255, 33 / .255);
  init_color(COLOR_GRASS, 134 / .255, 227 / .255, 142 / .255);  // 134, 227, 142
  init_color(COLOR_TALLGRASS, 81 / .255, 143 / .255, 104 / .255);  // 81,143,104

  int LEAVES = 81; // 170,209,108
  init_color(LEAVES, 170 / .255, 209 / .255, 108 / .255);

  int GRASS_HIGHLIGHT = 82; // 147, 236, 146
  init_color(GRASS_HIGHLIGHT, 147 / .255, 236 / .255, 146 / .255);

  init_pair(MOUNTAIN_PAIR, COLOR_BOULDER, COLOR_MOUNTAIN);
  init_pair(BOULDER_PAIR, COLOR_MOUNTAIN, COLOR_BOULDER);
  init_pair(TREE_PAIR, LEAVES, COLOR_TREE);
  init_pair(PATH_PAIR, COLOR_BLACK, COLOR_PATH);
  init_pair(MART_PAIR, COLOR_BLACK, COLOR_MART);
  init_pair(CENTER_PAIR, COLOR_BLACK, COLOR_CENTER);
  init_pair(GRASS_PAIR, GRASS_HIGHLIGHT, COLOR_GRASS);
  init_pair(TALLGRASS_PAIR, COLOR_TALLGRASS, COLOR_GRASS);

  init_pair(MOUNTAIN_WALKER, COLOR_BLACK, COLOR_MOUNTAIN);
  init_pair(GRASS_WALKER, COLOR_BLACK, COLOR_GRASS);
}

void io_reset_terminal(void)
{
  endwin();

  while (io_head) {
    io_tail = io_head;
    io_head = io_head->next;
    free(io_tail);
  }
  io_tail = NULL;
}

void io_queue_message(const char * format, ...)
{
  io_message_t *tmp;
  va_list ap;

  if (!(tmp = (io_message_t *) malloc(sizeof (*tmp)))) {
    perror("malloc");
    exit(1);
  }

  tmp->next = NULL;

  va_start(ap, format);

  vsnprintf(tmp->msg, sizeof (tmp->msg), format, ap);

  va_end(ap);

  if (!io_head) {
    io_head = io_tail = tmp;
  } else {
    io_tail->next = tmp;
    io_tail = tmp;
  }
}

static void io_print_message_queue(uint32_t y, uint32_t x)
{
  int curr = MOUNTAIN_WALKER;

  while (io_head) {
    io_tail = io_head;
    attron(COLOR_PAIR(curr));
    mvprintw(y, x, "%-80s", io_head->msg);
    attroff(COLOR_PAIR(curr));
    io_head = io_head->next;
    if (io_head) {
      attron(COLOR_PAIR(curr));
      mvprintw(y, x + 70, "%10s", " --more-- ");
      attroff(COLOR_PAIR(curr));
      refresh();
      getch();
    }
    free(io_tail);
  }
  io_tail = NULL;
}

/**************************************************************************
 * Compares trainer distances from the PC according to the rival distance *
 * map.  This gives the approximate distance that the PC must travel to   *
 * get to the trainer (doesn't account for crossing buildings).  This is  *
 * not the distance from the NPC to the PC unless the NPC is a rival.     *
 *                                                                        *
 * Not a bug.                                                             *
 **************************************************************************/
static int compare_trainer_distance(const void *v1, const void *v2)
{
  const Character *const *c1 = (const Character *const *) v1;
  const Character *const *c2 = (const Character *const *) v2;

  return (world.rival_dist[(*c1)->pos[dim_y]][(*c1)->pos[dim_x]] -
          world.rival_dist[(*c2)->pos[dim_y]][(*c2)->pos[dim_x]]);
}

static Character *io_nearest_visible_trainer()
{
  Character **c, *n;
  uint32_t x, y, count;

  c = (Character **) malloc(world.cur_map->num_trainers * sizeof (*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
          &world.pc) {
        c[count++] = world.cur_map->cmap[y][x];
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof (*c), compare_trainer_distance);

  n = c[0];

  free(c);

  return n;
}

void io_display_pokedex(std::string stats)
{
  /*
    Left: "⎢"
    Right: "⎥"
    Top: "⎺"
    Bottom: "⎽"
    Corner Top Left: ""
    Corner Bottom Left: ""
    Corner Top Right: ""
    Corner Bottom Right: ""
  */
  int dex_red = 155;
  int dex_light_red = 156;
  // int dex_grey = 157;
  // int dex_screen = 158;
  // int dex_green = 159;
  // int dex_blue = 160;

  int dlrc = 161;

  init_pair(dex_red, COLOR_BLACK, COLOR_RED);
    init_color(dlrc, 255 / .255, 59 / .255, 59 / .255);
  init_pair(dex_light_red, COLOR_BLACK, dlrc);
  int curr = dex_red;

  // 30w
  int /* dex_h = 20, */ dex_w = 30;

  attron(COLOR_PAIR(curr));
  // Left
  for(int i = 2; i < 21; i++) {
    for(int j = 6 + 2; j < 6 + 2 + dex_w + 4; j++) {
      mvprintw(i, j, " ");
      if(j == 6 + 2) mvprintw(i, j, "⎢");
      if(j == 6 + 5 + dex_w) mvprintw(i, j, "⎥");
    }
  }
  for(int i = 6 + 3; i < 6 + 2 + dex_w + 4 - 1; i++) {
    mvprintw(21, i, "⎽"); 
  }

  // Right
  for(int i = 6 + (6 + dex_w + 4); i < 6 + (1 + dex_w + 4 + dex_w); i++) {
    mvprintw(5, i, " ");
  }
  for(int i = 6; i < 21; i++) {
    mvprintw(i, 6 + (5 + dex_w) + dex_w, " ");
  }
  for(int i = 7 + (5 + dex_w); i < 6 + (5 + dex_w) + dex_w; i++) {
    mvprintw(21, i, " ");
  }
  mvprintw(4, 6 + (2 + dex_w + 4), " ");
  mvprintw(4, 6 + (3 + dex_w + 4), " ");
  mvprintw(4, 6 + (4 + dex_w + 4), " ");
  mvprintw(5, 6 + (5 + dex_w + 4), " ");
  // for(int i = (MAP_Y / 2) - (dex_h / 2) + 3; i < (MAP_Y / 2) + (dex_h / 2) + 1; i++) {
  //   for(int j = (MAP_X / 2) - (dex_w / 2); j < (MAP_X / 2) + (dex_w / 2) + 4; j++) {
  //     mvprintw(i, j, " ");
  //   }
  // }
  attroff(COLOR_PAIR(curr));
  curr = dex_light_red;
  attron(COLOR_PAIR(curr));
  // Left
  for(int i = 6; i < 21; i++) {
    for(int j = 6 + 3; j < 6 + 3 + dex_w; j++) {
      mvprintw(i, j, " ");
      if(i == 20) mvprintw(i, j, "⎽");
    }
  }
  for(int i = 6 + 3; i < 6 + 5 + dex_w; i++) {
    mvprintw(1, i, "⎺");
  }
  for(int i = 6 + 2; i < 6 + 6 + dex_w; i++) {
    mvprintw(2, i, " ");
    if(i == 6 + 2) mvprintw(2, i, "⎢");
    if(i == 6 + 5 + dex_w) mvprintw(2, i, "⎥");
  }
  for(int i = 3; i < 5; i++) {
    for(int j = 6 + 2; j < 6 + 3 + dex_w; j++) {
      mvprintw(i, j, " ");
      if(j == 6 + 2) mvprintw(i, j, "⎢");
    }
  }
  for(int i = 6 + dex_w + 2; i > 6 + dex_w - 2; i--) {
    mvprintw(5, i, " ");
  }

  // Right
  for(int i = 6; i < 21; i++) {
    for(int j = 6 + (6 + dex_w); j < 6 + (5 + dex_w) + dex_w; j++) {
      mvprintw(i, j, " ");
    }
  }
  mvprintw(5, 6 + (6 + dex_w), " ");
  mvprintw(5, 6 + (7 + dex_w), " ");
  mvprintw(5, 6 + (8 + dex_w), " ");

  // for(int i = (MAP_Y / 2) - (dex_h / 2) + 5; i < (MAP_Y / 2) + (dex_h / 2); i++) {
  //   for(int j = (MAP_X / 2); j < (MAP_X / 2) + (dex_w / 2) + 3; j++) {
  //     mvprintw(i, j, " ");
  //   }
  // }
  attroff(COLOR_PAIR(curr));
  curr = dex_red;
  attron(COLOR_PAIR(curr));
  mvprintw(3, 6 + (dex_w + 2), " ");
  mvprintw(3, 6 + (dex_w + 1), " ");
  mvprintw(3, 6 + (dex_w), " ");
  mvprintw(4, 6 + (dex_w - 1), " ");
  attroff(COLOR_PAIR(curr));

  // int y = 8;
  char * t_stat = (char *) stats.c_str();
  for(int y = 8; y < 20; y++)
  {
    for(int i = 0; i < dex_w - 4; i++) {
      if(i < strlen(t_stat)) {
        if(t_stat[i] != '\n') mvaddch(y, i+11, t_stat[i]);
        else {
          strncpy(t_stat, t_stat + i, strlen(t_stat));
          for(int j = i; j < dex_w - 4; j++) {
            mvprintw(y, j+11, " ");
          }
          i = 0;
          y++;
          mvprintw(y, 11, " ");
        }
      }
      else mvprintw(y, i+11, " ");
    }
    strncpy(t_stat, t_stat + (dex_w-4), strlen(t_stat));
  }
}

void io_display_msg(std::string msg)
{
  int curr = MOUNTAIN_WALKER;

  attron(COLOR_PAIR(curr));
  for(int i = 0; i < MAP_X; i++) {
    mvprintw(0, i, " ");
  }
  for(int i = 0; i < msg.size() && i < MAP_X; i++) {
    mvaddch(0, i, msg[i]);
  }
  attroff(COLOR_PAIR(curr));
  refresh();
}

void io_display_bottom()
{
  int curr = MOUNTAIN_WALKER;
  Character *c;

  attron(COLOR_PAIR(curr));
  for(int i = MAP_Y+1; i < MAP_Y+3; i++) {
    for(int j = 0; j < MAP_X; j++) {
      mvprintw(i, j, " ");
    }
  }
  // io_nearest_visible_trainer();
  // for(int i = 0; i < strlen(msg) && i < MAP_X; i++) {
  //   // if(msg[i] == '\n') break;
  //   mvaddch(0, i, msg[i]);
  // }

  mvprintw(23, 1, "PC position is (%2d,%2d) on map %d%cx%d%c.",
           world.pc.pos[dim_x],
           world.pc.pos[dim_y],
           abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_x] - (WORLD_SIZE / 2) >= 0 ? 'E' : 'W',
           abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_y] - (WORLD_SIZE / 2) <= 0 ? 'N' : 'S');
  mvprintw(22, 1, "%d known %s.", world.cur_map->num_trainers,
           world.cur_map->num_trainers > 1 ? "trainers" : "trainer");
  mvprintw(22, 30, "Nearest visible trainer: ");
  if ((c = io_nearest_visible_trainer())) {
    mvprintw(22, 55, "%c at %d %c by %d %c.",
             c->symbol,
             abs(c->pos[dim_y] - world.pc.pos[dim_y]),
             ((c->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ?
              'N' : 'S'),
             abs(c->pos[dim_x] - world.pc.pos[dim_x]),
             ((c->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ?
              'W' : 'E'));
  } else {
    mvprintw(22, 55, "NONE.");
    
  }

  attroff(COLOR_PAIR(curr));
}

// /*
void io_display()
{
  int x, y;

  char * symbol;
  int curr;

  // character_t *c;

  // clear();
  refresh();

  io_display_msg(" ");
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
        switch (world.cur_map->map[y][x]) {
          case ter_boulder:
            curr = BOULDER_PAIR;
            symbol = (char *) "▢";
            break;
          case ter_mountain:
            // char *symbols[] = {"◸", "◹", "◺", " "};
            // char * symbol = symbols[rand() % 4];
            curr = MOUNTAIN_PAIR;
            symbol = (char *) " ";
            break;
          case ter_tree:
          case ter_forest:
            curr = TREE_PAIR;
            symbol = (char *) "░";
            break;
          case ter_exit:
          case ter_path:
            curr = PATH_PAIR;
            symbol = (char *) " ";
            break;
          case ter_mart:
            curr = MART_PAIR;
            symbol = (char *) " ";
            break;
          case ter_center:
            curr = CENTER_PAIR;
            symbol = (char *) " ";
            break;
          case ter_grass:
            curr = TALLGRASS_PAIR;
            symbol = (char *) "⌘";
            break;
          case ter_clearing:
            curr = GRASS_PAIR;
            symbol = (char *) "∭";
            break;
          default:
            curr = MOUNTAIN_WALKER;
            symbol = (char *) "0";
            break;
        }
      // }
      if (world.cur_map->cmap[y][x]) {
        // mvaddch(y, x, world.cur_map->cmap[y][x]->symbol);
        symbol = &world.cur_map->cmap[y][x]->symbol;
        if (curr == MOUNTAIN_PAIR) curr = MOUNTAIN_WALKER;
        if (curr == GRASS_PAIR || curr == TALLGRASS_PAIR) curr = GRASS_WALKER;
      }
      attron(COLOR_PAIR(curr));
      mvprintw(y+1, x, (char *) symbol);
      attroff(COLOR_PAIR(BOULDER_PAIR));
    }
  }

  io_display_bottom();

  io_print_message_queue(0, 0);
}

// Credit: https://stackoverflow.com/questions/5294955/how-to-scale-down-a-range-of-numbers-with-a-known-min-and-max-value
int scaleBetween(int unscaledNum, int minAllowed, int maxAllowed, int min, int max) {
  return (maxAllowed - minAllowed) * (unscaledNum - min) / (max - min) + minAllowed;
}

std::string io_get_move(Pokemon *enemy)
{
  coord_t coords[6];
  std::vector<std::string> actions;
  int i;
  bool  fight = false,
        bag = false,
        sp = false,
        rev = false;

  for(i = 0; i < 6; i++) {
    if(i == 0) coords[i] = newc((MAP_X / 2) + 4, MAP_Y);
    else if(i == 1) coords[i] = newc(((MAP_X / 2) + (MAP_X / 4)), MAP_Y);
    else if(i == 2) coords[i] = newc((MAP_X / 2) + 4, MAP_Y+1);
    else if(i == 3) coords[i] = newc(((MAP_X / 2) + (MAP_X / 4)), MAP_Y+1);
    else if(i == 4) coords[i] = newc((MAP_X / 2) + 4, MAP_Y+2);
    else if(i == 5) coords[i] = newc(((MAP_X / 2) + (MAP_X / 4)), MAP_Y+2);
  }

  actions.push_back("FIGHT");
  actions.push_back("BAG");
  actions.push_back("RUN");
  actions.push_back("POKEMON");
  // Display Possible actions
  int curr = COLOR_WHITE;
  attron(COLOR_PAIR(curr));

  for(i = 0; i < actions.size(); i++) {
    mvprintw(coords[i].y, coords[i].x+1, actions[i].c_str());
  }

  i = 0;
  mvprintw(coords[i].y, coords[i].x, "▶");

  attroff(COLOR_PAIR(curr));

  // Handle Actions
  while(true) {
    // Place Cursor at current action
    // ▶
    switch(getch())
    {
      case 'b':
          fight = false;
          bag = false;
          sp = false;
          rev = false;
          actions.clear();
          actions.push_back("FIGHT");
          actions.push_back("BAG");
          actions.push_back("RUN");
          actions.push_back("POKEMON");
          i = 0;
        break;
      case KEY_LEFT:
        // return "Left";
        i--;
        break;
      case KEY_RIGHT:
        // return "Right";
        i++;
        break;
      case KEY_UP:
        // return "Up";
        i-=2;
        break;
      case KEY_DOWN:
        // return "Down";
        i+=2;
        break;
      case KEY_ENTER:
      case '\n':
        if(!strcmp(actions[i].c_str(), "FIGHT")) {
          actions.clear();
          for(int k = 0; k < 4; k++) {
            if(strcmp(world.pc.captured[world.pc.curr_pokemon]->get_move_str(k), "")) actions.push_back(world.pc.captured[world.pc.curr_pokemon]->get_move_str(k));
          }
          fight = true;
        }
        else if(fight) {
          int dmg;
          dmg = ( ( ( ( (((2 * world.pc.captured[world.pc.curr_pokemon]->get_lvl()) / 5) + 2) * moves[world.pc.captured[world.pc.curr_pokemon]->get_species_index()].power
                  * (world.pc.captured[world.pc.curr_pokemon]->get_atk() / world.pc.captured[world.pc.curr_pokemon]->get_def()) ) / 50) + 2)
                  * 1 * abs(rand() % 100 - 85) * 1 * 1 );
          // dmg = world.pc.captured[world.pc.curr_pokemon]->get_move(i).damage_class_id;
          if(enemy) enemy->damage(dmg);
          std::string ret_move;
          ret_move = world.pc.captured[world.pc.curr_pokemon]->get_species();
          ret_move += " used ";
          ret_move += world.pc.captured[world.pc.curr_pokemon]->get_move_str(i);
          if(!strcmp(world.pc.captured[world.pc.curr_pokemon]->get_move_str(i), "")) return "Try again...";
          return ret_move;
        }
        else if(!strcmp(actions[i].c_str(), "RUN")) {
          if(rand() % 2 == 0 || enemy == world.pc.captured[world.pc.curr_pokemon]) return "Successfully Got Away!";
          return "Couldn't Escape!";
        }
        else if(!strcmp(actions[i].c_str(), "BAG")) {
          if(world.pc.Bag.size() <= 0) return "Bag is Empty!  Try again.";
          actions.clear();
          int pokeballs = 0, potions = 0, revives = 0;
          std::string ret_s;
          for(int k = 0; k < world.pc.Bag.size(); k++) {
            if(world.pc.Bag[k]->itype == pokeball) pokeballs++;
            else if(world.pc.Bag[k]->itype == potion) potions++;
            else if(world.pc.Bag[k]->itype == revive) revives++;
          }
          for(int k = 0; k < revive+1; k++) { // Index by last type of item
            ret_s = "";
            if(k == pokeball && pokeballs > 0) ret_s = "Pokeball [" + std::to_string(pokeballs) + "]";
            if(k == potion && potions > 0) ret_s = "Potion [" + std::to_string(potions) + "]";
            if(k == revive && revives > 0) ret_s = "Revive [" + std::to_string(revives) + "]";
            if(ret_s != "") actions.push_back(ret_s);
          }
          bag = true;
        }
        else if(bag) {
          // If pokeball -> then captured or not.
          // If potion / revive, do negative damage to pokemon.
          std::string ret_val;
          Item *ci;
          for(int k = 0; k < world.pc.Bag.size(); k++) {
            if(world.pc.Bag[k]->itype == i) {
              ci = world.pc.Bag[k];
            }
          }
          if(rev) {
            if(world.pc.captured[i]->get_hp() <= 0) {
              world.pc.captured[i]->damage(-1 * (world.pc.captured[i]->get_max_hp() / 2));
              ret_val = "Successfully Revived.  Try again.";
              for(int k = 0; k < world.pc.Bag.size(); k++) {
                if(world.pc.Bag[k]->itype == revive) {
                  ci = world.pc.Bag[k];
                }
              }
            }
            else return "Try again with a different Pokemon.";
          }
          else if(i == pokeball) {
            ret_val = "Your pokedex is full!  Try again.";
            for(int k = 0; k < 6; k++) {
              if(!world.pc.captured[k]) {
                ret_val = "Successfully Captured!";
                break;
              }
            }
          }
          else if(i == revive) {
            actions.clear();
            for(int k = 0; k < 6; k++) {
              if(world.pc.captured[k]) actions.push_back(world.pc.captured[k]->get_species());
            }
            rev = true;
            break;
          }
          else if(i == potion) {
            if(!(world.pc.captured[world.pc.curr_pokemon]->get_hp() == world.pc.captured[world.pc.curr_pokemon]->get_max_hp())) {
              if(world.pc.captured[world.pc.curr_pokemon]->get_hp()+ci->ability <= world.pc.captured[world.pc.curr_pokemon]->get_max_hp()) {
                world.pc.captured[world.pc.curr_pokemon]->damage(-1 * (ci->ability));
              }
              else {
                world.pc.captured[world.pc.curr_pokemon]->damage(-1 * (world.pc.captured[world.pc.curr_pokemon]->get_max_hp() - world.pc.captured[world.pc.curr_pokemon]->get_hp()));
              }
              ret_val = "Restored HP.";
            }
            else return "HP cannot be restored.  Try again.";
          }
          world.pc.Bag.erase(std::find(world.pc.Bag.begin(), world.pc.Bag.end(), ci));
          return ret_val == "" ? "Try again..." : ret_val;
        }
        else if(!strcmp(actions[i].c_str(), "POKEMON")) {
          actions.clear();
          for(int k = 0; k < 6; k++) {
            if(world.pc.captured[k]) actions.push_back(world.pc.captured[k]->get_species());
          }
          sp = true;
        }
        else if(sp) {
          std::string ret_val;
          if(world.pc.captured[i]->get_hp() > 0) {
            world.pc.curr_pokemon = i;
            ret_val = "Go ";
            ret_val += world.pc.captured[i]->get_species();
            ret_val += "!";
            return ret_val;
          }
          ret_val = "";
          ret_val += world.pc.captured[i]->get_species();
          ret_val += " needs revive.  Try again.";
          return ret_val;
        }
        else {
          return "Try again...";
        }
        break;
      case 'Q':
        exit(1);
        break;
      default:
        break;
    }
    if(i == -2) i = 2;
    else if(i == -1) i = 1;
    else if(i < 0) i = actions.size() - 1;
    if(i >= actions.size()) i = 0;

    attron(COLOR_PAIR(curr));

    for(int j = MAP_Y; j < MAP_Y + 3; j++) {
      for(int k = (MAP_X / 2) + 2; k < MAP_X; k++) {
        mvprintw(j, k, " ");
      }
    }

    mvprintw(coords[i].y, coords[i].x, "▶");

    refresh();

    for(int j = 0; j < actions.size(); j++) {
      mvprintw(coords[j].y, coords[j].x+1, actions[j].c_str());
    }
    attroff(COLOR_PAIR(curr));

  }
}

void io_display_battle(std::string message, Pokemon *pc = NULL, Pokemon *enemy = NULL, PVP_C *pvp_enemy = NULL)
{
  int curr = COLOR_WHITE;
  int i, j;
  std::string name, level, health_bar, health;

  refresh();

  attron(COLOR_PAIR(curr));

  for(i = 0; i < MAP_Y + 3; i++) {
    for(j = 0; j < MAP_X; j++) {
      mvprintw(i, j, " ");
    }
  }

  if(pc) {
    int start_y = MAP_Y - 5;
    name = pc->get_species();
    level = "Lv" + std::to_string(pc->get_lvl());
    mvprintw(start_y, MAP_X - name.size() - 6 - level.size() - 1, name.c_str()); // Name
    mvprintw(start_y, MAP_X - 7 - level.size(), (!strcmp(pc->get_gender_string(), "female") ? "♀" : "♂")); // Gender
    mvprintw(start_y, MAP_X - level.size() - 1, level.c_str()); // Level

    int scaled_hp = scaleBetween(pc->get_hp(), 0, 10, 0, pc->get_max_hp());
    health_bar = "HP[";
    for(i = 0; i < scaled_hp; i++) {
      health_bar += "*";
    }
    for(j = i; j < 10; j++) {
      health_bar += "-";
    }
    health_bar += "]";
    mvprintw(start_y+1, MAP_X - health_bar.size() - 1, health_bar.c_str());

    health = "";
    health += std::to_string(pc->get_hp());
    health += "/";
    health += std::to_string(pc->get_max_hp());
    mvprintw(start_y+2, MAP_X - health.size() - 1, health.c_str());

    for(int i = 0; i < 6; i++) {
      if(world.pc.captured[i]) {
        if(world.pc.captured[i]->get_hp() > 0) mvprintw(MAP_Y-2, MAP_X-2-i, "◉");
        else mvprintw(MAP_Y-2, MAP_X-2-i, "◎");
      }
    }
  }
  if(enemy) {
    name = enemy->get_species();
    level = "Lv" + std::to_string(enemy->get_lvl());
    mvprintw(2, 1, name.c_str()); // Name
    mvprintw(2, name.size() + 1, (!strcmp(enemy->get_gender_string(), "female") ? "♀" : "♂"));
    mvprintw(2, name.size() + 7, level.c_str());

    int scaled_hp = scaleBetween(enemy->get_hp(), 0, 10, 0, enemy->get_max_hp());
    health_bar = "HP[";
    for(i = 0; i < scaled_hp; i++) {
      health_bar += "*";
    }
    for(j = i; j < 10; j++) {
      health_bar += "-";
    }
    health_bar += "]";
    mvprintw(3, (name.size() + 7 + level.size() - health_bar.size()), health_bar.c_str());

    health = "";
    health += std::to_string(enemy->get_hp());
    health += "/";
    health += std::to_string(enemy->get_max_hp());
    mvprintw(4, (name.size() + 7 + level.size() - health.size()), health.c_str());

    if(world.pc.curr_opponent) {
      for(int i = 0; i < 6; i++) {
        if(world.pc.curr_opponent->captured[i]) {
          if(world.pc.curr_opponent->captured[i]->get_hp() > 0) mvprintw(5, name.size() + 7 + level.size()-1-i, "◉");
          else mvprintw(5, name.size() + 7 + level.size()-1-i, "◎");
        }
      }
    }
  }
  if(pvp_enemy) {
    name = pvp_enemy->name;
    level = "Lv" + std::to_string(pvp_enemy->level);
    mvprintw(2, 1, name.c_str()); // Name
    mvprintw(2, name.size() + 1, (!strcmp(pvp_enemy->gender.c_str(), "female") ? "♀" : "♂"));
    mvprintw(2, name.size() + 7, level.c_str());

    int scaled_hp = scaleBetween(pvp_enemy->hp, 0, 10, 0, pvp_enemy->max_hp);
    health_bar = "HP[";
    for(i = 0; i < scaled_hp; i++) {
      health_bar += "*";
    }
    for(j = i; j < 10; j++) {
      health_bar += "-";
    }
    health_bar += "]";
    mvprintw(3, (name.size() + 7 + level.size() - health_bar.size()), health_bar.c_str());

    health = "";
    health += std::to_string(pvp_enemy->hp);
    health += "/";
    health += std::to_string(pvp_enemy->max_hp);
    mvprintw(4, (name.size() + 7 + level.size() - health.size()), health.c_str());
  }
  if(!pc && !enemy) {
    for(i = MAP_Y - 1; i < MAP_Y + 3; i++) {
      for(j = 0; j < MAP_X; j++) {
        mvprintw(i, j, " ");
      }
    }
  }

  i = 0;
  j = 0;

  for(j = 0; j < MAP_X; j++) {
    mvprintw(MAP_Y - 1, j, "-");
  }
  for(j = MAP_Y; j < MAP_Y + 3; j++) {
    mvprintw(j, (MAP_X / 2) + 1, "|");
  }
  bool skip = false;
  timeout(20);
  for(j = ((MAP_X / 4) - (message.size() / 2)); j < (MAP_X / 2) && i < message.size(); j++ && i++) {
    mvaddch(MAP_Y, j, message[i]);
    if(!skip) {
      if(getch() != ERR) { timeout(-1); skip = true; }
    }
  }
  timeout(GLOBAL_RESET);

  attroff(COLOR_PAIR(curr));

  getch();  // Pause until key
}

void io_display_pvp_battle(std::string message)
{
  io_display_battle(message, world.pc.captured[world.pc.curr_pokemon]);
}

void io_display_pvp_battle(PVP_C *pvp_enemy)
{
  io_display_battle(pvp_enemy->message, world.pc.captured[world.pc.curr_pokemon], NULL, pvp_enemy);
}

uint32_t io_teleport_pc(pair_t dest)
{
  /* Just for fun. And debugging.  Mostly debugging. */

  do {
    dest[dim_x] = rand_range(1, MAP_X - 2);
    dest[dim_y] = rand_range(1, MAP_Y - 2);
  } while (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]                  ||
           move_cost[char_pc][world.cur_map->map[dest[dim_y]]
                                                [dest[dim_x]]] == INT_MAX ||
           world.rival_dist[dest[dim_y]][dest[dim_x]] < 0);

  return 0;
}

static void io_scroll_trainer_list(char (*s)[40], uint32_t count)
{
  uint32_t offset;
  uint32_t i;

  offset = 0;

  while (1) {
    for (i = 0; i < 13; i++) {
      mvprintw(i + 6, 19, " %-40s ", s[i + offset]);
    }
    switch (getch()) {
    case KEY_UP:
      if (offset) {
        offset--;
      }
      break;
    case KEY_DOWN:
      if (offset < (count - 13)) {
        offset++;
      }
      break;
    case 27:
      return;
    }

  }
}

static void io_list_trainers_display(Npc **c,
                                     uint32_t count)
{
  // clear();
  uint32_t i;
  char (*s)[40]; /* pointer to array of 40 char */

  s = (char (*)[40]) malloc(count * sizeof (*s));

  mvprintw(3, 19, " %-40s ", "");
  /* Borrow the first element of our array for this string: */
  snprintf(s[0], 40, "You know of %d trainers:", count);
  mvprintw(4, 19, " %-40s ", s[0]);
  mvprintw(5, 19, " %-40s ", "");

  for (i = 0; i < count; i++) {
    snprintf(s[i], 40, "%16s %c: %2d %s by %2d %s",
             char_type_name[c[i]->ctype],
             c[i]->symbol,
             abs(c[i]->pos[dim_y] - world.pc.pos[dim_y]),
             ((c[i]->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ?
              "North" : "South"),
             abs(c[i]->pos[dim_x] - world.pc.pos[dim_x]),
             ((c[i]->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ?
              "West" : "East"));
    if (count <= 13) {
      /* Handle the non-scrolling case right here. *
       * Scrolling in another function.            */
      mvprintw(i + 6, 19, " %-40s ", s[i]);
    }
  }

  if (count <= 13) {
    mvprintw(count + 6, 19, " %-40s ", "");
    mvprintw(count + 7, 19, " %-40s ", "Hit escape to continue.");
    while (getch() != 27 /* escape */)
      ;
  } else {
    mvprintw(19, 19, " %-40s ", "");
    mvprintw(20, 19, " %-40s ",
             "Arrows to scroll, escape to continue.");
    io_scroll_trainer_list(s, count);
  }

  free(s);
}

static void io_list_trainers()
{
  Character **c;
  uint32_t x, y, count;

  c = (Character **) malloc(world.cur_map->num_trainers * sizeof (*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
          &world.pc) {
        c[count++] = world.cur_map->cmap[y][x];
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof (*c), compare_trainer_distance);

  /* Display it */
  io_list_trainers_display((Npc **)(c), count);
  free(c);

  /* And redraw the map */
  io_display();
}

/// Stack Overflow https://stackoverflow.com/questions/26920261/read-a-string-with-ncurses-in-c
std::string getstring()
{
    std::string input;

    // let the terminal do the line editing
    // nocbreak();
    echo();

    // this reads from buffer after <ENTER>, not "raw" 
    // so any backspacing etc. has already been taken care of
    int ch = getch();

    while ( ch != '\n' )
    {
        input.push_back( ch );
        ch = getch();
        if(ch == KEY_BACKSPACE) input.pop_back();
    }

    // restore your cbreak / echo settings here
    // cbreak();
    noecho();
    curs_set(0);

    return input;
}

void io_pokemart()
{
  io_display_msg("Welcome to the Pokemart.  Could I interest you in some Pokeballs?");
  // mvprintw(0, 0, "Welcome to the Pokemart.  Could I interest you in some Pokeballs?");
  refresh();
  getch();
}

void io_pokemon_center()
{
  // io_display_msg("Welcome to the Pokemon Center.  How can Nurse Joy assist you?");

  int ch;
  connect_t server_c, client_c;
  std::string my_enc, op_enc, msg;
  server_c = my_connection();
  my_enc = connect_enc(get_connect_str(server_c));

  msg = "Enter your ID (" + my_enc + ") or opponent: ";
  io_display_msg(msg);
  // echo();
  // refresh();
  // curs_set(1);
  // move(0, msg.size());
  // op_enc = getstring();
  // op_enc = check_in(op_enc);
  // curs_set(0);

  // mvprintw(0, 0, "Enter y [-200, 200]:          ");
  refresh();
  // mvscanw(0, msg.size(), (char *)"%d", ch);
  attron(COLOR_PAIR(MOUNTAIN_WALKER));
  while((ch = getch()) && ch != '\n') {
    if(ch == 127) {
      op_enc.pop_back();
    }
    else op_enc += ch;
    // move(0, msg.size() + i);
    for(int i = msg.size(); i < 79; i++) {
      mvprintw(0, i, " ");
    }
    mvprintw(0, msg.size(), op_enc.c_str());
  }
  refresh();
  attroff(COLOR_PAIR(MOUNTAIN_WALKER));
  // noecho();
  curs_set(0);
  op_enc = check_in(op_enc);

  if(op_enc == "") {
    io_pokemon_center();
    return;
  }
  io_display_msg("Waiting for opponent...");
  if(op_enc == my_enc) {  // Server
    // io_display_msg("You are the server");
    server_driver(server_c);
  } else {                // Client
    // io_display_msg(op_enc);
    client_driver(get_connection(op_enc));
  }
  // mvprintw(0, 0, "Welcome to the Pokemon Center.  How can Nurse Joy assist you?");
  refresh();
  getch();
}

void io_battle(Character *aggressor, Character *defender)
{
  Npc *npc;
  Pc *pc;
  Pokemon *p;

  // io_display();
  // io_display_msg("Aww, how'd you get so strong?  You and your pokemon must share a special bond!");
  if (!(npc = dynamic_cast<Npc *>(aggressor))) {
    npc = dynamic_cast<Npc *>(defender);
  }
  if (!(pc = dynamic_cast<Pc *>(aggressor))) {
    pc = dynamic_cast<Pc *>(defender);
  }

  // Disable Trainers
  // world.pc.curr_opponent = NULL;
  // npc->defeated = 1;
  // return;

  std::string cp = char_type_name[npc->ctype];
  cp += " wants to battle!";

  io_display_battle(cp);

  /// Battle Sequence

  // world.pc.curr_pokemon = 0;

  world.pc.curr_opponent = npc;

  std::string ret_val;

  // p = new Pokemon(10);
  
  // /*
  for(int i = 0; i < 6; i++) {
    if(npc->captured[i]) {
      p = npc->captured[i];
      
      cp = "They sent out a ";
      cp += p->get_species();
      cp += "!";
      io_display_battle(cp, NULL, p);

      while(world.pc.captured[world.pc.curr_pokemon]->get_hp() <= 0) {
        if(world.pc.curr_pokemon++ >= 6) world.pc.curr_pokemon = 0;
      }

      if(i == 0) {
        cp = "Go! ";
        cp += world.pc.captured[world.pc.curr_pokemon]->get_species();
        cp += "!";
        io_display_battle(cp, world.pc.captured[world.pc.curr_pokemon], p);
      }

      while(p->get_hp() > 0) {
        cp = "What will ";
        cp += world.pc.captured[world.pc.curr_pokemon]->get_species();
        cp += " do?";
        io_display_battle(cp, world.pc.captured[world.pc.curr_pokemon], p);
        ret_val = io_get_move(p);
        if(p->get_hp() <= 0) break;
        if(ret_val.find("Try again") != std::string::npos) {
          io_display_battle(ret_val, world.pc.captured[world.pc.curr_pokemon], p);
        } else {
          if(ret_val.find("Away") != std::string::npos) io_display_battle("You can't run from a trainer.", world.pc.captured[world.pc.curr_pokemon], p);
          else {
            if(ret_val.find("Captured") != std::string::npos) io_display_battle("You can't capture their pokemon.", world.pc.captured[world.pc.curr_pokemon], p);
            else {
              io_display_battle(ret_val, world.pc.captured[world.pc.curr_pokemon], p);
              move_db cmove = p->get_move(rand() % 2);
              world.pc.captured[world.pc.curr_pokemon]->damage(cmove.damage_class_id);
              if(p->get_hp() <= 0) break;
              cp = p->get_species();
              cp += " used ";
              cp += cmove.identifier;
              io_display_battle(cp, world.pc.captured[world.pc.curr_pokemon], p);
            }
          }
        }
      }
      if(p->get_hp() <= 0) {
        std::string enemy = "Enemy ";
        enemy += p->get_species();
        enemy += " fainted!";
        io_display_battle(enemy, world.pc.captured[world.pc.curr_pokemon], p);
        // TODO Gain XP
        // Temp level up
        world.pc.captured[world.pc.curr_pokemon]->lvl_up();
        io_display_battle("Your Pokemon leveled up!", world.pc.captured[world.pc.curr_pokemon]);
      }
    }
  }
  // */
  io_display_battle("You defeated the trainer!");
  world.pc.curr_opponent = NULL;
  npc->defeated = 1;

  /// End Battle Sequence
  
  if (npc->ctype == char_hiker || npc->ctype == char_rival) {
    npc->mtype = move_wander;
  }
}

uint32_t move_pc_dir(uint32_t input, pair_t dest)
{
  dest[dim_y] = world.pc.pos[dim_y];
  dest[dim_x] = world.pc.pos[dim_x];

  switch (input) {
  case 1:
  case 2:
  case 3:
    dest[dim_y]++;
    break;
  case 4:
  case 5:
  case 6:
    break;
  case 7:
  case 8:
  case 9:
    dest[dim_y]--;
    break;
  }
  switch (input) {
  case 1:
  case 4:
  case 7:
    dest[dim_x]--;
    break;
  case 2:
  case 5:
  case 8:
    break;
  case 3:
  case 6:
  case 9:
    dest[dim_x]++;
    break;
  case '>':
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_mart) {
      io_pokemart();
    }
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_center) {
      io_pokemon_center();
    }
    break;
  }
  // if(world.cur_map->map[dest[dim_y]][dest[dim_x]] == ter_grass) {
  //   if(rand() % 10 == 1) {  // 10% Chance of encountering wild pokemon
  //     std::string enc_msg;
  //     int i = rand() % 897;
  //     int lvl = (1 + (rand() % ((199 - world.cur_idx[dim_x]) + (199 - world.cur_idx[dim_y])) / 2 ));
  //     lvl = ((lvl > 100) ? 100 : lvl);
  //     int hp = pokemon[i].stats.hp;
  //     hp = (((hp * 2) * lvl) / 100) + lvl + 10;
  //     int attack = pokemon[i].stats.attack;
  //     attack = (((hp * 2) * lvl) / 100) + 5;
  //     enc_msg = "\nWild " + (std::string) pokemon[i].identifier +
  //               " lvl " + std::to_string(lvl);
  //     // io_display_msg(enc_msg);
  //     // io_queue_message(enc_msg.c_str());
  //     enc_msg += "\nStats:\n";
  //     enc_msg += std::to_string(hp) + "hp, " + std::to_string(attack) + " attack...";
  //     enc_msg += "\nMoves:\n";
  //     int mvs = 0;
  //     while(mvs < 2) {
  //       for(int j = 0; j < 528239; j++) // pokemon_moves.size()
  //       {
  //         if(pokemon_moves[j].version_group_id == 19 && pokemon_moves[j].pokemon_move_method_id == 1)
  //         {
  //           if(rand() % 1000 == 0) {
  //             enc_msg += (std::string) moves[pokemon_moves[j].move_id].identifier;
  //             if(++mvs >= 2) break;
  //             enc_msg += ", ";
  //           }
  //         }
  //       }
  //     }
  //     io_display_pokedex(enc_msg);
  //     // io_queue_message(enc_msg.c_str());
  //     return 1;
  //   }
  //   // io_display_msg("The grass is whistling...");
  //   return 0;
  // }

  if ((world.cur_map->map[dest[dim_y]][dest[dim_x]] == ter_exit) &&
      (input == 1 || input == 3 || input == 7 || input == 9)) {
    // Exiting diagonally leads to complicated entry into the new map
    // in order to avoid INT_MAX move costs in the destination.
    // Most easily solved by disallowing such entries here.
    return 1;
  }

  if (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) {
    if (dynamic_cast<Npc *>(world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) &&
        ((Npc *) world.cur_map->cmap[dest[dim_y]][dest[dim_x]])->defeated) {
      // Some kind of greeting here would be nice
      return 1;
    } else if (dynamic_cast<Npc *>
               (world.cur_map->cmap[dest[dim_y]][dest[dim_x]])) {
      io_battle(&world.pc, world.cur_map->cmap[dest[dim_y]][dest[dim_x]]);
      // Not actually moving, so set dest back to PC position
      dest[dim_x] = world.pc.pos[dim_x];
      dest[dim_y] = world.pc.pos[dim_y];
    }
  }
  
  if (move_cost[char_pc][world.cur_map->map[dest[dim_y]][dest[dim_x]]] ==
      INT_MAX) {
    return 1;
  }

  return 0;
}

void io_teleport_world(pair_t dest)
{
  int x, y;
  
  world.cur_map->cmap[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = NULL;

  mvprintw(0, 0, "Enter x [-200, 200]: ");
  refresh();
  echo();
  curs_set(1);
  mvscanw(0, 21, (char*) "%d", &x);
  mvprintw(0, 0, "Enter y [-200, 200]:          ");
  refresh();
  mvscanw(0, 21, (char *) "%d", &y);
  refresh();
  noecho();
  curs_set(0);

  if (x < -200) {
    x = -200;
  }
  if (x > 200) {
    x = 200;
  }
  if (y < -200) {
    y = -200;
  }
  if (y > 200) {
    y = 200;
  }
  
  x += 200;
  y += 200;

  world.cur_idx[dim_x] = x;
  world.cur_idx[dim_y] = y;

  new_map(1);
  io_teleport_pc(dest);
}

void io_encounter_pokemon()
{
  Pokemon *p;
  
  int md = (abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)) +
            abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)));
  int minl, maxl;
  
  if (md <= 200) {
    minl = 1;
    maxl = md / 2;
  } else {
    minl = (md - 200) / 2;
    maxl = 100;
  }
  if (minl < 1) {
    minl = 1;
  }
  if (minl > 100) {
    minl = 100;
  }
  if (maxl < 1) {
    maxl = 1;
  }
  if (maxl > 100) {
    maxl = 100;
  }

  p = new Pokemon(rand() % (maxl - minl + 1) + minl);

  //  std::cerr << *p << std::endl << std::endl;

  // char c_msg[1000];

  // sprintf(c_msg, "%s%s%s: HP:%d ATK:%d DEF:%d SPATK:%d SPDEF:%d SPEED:%d %s",
  //                  p->is_shiny() ? "*" : "", p->get_species(),
  //                  p->is_shiny() ? "*" : "", p->get_hp(), p->get_atk(),
  //                  p->get_def(), p->get_spatk(), p->get_spdef(),
  //                  p->get_speed(), p->get_gender_string());

  // std::string s_msg = c_msg;
  // // printf("%s\n", c_msg);
  // // std::cout << s_msg;
  // io_display_pokedex(s_msg);
  // // io_display_pokedex("");
  // refresh();
  // getch();

  std::string cp = "A wild ";
  cp += p->get_species();
  cp += " appeared!";

  io_display_battle(cp, NULL, p);

  // world.pc.curr_pokemon = 0;

  while(world.pc.captured[world.pc.curr_pokemon]->get_hp() <= 0) {
    world.pc.curr_pokemon++;
  }

  cp = "Go! ";
  cp += world.pc.captured[world.pc.curr_pokemon]->get_species();
  cp += "!";
  io_display_battle(cp, world.pc.captured[world.pc.curr_pokemon], p);

  std::string ret_val;

  while(p->get_hp() > 0) {
    cp = "What will ";
    cp += world.pc.captured[world.pc.curr_pokemon]->get_species();
    cp += " do?";
    io_display_battle(cp, world.pc.captured[world.pc.curr_pokemon], p);
    ret_val = io_get_move(p);
    if(p->get_hp() <= 0) break;
    if(ret_val.find("Try again") != std::string::npos) {
      io_display_battle(ret_val, world.pc.captured[world.pc.curr_pokemon], p);
    } else {
      if(ret_val.find("Away") != std::string::npos) break;
      if(ret_val.find("Captured") != std::string::npos) break;
      io_display_battle(ret_val, world.pc.captured[world.pc.curr_pokemon], p);
      move_db cmove = p->get_move(rand() % 2);
      world.pc.captured[world.pc.curr_pokemon]->damage(cmove.damage_class_id);
      if(p->get_hp() <= 0) break;
      cp = p->get_species();
      cp += " used ";
      cp += cmove.identifier;
      io_display_battle(cp, world.pc.captured[world.pc.curr_pokemon], p);
    }
  }
  if(p->get_hp() <= 0) {
    std::string enemy = "Enemy ";
    enemy += p->get_species();
    enemy += " fainted!";
    io_display_battle(enemy, world.pc.captured[world.pc.curr_pokemon], p);
    io_display_battle("You won!  Congrats!", world.pc.captured[world.pc.curr_pokemon]);
    // TODO Gain XP
    // Temp level up
    world.pc.captured[world.pc.curr_pokemon]->lvl_up();
    io_display_battle("Your Pokemon leveled up!", world.pc.captured[world.pc.curr_pokemon]);
  }
  else if(ret_val.find("Captured") != std::string::npos) {
    for(int k = 0; k < 6; k++) {
      if(!world.pc.captured[k]) {
        world.pc.captured[k] = p;
        break;
      }
    }
    io_display_battle(ret_val, world.pc.captured[world.pc.curr_pokemon]);
    return;
    // TODO Gain XP
  }
  else if(ret_val.find("Away") != std::string::npos) {
    io_display_battle(ret_val, NULL, p);
  }

  // TODO
  /// When the game starts, allow the selection of 1 of 3 randomly generated pokemon.
  /// When the game starts, generate all of the trainers with 1 pokemon and 60% chance of n+1 (max 6)

  // io_queue_message("%s%s%s: HP:%d ATK:%d DEF:%d SPATK:%d SPDEF:%d SPEED:%d %s",
  //                  p->is_shiny() ? "*" : "", p->get_species(),
  //                  p->is_shiny() ? "*" : "", p->get_hp(), p->get_atk(),
  //                  p->get_def(), p->get_spatk(), p->get_spdef(),
  //                  p->get_speed(), p->get_gender_string());
  // io_queue_message("%s's moves: %s %s", p->get_species(),
  //                  p->get_move(0), p->get_move(1));

  // Later on, don't delete if captured
  delete p;
}

void io_handle_input(pair_t dest)
{
  uint32_t turn_not_consumed;
  int key;
  bool dexs;

  do {
    switch (key = getch()) {
    case '7':
    case 'y':
    case KEY_HOME:
      turn_not_consumed = move_pc_dir(7, dest);
      break;
    case '8':
    case 'k':
    case KEY_UP:
      turn_not_consumed = move_pc_dir(8, dest);
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      turn_not_consumed = move_pc_dir(9, dest);
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      turn_not_consumed = move_pc_dir(6, dest);
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      turn_not_consumed = move_pc_dir(3, dest);
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      turn_not_consumed = move_pc_dir(2, dest);
      break;
    case '1':
    case 'b':
    case KEY_END:
      turn_not_consumed = move_pc_dir(1, dest);
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      turn_not_consumed = move_pc_dir(4, dest);
      break;
    case '5':
    case ' ':
    case '.':
    case KEY_B2:
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    case '>':
      turn_not_consumed = move_pc_dir('>', dest);
      break;
    case 'Q':
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      world.quit = 1;
      turn_not_consumed = 0;
      break;
      break;
    case 't':
      /* Teleport the PC to a random place in the map.              */
      io_teleport_pc(dest);
      turn_not_consumed = 0;
      break;
    case 'T':
      /* Teleport the PC to any map in the world.                   */
      io_teleport_world(dest);
      turn_not_consumed = 0;
      break;
    case 'm':
      io_list_trainers();
      turn_not_consumed = 1;
      break;
    case 'p':
      if(!dexs) { io_display_pokedex(""); dexs = true; }
      else { io_display(); dexs = false; }
      turn_not_consumed = 1;
      break;
    case 'q':
      /* Demonstrate use of the message queue.  You can use this for *
       * printf()-style debugging (though gdb is probably a better   *
       * option.  Not that it matters, but using this command will   *
       * waste a turn.  Set turn_not_consumed to 1 and you should be *
       * able to figure out why I did it that way.                   */
      io_queue_message("This is the first message.");
      io_queue_message("Since there are multiple messages, "
                       "you will see \"more\" prompts.");
      io_queue_message("You can use any key to advance through messages.");
      io_queue_message("Normal gameplay will not resume until the queue "
                       "is empty.");
      io_queue_message("Long lines will be truncated, not wrapped.");
      io_queue_message("io_queue_message() is variadic and handles "
                       "all printf() conversion specifiers.");
      io_queue_message("Did you see %s?", "what I did there");
      io_queue_message("When the last message is displayed, there will "
                       "be no \"more\" prompt.");
      io_queue_message("Have fun!  And happy printing!");
      io_queue_message("Oh!  And use 'Q' to quit!");

      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    default:
      /* Also not in the spec.  It's not always easy to figure out what *
       * key code corresponds with a given keystroke.  Print out any    *
       * unhandled key here.  Not only does it give a visual error      *
       * indicator, but it also gives an integer value that can be used *
       * for that key in this (or other) switch statements.  Printed in *
       * octal, with the leading zero, because ncurses.h lists codes in *
       * octal, thus allowing us to do reverse lookups.  If a key has a *
       * name defined in the header, you can use the name here, else    *
       * you can directly use the octal value.                          */
      std::string km = "Unbound key: ";
      km += key;
      io_display_msg(km);
      // mvprintw(0, 0, "Unbound key: %#o ", key);
      turn_not_consumed = 1;
    }
    refresh();
  } while (turn_not_consumed);
}
