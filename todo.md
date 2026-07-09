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
- [ ]
- [ ] the game will have multiple levels, create the archtecture for that. you can do 
- [ ]