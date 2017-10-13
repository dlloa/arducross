#include <Arduboy2.h>
#include "boards.h"

//TODO
// + Truncate Hint lists for smaller boards
// + Increase size of board if playing with 5x5 or 10x10
// ~ Better Title Screen
// ~ Add more Picross
// + Saving

// + Pause Menu
// + Give Up 
// - Timer

// Arduboy Stuff
Arduboy2 arduboy;
#define FRAMERATE 20
bool DEBUG = false;

//Arducross Version
#define VERSION 1
//EEPROM Addresses for Picross
#define ESTATUS 0 // Byte for VERSION and SAVED STATUS : VVVVCCCC V=VERSION C=CLEAR/SAVE
#define EBOARDNUMBER 1 // Byte for Board Number for reload
#define ESAVEDBOARD 2 // Saved current board number
//ESTATUS SAVED STATUS
#define CLEAR 0
#define SAVED 1

// Board Constants 
#define BLOCKSIZE 3
#define BORDERSIZE 1
#define MAXBOARDSIZE 16 // 15 x 15 Standard, 16th is the size of the board
#define REPEATDELAY 4

// displayMode Constants
#define TITLESCREEN 0 
#define GAMEINPLAY 1 // Gameplay - Normal Hints on Left
#define DEBUGBOARD 2 // Gameplay - Hints for Onscreen Board (CURRENT)
#define ROWVALUE 3 // Gameplay - uint16 Values on Screen
#define COMPLETEVALUE 4 // Gameplay - uint16 Values of Completed Board
#define PAUSEMENU 5
byte displayMode = TITLESCREEN + DEBUG;

// Board Stuff
#define COMPLETE 0 //  Complete Board, what the player is working toward
#define CURRENT 1 // Current Board, board that is on the screen
uint16_t Board[2][MAXBOARDSIZE];
byte boardSize = 0;

// Current Position of the Chisel
byte xPos, yPos = 0;

// Button States
byte currButtons, prevButtons;

// For Quicker Movement
byte repeatDelay = 0;

// Title Screen Menu Stuff
#define BOARDSELECT 0
#define RANDOM 1
#define DEBUGMODE 2 
#define LOAD 3
#define TITLESCREENITEMS 4
const char titleScreenOptions[][12] = {
  "Board #","Random","Debug Mode", "Load Saved"
};
byte titleSelect = 0;
byte boardSelect = 0;
bool savedBoard = false;
byte savedBoardNumber = 0;

// Pause Menu Stuff 
#define CANCEL 0
#define GIVEUP 1
#define SAVE 2
#define PAUSEMENUITEMS 3
#define PAUSEDELAY 10
const char pauseScreenOptions[][12] = {
  "Back","Give Up","Save"
};
byte pauseDelay = 0;
byte pauseSelect = 0;

bool gamewon;

void printBoard( byte boardToPrint ){
  gamewon = true;
  //Quick add for increasing blocksize for smaller boards dont look oh god
  byte embiggen = 0;
  if( boardSize <= 10 ){
    embiggen += 2;
  } 
  if( boardSize <= 5 ){
    embiggen += 5;
  }

  // Print the blocks
  for( byte i = 0; i < boardSize; i++ ){
    // Check for completion
    // Mask Values with the Board size
    uint16_t boardmask = 0;
    boardmask = ~boardmask;
    boardmask = boardmask >> (16 - boardSize);
    if( ( boardmask & Board[COMPLETE][i] ) != ( boardmask & Board[CURRENT][i] ) ){
      gamewon = false;
    }

    //Iterate through each bit, 
    for( byte j = 0; j < boardSize; j++ ){
      //Bit = Block
      if( Board[boardToPrint][i] & ( 1 << j ) ){
        //If bit exits, displays Block.
        arduboy.fillRect( ( j * ((BLOCKSIZE + embiggen) + BORDERSIZE)) + 64, (i * ((BLOCKSIZE + embiggen) + BORDERSIZE)) + 1, BLOCKSIZE + embiggen, BLOCKSIZE + embiggen, WHITE);
      }
      else{
        arduboy.drawRect( j* (4 + embiggen) + 63, i * (4 + embiggen), 5 + embiggen, 5 + embiggen, WHITE );
      }
    }
    
  }

  //Draw Cursor
  byte paintColor = WHITE;
  if( !(Board[boardToPrint][yPos] & ( 1 << xPos )) ){
    paintColor = BLACK;
  }

  drawDottedSquare(xPos * (4 + embiggen) + 63, yPos * (4 + embiggen) , 4 + embiggen );
  //arduboy.drawRect( ( xPos * (4 + embiggen)) + 63, ( yPos * (4 + embiggen) ) , 5 + embiggen, 5 + embiggen, paintColor );

}

// Loads a board from PROGMEM into Complete Board Board[COMPLETE]
void loadBoard( byte toload ){
  // Load 16th value for boardsize
  boardSize = pgm_read_word_near( (toload * MAXBOARDSIZE) + LOADEDBOARD + 15 );

  for( byte i = 0; i < boardSize; i++ ){
    // PROGMEM read with pgm_read_word_near( )
    Board[COMPLETE][i] = pgm_read_word_near( (toload * MAXBOARDSIZE ) + LOADEDBOARD + i );
  }
}

//blargh dont look
void drawDottedSquare( byte x, byte y, byte size ){
  for( byte i = 0; i < size; i++ ){
    byte pixelColor = WHITE;
    if( i % 2 ){
      pixelColor = BLACK;
    }

    arduboy.drawPixel( x + i, y, pixelColor);
    arduboy.drawPixel( x + i, y + size, pixelColor);
    arduboy.drawPixel( x , y + i, pixelColor);
    arduboy.drawPixel( x + size, y + i, pixelColor);
  }
}

void resetGame(){
  displayMode = TITLESCREEN + DEBUG;
  gamewon = false;
  xPos = 0;
  yPos = 0;
  for ( byte i = 0; i < MAXBOARDSIZE; i++ ){
    Board[CURRENT][i] = 65535 - 32768;
  }
}

void setup() {
  
  // Arduboy Setup
  arduboy.begin();
  arduboy.initRandomSeed();
  arduboy.setFrameRate(FRAMERATE);

  // Check EEPROM for Saved Game
  byte eStatus = EEPROM.read(ESTATUS + EEPROM_STORAGE_SPACE_START); // VVVVCCCC V=VERSION C=CLEAR/SAVE
  //Version mismatch, board numbers may be different
  if( ( eStatus >> 4) != VERSION ){
    //Rewrite ESTATUS with new version number, clears save
    eStatus = VERSION << 4;
    EEPROM.update( ESTATUS+ EEPROM_STORAGE_SPACE_START, eStatus ); 
  }
  //Check if there is a saved game
  if( (eStatus & 0xF ) == SAVED ){
    //Show there is a saved game to load later
    savedBoard = true;
    //Get Board Number for later
    savedBoardNumber = EEPROM.read(EBOARDNUMBER + EEPROM_STORAGE_SPACE_START);
  }


  // Game Reset
  resetGame();

}

void loop() {
  
  //Wait until next frame, ready screen
  if (!(arduboy.nextFrame())) return;
  arduboy.clear();

  // New Button Inputs
  prevButtons = currButtons;
  currButtons = arduboy.buttonsState();

  // Moving in the same direction, start a delay
  if( prevButtons == currButtons && ( currButtons & ( LEFT_BUTTON + UP_BUTTON + RIGHT_BUTTON + DOWN_BUTTON )) ){
    repeatDelay++;
  }
  else{
    repeatDelay = 0;
  }

  // Menu Delay. Holding A + B opens in game menu
  if( displayMode > TITLESCREEN && prevButtons == currButtons && (currButtons & LEFT_BUTTON) && (currButtons & RIGHT_BUTTON) ){
    pauseDelay++;
    if( pauseDelay >= PAUSEDELAY ){
      pauseDelay = 0;
      displayMode = PAUSEMENU;
    }
  }
  else{
    pauseDelay = 0;
  }
  
  // Checks Inputs only if Inputs change, or repeatdelay exceeds max
  if( ( currButtons != prevButtons || repeatDelay > REPEATDELAY ) ){

    // D-PAD UP
    if( currButtons & UP_BUTTON ){
      // Moving Chisel Up
      if(  displayMode > TITLESCREEN && yPos > 0){
        yPos--;
      }
      // Pause Menu
      if( displayMode == PAUSEMENU ){
        pauseSelect--;
        if( pauseSelect > PAUSEMENUITEMS ){
          pauseSelect = PAUSEMENUITEMS - 1;
        }
      }

      if( displayMode == TITLESCREEN ){
        titleSelect--;
        if( titleSelect > TITLESCREENITEMS ){
          titleSelect = TITLESCREENITEMS - 1;
        }
      }
    }

    //D-PAD DOWN
    if( currButtons & DOWN_BUTTON ){
      // Moving Chisel Down
      if(  displayMode > TITLESCREEN && !(yPos >= boardSize - 1) ){
        yPos++;
      }
      // Pause Menu
      if( displayMode == PAUSEMENU ){
        pauseSelect++;
        pauseSelect = pauseSelect % PAUSEMENUITEMS;
      }
      if( displayMode == TITLESCREEN ){
        titleSelect++;
        titleSelect = titleSelect % TITLESCREENITEMS;
      }
    }

    //D-PAD LEFT
    if( currButtons & LEFT_BUTTON ){
      // Moving Chisel Left
      if(  displayMode > TITLESCREEN && xPos > 0 ){
        xPos --;
      }
      // Selecting board
      if( displayMode == TITLESCREEN && titleSelect == BOARDSELECT ){
        boardSelect--;
        if( boardSelect > NUMBEROFBOARDS ){
          boardSelect = NUMBEROFBOARDS - 1;
        }
      }

    }

    //D-PAD RIGHT
    if( currButtons & RIGHT_BUTTON ){
      // Moving Chisel Right
      if(  displayMode > TITLESCREEN && !(xPos >= boardSize - 1) ){
        xPos ++;
      }
      // Title Select
      if( displayMode == TITLESCREEN && titleSelect == BOARDSELECT ){
        boardSelect++;
        boardSelect = boardSelect % NUMBEROFBOARDS;
      }
    }

    // LEFT FACE BUTTON
    if( currButtons & A_BUTTON ){
      if( displayMode > TITLESCREEN ){
        //Clears Bit xPos in uint16 yPos
        Board[CURRENT][yPos] &= ~( 1 << xPos );
      }
    }
    // RIGHT FACE BUTTON
    if( currButtons & B_BUTTON ){
      if( displayMode > TITLESCREEN ){
        //Sets Bit xPos in uint16 yPos
        Board[CURRENT][yPos] |= ( 1 << xPos );
      }
    }

    //DEBUG STUFF
    //Pressing Left and Right together will change current screen in debug mode
    if( currButtons & LEFT_BUTTON && currButtons & RIGHT_BUTTON && DEBUG ){
      displayMode++;
      displayMode = displayMode % 5;
    }

    // EITHER FACE BUTTON IS PRESSED and nothing held down
    if( prevButtons == 0 && (currButtons & A_BUTTON || currButtons & B_BUTTON) ){
      
      // 
      if( displayMode == TITLESCREEN ){
        //DEBUG Stuff
        if( titleSelect == DEBUGMODE ){
          DEBUG = true;
        }
        if( titleSelect == RANDOM ){
          xPos = 0;
          yPos = 0;
          byte randomBoard = rand() % NUMBEROFBOARDS;
          loadBoard( randomBoard );
          displayMode++;
        }
        if( titleSelect == BOARDSELECT ){
          xPos = 0;
          yPos = 0;
          loadBoard( boardSelect );
          displayMode++;
        }
        if( titleSelect == LOAD && savedBoard ){

          // Load Board from EEPROM
          for( int bytenum = 0; bytenum < 16 * 2; bytenum += 2){
            //Load from EEPROM
            byte upperbyte = EEPROM.read(ESAVEDBOARD + EEPROM_STORAGE_SPACE_START + bytenum);
            byte lowerbyte = EEPROM.read(ESAVEDBOARD + EEPROM_STORAGE_SPACE_START + bytenum + 1);
            //Combine two byte 16 bit num
            uint16_t rowvalue = upperbyte;
            rowvalue = ( rowvalue << 8 ) | lowerbyte;
            //Add to currentBoard
            Board[CURRENT][ bytenum / 2 ] = rowvalue;
          }
          xPos = 0;
          yPos = 0;
          boardSelect = savedBoardNumber;
          loadBoard(boardSelect);
          displayMode++;

        }

      }

      if( displayMode == PAUSEMENU ){
        //Giveup and fill board
        if( pauseSelect == GIVEUP ){
          for( byte i = 0; i < boardSize; i++ ){
            Board[CURRENT][i] = Board[COMPLETE][i];
          }
          displayMode = GAMEINPLAY;
        }
        //Cancel and return to game
        if( pauseSelect == CANCEL ){
          displayMode = GAMEINPLAY;
        }
        //Save Board to EEPROM and go to TITLESCREEN
        if( pauseSelect == SAVE ){
          displayMode = TITLESCREEN;

          //Save Current Board to EEPROM to load later
          for( int bytenum = 0; bytenum < 16 * 2; bytenum+= 2){
            //Get Board Value
            uint16_t rowvalue = Board[CURRENT][ bytenum / 2 ];
            //Break into two bytes for EEPROM
            byte upperbyte = rowvalue >> 8;
            byte lowerbyte = rowvalue & 0xFF;
            //Save into EEPROM
            EEPROM.update( ESAVEDBOARD + EEPROM_STORAGE_SPACE_START + bytenum, upperbyte);
            EEPROM.update( ESAVEDBOARD + EEPROM_STORAGE_SPACE_START + bytenum + 1, lowerbyte);
          }

          // Write ESTATUS and show a SAVED status
          byte eStatus = (VERSION << 4) | SAVED;
          EEPROM.update(ESTATUS + EEPROM_STORAGE_SPACE_START, eStatus);
          // Write EBOARDNUMBER with current selected board
          EEPROM.update(EBOARDNUMBER + EEPROM_STORAGE_SPACE_START, boardSelect);

          setup();

        }
      }

      //Reset and go to title screen on button press
      if( gamewon && prevButtons == 0 ) {
        if(savedBoard){
          byte eStatus = (VERSION << 4);
          EEPROM.update(ESTATUS + EEPROM_STORAGE_SPACE_START, eStatus);
        }
        resetGame();
      }
    }
  }

  //Determines what to display
  switch (displayMode){

    case TITLESCREEN:
      //Debug Indicator
      if( DEBUG ){
        arduboy.setCursor( 24, 8);
        arduboy.print("DEBUG");
      }
      //Print Title of Game
      arduboy.setCursor(38,16);
      arduboy.print("ARDUCROSS");

      //Print menu items
      for( int i = 0; i < TITLESCREENITEMS; i++ ){
        arduboy.setCursor( 8, 32 + ( i * 8 ));
        arduboy.print(titleScreenOptions[i]);
      }

      //Show current board selection
      if( titleSelect == BOARDSELECT ){
        arduboy.setCursor( 56, 32);
        arduboy.print( boardSelect );
      }

      //Current Selection
      arduboy.setCursor(0, 32 + ( titleSelect * 8 ));
      arduboy.print("-");

      //Show Saved Game?
      arduboy.setCursor( 72, 32 + ( 3 * 8 ) ) ;
      if( savedBoard ){
        arduboy.print(savedBoardNumber);
      }
      else{
        arduboy.print("EMPTY"); 
      }

      break;

    case PAUSEMENU:

      //Pause menu title
      arduboy.setCursor(38,16);
      arduboy.print("PAUSED");
      //Print Menu Items
      for( int i = 0; i < PAUSEMENUITEMS; i++ ){
        arduboy.setCursor( 8, 32 + ( i * 8 ));
        arduboy.print(pauseScreenOptions[i]);
      }
      //Current Selection
      arduboy.setCursor(0, 32 + ( pauseSelect * 8 ));
      arduboy.print("-");

    break;

    //Game in Progress, Show necessary values
    case DEBUGBOARD:
    case ROWVALUE:
    case COMPLETEVALUE:
    case GAMEINPLAY:

      //DEBUG Cursor Location
      if( DEBUG && !( displayMode == ROWVALUE || displayMode == COMPLETEVALUE )){
        arduboy.setCursor(0,0);
        arduboy.print("X:");
        arduboy.setCursor(16, 0);
        arduboy.print(xPos);

        arduboy.setCursor(0,8);
        arduboy.print("Y:");
        arduboy.setCursor(16, 8);
        arduboy.print(yPos);
      }

      // Feedback on Board Complete
      if( gamewon ){
        arduboy.setCursor( 8, 16);
        arduboy.print("OK!");
      }

      //DEBUG - GAMEBOARD VALUES OF CURRENT GAMEBOARD
      if( displayMode == ROWVALUE || displayMode == COMPLETEVALUE){

        int subtract = 0; 
        if( displayMode == COMPLETEVALUE ){
          subtract = 1;
        }

        for( byte i = 0; i < 8; i++ ){
          arduboy.setCursor(0, i * 8);
          arduboy.print(Board[CURRENT - subtract][i]);
        }
        //Bottom 8 because im lazy
        for( byte i = 0; i < 7; i++){
          arduboy.setCursor(32, i * 8);
          arduboy.print(Board[CURRENT - subtract][i + 8]);
        }


      }
      else{

        //Horizontal/Vertical Hints
        //Iterate through bits of a row to get hints
        //Filling a Hint List of Vertical/Horizontal, 
        byte horizontalhints[8];
        byte verticalhints[8];
        //Increment for Hints, getting two at a time
        byte addHHint = 0;
        byte addVHint = 0;
        //Number of Hints for each
        byte hhintSize = 0;
        byte vhintSize = 0;

        // Chooses which Hints to get, CURRENT - [1] or COMPLETE - [0]
        byte SHOWCURRENTDEBUG = 0;
        if( displayMode == DEBUGBOARD ){
          SHOWCURRENTDEBUG = 1;
        }

        for( byte i = 0; i < boardSize; i++ ){

          //Checking Horizontal
          //Increase if successive bits cleared in a uint16
          if(!(Board[COMPLETE + SHOWCURRENTDEBUG][yPos] & (1 << i ))){
            addHHint++;
          }
          //Bit i is set, start new increment if there were consecutive cleared bits
          else{
            if( addHHint > 0 ){
              horizontalhints[hhintSize] = addHHint;
              addHHint = 0;
              hhintSize++;
            }
          }

          //Checking Vertical
          //Same as horizontal, but checks xPos bit on all uint16
          if( !(Board[COMPLETE + SHOWCURRENTDEBUG][i] & (1 << xPos) ) ){
            addVHint++;
          }
          else{
            if( addVHint > 0 ){
              verticalhints[vhintSize] = addVHint;
              addVHint = 0;
              vhintSize++;
            }
          }

        }
        //Adding last hint that loop didnt catch, could be 0
        horizontalhints[hhintSize] = addHHint;
        hhintSize++;
        verticalhints[vhintSize] = addVHint;
        vhintSize++;
        
        //Hints acquired, print them, loop for each. Offset for readability
        for( byte i = 0; i < hhintSize; i++ ){
          byte riseoffset = i % 2;
          arduboy.setCursor( i * 7, 50 - (riseoffset * 6));
          if( horizontalhints[i] != 0 )
            arduboy.print(horizontalhints[i]);
        }
        for( byte i = 0; i < vhintSize; i++ ){
          byte riseoffset = i % 2;
          arduboy.setCursor( 48 - (riseoffset * 6), i * 5 );
          if( verticalhints[i] != 0 ){
            arduboy.print(verticalhints[i]);
          }
        }
      }

      int SHOWCOMPLETEDEBUG = 0;
      if( displayMode == COMPLETEVALUE ){
        SHOWCOMPLETEDEBUG = 1;
      }
      printBoard( CURRENT - SHOWCOMPLETEDEBUG );
      break;
    
  }

  // Display everything printed
  arduboy.display();
}
