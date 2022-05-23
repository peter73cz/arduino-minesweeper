#include <Esplora.h>
#include <SPI.h>
#include <SD.h>
#include <TFT.h> 
#define SD_CS    8
#define SIZE     10

int x, y, oldx, oldy, menuPos, oldmenuPos, tileCounter;
unsigned long timer, startTime;
int ending;
bool pressed = false;

enum State {
  MENU, SCORE, DIFFICULTY, GAME, PAUSE, VICTORY, LOSE // atd
};
enum State state;

// ----------------------- BOARD -------------------- //
class Element {
  public:
    bool bomb;
    bool revealed;
    bool marked;
    int num;

    Element() {
      Clear();
    }

    void Clear() {
      bomb = false;
      revealed = false;
      marked = false;
      num = 0;
    }
};

Element board[SIZE][SIZE];

void generateBoard(int num) {
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      board[i][j].Clear();
    }
  }

  for (int i = 0; i < num; i++) {
    int x = random(0,SIZE);
    int y = random(0,SIZE);
    Serial.print("Random: ");
    Serial.print(x);
    Serial.print(" ");
    Serial.println(y);
    if (board[x][y].bomb == false) {
      board[x][y].bomb = true;
    }
    else i--;
  }

  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      board[i][j].num = surroundingBombs(i,j);
    }
  }
}

int surroundingBombs(int x, int y) {
  if (board[x][y].bomb == true) return -1;

  int total = 0;
  for (int xoff = -1; xoff <= 1; xoff++) {
    int a = x + xoff;
    if (a < 0 || a >= SIZE) continue;

    for (int yoff = -1; yoff <= 1; yoff++) {
      int b = y + yoff;
      if (b < 0 || b >= SIZE) continue;

      if (board[a][b].bomb == true) total++;
    }
  }
  return total;
}

// ----------------------- TILES -------------------- //

void printTile(int x, int y) {
  //Serial.print("Print: ");
  //Serial.println(board[x][y].revealed);
  if(board[x][y].revealed == true) {
    EsploraTFT.stroke(0,0,0);
    EsploraTFT.fill(200,200,200);
    EsploraTFT.rect(x*12 + 1,y*12 + 1,10,10);
      
    if(board[x][y].bomb == true) {
      EsploraTFT.noStroke(); 
      EsploraTFT.fill(0,0,0);
      EsploraTFT.circle(x*12 + 6, y*12 + 6, 2);
    }
    else {
      EsploraTFT.setTextSize(1);
      //Serial.print("Tile num: ");
      //Serial.println(board[x][y].num);
      if(board[x][y].num > 0) {
        char toPrint[] = {board[x][y].num + 48, 0};
        EsploraTFT.text(toPrint, x*12 + 3, y*12 + 3);      
      }
    }
  }
  else {
    EsploraTFT.stroke(0,0,0);
    EsploraTFT.fill(127,127,127);
    EsploraTFT.rect(x*12 + 1,y*12 + 1,10,10);
    
    if(board[x][y].marked == true) {
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(x*12 + 3,y*12 + 3,6,6);
    }
  }
}

void revealTile(int x, int y) {
  if(board[x][y].marked == true) return;

  // Reveal
  if(board[x][y].revealed == true ) return;
  
  board[x][y].revealed = true;
  printTile(x,y);

  if(board[x][y].bomb == true) {
    // TODO Lose
    ending = 0;
  }
  else if(--tileCounter == 0) {
    // TODO call Victory
    ending = 1;
  }

  // FloodFill
  if(board[x][y].num != 0) return;
 
  for (int xoff = -1; xoff <= 1; xoff++) {
    int a = x + xoff;
    if (a < 0 || a >= SIZE) continue;

    for (int yoff = -1; yoff <= 1; yoff++) {
      int b = y + yoff;
      if (b < 0 || b >= SIZE) continue;

      revealTile(a,b);
    }
  }
}

void markTile(int x, int y) {
  if(board[x][y].revealed == false) {
    board[x][y].marked = !board[x][y].marked;
    
    printTile(x,y);
  }
}
// ----------------------- GAME -------------------- //
void printTime() {
  EsploraTFT.noStroke();
  EsploraTFT.fill(255,255,255);
  EsploraTFT.rect(125,0,40,20);
  
  EsploraTFT.setTextSize(1);
  EsploraTFT.stroke(0,0,0);
  EsploraTFT.fill(0,0,0);

  int minutes = timer / 60;
  int seconds = timer % 60;
  char toPrint[] = {minutes/10 + 48, minutes % 10 + 48,':', seconds/10 + 48, seconds % 10 + 48, 0};
  EsploraTFT.text(toPrint, 130, 5);  
}

void printGame() {
  EsploraTFT.background(255,255,255);
  for(int i = 0; i < SIZE; i++) {
    for(int j = 0; j < SIZE; j++) {
      printTile(i,j);
    }
  }

  printTime();
}

void revealBombs() {
  for(int i = 0; i < SIZE; i++) {
    for(int j = 0; j < SIZE; j++) {
      if(board[x][y].bomb == true) {
        revealTile(i,j);
      }
    }
  }
}

void startGame(int bombs) {
  x = 0;
  y = 0;
  oldx = -1;
  oldy = -1;
  timer = 0;
  ending = -1;
  startTime = millis();
  tileCounter = SIZE*SIZE - bombs;
  
  generateBoard(bombs);
  printGame();
}

void endGame(bool result) {
  revealBombs();
  delay(2000);
  if (result == true) {
    // Victory menu
    state = VICTORY;
    menuPos = 0;
    
    EsploraTFT.background(255,255,255);

    EsploraTFT.setTextSize(2);
    EsploraTFT.stroke(0,255,0);
    EsploraTFT.fill(0,255,0);
    EsploraTFT.text("!!!VICTORY!!!",2, 83);
    drawVictory();
  }
  else {
    // Lose menu
    state = LOSE;
    menuPos = 0;
    
    EsploraTFT.background(255,255,255);

    
    EsploraTFT.setTextSize(2);
    EsploraTFT.stroke(255,0,0);
    EsploraTFT.fill(255,0,0);
    EsploraTFT.text("!!!LOSE!!!",22, 83);
    drawLose();
  }
}
// ----------------------- SETUP -------------------- //

void setup() {

  EsploraTFT.begin();
  EsploraTFT.background(255,255,255);
  EsploraTFT.stroke(0,0,0);

  x = 0;
  y = 0;
  menuPos = 0;
  state = MENU;
  drawMenu();
}

// ----------------------- GAME STATES -------------------- //

void drawMenu() {
  EsploraTFT.noStroke();
  EsploraTFT.fill(255,255,255);
  EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);
  EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
  if (menuPos == 0) {
      // PLAY
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);

      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("PLAY",55,23); 

      // SCOREBOARD
      /*
       EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("SCORE",48,53); 
      */
  }
  else if (menuPos == 1) {
      // PLAY
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("PLAY",55,23); 

      // SCOREBOARD

      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
      
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("SCORE",48,53); 
  }

  oldmenuPos = menuPos;
}
void Menu() {
  if(Esplora.readButton(SWITCH_1) == LOW && !pressed) {    
    state = DIFFICULTY;
    menuPos = 0;
    EsploraTFT.background(255,255,255);
    drawDifficulty();
    
    pressed = true;
    return;
  }
  
  int espY = Esplora.readJoystickY();
  espY = espY * (abs(espY) > 100);
  
  if (espY > 0 && menuPos < 0) menuPos++;
  if (espY < 0 && menuPos > 0) menuPos--;

  if(menuPos != oldmenuPos) {
    drawMenu();
  }
}

// --------------------- DIFFICULTY ------------------------ //
void drawDifficulty() {
  EsploraTFT.noStroke();
    EsploraTFT.fill(255,255,255);
    EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);
    EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
    EsploraTFT.rect(20,80,EsploraTFT.width() - 40,20);
    if (menuPos == 0) {
      // EASY
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);

      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("EASY",55,23); 

      // MEDIUM
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("MEDIUM",43,53); 

      //HARD
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("HARD",55,83); 

    }
    else if (menuPos == 1) {
      // EASY
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("EASY",55,23); 

      // MEDIUM
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
      
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("MEDIUM",43,53); 

      //HARD
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("HARD",55,83); 
    }
    else if (menuPos == 2) {
      // EASY
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("EASY",55,23); 
      
      // MEDIUM
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("MEDIUM",43,53); 

      //HARD
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,80,EsploraTFT.width() - 40,20);
      
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("HARD",55,83); 
    }

    oldmenuPos = menuPos;
}
void Difficulty() {
  // Choose difficulty
  if(Esplora.readButton(SWITCH_1) == LOW && !pressed) {
    state = GAME;
    startGame((menuPos + 1)*10);
    
    pressed = true;
    return;
  }
  // Back
  if(Esplora.readButton(SWITCH_4) == LOW && !pressed) {
    Serial.println("4");
    state = MENU;
    menuPos = 0;
    EsploraTFT.background(255,255,255);
    drawMenu();
    
    pressed = true;
    return;
  }
  
  int espY = Esplora.readJoystickY();
  espY = espY * (abs(espY) > 100);
  
  if (espY > 0 && menuPos < 2) menuPos++;
  if (espY < 0 && menuPos > 0) menuPos--;

  Serial.print("MenuPos: ");
  Serial.println(menuPos);
  if(menuPos != oldmenuPos) {
    drawDifficulty();
  }
  if(Esplora.readButton(SWITCH_1) && Esplora.readButton(SWITCH_2) && Esplora.readButton(SWITCH_3) && Esplora.readButton(SWITCH_4)) pressed = false;
  }

// ---------------------- GAME ----------------- //
void Game() {
  // TIMER
  if(((millis() - startTime) - timer*1000) > 1000) {
    timer = (millis() - startTime) / 1000;
    printTime();
  }

  // Reveal
  if(Esplora.readButton(SWITCH_1) == LOW && !pressed) {
    Serial.println("Reveal");
    revealTile(x,y);
    pressed = true;
  }
  // Mark
  if(Esplora.readButton(SWITCH_2) == LOW && !pressed) {
    Serial.println("Mark");
    markTile(x,y);
    pressed = true;
  }
  // Back / Pause
  if(Esplora.readButton(SWITCH_4) == LOW && !pressed) {
    Serial.println("Pause");
    state = PAUSE;
    menuPos = 0;
    drawPause();
    
    pressed = true;
  }

  if(ending >= 0) {
    endGame(ending);
    return;
  }
  
  int espX = Esplora.readJoystickX();
  espX = espX * (abs(espX) > 100);
  int espY = Esplora.readJoystickY();
  espY = espY * (abs(espY) > 100);
  
  if (espX > 0 && x > 0) x--;
  if (espX < 0 && x < SIZE - 1) x++;

  if (espY > 0 && y < SIZE - 1) y++;
  if (espY < 0 && y > 0) y--;

  if(x != oldx || y != oldy) {
    EsploraTFT.stroke(255,255,255);
    EsploraTFT.fill(255,255,255);
    EsploraTFT.rect(oldx*12, oldy*12, 12, 12);
    printTile(oldx, oldy);
    
    EsploraTFT.stroke(255,0,0);
    EsploraTFT.rect(x*12, y*12, 12, 12);
    printTile(x, y);
    
    oldx = x;
    oldy = y;
  }
}
// ---------------------- PAUSE ------------------- //
void drawPause() {
  EsploraTFT.stroke(0,0,0);
    EsploraTFT.fill(255,255,255);
    EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);
    EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
    EsploraTFT.rect(20,80,EsploraTFT.width() - 40,20);
    if (menuPos == 0) {
      // Continue
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);

      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("CONTINUE",33,23); 

      // Restart
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("RESTART",40,53); 

      // Main Menu
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("MAIN MENU",28,83); 

    }
    else if (menuPos == 1) {
      // EASY
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("CONTINUE",33,23); 

      // MEDIUM
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
      
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("RESTART",40,53); 

      //HARD
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("MAIN MENU",28,83); 
    }
    else if (menuPos == 2) {
      // EASY
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("CONTINUE",33,23); 
      
      // MEDIUM
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("RESTART",40,53); 

      //HARD
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,80,EsploraTFT.width() - 40,20);
      
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("MAIN MENU",28,83); 
    }

    oldmenuPos = menuPos;
}
void Pause() {
  if(Esplora.readButton(SWITCH_1) == LOW && !pressed) {
    // Continue
    if(menuPos == 0) {
      state = GAME;
      
      EsploraTFT.background(255,255,255);
      printGame();
    }
    // Restart
    else if (menuPos == 1) {
      state = DIFFICULTY;
      menuPos = 0;
      
      EsploraTFT.background(255,255,255);
      drawDifficulty();
    }
    // Main Menu
    else if (menuPos == 2) {
      state = MENU;
      menuPos = 0;
      EsploraTFT.background(255,255,255);
      drawMenu();
    }
    
    pressed = true;
    return;
  }
  
  int espY = Esplora.readJoystickY();
  espY = espY * (abs(espY) > 100);
  
  if (espY > 0 && menuPos < 2) menuPos++;
  if (espY < 0 && menuPos > 0) menuPos--;

  Serial.print("MenuPos: ");
  Serial.println(menuPos);
  if(menuPos != oldmenuPos) {
    drawPause();
  }
}
// ---------------------- LOSE --------------------- //

void drawLose() {
  EsploraTFT.noStroke();
  EsploraTFT.fill(255,255,255);
  EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);
  EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
  if (menuPos == 0) {
      // PLAY
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);

      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("RESTART",40,23); 

      // SCOREBOARD
       EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("MAIN MENU",28,53); 
  }
  else if (menuPos == 1) {
      // PLAY
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("RESTART",40,23); 

      // SCOREBOARD

      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
      
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("MAIN MENU",28,53); 
  }

  oldmenuPos = menuPos;
}
void Lose() {
  if(Esplora.readButton(SWITCH_1) == LOW && !pressed) {    
    // Restart
    if (menuPos == 0) {
      state = DIFFICULTY;
      menuPos = 0;
      
      EsploraTFT.background(255,255,255);
      drawDifficulty();
    }
    // Main Menu
    else if (menuPos == 1) {
      state = MENU;
      menuPos = 0;
      EsploraTFT.background(255,255,255);
      drawMenu();
    }
  }
  
  int espY = Esplora.readJoystickY();
  espY = espY * (abs(espY) > 100);
  
  if (espY > 0 && menuPos < 1) menuPos++;
  if (espY < 0 && menuPos > 0) menuPos--;

  if(menuPos != oldmenuPos) {
    drawLose();
  }
}

// ----------------------- VICTORY -----------------//

void drawVictory() {
  EsploraTFT.noStroke();
  EsploraTFT.fill(255,255,255);
  EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);
  EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
  if (menuPos == 0) {
      // RESTART
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,20,EsploraTFT.width() - 40,20);

      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("RESTART!",40,23); 

      // MAIN MENU
       EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("MAIN MENU",28,53); 
  }
  else if (menuPos == 1) {
      // RESTART
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(0,0,0);
      EsploraTFT.text("RESTART!",40,23); 

      // MAIN MENU
      EsploraTFT.noStroke();
      EsploraTFT.fill(0,0,0);
      EsploraTFT.rect(20,50,EsploraTFT.width() - 40,20);
      
      EsploraTFT.setTextSize(2);
      EsploraTFT.stroke(255,255,255);
      EsploraTFT.text("MAIN MENU",28,53); 
  }

  oldmenuPos = menuPos;
}
void Victory() {
  if(Esplora.readButton(SWITCH_1) == LOW && !pressed) {    
    // Restart
    if (menuPos == 0) {
      state = DIFFICULTY;
      menuPos = 0;
      
      EsploraTFT.background(255,255,255);
      drawDifficulty();
    }
    // Main Menu
    else if (menuPos == 1) {
      state = MENU;
      menuPos = 0;
      EsploraTFT.background(255,255,255);
      drawMenu();
    }
  }
  
  int espY = Esplora.readJoystickY();
  espY = espY * (abs(espY) > 100);
  
  if (espY > 0 && menuPos < 1) menuPos++;
  if (espY < 0 && menuPos > 0) menuPos--;

  if(menuPos != oldmenuPos) {
    drawVictory();
  }
}

// ----------------------- LOOP -------------------- //
void loop() {

  switch(state) {
    case MENU:
      Menu();
      break;
    case SCORE:
      break;
    case DIFFICULTY:
      Difficulty();
      break;
    case GAME:
      Game();
      break;
     case PAUSE:
      Pause();
      break;
     case VICTORY:
      Victory();
      break;
     case LOSE:
      Lose();
      break;                                                                                                                                                                                                                                                                                                                                                                                                                                                    
  }

  if(Esplora.readButton(SWITCH_1) && Esplora.readButton(SWITCH_2) && Esplora.readButton(SWITCH_3) && Esplora.readButton(SWITCH_4)) pressed = false;
  delay(100);
}
