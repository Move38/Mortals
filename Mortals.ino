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

#include "blinklib.h"
#include "blinkstate.h"

#define MAX_HEALTH 90
#define ATTACK_VALUE 5

byte team = 0;
byte health = 60;
bool isAttacking = false;

enum state {
  DEAD,
  ALIVE,
  ATTACK,
  INJURED
};

byte mode = ALIVE;

Timer healthTimer;
Timer attackTimer;  // time since first attack to still allow attacking (since it is impossible for all sides to meet at the same time)

#include "blinklib.h"
#include "blinkstate.h"

void setup() {
  healthTimer.set(1000);
}

void draw() {

  if(buttonDoubleClicked()){
    // reset game piece
  }

  if(isAlone()) {
    isAttacking = true;
    setState(ATTACK);
  }

  FOREACH_FACE(f) {
    
    if(neighborStateChanged(f)) {
      
      if(getNeighborState(f) == ATTACK) {
        
        // reduce health by attackStrength
        reduceHealth();
         
        // show injured on that face
        setFaceState(f,INJURED);
        
        // animate injury towards the face
        setFaceColor(f, RED);
      }

      // if I am attack && neighbor == injured
      if( isAttacking ) {

        attackTimer.set(100);
        
        if( getNeighborState(f) == INJURED ) {
          
          // increase health by attackStrength
          increaseHealth();
          
          // change state to alive
          setFaceState(f, ALIVE);
        }
      }
    }
  }

  if(attackTimer.isComplete()) {
    isAttacking = false;
    setState(ALIVE);
    attackTimer.stop();
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

void reduceHealth() {
  if(health > ATTACK_VALUE) {
    health -= ATTACK_VALUE;
  }
  else {
    health = 0;
  }
}

void increaseHealth() {
  if(health < MAX_HEALTH) {
    health += ATTACK_VALUE;
  }
  else {
    health = MAX_HEALTH;
  }  
}

