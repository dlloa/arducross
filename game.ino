#include <Arduboy2.h>
#include "boards.h"

//TODO
// - Truncate Hint lists for smaller boards
// - Auto Clear Row/Column in Board[CURRENT] if Board[COMPLETE] has 0 bytes or all xth bits cleared
// - Increase size of board if playing with 5x5 or 10x10
// - Better Title Screen

// Arduboy Stuff
Arduboy2 arduboy;
#define FRAMERATE 20
bool DEBUG = false;

// Board Constants 
#define BLOCKSIZE 3
#define BORDERSIZE 1
#define BOARDSIZE 15 // 15 x 15 Standard Hard Picross Board
#define REPEATDELAY 4

// displayMode Constants
#define TITLESCREEN 0 
#define COMPLETEBOARDHINT 1 // Gameplay - Normal Hints on Left
#define DEBUGBOARD 2 // Gameplay - Hints for Onscreen Board (CURRENT)
#define ROWVALUE 3 // Gameplay - Byte Values on Screen
#define COMPLETEVALUE 4 // Gameplay - Byte Values of Completed Board
byte displayMode = TITLESCREEN + DEBUG;

// Board Stuff
#define COMPLETE 0
#define CURRENT 1
uint16_t Board[2][BOARDSIZE];
byte boardSelect = 0;

// Current Position of the Chisel
byte xPos, yPos = 0;

// Button States
byte currButtons, prevButtons;

// For Quicker Movement
byte repeatDelay = 0;

// Complete Board Feedback
bool gamewon;

void printBoard( byte boardToPrint ){
  gamewon = true;
  // Iterate through each byte
  for( byte i = 0; i < BOARDSIZE; i++ ){

     // Check for completion
     if( Board[COMPLETE][i] != Board[CURRENT][i] ){
      gamewon = false;
     }

    //Iterate through each bit, 
    for( byte j = 0; j < BOARDSIZE; j++ ){
      //Bit = Block
      if( Board[boardToPrint][i] & ( 1 << j ) )
        //If bit exits, displays Block.
        arduboy.fillRect( ( j * (BLOCKSIZE + BORDERSIZE)) + 64, (i * (BLOCKSIZE + BORDERSIZE)) + 1, BLOCKSIZE, BLOCKSIZE, WHITE);
    }
    
  }
}

// Loads a board from PROGMEM
void loadBoard( byte toload ){
  for( byte i = 0; i < BOARDSIZE; i++ ){
    // PROGMEM read with pgm_read_word_near( )
    Board[COMPLETE][i] = pgm_read_word_near( (toload * BOARDSIZE ) + LOADEDBOARD + i );
  }
}

void setup() {
  
  // Arduboy Setup
  arduboy.begin();
  arduboy.initRandomSeed();
  arduboy.setFrameRate(FRAMERATE);

  //Fill Current Gameboard = New Game/Reset
  for ( byte i = 0; i < BOARDSIZE; i++ )
  {
    Board[CURRENT][i] = 65535 - 32768;
  }

}

void loop() {
  
  // Pause until next frame
  if (!(arduboy.nextFrame())) return;

  // Clearing Screen
  arduboy.clear();

  // New Button Inputs
  prevButtons = currButtons;
  currButtons = arduboy.buttonsState();

  // Moving in the same direction, start a delay
  if( prevButtons == currButtons && ( currButtons & ( LEFT_BUTTON + UP_BUTTON + RIGHT_BUTTON + DOWN_BUTTON )) ){
    repeatDelay++;
  }
  // Release or change in buttons restarts delay
  else{
    repeatDelay = 0;
  }
  
  // Checks Inputs only if Inputs change, or repeatdelay exceeds max
  if(( currButtons != prevButtons || repeatDelay > REPEATDELAY ) ){

    // Moving the Cursor // Selecting Board
    if( currButtons & UP_BUTTON ){
      if(  displayMode > TITLESCREEN && yPos > 0){
        yPos--;
      }
    }
    if( currButtons & DOWN_BUTTON ){
      if(  displayMode > TITLESCREEN && !(yPos >= BOARDSIZE - 1) ){
        yPos++;
      }
    }
    if( currButtons & LEFT_BUTTON ){
      if(  displayMode > TITLESCREEN && xPos > 0 ){
        xPos --;
      }

      if( boardSelect > 0 && displayMode == TITLESCREEN ){
        boardSelect--;
      }

    }
    if( currButtons & RIGHT_BUTTON ){
      if(  displayMode > TITLESCREEN && !(xPos >= BOARDSIZE - 1) ){
        xPos ++;
      }

      if( boardSelect < NUMBEROFBOARDS  && displayMode == TITLESCREEN ){
        boardSelect++;
      }
    }

    //Pressing Left and Right together will change current screen in debug mode
    if( currButtons & LEFT_BUTTON && currButtons & RIGHT_BUTTON && DEBUG ){
      
      displayMode++;
      displayMode = displayMode % 5;
      
    }

    // Flipping piece of the Current Game Board with A/B
    if( currButtons & A_BUTTON ){
      if( displayMode > TITLESCREEN ){
        //Clears Bit xPos in byte yPos
        Board[CURRENT][yPos] &= ~( 1 << xPos );
      }
    }
    if( currButtons & B_BUTTON ){
      if( displayMode > TITLESCREEN ){
        //Sets Bit xPos in byte yPos
        Board[CURRENT][yPos] |= ( 1 << xPos );
      }
    }

    if( currButtons & A_BUTTON || currButtons & B_BUTTON ){
      
      if( displayMode == TITLESCREEN ){

        //DEBUG Stuff
        if( boardSelect == NUMBEROFBOARDS ){
          DEBUG = true;
        }

        //Load Selected board
        else{
          loadBoard( boardSelect );
          // Going to Normal Gameplay
          displayMode++;
        } 
      }

      if( gamewon ){
        displayMode = TITLESCREEN + DEBUG;
        gamewon = false;
        setup();
      }

    }

  }

  //Determines what to display
  switch (displayMode){

    case TITLESCREEN:

      //Print Title of Game
      arduboy.setCursor(38,16);
      arduboy.print("ARDUCROSS");
      //Debug Indicator
      if( DEBUG ){
        arduboy.setCursor( 24, 8);
        arduboy.print("DEBUG");
      }
      // Board Select or Enter Debug
      arduboy.setCursor(60,32);
      if( boardSelect == NUMBEROFBOARDS )
        arduboy.print("DEBUG");
      else
        arduboy.print(boardSelect);

      break;

    //Game in Progress, Show necessary values
    case DEBUGBOARD:
    case ROWVALUE:
    case COMPLETEVALUE:
    case COMPLETEBOARDHINT:

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

      // Feedback on Board Comolete
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

        for( byte i = 0; i < BOARDSIZE; i++ ){

          //Checking Horizontal
          //Increase if successive bits cleared in a byte
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
          //Same as horizontal, but checks xPos bit on all bytes
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
      
      printBoard( CURRENT );
      //Draw Cursor with magic numbers
      arduboy.drawRect( ( xPos * 4) + 63, ( yPos * 4 ) , 5, 5, WHITE);

      break;
    
  }

  // Display everything printed
  arduboy.display();
}
