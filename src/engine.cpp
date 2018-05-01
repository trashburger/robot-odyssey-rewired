#include <stdint.h>
#include <emscripten.h>

extern "C" uint32_t EMSCRIPTEN_KEEPALIVE increment(uint32_t a) {
	return a+1;
}