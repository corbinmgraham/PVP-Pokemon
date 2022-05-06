# PVP Pokemon

**Author: Corbin Graham**

**Course: COM S 327**

## About

A Pokemon game where the player roams a __world__ of __maps__ where in each
map there is a different terrain and with the terrain a unique set of trainers.
Each biome has Pokemon unique to it and 
If the player (PC) is within the __vision__ of a trainer, they will be approached
and a battle will ensue.  Some trainers don't have a large vision radius so must
be approached by the PC to begin battling.

Level up your pokemon, then engage with other players in PVP.

## Features

* Auto-seed generator
* // TODO //

## Interface

**PC (@) is your character and the one that you move.**

#### Trainers

- **h:** Hiker
- **r:** Rival
- **w:** Wanderer
- **s:** Stander

#### Map

- **COMMAND:** Tall Grass - this is where you encounter wild Pokemon.
- **HASH:** Trees - This is an unmovable object
- **o:** Boulder - This is an unmovable object
// TODO //

#### Commands

- **p:** Pokedex (unfinished)
- **t:** List all known trainers
- **Arrow Keys:** Move character in direction of key
// TODO //

## Known Issues

**PVP**

- Once all your Pokemon have died or you defeated all of the opposing trainer's
    Pokemon, the game will crash becasue it is trying to access NULL objects.
- Game will share your moves with opponent instead of you

**Game**

- Game will crash when all your Pokemon die
- No way to save game progress
- Damage modifier is not correctly computed.

**Pokemon DB**

- Add to Public Version // TODO //

# Version History

All of the versions and what they implemented in the game

## 1.09

**PVP Battling**
Easy-to-Read connection code, PVP battling.  Enough said.

Connection Code
Connection code is your IP and an open port converted to a
easy-to-copy / easy-to-read string that can be shared with a friend.
Enter your own and you become the host of a PVP battle.  Enter another
player's and you become the client.  The simple logic protects your IP,
port, and makes it easier to connect.
These are seeded to prevent reusage on the same machine.  Random seeding
can fail however, so best practice is to remove seed values.

PVP Battling
Create a secure connection with an opponent and face your pokemon against
theirs.  This is the first implementation and there is a lot of logic
behind it so there are quite a few bugs.

__Testing on your own:__

- For the server-side: Copy and paste your own connection code exactly.
                     You should see text saying "Waiting for opponent..."
- Client-Side: Paste in the server code but remove any trailing 'x' or 'p'
                characters so you do not become the server (check that the
                pasted version is not the same as the displayed.)
             It is now your move.
             
It is a turn-based gameplay style.  You will see a message on your active screen
stating that it is waiting for the opponent's turn before doing anything more.
Switch to the now active screen and make your move.

If you lose, the game will exit because your pokemon have all died.

## 1.08

**Battle Sequence**
Finally implemented the stand-alone most important aspect of the entire
game.
The battle sequence resembles that of the original pokemon games...
minus sprites.  The same text, formatting, and battle-moves are there.

Select whichever item you want in the battle screen using the arrow keys
on the keyboard, and return to the previous menu using 'b'.

Defeat trainers, collect pokemon, and play the game as intended.

## 1.07

A full transition to C++!

Still with a slightly C-style but more object orientation (specifically
for DB parsing).

**Implemented the Pokedex** (still needs some graphical improvements) to
disply wild Pokemon.  This can be used in the future as an interface for
displaying captured Pokemon, owned Pokemon, and bag items.

**Implemented Wild Pokemon!**

Wild Pokemon will display their stats and name based on their location.
They will progressively get stronger as you make your way outwards from
the center map.

**Disabled Battle Mode**

Battle mode (and my approach algorithm) are disabled until they can be
fully implemented, this is merely a time-saving device since we do not
yet have an entire battle sequence, it makes debugging and writing much
easier until it is needed.

Pokemon will not be able to approach, only be landed on and attacked /
attack the PC.

## 1.06

Made the transition to C++...

Re-implemented all core features in a C-style C++.

Parsed DB files for data and stored it into a tagged vector where each
value could be accessed by its according tag and id #.

## 1.05

Implemented a graphical design nostalgic of the original pokemon games,
implemented major key commands.

Used ncurses to handle graphical and interactive functions.

The core of the game operates the same and is for the most part untouched
but the graphical features are all new.

Brought together most of 1.02 with 1.04.

## 1.04

Using my algorithm for distance finding, the trainers now have the ability
to approach the PC.  This means that the PC has no way to run from the
trainers when spotted.  They will see a message warning them that they have
been spotted then the game will freeze until they are approached.

This uses an interpreted map from 1.03 and a new function that uses that
interpreted map to find the PC from a vision radius (different for different
trainer types) similar to game turns.

If the PC is spotted, the trainer will approach and begin a battle summoning
the battle sequence.  (Not yet implemented).

**Pseudocode**
Stack - 1 move per turn, sleep for 3ms every turn
        Characters
    
    - Generate Map
    - Insert Characters
    - Give characters a vision radius
        ** Each character has their own movement function that is stored
        in their struct.
        ** Vision radius only goes for the R (radius) turns.  This will
        prevent it from overflowing.
            * if the PC falls within the vision radius at their turn,
            find the shortest path and approach the PC -> attack()
                - For this exercise, freeze on attack then remove them
                and place them back at their starting position
            * else move to the next spot in their sequence based on their
            movement style
        
    - Give characters a movement radius
        * move within their radius of movement defined from a movement
        size.  N,E,S,W: Hikers move only E,W for 10 moves.  Move in a line
        10 spaces or until they hit an impass, then set their movement
        direction to reverse and if they hit an impass, shorten that movement
        for that one series.

    gen_map()
    gen_characters() - give vision radius, movement(), insert into map
    while(true) -> for(all characters) -> if(move()->finds PC) -> attack() -> reset
                                       -> sleep()

    Set the map imap from the PC as CENTER
    Check each enemy radius and if the PC is within, ENEMY = CENTER

## 1.03

**Required Terminal Size: 240x23** (Height can be changed but
                                    output is dependant on
                                    width for readable format)

A program which generates terrain from random seed values and
can assess distances from a single point at any location within
map.

* Generates a 'map' with dimensions 80x21
* Generates Tall Grass regions (minimum 2)
* Generates Path with 4 exits at borders (N,E,S,W)
* Generates Poke-Centers along the x-axis in semi random areas
* Custom algorithm for path finding

Character Values (defined)
* EMPTY '.'   // Placeholder (Clearing)
* BOULDER '%' // Unmovable boulder (/border)
* GRASS ';'   // Grass Character
* TREE '^'    // Tree Character (For enhancement)
* Path /#/    // Path Character

* PLAYER '@', -2 (PC) // undef

#### Output

1. A Map (Randomly generated terrain, this is what the player
    sees)
2. Binary interpretation (0 or -1 for passable or impassable)
3. Visual Paths ('*' represents a path space,
                    this is essentially a visually pleasing
                    binary interpretation.)
4. All possible paths with their actual distance from PC (player).
    * The format is 00 with 99 being the assumed maximum moves
        from the PC to that spot.
    * This is not assuming game time but only the actual distance
        from PC to any space.  Because the 'mob/trainer' characters
        have access to the location of the PC, we can do a reverse
        path find (from PC to mob) then follow the best option
        available.  This ranks all terrain evenly because my map
        does not have varying traversable terrain types.
            * This can be added in the 'NEXT' incrementation to
                account for varying terrain traversal time but in
                the actual pokemon games, your character is stopped
                when spotted so it is spotting I wished to account
                for instead of game time.
            * Giving the 'mob/trainer' a vision radius of varying
                size (to account for different skills) would be
                more realistic because they will not be chasing
                while I run.

#### TMI

I decided that it would be best to find the distance every point
is from the character value.  A* and other searching algorithms
work best for finding a single path but since we needed to find
every possible path, I went with a diamond pattern to expand
until all of the map had been reached.  In the case that there
are unreachable spaces, the program will keep trying for up to
999 cases which I found to be the most manageable for X*Y.

The algorithm:
      *
    * @ *
      *
    I imagined the map to be connected nodes where each knew of
    its direct neighbors only.  We will loop through the entire
    map for all available spaces each time incrementing the
    distance from our original point.
    E.X. The first point will be 1 (directly around PC).  I then
    look at each node directly touching @.  The second point will
    be 2 (directly around 1).  I then look at each point around 1
    and if it is available (has not been changed or is not impassable),
    set it to the current point which is now 2.  And continue until
    there are no longer any reachable points.  This means all points
    are now set or the program times out because a small few can't be reached.
    __Something to add:__ I wanted to compare the current iteration of
    the map with the previous to check for no-change (this means the
    map had no more accessible points) but was unable to figure it out
    in a decent amount of time so I opted for a timeout funciton
    instead.

The algorithm can be applied to any map!  My map in 1.01 is very
'flat', so there isn't much interference or untouched space
resulting in very few '   '(gaps) in the output.

## 1.02

**Required Terminal Size: 84x27** (Custom Display)

The program randomly generates terrains and connects them for free
movement between each unique terrain.

* Generates a 'map' with dimensions 80x21
* Generates Tall Grass regions (minimum 2)
* Generates Path with 4 exits at borders (N,E,S,W)
* Generates Poke-Centers along the x-axis in semi random areas

##### Commands

* N/n - Move North
* E/e - Move East
* S/s - Move South
* W/w - Move West
* F (X) (Y)
* q - Quit

## 1.01

A program which generates terrain from seed values.
* Generates a 'map' with dimensions 80x21
* Generates Tall Grass regions (minimum 2)
* Generates Path with 4 exits at borders (N,E,S,W)
* Generates Poke-Centers along the x-axis in semi random areas

Character Values (defined)
* EMPTY '.'   // Placeholder (Clearing)
* BOULDER '%' // Unmovable boulder (/border)
* GRASS ';'   // Grass Character
* TREE '^'    // Tree Character (For enhancement)

Character Values (not defined)
* Path /#/

#### More About (TMI)

I initially started by implementing seed generation and
then outlining the path of the seeds but ran into many issues
especially the issue of an x reset...

I found out that if the x value surpassed all the x values which
existed that it would simply return to 0.  This made it so my
seeds were beginning to spill into the territory of others and
ignored clear borders.

The solution I found was to instead ignore the borders of seeds
and constrain them by the borders of the map.

**The most important issue** which I had and the most important
factor to the final outcome of the design was angled paths.
After I initally implemented the path as ONLY '#', I did not like
the design and had to border it.  This created an issue in the
design of angled paths and I did not enjoy the design of the angled path,
especially when all of the grass terrain was square shaped.  I could
have randomized this (I tried several patterns; diamond, angled, circle),
but if the pattern was to be random as the path, then they often didn't
line up to be the same shape and it looked __off__.

After many implementations, changes, and restarts, the final
design brought together all of the designs which I favored:
straight lines, clean borders, gentle terrain;
while it does not have the complex design of angles and random shapes,
there is something beautiful in its simplicity and reminds me more of
the Pokemon games I grew up with.

## Contents

* Makefile
* README
* Game Files

## Usage

**Making**
==========
```bash
// Make (without running)
> make

// Clean
> make clean
```

**Running**
===========
```bash
> ./poke327 <seed>
```
