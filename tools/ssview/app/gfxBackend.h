#pragma once

void
vwBackendInit();

void
vwBackendInstallFont(const void* fontData, int fontDataSize, int fontSize);

bool
vwBackendDraw();  // Return true is something has been drawn

bool
vwCaptureScreen(int* width, int* height, uint8_t** buffer);

void
vwBackendUninit();
