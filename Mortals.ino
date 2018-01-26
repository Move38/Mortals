/*
 * Mortals for Blinks
 * 
 *  Setup: 2 player game. Tiles die slowly over time, all at the same rate. 
 *  Moving a single tile to a new place allows it to suck life from 
 *  surrounding tiles, friend or foe.
 *  
 *  Blinks start with 60 seconds of life
 *  
 *  When a tile is moved alone, it sucks 5 seconds of health from each one of
 *  its newly found neighbors. A non-moved neighbor with a new neighbor looses 5 seconds
 *  and animates in the direction it lost the life (i.e. where the neighbor showed up)
 *  
 *   Tiles resets health to full when triple press occurs
 *
 *    States for game piece.
 *    Alive/Dead
 *    Team
 *    Attack, Injured
 *
 *  Technical Details:
 *  A long press on the tile changes the color of the tile for prototyping (switch state 1 or 2)
 *
 * 
 */

byte team = 0;
byte health = 60;

enum state {
  DEAD,
  ALIVE,
  ATTACK,
  INJURED
}

byte mode = ALIVE;

Timer healthTimer;

#include "blinklib.h"
#include "blinkstate.h"

void setup() {
  healthTimer.set(1000);
}

void draw() {

  if(buttonDoubleClicked()){
    // reset game piece
  }

  if(isAlone) {
    // show attack mode on all sides
    setState(ATTACK);
  }

  FOREACH_FACE(f) {
    if(neighborDidChange(f)) {
      // if neighbor is attack
      // reduce health by 5
      // show injured on that face
      // animate injury towards the face

      // if I am attack && neighbor == injured
      // increase health by 5
      // change state to alive
    }
  }

  // reduce health with time 1 unit every 1000ms
  if( health > 0 ) {
    // ALIVE
    if(healthTimer.isComplete()){
      health--;    
      healthTimer.set(1000);
    }
  }
  else {
    healthTimer.stop();
    // DEAD
  }
}
