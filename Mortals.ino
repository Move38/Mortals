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
*  When a group of 3 is all dead and each Blink has only 2 adjacent neighbors 
*  (after 2 seconds of being this way, get into ready mode)
*  When in ready mode, if connected to another ready mode Blink, choose random team, until it doesn't match
*  if teams don't match, let the games begin.
*  Let the two adjacent Blinks on your team know what team we are and start a count down for the game to begin
*
*/

#define ATTACK_VALUE 5                // Amount of health you loose when attacked.
#define ATTACK_DURRATION_MS   100     // Time between when we see first new neighbor and when we stop attacking.
#define HEALTH_STEP_TIME_MS  1000     // Health decremented by 1 unit this often

#define INJURED_DURRATION_MS  100     // How long we stay injured after we are attacked. Prevents multiple hits on the same attack cycle.

#define INITIAL_HEALTH         60
#define MAX_HEALTH             90

#define MAX_TEAMS           4      

word health;

byte team = 0;
Color teamColor = makeColorHSB(60,255,255);

Timer healthTimer;  // Count down to next time we loose a unit of health

enum State {
  DEAD,
  ALIVE,
  ENGUARDE,   // I am ready to attack!
  ATTACKING,  // Short window when I have already come across my first victim and started attacking
  INJURED,
  READY,
  TEAM_A_START,
  TEAM_B_START,
};

byte mode = DEAD;

Timer modeTimeout;     // Started when we enter ATTACKING, when it expires we switch back to normal ALIVE.
// Started when we are injured to make sure we don't get injured multiple times on the same attack

void setup() {
}


void loop() {
  
  // Update our mode first

  if(buttonDoubleClicked()) {
    // reset game piece
    mode=ALIVE;
    health=INITIAL_HEALTH;
    healthTimer.set(HEALTH_STEP_TIME_MS);
  }

  if(buttonLongPressed()) {
    // change team
    team = (team + 1) % MAX_TEAMS;
    teamColor = makeColorHSB(60 + team * 50, 255, 255);
  }
  
  if (healthTimer.isExpired()) {
    
    if (health>0) {
      
      health--;
      healthTimer.set(HEALTH_STEP_TIME_MS);
      
    } else {
    
      mode = DEAD;

    }
    
  }
  
  if ( mode != DEAD ) {
    
    if(isAlone()) {
      
      mode = ENGUARDE;      // Being lonesome makes us ready to attack!
      
      } else {  // !isAlone()
      
      if (mode==ENGUARDE) {     // We were ornery, but saw someone so we begin our attack in earnest!
        
        mode=ATTACKING;
        modeTimeout.set( ATTACK_DURRATION_MS );
      }
      
    }
    
    
    if (mode==ATTACKING || mode == INJURED ) {
      
      if (modeTimeout.isExpired()) {
        mode=ALIVE;
      }
    }
  } // !DEAD
  
  FOREACH_FACE(f) {
    
    
    if(!isValueReceivedOnFaceExpired(f)) {
      
      byte neighborMode = getLastValueReceivedOnFace(f); 
      
      if ( mode == ATTACKING ) {
        
        // We take our flesh when we see that someone we attacked is actually injured
        
        if ( neighborMode == INJURED ) {
          
          // TODO: We should really keep a per-face attack timer to lock down the case where we attack the same tile twice in a since interaction.
          
          health = min( health + ATTACK_VALUE , MAX_HEALTH );
          
        }
        
      } else if ( mode == ALIVE ) {
        
        if ( neighborMode == ATTACKING ) {
          
          health = max( health - ATTACK_VALUE , 0 ) ;
          
          mode = INJURED;
          
          modeTimeout.set( INJURED_DURRATION_MS );
          
        }
        
      } else if (mode==INJURED) {
        
        if (modeTimeout.isExpired()) {
          
          mode = ALIVE;
          
        }        
      }
    }
  }
  
  
  // Update our display based on new state
  
  switch (mode) {
    
    case DEAD:
      /* 
       * animate like a ghost 
       * circle around as a dimly lit soul
       */
      setColor(OFF);
      FOREACH_FACE(f) {
        setFaceColor(f, dim( WHITE, 60 + 55 * sin_d( (60*f + millis()/8) % 360)));
      }
//      setColor( dim( WHITE , 127 + 126 * sin_d( (millis()/10) % 360) ) );`
      break;
    
    case ALIVE:
      /* 
       * animate to breath 
       * the less health we have, the shorter of breath
       * we become
       */
      setColor( dim( teamColor , map_m( (health * MAX_BRIGHTNESS ) / MAX_HEALTH, 0, MAX_HEALTH, 1, 255)  ) );
      break;
    
    case ENGUARDE:
      /* 
       * animate with a spin
       * wind up to deliver a punch
       * TODO: soften the spin with fades on either side...
       */
      setColor( OFF );
      setFaceColor( (millis()/100) % FACE_COUNT, teamColor );
      break;
    
    case ATTACKING:
      /* 
       * animate bright on the side of the attack
       * fall off like the flash of a camera bulb
       */
      setColor( OFF );
      setFaceColor( rand(FACE_COUNT), teamColor );
      break;
    
    case INJURED:
      /* 
       * animate to bleed
       * glow red on the side we were hit
       * fall off like the flash of a camera bulb
       */
      setColor( RED );
      break;
    
  }
  
  setValueSentOnAllFaces( mode );       // Tell everyone around how we are feeling
  
}


// Sin in degrees ( standard sin() takes radians )

float sin_d( uint16_t degrees ) {

    return sin( ( degrees / 360.0F ) * 2.0F * PI   );
}


/*
 *  Determine if we are in the READY Mode
 */

bool isBlinkInReadyConfiguration() {

  // first count neighbors, if we have more or less than 2, then we are not in the ready mode
  byte numNeighbors = 0;
  
  FOREACH_FACE(f) {
    if(!isValueReceivedOnFaceExpired(f)) {
      numNeighbors++;
    }
  }

  if(numNeighbors != 2) {
    return false;
  }
  
  // we have 2 neighbors, let's make sure they are dead or ready
  FOREACH_FACE(f) {
    if(!isValueReceivedOnFaceExpired(f)) {
    
      byte neighborMode = getLastValueReceivedOnFace(f); 

      if(neighborMode != DEAD && neighborMode != READY) {
        return false; 
      }
    }
  }

  // great we have 2 neighbors that are either dead or ready
  // let's check to see if they are adjacent to one another
  byte NO_FACE = 255;
  
  byte firstNeighborFace = NO_FACE;
  byte secondNeighborFace = NO_FACE;
  
  FOREACH_FACE(f) {
    if(!isValueReceivedOnFaceExpired(f)) {
      
      if(firstNeighborFace == NO_FACE) {
        firstNeighborFace = f;
      }else {
        secondNeighborFace = f;
      }
    }
  }

  // now that we have the two faces, are they adjacent?
  if( (firstNeighborFace == 0 && secondNeighborFace  == 1 ) ||
      (firstNeighborFace == 1 && secondNeighborFace  == 2 ) ||
      (firstNeighborFace == 2 && secondNeighborFace  == 3 ) ||
      (firstNeighborFace == 3 && secondNeighborFace  == 4 ) ||
      (firstNeighborFace == 4 && secondNeighborFace  == 5 ) ||
      (firstNeighborFace == 0 && secondNeighborFace  == 5 ) )
  {  
    return true;
  }

  return false;
}
