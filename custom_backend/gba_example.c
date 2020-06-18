// may need devkitpro / devkitarm
// https://github.com/JamieDStewart/GBA_VSCode_Basic

#include "gba/fbg_gba.h"

int main()
{
	struct _fbg *fbg = fbg_gbaSetup(3);

	fbg_line(fbg, 0, 0, fbg->width, fbg->height, 255, 0, 0);

	while (1) {
		fbg_draw(fbg);
	}

	return 0;
}