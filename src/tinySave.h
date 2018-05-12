#pragma once
#include <stdint.h>
#include "roData.h"

// Holds a minified saved game file
class TinySave {
public:
	uint8_t buffer[0x10000];
	uint32_t size;

	void compress(const ROSavedGame* src);
	bool decompress(ROSavedGame *dest);
};