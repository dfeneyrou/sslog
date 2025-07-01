#pragma once

void
vwBackendInit();

bool
vwBackendDraw();  // Return true is something has been drawn

bool
vwCaptureScreen(int* width, int* height, uint8_t** buffer);

void
vwBackendUninit();
