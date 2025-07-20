/*
 * @file platform_web.c
 * @brief Implementation of platform-specific functions for web (Emscripten)
 * environment.
 */

#include <emscripten.h> // Required for emscripten_sleep and EM_ASM_INT macros

extern struct forth_ctx ctx;
int emforth_web_getchar()
{
	while (1) {
		/*
		 * Execute inline JavaScript to request a character from the
		 * `Module.stdin` function defined in `template.html`.
		 *
		 * - If `Module.stdin()` returns `undefined` (JS buffer is
		 * empty), we treat this as a signal to yield and return a
		 * sentinel value (-2).
		 * - Otherwise, return the character code received from JS.
		 */
		int ch = EM_ASM_INT({
			var charCode = Module.stdin();
			if (charCode == = undefined) {
				/* Sentinel value: indicates that C should yield
				 */
				return -2;
			}
			/* Return the character received from JS */
			return charCode;
		});

		if (ch == -2) {
			/*
			 * No character available from JavaScript's input
			 * buffer. Call emscripten_sleep(0) to explicitly yield
			 * control back to the browser's event loop. ASYNCIFY
			 * will suspend the C stack at this point, and resume
			 * when JS puts data into the input buffer and
			 * Module.stdin() can return a character.
			 */
			emscripten_sleep(0); /* Sleep for 0ms, effectively a
						non-blocking yield */
		} else {
			/*
			 * A character has been received from JavaScript.
			 * Return it to the calling C function.
			 */
			return ch;
		}
	}
}
