/*
  Mortals for Blinks

   Setup: 2 player game. Tiles die slowly over time, all at the same rate.
   Moving a single tile to a new place allows it to suck life from
   surrounding tiles, friend or foe.

   Blinks start with 60 seconds of life

   When a tile is moved alone, it sucks 5 seconds of health from each one of
   its newly found neighbors. A non-moved neighbor with a new neighbor looses 5 seconds
   and animates in the direction it lost the life (i.e. where the neighbor showed up)

    Tiles resets health to full when triple press occurs

     States for game piece.
     Alive/Dead
     Team
     Attack, Injured

   Technical Details:
   A long press on the tile changes the color of the tile for prototyping (switch state 1 or 2)


*/

#define ATTACK_VALUE                5   // Amount of health you loose when attacked.
#define ATTACK_DURRATION_MS       100   // Time between when we see first new neighbor and when we stop attacking.
#define HEALTH_STEP_TIME_MS      1000   // Health decremented by 1 unit this often

#define INJURED_DURRATION_MS      750   // How long we stay injured after we are attacked. Prevents multiple hits on the same attack cycle.
#define INJURY_DECAY_VALUE         10   // How much the injury decays each interval
#define INJURY_DECAY_INTERVAL_MS   30   // How often we decay the the injury

#define INITIAL_HEALTH             60
#define MAX_HEALTH                 90

#define MAX_TEAMS                   4

#define COINTOSS_FLIP_DURATION    100   // how long we commit to our cointoss for
#define GAME_START_DURATION       300   // wait for all teammates to get the signal to start

byte team = 0;

int health;

Timer healthTimer;  // Count down to next time we loose a unit of health
Timer injuryDecayTimer; // Timing to fade away the injury
Timer cointossTimer;
Timer startGameTimer;

byte injuryBrightness = 0;
byte injuredFace;

byte deathBrightness = 0;

enum State {
  DEAD,
  ALIVE,
  ENGUARDE,   // I am ready to attack!
  ATTACKING,  // Short window when I have already come across my first victim and started attacking
  INJURED,
  READY,
  NEIGHBORSREADY,
  TEAM_A_COINTOSS,
  TEAM_B_COINTOSS,
  TEAM_A_START,
  TEAM_B_START
};

byte mode = DEAD;

Timer modeTimeout;     // Started when we enter ATTACKING, when it expires we switch back to normal ALIVE.
// Started when we are injured to make sure we don't get injured multiple times on the same attack

void setup() {
}


void loop() {

  // Update our mode first

  //  if (buttonDoubleClicked()) {
  //    // reset game piece
  //    mode = ALIVE;
  //    health = INITIAL_HEALTH;
  //    healthTimer.set(HEALTH_STEP_TIME_MS);
  //  }
  //
  //  if (buttonLongPressed()) {
  //    // change team
  //    team = (team + 1) % MAX_TEAMS;
  //  }

  if ( mode != TEAM_A_COINTOSS && mode != TEAM_B_COINTOSS && mode != TEAM_A_START && mode != TEAM_B_START && mode != NEIGHBORSREADY ) {

    if (healthTimer.isExpired()) {

      if (health > 0) {

        health--;
        healthTimer.set(HEALTH_STEP_TIME_MS);

      } else {

        mode = DEAD;

      }
    }

  }

  if ( mode == TEAM_A_START || mode == TEAM_B_START ) {

    if ( startGameTimer.isExpired() ) {

      // let's start the game
      if ( mode == TEAM_A_START ) {
        team = 0;
      }

      if ( mode == TEAM_B_START ) {
        team = 2;
      }

      // reset game piece
      mode = ALIVE;
      health = INITIAL_HEALTH;
      healthTimer.set(HEALTH_STEP_TIME_MS);
    }
  }

  bool hasPickedTeam = false;

  if ( mode == TEAM_A_COINTOSS || mode == TEAM_B_COINTOSS ) {

    // if the team across the way chooses differently,
    // wonderful, let's start this game with our team
    if ( isBlinkReadyToStartCountdown( mode ) ) {

      if ( mode == TEAM_A_COINTOSS ) {

        mode = TEAM_A_START;
        hasPickedTeam = true;
        startGameTimer.set(GAME_START_DURATION);

      } else if ( mode == TEAM_B_COINTOSS ) {

        mode = TEAM_B_START;
        hasPickedTeam = true;
        startGameTimer.set(GAME_START_DURATION);
      }
    }
  }

  if ( !hasPickedTeam ) {
    // it is possible that we about to start the game.
    if (isBlinkTeamCaptain()) {
      // let's get this party started
      // pick a team color (coin toss)
      if (cointossTimer.isExpired() ) {
        if (rand(10) < 5) {
          mode = TEAM_A_COINTOSS;
        }
        else {
          mode = TEAM_B_COINTOSS;
        }
        cointossTimer.set(COINTOSS_FLIP_DURATION);
      }
    }
  }

  // check for starting neighbors
  if ( mode == NEIGHBORSREADY && isNotTeamCaptain() ) {
    FOREACH_FACE( f ) {
      if (!isValueReceivedOnFaceExpired(f)) {

        byte neighborMode = getLastValueReceivedOnFace(f);

        if (neighborMode == TEAM_A_START) {
          mode = TEAM_A_START;
          if ( startGameTimer.isExpired() ) {
            startGameTimer.set(GAME_START_DURATION);
          }
        }
        else if (neighborMode == TEAM_B_START) {
          mode = TEAM_B_START;
          if ( startGameTimer.isExpired() ) {
            startGameTimer.set(GAME_START_DURATION);
          }
        }

      }
    }
  }

  if ( mode == DEAD ) {

    if (isBlinkInReadyConfiguration()) {

      mode = READY; // Signal that we are ready

      // if I am team captain (i.e. I have 3 neighbors)
      // pick a team color at random and see if we can stick with it
      // once confirmed share the team with our side, and change our state to the team start
    }
  }
  else {  // !DEAD

    if (isAlone()) {

      mode = ENGUARDE;      // Being lonesome makes us ready to attack!

    } else {  // !isAlone()

      if (mode == ENGUARDE) {   // We were ornery, but saw someone so we begin our attack in earnest!

        mode = ATTACKING;
        modeTimeout.set( ATTACK_DURRATION_MS );
      }

    }


    if (mode == ATTACKING || mode == INJURED ) {

      if (modeTimeout.isExpired()) {
        mode = ALIVE;
      }
    }
  } // !DEAD

  if ( mode == READY ) {
    // are neighbors ready...
    bool neighborsReady = true;

    FOREACH_FACE(f) {

      if (!isValueReceivedOnFaceExpired(f)) {

        byte neighborMode = getLastValueReceivedOnFace(f);

        if (neighborMode != READY && neighborMode != NEIGHBORSREADY) {
          neighborsReady = false;
        }
      }
    }

    if ( neighborsReady ) {
      mode = NEIGHBORSREADY;
    }
  }

  if ( mode == NEIGHBORSREADY) {
    // select team ... check above we do this below the ready check...
  }


  FOREACH_FACE(f) {


    if (!isValueReceivedOnFaceExpired(f)) {

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

          injuredFace = f;  // set the face we are injured on

          injuryBrightness = 255; // Start very injured

          modeTimeout.set( INJURED_DURRATION_MS );

        }

      } else if (mode == INJURED) {

        if (modeTimeout.isExpired()) {

          mode = ALIVE;

        }
        else {

          if ( injuryDecayTimer.isExpired() ) {

            injuryDecayTimer.set( INJURY_DECAY_INTERVAL_MS );

            injuryBrightness -= INJURY_DECAY_VALUE;
          }
        }
      }
    }
  }


  // Update our display based on new state

  switch (mode) {

    case DEAD:
      displayGhost();
      break;

    case ALIVE:
      displayAlive();
      break;

    case ENGUARDE:
      displayEnguarde();
      break;

    case ATTACKING:
      displayAttack();
      break;

    case INJURED:
      displayInjured( injuredFace );
      break;

    case READY:
      setColor(OFF);
      FOREACH_FACE(f) {
        setFaceColor(f, dim( WHITE, 60 + 55 * sin_d( (60 * f + millis() / 8) % 360)));
      }

      //      setColor( YELLOW );
      break;

    case NEIGHBORSREADY:
      setColor(OFF);
      FOREACH_FACE(f) {
        setFaceColor(f, dim( GREEN, 120 + 55 * sin_d( (60 * f + millis() / 8) % 360)));
      }

      //      setColor( GREEN );
      break;

    case TEAM_A_COINTOSS:
      setColor(OFF);
      FOREACH_FACE(f) {
        setFaceColor(f, dim( GREEN, 120 + 55 * sin_d( (60 * f + millis() / 8) % 360)));
      }
      break;

    case TEAM_B_COINTOSS:
      setColor(OFF);
      FOREACH_FACE(f) {
        setFaceColor(f, dim( GREEN, 120 + 55 * sin_d( (60 * f + millis() / 8) % 360)));
      }
      break;

    case TEAM_A_START:
      setColor( makeColorHSB(60 + 0 * 50, 255, 255));
      break;

    case TEAM_B_START:
      setColor( makeColorHSB(60 + 2 * 50, 255, 255));
      break;
  }

  if ( mode == TEAM_A_START || mode == TEAM_B_START ) {

    // send team to the players on our team
    setValueSentOnAllFaces( mode );       // Tell everyone around how we are feeling
    //    sendValueToTeam();
  }
  else if ( mode == TEAM_A_COINTOSS || mode == TEAM_B_COINTOSS ) {

    sendValueToOpponent();

  }
  else {

    setValueSentOnAllFaces( mode );       // Tell everyone around how we are feeling

  }
}

/*
   This map() functuion is now in Arduino.h in /dev
   It is replicated here so this skect can compile on older API commits
*/

long map_m(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*
   Sin in degrees ( standard sin() takes radians )
*/

float sin_d( uint16_t degrees ) {

  return sin( ( degrees / 360.0F ) * 2.0F * PI   );
}

/*
    Function to return a brightness while breathing based on the period of the breath
*/

byte breathe(word period_ms, byte minBrightness, byte maxBrightness) {

  byte brightness;


  brightness = map_m( 50 * (1 + sin_d( (360 * ((millis() % period_ms)) / (float)period_ms ))), 0, 100, minBrightness, maxBrightness);

  // TODO: if breaths are very short, be sure to focus on the extremes (i.e. light/dark)

  return brightness;
}

/*
  get the team color for our team
*/
Color teamColor( byte t ) {

  return makeColorHSB(60 + t * 50, 255, 255);

}

/*
   Display state for living Mortals
*/
void displayAlive() {
  if ( health > 50 ) {
    deathBrightness = 255;
    setColor( dim( teamColor( team ), breathe(6400, 128, 255) ) );
  } else if ( health <= 50 && health > 40 ) {
    setColor( dim( teamColor( team ), breathe(3200, 96, 255) ) );
  } else if ( health <= 40 && health > 30 ) {
    setColor( dim( teamColor( team ), breathe(1600, 64, 255) ) );
  } else if ( health <= 30 && health > 20 ) {
    setColor( dim( teamColor( team ), breathe(800, 64, 255) ) );
  } else if ( health <= 20 && health > 10 ) {
    setColor( dim( teamColor( team ), breathe(400, 32, 255) ) );
  } else if ( health <= 10 && health > 0 ) {
    setColor( dim( teamColor( team ), breathe(200, 32, 255) ) );
  } else {
    // glow bright white and fade out when we die
    setColor( dim(WHITE, deathBrightness) );
    if (deathBrightness > 7) {
      deathBrightness -= 8;
    }
  }
}

/*
   Display state for injured Mortal
   takes the face we were injured on

*/
void displayInjured(byte face) {

  // first we display our health
  displayAlive();

  // then we update the side that was injured

  // brighten the sides with neighbors
  if ( injuryBrightness > 32 ) {
    setFaceColor( face, dim( RED, injuryBrightness) );
  }

}

/*

*/
void displayGhost() {

  setColor(OFF);

  FOREACH_FACE(f) {

    setFaceColor(f, dim( RED, 32 + 32 * sin_d( (60 * f + millis() / 8) % 360)));  // slow dim rotation, just take my word for it :)

  }
}

/*

*/
void displayEnguarde() {

  setColor( OFF );

  setFaceColor( (millis() / 100) % FACE_COUNT, teamColor( team ) );

}

/*

*/
void displayAttack() {

  setColor( OFF );

  setFaceColor( rand(FACE_COUNT), teamColor( team ) );

}

/*
    Determine if we are in the READY Mode
*/

bool isBlinkInReadyConfiguration() {

  // first count neighbors, if we have more or less than 2, then we are not in the ready mode
  byte numNeighbors = 0;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      numNeighbors++;
    }
  }

  if (numNeighbors != 2) {
    return false;
  }

  // we have 2 neighbors, let's make sure they are dead or ready
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {

      byte neighborMode = getLastValueReceivedOnFace(f);

      if (neighborMode != DEAD && neighborMode != READY && neighborMode != NEIGHBORSREADY) {
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
    if (!isValueReceivedOnFaceExpired(f)) {

      if (firstNeighborFace == NO_FACE) {
        firstNeighborFace = f;
      } else {
        secondNeighborFace = f;
      }
    }
  }

  // now that we have the two faces, are they adjacent?
  if ( (firstNeighborFace == 0 && secondNeighborFace  == 1 ) ||
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


/*
   determine if we are in the start configuration
*/

bool isBlinkTeamCaptain() {

  // first of all I can't be alive, because in that case, the game is still in progress
  if ( mode == ALIVE || mode == ENGUARDE || mode == ATTACKING || mode == INJURED ) {
    return false;
  }

  // first count neighbors, if we have more or less than 3, then we are not in the ready mode
  byte numNeighbors = 0;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      numNeighbors++;
    }
  }

  if (numNeighbors != 3) {
    return false;
  }

  // we have 3 neighbors, let's make sure they are dead or ready
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {

      byte neighborMode = getLastValueReceivedOnFace(f);

      if (neighborMode != DEAD && neighborMode != READY && neighborMode != NEIGHBORSREADY && neighborMode != TEAM_A_COINTOSS && neighborMode != TEAM_B_COINTOSS) {
        return false;
      }
    }
  }

  // great we have 2 neighbors that are either dead or ready
  // let's check to see if they are adjacent to one another
  byte NO_FACE = 255;

  byte firstNeighborFace = NO_FACE;
  byte secondNeighborFace = NO_FACE;
  byte thirdNeighborFace = NO_FACE;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {

      if (firstNeighborFace == NO_FACE) {
        firstNeighborFace = f;
      } else if (secondNeighborFace  == NO_FACE) {
        secondNeighborFace = f;
      } else {
        thirdNeighborFace = f;
      }
    }
  }

  // now that we have the three faces, are they in an acceptable arrangement?
  if ( (firstNeighborFace == 0 && secondNeighborFace  == 1 && thirdNeighborFace == 3) ||
       (firstNeighborFace == 0 && secondNeighborFace  == 1 && thirdNeighborFace == 4) ||
       (firstNeighborFace == 1 && secondNeighborFace  == 2 && thirdNeighborFace == 4) ||
       (firstNeighborFace == 1 && secondNeighborFace  == 2 && thirdNeighborFace == 5) ||
       (firstNeighborFace == 2 && secondNeighborFace  == 3 && thirdNeighborFace == 5) ||
       (firstNeighborFace == 0 && secondNeighborFace  == 2 && thirdNeighborFace == 3) ||
       (firstNeighborFace == 0 && secondNeighborFace  == 3 && thirdNeighborFace == 4) ||
       (firstNeighborFace == 1 && secondNeighborFace  == 3 && thirdNeighborFace == 4) ||
       (firstNeighborFace == 1 && secondNeighborFace  == 4 && thirdNeighborFace == 5) ||
       (firstNeighborFace == 2 && secondNeighborFace  == 4 && thirdNeighborFace == 5) ||
       (firstNeighborFace == 0 && secondNeighborFace  == 2 && thirdNeighborFace == 5) ||
       (firstNeighborFace == 0 && secondNeighborFace  == 3 && thirdNeighborFace == 5) )
  {
    return true;
  }

  return false;
}

/*
   return true if one of our neighbors has coin tossed the other team
*/

bool isBlinkReadyToStartCountdown(int team) {

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborMode = getLastValueReceivedOnFace(f);
      if ( team == TEAM_A_COINTOSS && (neighborMode == TEAM_B_COINTOSS || neighborMode == TEAM_B_START) ) {
        return true;
      }
      else if ( team == TEAM_B_COINTOSS && (neighborMode == TEAM_A_COINTOSS || neighborMode == TEAM_A_START) ) {
        return true;
      }
    }
  }

  return false;

}

/*
   send the value of our team to our team so we know to start
*/

void sendValueToTeam() {

  // Send the value to the two adjacent neighbors
  byte NO_FACE = 255;

  byte firstNeighborFace = NO_FACE;
  byte secondNeighborFace = NO_FACE;
  byte thirdNeighborFace = NO_FACE;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {

      if (firstNeighborFace == NO_FACE) {
        firstNeighborFace = f;
      } else if (secondNeighborFace  == NO_FACE) {
        secondNeighborFace = f;
      } else {
        thirdNeighborFace = f;
      }
    }
  }
  if ( (firstNeighborFace == 0 && secondNeighborFace  == 1 && thirdNeighborFace == 3) ||
       (firstNeighborFace == 0 && secondNeighborFace  == 1 && thirdNeighborFace == 4) ) {
    setValueSentOnFace( mode, 0);
    setValueSentOnFace( mode, 1);
  }
  else if (
    (firstNeighborFace == 1 && secondNeighborFace  == 2 && thirdNeighborFace == 4) ||
    (firstNeighborFace == 1 && secondNeighborFace  == 2 && thirdNeighborFace == 5) ) {
    setValueSentOnFace( mode, 1);
    setValueSentOnFace( mode, 2);
  }
  else if (
    (firstNeighborFace == 2 && secondNeighborFace  == 3 && thirdNeighborFace == 5) ||
    (firstNeighborFace == 0 && secondNeighborFace  == 2 && thirdNeighborFace == 3) ) {
    setValueSentOnFace( mode, 2);
    setValueSentOnFace( mode, 3);
  }
  else if (
    (firstNeighborFace == 0 && secondNeighborFace  == 3 && thirdNeighborFace == 4) ||
    (firstNeighborFace == 1 && secondNeighborFace  == 3 && thirdNeighborFace == 4) ) {
    setValueSentOnFace( mode, 3);
    setValueSentOnFace( mode, 4);
  }
  else if (
    (firstNeighborFace == 1 && secondNeighborFace  == 4 && thirdNeighborFace == 5) ||
    (firstNeighborFace == 2 && secondNeighborFace  == 4 && thirdNeighborFace == 5) ) {
    setValueSentOnFace( mode, 4);
    setValueSentOnFace( mode, 5);
  }
  else if (
    (firstNeighborFace == 0 && secondNeighborFace  == 2 && thirdNeighborFace == 5) ||
    (firstNeighborFace == 0 && secondNeighborFace  == 3 && thirdNeighborFace == 5) ) {
    setValueSentOnFace( mode, 0);
    setValueSentOnFace( mode, 5);
  }
}

/*
   send the value of our team to our team so we know to start
*/

void sendValueToOpponent() {

  // Send the value to the two adjacent neighbors
  byte NO_FACE = 255;

  byte firstNeighborFace = NO_FACE;
  byte secondNeighborFace = NO_FACE;
  byte thirdNeighborFace = NO_FACE;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {

      if (firstNeighborFace == NO_FACE) {
        firstNeighborFace = f;
      } else if (secondNeighborFace  == NO_FACE) {
        secondNeighborFace = f;
      } else {
        thirdNeighborFace = f;
      }
    }
  }
  if ( (firstNeighborFace == 0 && secondNeighborFace  == 1 && thirdNeighborFace == 3) ||
       (firstNeighborFace == 0 && secondNeighborFace  == 3 && thirdNeighborFace == 5) ) {
    setValueSentOnFace(mode, 3);
  }
  else if (
    (firstNeighborFace == 0 && secondNeighborFace  == 1 && thirdNeighborFace == 4) ||
    (firstNeighborFace == 1 && secondNeighborFace  == 2 && thirdNeighborFace == 4) ) {
    setValueSentOnFace(mode, 4);
  }
  else if (
    (firstNeighborFace == 1 && secondNeighborFace  == 2 && thirdNeighborFace == 5) ||
    (firstNeighborFace == 2 && secondNeighborFace  == 3 && thirdNeighborFace == 5) ) {
    setValueSentOnFace(mode, 5);
  }
  else if (
    (firstNeighborFace == 0 && secondNeighborFace  == 2 && thirdNeighborFace == 3) ||
    (firstNeighborFace == 0 && secondNeighborFace  == 3 && thirdNeighborFace == 4) ) {
    setValueSentOnFace(mode, 0);
  }
  else if (
    (firstNeighborFace == 1 && secondNeighborFace  == 3 && thirdNeighborFace == 4) ||
    (firstNeighborFace == 1 && secondNeighborFace  == 4 && thirdNeighborFace == 5) ) {
    setValueSentOnFace(mode, 1);
  }
  else if (
    (firstNeighborFace == 2 && secondNeighborFace  == 4 && thirdNeighborFace == 5) ||
    (firstNeighborFace == 0 && secondNeighborFace  == 2 && thirdNeighborFace == 5) ) {
    setValueSentOnFace(mode, 2);
  }
}

/*
   returns true if not the team captain
*/
bool isNotTeamCaptain() {

  byte numNeighbors = 0;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      numNeighbors++;
    }

    return ( numNeighbors != 3 );
  }
}

