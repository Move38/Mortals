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
#define ZOMBIE_VALUE                5   // Amount of health you loose when attacked by the dead
#define LIFE_DRAIN_VALUE            5   // Amount of health you loose when attacked by the dead

#define ATTACK_VALUE                5   // Amount of health you loose when attacked.
#define ATTACK_DURRATION_MS       100   // Time between when we see first new neighbor and when we stop attacking.
#define DRAIN_STEP_TIME_MS       1000   // Health decremented by 1 unit this often

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

Timer drainTimer;
Timer injuryDecayTimer; // Timing to fade away the injury

byte injuryBrightness = 0;
byte injuredFace;

byte deathBrightness = 0;

enum moveState {
  STILL,
  START_MOVE,
  END_MOVE
};

byte turnMode = STILL;

byte turnModeReceived[6];

bool isNeighborPresent[6];

enum gameState {
  DEAD,
  ALIVE,
  ENGUARDE,   // I am ready to attack!
  ATTACKING,  // Short window when I have already come across my first victim and started attacking
  INJURED
};

byte gameMode = DEAD;

byte gameModeReceived[6];

byte totalMoves;

bool bMoveCompleted = false;

Timer modeTimeout;     // Started when we enter ATTACKING, when it expires we switch back to normal ALIVE.
// Started when we are injured to make sure we don't get injured multiple times on the same attack

//Debugging code
#include "Serial.h"

ServicePortSerial Serial;

void setup() {
  // perhaps we should initialize everything here to be safe
  Serial.begin();

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      isNeighborPresent[f] = true;
    } else {
      isNeighborPresent[f] = false;
    }
  }

}


void loop() {

  //Debug code
  Serial.println(health);

  //Reset the game to the initial state
  if (buttonDoubleClicked()) {
    // reset game piece
    gameMode = ALIVE;
    health = INITIAL_HEALTH;
  }

  //Change teams by long pressing
  if (buttonLongPressed()) {
    // change team
    team = (team + 1) % MAX_TEAMS;
  }

  //Get the data from other Blinks and dissect the data into the other Blink's turnMode and gameMode
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte dataReceived = getLastValueReceivedOnFace(f);
      gameModeReceived[f] = dataReceived % 10;
      turnModeReceived[f] = dataReceived / 10; // rounds down (i.e. 9/10 = 0)
    }
  }


  /*
     First set up the switch statement that will handle when a turn is done
  */
  switch (turnMode) {
    case STILL:       stillLoop();      break;
    case START_MOVE:  moveStartLoop();  break;
    case END_MOVE:    moveEndLoop();    break;
  }

  // keep track of our previous state of neighbors to understand when a turn has started or ended
  FOREACH_FACE(f) {
    isNeighborPresent[f] = !isValueReceivedOnFaceExpired(f);
  }

  if (gameMode == ATTACKING || gameMode == INJURED ) {

    if (modeTimeout.isExpired()) {
      gameMode = ALIVE;
    }
  }

  // Update our display based on new state

  switch (gameMode) {

    case DEAD:
      deadMode();
      displayGhost();
      break;

    case ALIVE:
      aliveMode();
      displayAlive();
      break;

    case ENGUARDE:
      enguardeMode();
      displayEnguarde();
      break;

    case ATTACKING:
      attackMode();
      displayAttack();
      break;

    case INJURED:
      injuredMode();
      displayInjured( injuredFace );
      break;

  }

  switch (turnMode) {
    case STILL:       setColorOnFace(MAGENTA, 0);      break;
    case START_MOVE:  setColorOnFace(ORANGE, 0);       break;
    case END_MOVE:    setColorOnFace(CYAN, 0);        break;
  }

  //Get your turn and game modes and send them both

  byte sendData = (turnMode * 10) + gameMode;

  setValueSentOnAllFaces(sendData);

}


/*
   TURN MODE LOOPS
*/
void stillLoop () {
  //check surroundings for MISSING NEIGHBORS or neighbors already in distress
  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f) && isNeighborPresent[f] == true) { //missing neighbor
      turnMode = START_MOVE;
    } else if (!isValueReceivedOnFaceExpired(f) && turnModeReceived[f] == START_MOVE) { //detecting a distressed neighbor
      turnMode = START_MOVE;
    }
  }

}

void moveStartLoop () {

  //check surroundings for NEW NEIGHBORS or neighbors in the resolution state
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f) && isNeighborPresent[f] == false) { //new neighbor
      turnMode = END_MOVE;
    } else if (!isValueReceivedOnFaceExpired(f) && turnModeReceived[f] == END_MOVE) { //next to a resolved neighbor
      turnMode = END_MOVE;
    }
  }


}

void moveEndLoop () {
  turnMode = STILL;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f) && turnModeReceived[f] == START_MOVE) {
      turnMode = END_MOVE;
    }
  }

  if (turnMode == STILL) {
    //This happens once at the end of the move
    totalMoves++;
    bMoveCompleted = true;
  }

}

/*
   GAME MODE FUNCTIONS
*/

void aliveMode() {

  FOREACH_FACE(f) {

    if (!isValueReceivedOnFaceExpired(f)) {

      byte neighborMode = getLastValueReceivedOnFace(f) % 10;

      //If someone is attacking you
      if ( neighborMode == ATTACKING ) {

        gameMode = INJURED;

        injuredFace = f;  // set the face we are injured on

        injuryBrightness = 255; // Start very injured

        modeTimeout.set( INJURED_DURRATION_MS );

      }
    }
  }

  // if move completed, let our dead neighbors suck our life away
  if (bMoveCompleted) {

    if ( health > 0 ) {
      byte numDeadNeighbors = 0;

      //Dead Blinks will also drain life
      FOREACH_FACE(f) {
        if (!isValueReceivedOnFaceExpired(f)) {
          if (gameModeReceived[f] == DEAD) {
            numDeadNeighbors++;
            Serial.println (numDeadNeighbors);
          }
        }
      }
      health = max(health - LIFE_DRAIN_VALUE, 0);   // remove health from turn completion
      health = max(health - ZOMBIE_VALUE * numDeadNeighbors, 0); // remove health for every zombie attached

      if (gameMode == INJURED) {
        health = max(health - ATTACK_VALUE, 0); // remove health if attacked
      }

      bMoveCompleted = false;

    }
    else {
      gameMode = DEAD;
    }
  }
  
  //If you are alone, ENGUARDE!
  if (isAlone()) {

    gameMode = ENGUARDE;      // Being lonesome makes us ready to attack!

  }
}

void enguardeMode() {
  if (!isAlone()) {
    gameMode = ATTACKING;
    modeTimeout.set(ATTACK_DURRATION_MS);
  }
}

void attackMode() {

  FOREACH_FACE(f) {

    if (!isValueReceivedOnFaceExpired(f)) {

      byte neighborMode = getLastValueReceivedOnFace(f) % 10;

      if ( gameModeReceived[f] == INJURED ) {

        // TODO: We should really keep a per-face attack timer to lock down the case where we attack the same tile twice in a since interaction.

        // only add health once on the end of the move (or return to idle/still)

        if (bMoveCompleted) {
          health = min( health + ATTACK_VALUE , MAX_HEALTH );
          gameMode = ALIVE;
          bMoveCompleted = false;
        }
      }
    }
  }
}

void injuredMode() {


  if (health > 0) {

    // nothing to do here (our life was sucked when alive)

  } else {

    gameMode = DEAD;

  }

  if (modeTimeout.isExpired()) {
    gameMode = ALIVE;
  }

  // TODO: "get your animation out of my game logic" -Jon
  // NOTE: Jon did this in the first place :)
  //Animate bright red and fade out over the course of
  if ( injuryDecayTimer.isExpired() ) {

    injuryDecayTimer.set( INJURY_DECAY_INTERVAL_MS );

    injuryBrightness -= INJURY_DECAY_VALUE;


  }

}

void deadMode() {
  // don't need to do anything here except pass move progress if need be
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
   -------------------------------------------------------------------------------------
                                 START DISPLAY
   -------------------------------------------------------------------------------------
*/


/*
   Display state for living Mortals
*/
void displayAlive() {

  byte displayHealth = health / 10;

  //Look at each face, if the face is greater than the integer of health, turn it off. If it's less than, turn it on.
  FOREACH_FACE(f) {
    if (f > displayHealth) {
      setFaceColor(f, OFF);
    } else {//(f < displayHealth)
      setFaceColor(f, teamColor(team));
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
   -------------------------------------------------------------------------------------
                                 END DISPLAY
   -------------------------------------------------------------------------------------
*/
