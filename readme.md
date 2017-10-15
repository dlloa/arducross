<h1>Arducross - Picross for Arduboy</h1>
Very simple Picross clone. Currently only has 22 boards.</br>

https://community.arduboy.com/t/wip-arducross-picross-for-arduboy/3928

**How to Play** </br>
http://www.thonky.com/picross/

**Controls - Title Screen** </br>
Up/Down: Change Option </br>
Left/Right: Select board to play </br>
A/B: Select Option </br>

**Controls - Gameplay** </br>
D-Pad: Moves Chisel </br>
A: Clears Block </br>
B: Sets Block </br>
Left+Right 2 Seconds: Pause Menu </br>

**Controls - Pause Screen** </br>
D-Pad: Change Option
A/B: Select Option

** SAVING **
Pausing and Saving is now an option. </br>
Saving WILL OVERWRITE the first 34 BYTES from EEPROM_STORAGE_SPACE_START. </br>

BYTE EEPROM_STORAGE_SPACE_START + 1: Version and Status </br>
BYTE EEPROM_STORAGE_SPACE_START + 2: Board Number </br>
BYTE EEPROM_STORAGE_SPACE_START + 3-34: Board Status </br>

OK! is displayed when you complete the board. Pressing A/B after restarts the game.

**Controls - Debug Mode**</br>
Left + Right: Changes screen</br>
0 - Title Screen</br>
1 - Normal game with hints</br>
2 - Hints are for current on screen board</br>
3 - Shows Byte values for each row of on screen board</br>
4 - Shows Byte values for each row of complete board</br>
