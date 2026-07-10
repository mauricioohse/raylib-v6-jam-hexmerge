# todo


- [x] clean up template, remove initial animation, preserve basic systems
- [ ] upload to github
- [x] load pngs
- [ ] create gs (gamestate) struct
- [x] draw a bee
- [x] draw hex grid
- [x] move bee in hex grid with A/D inputs
- [x] leave a painting trail from bee
- [x] algorithm to paint all hexes when encircled by a painting trail from bee
- [x] enemies that kill on touch, basic AI
    - the player will start with three lives.  
    - make a spritesheet of size 16x16 with two sprites: a full red pixel art heart and a greyed out empty heart for when the live is lost
    - the game starts from the beginning when the 3 lives are lost
- [x] the hexagon tiles sprite should be brown-tinted by default and removes the tint when they are "painted" by the bee trail closed shape 
- [x]  flowers/seeds are the level objective
    - flower sprite is 6 sprites in a vertical spritesheed, 16x(16*6): the flower_1 is an unseeded sprout, flower_2 and 3 are the seed sprouting and becoming a full flower, and the flower_4 to 6 is idle animation of the flower
    - the sprite is a top down view of a dandelion, please generate the PNG in anyway you have to do it
    - the level is won if the bee is able to sprout all the level seeds and have them meet (the hexagons need to be in the same "painted" alive shape)
- [x] the game starts with a main menu having "START GAME" button, a small "speaker" symbol with a number indicating volume (default 5), two arrows on the left and right of the volume number to increase or decrease.
- [x] the game will have multiple levels with set seed (they are not random)
    - create a flexible hex struct archtecture so that I can load and unload levels (considering bee starting positioning and different number of HEX radius. see other points to have clear goal)
    - first level will be just a hex grid of radius 1 (so it will be 7 hexagons). no enemies, just the bee and a seed on the center. add a text box on the right saying "the plants are suffering and need bee pollen to sprout, encircle the seed to help!". you may adjust this text
    - second level make it radius 2,  2 seeds on oposite sides with a text mentioning that all seeds must sprout and must connect. 
    - third level again 2 seeds, but now we introduce the enemy "wasp". also rename all mentions of spiders to wasp actually. 
    - fourth level radius 3, three enemies and three seeds.
    - prepare for up to 9 levels but only implement the first 4 for the moment
    - on DEBUG builds, we should be able to change between levels by pressing number 1-9 on keyboard. this should also reset the lives to three 

    
- [x] implement the remaining levels
    - [x] level five introduces the edge moving wasps. same 3 radius but with 2 wasps on the edge and 2 wasps inside (one random, one chase). the black should move 1.5 times faster than the bee.
    - [x] change the bee trail to be yellow instead of blue
    - [x] level 6 "Tight squeeze": radius 2, 2 opposite seeds, 1 black + 1 purple + 1 green
    - [x] level 7 twin seeds intro: radius 2, one twin pair, no wasps, bubbly bond trail
    - [x] level 8: two twin pairs, radius 3, 1 black + 1 chaser + 1 random
    - [x] level 9 "Full swarm": radius 3, 4 diamond seeds, one of each wasp (red/purple/green/black)
    - [x] level 10: star seed tutorial (blue center jail hex, eat wasps)
    - [x] level 11: 2 stars + seeds + 5 wasps (2 black, chase, random, mixed)
    - [x] level 12: radius 4, 4-5 seeds, 5 wasps
    - [x] level 13: radius 4, 12 seeds (one twin pair) + 2 stars
- [x] star seeds: 4s power, rainbow bee, flee/slow wasps, jail 8s, star_power music
- [x] add run timer; ending shows time, saves sorted best times to file, lists top times center (works on emscripten/web too)
- [x] per-level timer; ending shows win/lose + per-level times; CSV history (file / localStorage); score by time only
- [x] twin seeds: pair tint + bubble trail; fill fails if exactly one twin enclosed
- [x] DEBUG: ,/< previous level, ./> next level
- [x] fire + water tiles (paint fire alone = death; fire+water together extinguishes)
- [x] levels 14–17 (fire/water tutorial + content)
- [x] 5 lives; +1 life on clean clear (max 5); checkpoints on teaching levels
- [x] HARDCORE mode (no lives/checkpoints, separate scoreboard)
- [ ] discuss/add map mechanics beyond wasp AI (blocked hexes, wrap bridges)

