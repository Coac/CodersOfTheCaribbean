# CodersOfTheCaribbean
[Codingame](https://www.codingame.com/leaderboards/challenge/coders-of-the-caribbean/global) Coders of the Caribbean AI in C++ for an 1 week contest.

## Strategies
- Rule based : custom pathfinding (works not great) to get the nearest Barrel and 
fire on the closest enemy when ability is ready.
- Full random : get random actions and simulate game with depth 5, and play the actions with the highest score

## Misc
Since Codingame does not use the gcc optimization, the STL is very slow. So, I used simple `List` implementation. 
It has `begin` and `end` iterators, so it is compatible with the C++11 for loop. 

The code can be improved using static allocation instead of using `new` and pointer. The performance is not great.

The code related to the engine is inspired a lot by the Codiname Java Referee.



