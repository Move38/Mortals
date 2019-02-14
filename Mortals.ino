/*
  Mortals for Blinks

   Setup: 2 player game. Create 2 teams – switch team by holding down until color changes (~3s)

   Each Blink starts with 60 seconds of life.

   When a tile is moved alone, it sucks 5 seconds of health from the neighbors it
   attaches to. This attacked neighbor looses 5 seconds
   and shows its injured side (i.e. where the neighbor showed up)

   Blinks reset game when double clicked
   Blinks start game with single click on any Blink

     States for game piece.
     Alive/Dead
     Team
     Attack, Injured

   Technical Details:
*/

#define ATTACK_VALUE                5   // Amount of health you loose when attacked.
#define ATTACK_DURRATION_MS       500   // Time between when we see first new neighbor and when we stop attacking.
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

#define START_DELAY     100
Timer startTimer;

#define WAITING_ANIMATION_DELAY 500
Timer waitingTimer;
bool showOdds = false;

byte injuryBrightness = 0;
byte injuredFace;

byte deathBrightness = 0;

bool attackSuccess[6];

enum State {
  DEAD,
  ALIVE,
  ENGUARDE,   // I am ready to attack!
  ATTACKING,  // Short window when I have already come across my first victim and started attacking
  INJURED
};

byte mode = DEAD;

enum GameState {
  PLAY,
  WAITING,
  START
};

byte gameState = WAITING;

byte neighbors[6];

Timer modeTimeout;     // Started when we enter ATTACKING, when it expires we switch back to normal ALIVE.
// Started when we are injured to make sure we don't get injured multiple times on the same attack


void setup() {
  // perhaps we should initialize everything here to be safe
}


void loop() {

  if (buttonSingleClicked()) {
    if (gameState == WAITING) {
      changeGameState( START );
    }
  }

  if (buttonDoubleClicked()) {
    // reset game and go into waiting mode
    mode = DEAD;
    changeGameState( WAITING );
  }


  if (buttonLongPressed()) {
    // change team
    team = getNextTeam();
  }

  // get our neighbor data
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      neighbors[f] = getLastValueReceivedOnFace(f);
    }
  }

  if (healthTimer.isExpired()) {

    if (health > 0) {

      byte numDeadNeighbors = 0;

      //Dead Blinks will also drain life
      FOREACH_FACE(f) {
        if (!isValueReceivedOnFaceExpired(f)) {
          if (getGameMode(neighbors[f]) == DEAD) {
            numDeadNeighbors++;
          }
        }
      }

      //Remove extra health for every dead neighbor attached
      health = (health - 1) - numDeadNeighbors;
      healthTimer.set(HEALTH_STEP_TIME_MS);

    } else {

      mode = DEAD;

    }

  }

  if ( mode != DEAD ) {

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
        FOREACH_FACE(f) {
          if (attackSuccess[f]) {
            health = min( health + ATTACK_VALUE , MAX_HEALTH );
          }
        }
      }
    }
  } // !DEAD

  // check our surroundings
  FOREACH_FACE(f) {


    if (!isValueReceivedOnFaceExpired(f)) {

      byte neighborMode = getGameMode(neighbors[f]);

      if ( mode == ATTACKING ) {

        // We take our flesh when we see that someone we attacked is actually injured

        if ( neighborMode == INJURED ) {

          // TODO: We should really keep a per-face attack timer to lock down the case where we attack the same tile twice in a since interaction.

          attackSuccess[f] = true;
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
      }
    }
  }

  // Update our display based on new state

  switch (mode) {

    case DEAD:
      displayGhost();
      break;

    case ALIVE:
      resetAttackSuccess();
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
  }


  // let's start updating game state
  switch (gameState) {
    case PLAY:     playUpdate();      break;
    case WAITING:  waitingUpdate();   break;
    case START:    startUpdate();     break;
  }

  byte data = (gameState << 3) + mode;
  setValueSentOnAllFaces( data );       // Tell everyone around how we are feeling

}


/*
   -------------------------------------------------------------------------------------
                                 START GAME LOGIC
   -------------------------------------------------------------------------------------
*/
void changeGameState(byte state) {

  switch (state) {
    case PLAY:          break;
    case WAITING:       break;
    case START:   startTimer.set(START_DELAY);  break;
  }
  gameState = state;
}

void startGame() {
  if (startTimer.isExpired()) {
    mode = ALIVE;
    changeGameState( PLAY );
    health = INITIAL_HEALTH;
    healthTimer.set(HEALTH_STEP_TIME_MS);
  }
}

byte getNextTeam() {
  return (team + 1) % MAX_TEAMS;
}

void playUpdate() {
  // if neighbor is in waiting mode, become waiting
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      if (getGameState(neighbors[f]) == WAITING) {
        changeGameState( WAITING );
        mode = DEAD;
      }
    }
  }
}

void waitingUpdate() {
  // if neighbor is in start mode, transition to start mode
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      if (getGameState(neighbors[f]) == START) {
        changeGameState( START );
      }
    }
  }
}

void startUpdate() {
  // if all neighbors are in start
  bool allReady = true;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      if (getGameState(neighbors[f]) != START && getGameState(neighbors[f]) != PLAY) {
        allReady = false;
      }
    }
  }

  if (isAlone()) {
    allReady = false;
  }

  if (allReady) {
    startGame();
  }
}

void resetAttackSuccess() {
  FOREACH_FACE(f) {
    attackSuccess[f] = false;
  }
}

/*
   -------------------------------------------------------------------------------------
                                 END GAME LOGIC
   -------------------------------------------------------------------------------------
*/


/*
   -------------------------------------------------------------------------------------
                                 START DISPLAY
   -------------------------------------------------------------------------------------
*/


/*
   Display state for living Mortals
*/
void displayAlive() {
  setColor(OFF);
  FOREACH_FACE(f) {

    if ( f <=  (health / 10) ) {
      // show health on the number of faces to represent 10 health for each light
      setColorOnFace(teamColor( team ), f);
      // TODO: FLASH the last 10 seconds of life
    }
    else {
      // turn out the lights on faces to show a loss of health over time
      setColorOnFace(OFF, f);
    }
  }

  if (health <= 0 ) {

    // glow bright white and fade out when we die
    setColor( dim(WHITE, deathBrightness) );

    if (deathBrightness > 7) {
      deathBrightness -= 8;
    }
  }

  // don't show the sucking of energy when in the death phase
  if (deathBrightness == 255) {
    // show the dead sucking life
    FOREACH_FACE(f) {

      if (!isValueReceivedOnFaceExpired(f)) {

        if (getGameMode(neighbors[f]) == DEAD) {

          // pulse red on injured face
          byte bri = breathe(600, 32, 255);

          if ( f <= (health / 10) ) {
            // if the tile is alive and showing life on this face, alternate red and team color
            if ( (millis() / 600) % 2 == 0 ) {
              setColorOnFace( dim(RED, bri), f);
            }
            else {
              setColorOnFace( dim( teamColor( team ), bri), f);
            }
          }
          else {
            // else show red
            setColorOnFace( dim(RED, bri), f);
          }
        }
      }
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
  if ( injuryDecayTimer.isExpired() ) {
    injuryDecayTimer.set( INJURY_DECAY_INTERVAL_MS );
    injuryBrightness -= INJURY_DECAY_VALUE;
  }

  // brighten the sides with neighbors
  if ( injuryBrightness > 32 ) {
    setFaceColor( face, dim( RED, injuryBrightness) );
  }

}

/*

*/
void displayGhost() {

  setColor(OFF);

  // check game state
  if ( gameState == PLAY ) {
    FOREACH_FACE(f) {

      setFaceColor(f, dim( RED, 64 + 32 * sin_d( (60 * f + millis() / 8) % 360)));  // slow dim rotation, just take my word for it :)

    }
  }
  else if (gameState == WAITING ) {

    setColor( dim(teamColor( team ), 92) );
    setFaceColor( 1, dim( teamColor( team ), 159 + 96 * sin_d( ( millis() / 4) % 360) ) );

  }
  else if (gameState == START ) {
    setColor(WHITE);  // quick flash of white to start the game (only a single frame)
    deathBrightness = 255;  // reset the death brightness
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

  setFaceColor( random(FACE_COUNT), teamColor( team ) );

}
/*
   -------------------------------------------------------------------------------------
                                 END DISPLAY
   -------------------------------------------------------------------------------------
*/


/*
   -------------------------------------------------------------------------------------
                                 HELPER FUNCTIONS
   -------------------------------------------------------------------------------------
*/

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

  switch (t) {
    case 0: return MAGENTA;
    case 1: return GREEN;
    case 2: return ORANGE;
    case 3: return BLUE;
  }
  //return makeColorHSB(60 + t * 50, 255, 255);
}

byte getGameMode(byte data) {
  return data & 7;  // 00000111 -> keeps the last 3 digits in binary
}

byte getGameState(byte data) {
  return data >> 3; // 00000XXX -> moves all digits to the right 3 times
}
