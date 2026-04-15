#pragma once
#include "departure.h"

void displayInit();
void displayBoot(const char* statusLine);
void displayRender(Departure* deps, int count);
void displayError(const char* message);
void displayPortal(const char* apName);
