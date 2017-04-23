# CodersOfTheCaribbean
[Codingame](https://www.codingame.com/leaderboards/challenge/coders-of-the-caribbean/global) Coders of the Caribbean AI in C++ for an 1 week contest.

## Strategies
- Rule based : custom pathfinding (works not great) to get the nearest Barrel and 
fire on the closest enemy when ability is ready.
- Full random : get random actions and simulate game with depth 5, and play the actions with the highest score

## Rule based strategy
The rule based strategy was used to get out the Bronze league. It's no more used since its efficiency
is a lot lower than the full random strategy.

## Full Random strategy

### Evaluation function : 

#### Make the crews survive !
```
Ally ships health
```

#### Take theses water of life
```
Rum barrel distance
```

#### Flash MacQueen
```
Ally ships speed
```

#### "Suicide" behaviour
```
Max ally ship health
```

#### Make sure the dropped rum will get by an ally ship 
```
Distance to ally ship
```

#### Avoid potential dropped mine
```
Enemy position behind stern
```

### Replace actions
The actions computed by the full random strategy are only movement related.
`WAIT` actions and `SLOWER` with speed already equals to `0` are replaced with `FIRE` and `MINE`. 
The latter are computed by a heuristic function which simulate enemy moves with also a full random strategy with his
own `eval` function.

### Improvements
- Compute the `FIRE` and `MINE` actions in the random strategy
- Use something more like genetic algorithm to converge to the optimal solution.
- Find the right hyperparameters (a lot empirical testing)
- Better game engine
- Use all C++ tricks to improve overall performance


## Misc
Since Codingame does not use the gcc optimization, the STL is very slow. So, I used simple `List` implementation. 
It has `begin` and `end` iterators, so it is compatible with the C++11 for loop. 

The code can be improved using static allocation instead of using `new` and pointer. The performance is not great.

The code related to the engine is inspired a lot by the Codiname Java Referee.



