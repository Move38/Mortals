#line 86 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void setup();
#line 91 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void loop();
#line 261 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void changeGameState(GameState state);
#line 271 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void startGame();
#line 280 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
byte getNextTeam();
#line 284 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void playUpdate();
#line 296 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void waitingUpdate();
#line 307 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void startUpdate();
#line 345 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void displayAlive();
#line 373 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void displayInjured(byte face);
#line 394 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void displayGhost();
#line 420 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void displayEnguarde();
#line 431 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
void displayAttack();
#line 456 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
long map_m(long x, long in_min, long in_max, long out_min, long out_max);
#line 465 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
float sin_d( uint16_t degrees );
#line 474 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
byte breathe(word period_ms, byte minBrightness, byte maxBrightness);
#line 489 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
Color teamColor( byte t );
#line 495 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
Mode getGameMode(byte data);
#line 499 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"
GameState getGameState(byte data);
#line 86 "C:\\Users\\passp\\Documents\\GitHub\\Mortals\\Mortals.ino"