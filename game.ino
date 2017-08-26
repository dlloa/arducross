#include <Arduboy2.h>
#include "boards.h"

//TODO
// + Truncate Hint lists for smaller boards
// + Auto Clear Row/Column in Board[CURRENT] if Board[COMPLETE] has values of 0 or all xth bits cleared
// + Increase size of board if playing with 5x5 or 10x10
// - Better Title Screen
// - Add more Picross

// + Give Up 
// - Timer

// Arduboy Stuff
Arduboy2 arduboy;
#define FRAMERATE 20
bool DEBUG = false;

// Board Constants 
#define BLOCKSIZE 3
#define BORDERSIZE 1
#define MAXBOARDSIZE 16 // 15 x 15 Standard, 16th is the size of the board
#define REPEATDELAY 4

// displayMode Constants
#define TITLESCREEN 0 
#define COMPLETEBOARDHINT 1 // Gameplay - Normal Hints on Left
#define DEBUGBOARD 2 // Gameplay - Hints for Onscreen Board (CURRENT)
#define ROWVALUE 3 // Gameplay - uint16 Values on Screen
#define COMPLETEVALUE 4 // Gameplay - uint16 Values of Completed Board
byte displayMode = TITLESCREEN + DEBUG;

// Board Stuff
#define COMPLETE 0
#define CURRENT 1
uint16_t Board[2][MAXBOARDSIZE];
byte boardSelect, boardSize = 0;

// Current Position of the Chisel
byte xPos, yPos = 0;

// Button States
byte currButtons, prevButtons;

// For Quicker Movement
byte repeatDelay = 0;
// 
byte giveupDelay = 0;

// Complete Board Feedback
bool gamewon;

void printBoard( byte boardToPrint ){
  gamewon = true;
  // Iterate through each uint16

  //Quick add for increasing blocksize for smaller boards
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

// Loads a board from PROGMEM
void loadBoard( byte toload ){
  // Load 16th value for boardsize
  boardSize = pgm_read_word_near( (toload * MAXBOARDSIZE) + LOADEDBOARD + 15 );

  for( byte i = 0; i < boardSize; i++ ){
    // PROGMEM read with pgm_read_word_near( )
    Board[COMPLETE][i] = pgm_read_word_near( (toload * MAXBOARDSIZE ) + LOADEDBOARD + i );
  }
}

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

void setup() {
  
  // Arduboy Setup
  arduboy.begin();
  arduboy.initRandomSeed();
  arduboy.setFrameRate(FRAMERATE);

  // Game Reset
  displayMode = TITLESCREEN + DEBUG;
  gamewon = false;
  xPos = 0;
  yPos = 0;

  //Fill Current Gameboard = New Game/Reset
  for ( byte i = 0; i < MAXBOARDSIZE; i++ )
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
  // Give up delay, same as above, but for ending a board
  if( prevButtons == currButtons && (currButtons & A_BUTTON) && (currButtons & B_BUTTON) ){
    giveupDelay++;
    //At half a second, give up
    if( giveupDelay == 20 ){
      // Give up and show complete board
      for( byte i = 0; i < boardSize; i++ ){
        Board[CURRENT][i] = Board[COMPLETE][i];
      }
    }
  }
  else{
    giveupDelay = 0;
  }
  
  // Checks Inputs only if Inputs change, or repeatdelay exceeds max
  if( ( currButtons != prevButtons || repeatDelay > REPEATDELAY ) ){

    // Moving the Cursor // Selecting Board
    if( currButtons & UP_BUTTON ){
      if(  displayMode > TITLESCREEN && yPos > 0){
        yPos--;
      }
    }
    if( currButtons & DOWN_BUTTON ){
      if(  displayMode > TITLESCREEN && !(yPos >= boardSize - 1) ){
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
      if(  displayMode > TITLESCREEN && !(xPos >= boardSize - 1) ){
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
        //Clears Bit xPos in uint16 yPos
        Board[CURRENT][yPos] &= ~( 1 << xPos );
      }
    }
    if( currButtons & B_BUTTON ){
      if( displayMode > TITLESCREEN ){
        //Sets Bit xPos in uint16 yPos
        Board[CURRENT][yPos] |= ( 1 << xPos );
      }



    }

    if( (currButtons & A_BUTTON || currButtons & B_BUTTON) ){
      
      if( displayMode == TITLESCREEN ){

        //DEBUG Stuff
        if( boardSelect == NUMBEROFBOARDS ){
          DEBUG = true;
        }

        //Load Selected board
        else{
          xPos = 0;
          yPos = 0;
          loadBoard( boardSelect );
          // Going to Normal Gameplay
          displayMode++;
        } 
      }

      if( gamewon && prevButtons == 0 ) {

        // Game Reset
        displayMode = TITLESCREEN + DEBUG;
        gamewon = false;
        xPos = 0;
        yPos = 0;
        //Fill Current Gameboard = New Game/Reset
        for ( byte i = 0; i < MAXBOARDSIZE; i++ )
        {
          Board[CURRENT][i] = 65535 - 32768;
        }
        //setup();
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

      byte SHOWCOMPLETEDEBUG = 0;
      if( displayMode == COMPLETEVALUE ){
        SHOWCOMPLETEDEBUG = 1;
      }
      printBoard( CURRENT - SHOWCOMPLETEDEBUG );
      break;
    
  }

  // Display everything printed
  arduboy.display();
}
