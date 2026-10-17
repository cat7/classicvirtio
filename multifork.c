/* Copyright (c) 2023 Elliot Nunn */
/* Licensed under the MIT license */

#include <string.h>
#include "printf.h"
#include "multifork.h"

struct MFImpl MF;

void MFChoose(const char *suggest) {
	if (suggest && !strcmp(suggest, "3")) {
		MF = MF3; // .idump/.rdump format
	} else {
		MF = MF1; // default: xattr-based metadata
	}
}
