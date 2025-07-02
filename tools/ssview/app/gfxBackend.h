#pragma once

void
appBackendInit();

bool
appBackendDraw();  // Return true is something has been drawn

bool
appCaptureScreen(int* width, int* height, uint8_t** buffer);

void
appBackendUninit();
