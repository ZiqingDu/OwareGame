# OwareGame

Oware is a family of board games played all around the world, known also as ‘sowing’ games or ‘count-and-capture’ games. Oware games are deterministic, have perfect information and the available moves are always less or equal to the number of pits in a row. In general board games are well suited to simulate a rational behaviour (AI), where board configuration is the state of the game and set of legal actions which represents the legal moves.

# AI implementation
## Negamax
Negamax is a variant of Minimax, which can also minimize the maximum loss (or maximize the minimum gain) but in a simple format. The algorithm starts from an initial state and builds up a tree of the game states, then it computes the best decision doing a recursive calculation which assumes that the first player tries to maximize his rewards. 

## Alpha-beta pruning

## Iterative deepening search with clock
Iterative deepening is a strategy where instead of immedi- ately searching to one large depth and returning the value, the search is repeated, each time to a higher depth, and the Minimax value is returned, then the best move is the one calculated by the highest search. So at each of the top level searches there is a call to Minmax, where t is result of the previous search (fed again in the Minmax). And also the consumed time is used as the constraint for it.

## Transposition tables and Zobrist Keys
The board games like Mancala, Oware are good in using the transposition tables. Because they allow the transposi- tion of moves, which means that different moves can result in the same positions. Which subsequently means that that in alpha beta that positions can show up several times in a search tree. So it will be very useful to store that information for future and then to access it. For that a data structure is very useful, where the quick lookups can be made, which saves time, as the bestMove value is already found. And if the value is not yet in the transposition table, the bestMove for that computer and stores in the table. 
To make a lookup in the transposition table, the Zobrist key for the given game position is calculated and hashed, that gives an index in a transposition table. 


# Game Rule
## modified rules:  
There are 12 holes, 6 per player. The first player is on the top. Holes are number from 1 to 6 for the first player and from 7 to 12 for the second. Hole 7 follows clockwise hole 6. Hole 1 follows clockwise hole 12.
At the beginning there are 3 seeds of each color per hole. Each player has one special seed, which is red and black.
 
-- Object
The game starts with 3+3 seeds in each hole and one special seed put in another hole (the first player decide its positiion first, then the second player). The object of the game is to capture more seeds than one's opponent. Since there is an even number of seeds, it is possible for the game to end in a draw, where each player has captured 38.

## additional information here:
Capturing :
The last seed determines the color of the captured seeds
e.g.: if the last seed is red then only red sees can be captured
 
If the last seed is special then red and black seeds can be captured
ATTENTION: this is done by pit
The color of the seed that has been captured in the pit I determines the color of the seed that can be captured in the previous pit (i-1)
 
Example: 2B4R; 3R; 2R2B; 2R2B; 2R2B1special
For the first  pit (2R2B1S). The color is R&B: all the seeds are taken
For the pit 2R2B, the color is R&B : all the seeds are taken
For the pit 3R, the color is R&B: all the seeds are taken
For the pit 2B4R, the color is R. No seed is taken
Another example : 2R2B; 2R4B1S
2R4B1S: The color is R&B (because of the special), 2R and the special are captured
2R2B The color is R, 2R are taken

