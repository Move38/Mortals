# Mortals
###### by Jonathan Bobrow, Nick Bentley, Justin Ha

_INSERT GIF OF GAMEPLAY_
  
| Number of Blinks | Number of Players | Duration of Gameplay | Recommended Ages |
|------------------|:-----------------:|:--------------------:|-----------------:|
| 6+               | 2                 |  < 6 minutes         | 12 & Up           |

## Objective
Be the last living Mortal

## Setup
All six Mortals start in the initial configuration, separated by camps, team A on one side and team B on the other. The moment the game starts, all six Mortals begin their slow descent (losing health/time). Double-click resets all connected Blinks. Single-click starts the game (this propogates to all connected Blinks)

## Gameplay
Each Mortal has 60 seconds to live. The only way to live longer is to steal life from other Mortals.

Players take turns moving a single Mortal from an outside location to a new location. Breaking the board to move multiple Mortals is also a legal move, as long as the board remains in 2 parts.

## Technical Description

60 seconds of lifetime
When moved alone, enter attack mode and Gain 5 seconds of life from each new neighbor
When attacked, Lose 5 seconds of life in the direction of the attacker
If 60 seconds expire, become a ghost
If only one Mortal remains, celebrate the winner.
