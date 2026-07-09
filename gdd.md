# Beehold GDD

![First Concept Drawing](./@gdd-images/first-concept-drawing.png)

Beehold will be a color-painting domination pacman-like in a hex grid. Game developed for raylib jam!

## Gameplay

Play as a bee sharing your pollen with flowers in drawn hexagons. you need to move and encircle groups of hexagon so that the seeds are properly fertilzed and can grow into big strong flowers.

At the same time, you are running from wasps that want to eat you! this is an arcade high score game.

the map is a hexagon grid. Bee and enemies move alongside the edges. Bee movement is using A/D or left/righ arrow. The bee stays stopped at a junction if no input is given, but the input can be buffered before reaching the vertex junction. bee can never move backward. while moving, the bee leaves a pollen trail alongside the edge. whenever the pollen trail makes a closed shape, all hexagons inside the closed shape are alive, making the grass grow and all seeds to sprout. to go to the next level, the bee needs to sprout all seeds and connect all seeds hexagons in one shape.

the enemies have different sets of movements. red wasps move randomly (full speed). purple wasps always move closer to the bee, but are slower. green wasps move randomly 50% of the time and towards the bee the other 50% (also slower). black wasps always move on the board edge and are 1.5x faster than the bee.



## game jam theme

hex + merge

the way I apply these themes are the hex grid that limits the movement of the bee + the merging of coloring hexes.


