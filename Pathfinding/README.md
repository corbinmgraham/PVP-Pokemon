# Pathfinding (1.04)

Author: Corbin Graham

README Format: Markdown (MD)

### About

A pokemon game where the player roams a __world__ of __maps__ where in each
map there is a different terrain and with the terrain a unique set of trainers.
If the player (PC) is within the __vision__ of a trainer, they will be approached
and a battle will ensue.  Some trainers don't have a large vision radius so must
be approached by the PC to begin battling.

**Exit the program with ^C**

#### 1.04
====================================================================

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

#### 1.03
====================================================================

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

* PLAYER '@', -2 (PC) / undef

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

### Features

* Auto-seed generator
* Ultimate path-finding

### Contents

* Makefile
* README
* CHANGELOG
* gen_map.c

### Usage

Making
```bash
// Make and run
> make

// Make without running
> make gen_map

// Run without making
> make run

// Clean
> make clean
```

Running
```bash
> ./gen_map.run
```
