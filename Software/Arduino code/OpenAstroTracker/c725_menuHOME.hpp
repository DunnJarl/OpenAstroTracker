#pragma once

#include "b_setup.hpp"

#if HEADLESS_CLIENT == 0
byte subGoIndex = 0;

bool processHomeKeys() {
  byte key;
  bool waitForRelease = false;

  if (lcdButtons.keyChanged(&key)) {
    waitForRelease = true;
    switch (key) {
      case btnSELECT: {
        if (subGoIndex == 0) {
          mount.goHome(); 
        }
        else {
          mount.park();
        }
      }
      break;

      case btnUP:
      case btnDOWN:
      case btnLEFT: {
        subGoIndex = 1 - subGoIndex;
      }
      break;

      case btnRIGHT: {
        lcdMenu.setNextActive();
      }
      break;
    }
  }

  return waitForRelease;
}

void printHomeSubmenu() {
  char scratchBuffer[16];
  if (mount.isParked() && (subGoIndex == 1)) {
    lcdMenu.printMenu("Parked...");
  }
  else {
    strcpy(scratchBuffer, " Home  Park");
    scratchBuffer[subGoIndex * 6] = '>';
    lcdMenu.printMenu(scratchBuffer);
  }
}

#endif
