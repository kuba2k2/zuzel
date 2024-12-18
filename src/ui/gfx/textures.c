// Copyright (c) Kuba Szczodrzyński 2024-12-18.

#include "gfx.h"

void texture_load(SDL_Renderer *renderer, SDL_Texture **texture, int width, int height, const uint8_t *data) {
	*texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureScaleMode(*texture, SDL_ScaleModeNearest);

	uint32_t *pixels;
	int pitch;
	SDL_LockTexture(*texture, NULL, (void **)&pixels, &pitch);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint8_t pixel = *data++;
			if (pixel & 0x80)
				pixels[y * width + x] = 0xFFFFFF00 | (pixel & 0x7F);
			else
				pixels[y * width + x] = 0x00000000 | (pixel & 0x7F);
		}
	}
	SDL_UnlockTexture(*texture);
}

SDL_Texture *texture_button_face		 = NULL;
const int texture_button_face_width		 = 198;
const int texture_button_face_height	 = 18;
const uint8_t texture_button_face_data[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00,
	0x83, 0x00, 0x83, 0x00, 0x03, 0x04, 0x03, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0x00, 0x00, 0x00,
	0x00, 0x04, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86,
	0x86, 0x00, 0x04, 0x06, 0x03, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x04, 0x03, 0x03, 0x03, 0x00, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	0x03, 0x00, 0x00, 0x00, 0x04, 0x03, 0x00, 0x03, 0x04, 0x03, 0x03, 0x86, 0x86, 0x03, 0x00, 0x86, 0x03, 0x00, 0x00,
	0x00, 0x83, 0x04, 0x03, 0x00, 0x03, 0x04, 0x00, 0x00, 0x03, 0x06, 0x04, 0x04, 0x09, 0x03, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x83, 0x03, 0x03, 0x00, 0x03, 0x06, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x86, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x06, 0x00, 0x00, 0x06, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA7, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x09, 0x82, 0x09,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x02, 0x02, 0x09, 0x09, 0x10, 0x07, 0x05, 0x07, 0x82, 0x82, 0x09, 0x02,
	0x02, 0x02, 0x85, 0x82, 0x09, 0x05, 0x05, 0x0B, 0x05, 0x02, 0x02, 0x05, 0x0B, 0x10, 0x07, 0x02, 0x02, 0x02, 0x07,
	0x02, 0x02, 0x00, 0x02, 0x05, 0x0B, 0x89, 0x82, 0x02, 0x02, 0x00, 0x87, 0x00, 0x02, 0x05, 0x09, 0x82, 0x00, 0x00,
	0x02, 0x09, 0x02, 0x02, 0x07, 0x02, 0x02, 0x07, 0x02, 0x07, 0x10, 0x09, 0x00, 0x89, 0x89, 0x02, 0x09, 0x07, 0x09,
	0x02, 0x00, 0x07, 0x0E, 0x09, 0x02, 0x02, 0x82, 0x02, 0x02, 0x82, 0x00, 0x0B, 0x07, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x07, 0x02, 0x00, 0x85, 0x0E, 0x05, 0x82, 0x02, 0x02, 0x02, 0x07, 0x07, 0x09, 0x0B, 0x02, 0x87, 0x00,
	0x02, 0x02, 0x0B, 0x05, 0x02, 0x82, 0x00, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x02, 0x09, 0x07, 0x07, 0x02,
	0x87, 0x00, 0x00, 0x8B, 0x82, 0x02, 0x02, 0x85, 0x00, 0x82, 0x02, 0x07, 0x02, 0x02, 0x05, 0x05, 0x89, 0x02, 0x0E,
	0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x05, 0x02, 0x02, 0x85, 0x02, 0x02, 0x02, 0x02, 0x85, 0x85, 0x02, 0x02, 0x02,
	0x05, 0x87, 0x89, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x02, 0x00, 0x00, 0x89, 0x09, 0x12, 0x09, 0x00, 0x05, 0x0B,
	0x02, 0x02, 0x02, 0x07, 0x82, 0x02, 0x02, 0x02, 0x02, 0x00, 0x87, 0x02, 0x07, 0x10, 0x07, 0x02, 0x00, 0x85, 0x02,
	0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x05, 0x05, 0x0B, 0x10, 0x0E, 0x82, 0x02,
	0x07, 0x0E, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x89, 0x02, 0x07, 0x0B, 0x10, 0x0B, 0x0E, 0x0B, 0x05,
	0x02, 0x02, 0x07, 0x10, 0x05, 0x02, 0x02, 0x02, 0x02, 0x00, 0x85, 0x09, 0x85, 0x00, 0x02, 0x82, 0x89, 0x00, 0x02,
	0x82, 0x8E, 0x02, 0x0E, 0x07, 0x02, 0x02, 0x07, 0x12, 0x07, 0x05, 0x09, 0x05, 0x02, 0x05, 0x09, 0x09, 0x02, 0x85,
	0x00, 0x82, 0x02, 0x00, 0x8C, 0x8B, 0x00, 0x07, 0x0E, 0x07, 0x02, 0x02, 0x00, 0x05, 0x82, 0x02, 0x07, 0x0B, 0x82,
	0x89, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x09, 0x05, 0x00, 0x87, 0x00, 0x02, 0x02, 0x05,
	0x07, 0x82, 0x02, 0x00, 0x89, 0x82, 0x07, 0x10, 0x07, 0x00, 0x02, 0x02, 0x07, 0x12, 0x09, 0x07, 0x82, 0x89, 0x00,
	0x07, 0x0E, 0x09, 0x09, 0x05, 0x02, 0x07, 0x02, 0x82, 0x02, 0x02, 0x07, 0x02, 0x00, 0x85, 0x02, 0x0B, 0x05, 0x02,
	0x02, 0x82, 0x02, 0x02, 0x02, 0x00, 0x8B, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02,
	0x02, 0x02, 0x02, 0x09, 0x00, 0x03, 0x02, 0x00, 0x09, 0x05, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02, 0x07, 0x09, 0x05,
	0x09, 0x19, 0x09, 0x02, 0x07, 0x0E, 0x07, 0x07, 0x17, 0x10, 0x05, 0x00, 0x85, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x05, 0x07, 0x87, 0x00, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x0B, 0x05, 0x02, 0x02, 0x07, 0x09, 0x12, 0x07, 0x02,
	0x07, 0x10, 0x09, 0x02, 0x05, 0x02, 0x05, 0x09, 0x05, 0x02, 0x02, 0x02, 0x02, 0x07, 0x0E, 0x05, 0x00, 0x07, 0x09,
	0x02, 0x02, 0x87, 0x89, 0x89, 0x02, 0x0E, 0x07, 0x02, 0x82, 0x87, 0x02, 0x09, 0x05, 0x05, 0x0B, 0x10, 0x07, 0x02,
	0x02, 0x05, 0x02, 0x02, 0x02, 0x05, 0x0B, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x09, 0x05, 0x02, 0x02, 0x02, 0x00,
	0x87, 0x00, 0x00, 0x87, 0x82, 0x85, 0x00, 0x02, 0x02, 0x07, 0x02, 0x82, 0x09, 0x07, 0x09, 0x05, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x85, 0x07, 0x07, 0x0E, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x87, 0x00,
	0x02, 0x02, 0x07, 0x02, 0x02, 0x00, 0x87, 0x82, 0x05, 0x0E, 0x07, 0x02, 0x02, 0x02, 0x02, 0x07, 0x17, 0x10, 0x07,
	0x02, 0x85, 0x02, 0x09, 0x05, 0x02, 0x02, 0x82, 0x02, 0x02, 0x00, 0x89, 0x05, 0x07, 0x02, 0x00, 0x82, 0x0B, 0x05,
	0x05, 0x82, 0x85, 0x82, 0x09, 0x05, 0x8E, 0x82, 0x82, 0x89, 0x07, 0x10, 0x03, 0x00, 0x02, 0x00, 0x82, 0x0B, 0x02,
	0x82, 0x02, 0x07, 0x0E, 0x05, 0x02, 0x02, 0x02, 0x85, 0x82, 0x00, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07,
	0x02, 0x82, 0x09, 0x09, 0x0E, 0x0E, 0x0E, 0x00, 0x89, 0x82, 0x8B, 0x00, 0x02, 0x02, 0x05, 0x09, 0x02, 0x02, 0x07,
	0x02, 0x07, 0x12, 0x07, 0x07, 0x10, 0x0B, 0x05, 0x09, 0x12, 0x09, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02, 0x00, 0x87,
	0x02, 0x07, 0x09, 0x02, 0x02, 0x05, 0x07, 0x85, 0x00, 0x85, 0x94, 0x85, 0x02, 0x02, 0x07, 0x02, 0x00, 0x02, 0x02,
	0x02, 0x05, 0x0B, 0x09, 0x05, 0x02, 0x02, 0x02, 0x0B, 0x05, 0x02, 0x02, 0x02, 0x02, 0x05, 0x0E, 0x0E, 0x07, 0x02,
	0x02, 0x09, 0x87, 0x02, 0x05, 0x02, 0x85, 0x82, 0x89, 0x82, 0x05, 0x10, 0x10, 0x09, 0x02, 0x02, 0x02, 0x02, 0x05,
	0x0B, 0x09, 0x02, 0x00, 0x85, 0x82, 0x02, 0x02, 0x02, 0x07, 0x02, 0x07, 0x10, 0x07, 0x05, 0x02, 0x02, 0x85, 0x02,
	0x05, 0x05, 0x09, 0x02, 0x02, 0x02, 0x02, 0x07, 0x12, 0x09, 0x02, 0x02, 0x02, 0x02, 0x07, 0x02, 0x85, 0x00, 0x02,
	0x02, 0x02, 0x00, 0x07, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x00, 0x02, 0x0B, 0x09, 0x10,
	0x09, 0x05, 0x07, 0x07, 0x0B, 0x07, 0x09, 0x07, 0x09, 0x05, 0x02, 0x02, 0x02, 0x0B, 0x05, 0x02, 0x05, 0x0B, 0x07,
	0x05, 0x86, 0x06, 0x05, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x0B, 0x05, 0x02, 0x02, 0x02, 0x02, 0x05, 0x0B, 0x02,
	0x02, 0x02, 0x02, 0x05, 0x07, 0x82, 0x02, 0x02, 0x09, 0x12, 0x0B, 0x02, 0x82, 0x82, 0x02, 0x02, 0x00, 0x00, 0x87,
	0x00, 0x82, 0x87, 0x02, 0x0B, 0x05, 0x02, 0x02, 0x02, 0x02, 0x82, 0x00, 0x00, 0x07, 0x02, 0x87, 0x02, 0x02, 0x82,
	0x0B, 0x0B, 0x07, 0x02, 0x02, 0x02, 0x05, 0x0E, 0x19, 0x07, 0x02, 0x02, 0x02, 0x00, 0x02, 0x07, 0x00, 0x87, 0x00,
	0x02, 0x00, 0x87, 0x02, 0x09, 0x02, 0x05, 0x02, 0x02, 0x00, 0x89, 0x82, 0x07, 0x07, 0x09, 0x05, 0x02, 0x02, 0x02,
	0x02, 0x07, 0x02, 0x02, 0x09, 0x10, 0x07, 0x02, 0x00, 0x89, 0x00, 0x02, 0x02, 0x09, 0x09, 0x02, 0x05, 0x0B, 0x09,
	0x05, 0x02, 0x02, 0x07, 0x05, 0x07, 0x09, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x85, 0x02,
	0x00, 0x00, 0x02, 0x07, 0x0E, 0x05, 0x00, 0x8B, 0x89, 0x87, 0x00, 0x07, 0x0B, 0x02, 0x07, 0x02, 0x02, 0x02, 0x0E,
	0x02, 0x02, 0x07, 0x10, 0x0B, 0x05, 0x09, 0x1B, 0x0E, 0x07, 0x0E, 0x07, 0x05, 0x0B, 0x0E, 0x07, 0x02, 0x02, 0x02,
	0x05, 0x02, 0x02, 0x02, 0x02, 0x07, 0x0E, 0x0B, 0x12, 0x07, 0x02, 0x02, 0x07, 0x02, 0x00, 0x85, 0x0B, 0x07, 0x02,
	0x02, 0x07, 0x07, 0x10, 0x05, 0x02, 0x07, 0x05, 0x07, 0x82, 0x00, 0x05, 0x09, 0x02, 0x82, 0x85, 0x85, 0x00, 0x00,
	0x89, 0x02, 0x09, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x02, 0x07, 0x12, 0x07, 0x02, 0x02,
	0x00, 0x07, 0x02, 0x02, 0x09, 0x19, 0x0B, 0x0E, 0x07, 0x02, 0x02, 0x00, 0x89, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x00, 0x89, 0x90, 0x85, 0x02, 0x02, 0x05, 0x02, 0x82, 0x85, 0x87, 0x00, 0x02, 0x00, 0x87, 0x02, 0x09, 0x82,
	0x07, 0x17, 0x09, 0x00, 0x87, 0x00, 0x02, 0x02, 0x00, 0x8B, 0x85, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x89,
	0x82, 0x89, 0x00, 0x00, 0x02, 0x02, 0x07, 0x12, 0x12, 0x82, 0x82, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x82, 0x00, 0x85, 0x02, 0x07, 0x07, 0x0B, 0x05, 0x05, 0x02, 0x02, 0x02, 0x00, 0x87, 0x00, 0x02, 0x82, 0x89, 0x94,
	0x82, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x02, 0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x00, 0x87, 0x82, 0x09, 0x07, 0x09, 0x05, 0x02, 0x82, 0x02, 0x82, 0x02, 0x02, 0x85, 0x00,
	0x02, 0x02, 0x02, 0x00, 0x85, 0x00, 0x85, 0x02, 0x02, 0x02, 0x00, 0x00, 0x02, 0x02, 0x00, 0x82, 0x87, 0x00, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x85, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x0B, 0x07, 0x02, 0x82, 0x00,
	0x09, 0x05, 0x07, 0x10, 0x05, 0x02, 0x02, 0x02, 0x00, 0x85, 0x02, 0x07, 0x09, 0x09, 0x05, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x07, 0x02, 0x09, 0x05, 0x02, 0x05, 0x0B, 0x07, 0x07, 0x02, 0x82, 0x8B, 0x00, 0x09, 0x02, 0x82,
	0x02, 0x02, 0x00, 0x85, 0x05, 0x02, 0x02, 0x05, 0x82, 0x05, 0x0E, 0x07, 0x02, 0x02, 0x82, 0x02, 0x07, 0x05, 0x89,
	0x00, 0x02, 0x02, 0x02, 0x05, 0x09, 0x05, 0x02, 0x02, 0x02, 0x05, 0x0B, 0x10, 0x07, 0x07, 0x0E, 0x07, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x09, 0x09, 0x07, 0x02, 0x02, 0x82,
	0x82, 0x00, 0x02, 0x09, 0x09, 0x09, 0x05, 0x09, 0x17, 0x0B, 0x10, 0x02, 0x85, 0x00, 0x02, 0x00, 0x87, 0x82, 0x09,
	0x07, 0x07, 0x02, 0x07, 0x10, 0x10, 0x07, 0x07, 0x82, 0x82, 0x02, 0x05, 0x07, 0x07, 0x02, 0x85, 0x82, 0x87, 0x00,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x10, 0x05, 0x85, 0x82, 0x00, 0x02, 0x02, 0x02, 0x02, 0x07, 0x02, 0x85,
	0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E, 0x07, 0x05, 0x09, 0x05, 0x02, 0x02, 0x05, 0x09, 0x09, 0x02,
	0x02, 0x02, 0x07, 0x02, 0x09, 0x19, 0x12, 0x07, 0x05, 0x00, 0x8C, 0x00, 0x09, 0x05, 0x02, 0x02, 0x02, 0x82, 0x05,
	0x82, 0x89, 0x02, 0x0E, 0x07, 0x02, 0x82, 0x83, 0x02, 0x02, 0x02, 0x05, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00,
	0x87, 0x00, 0x02, 0x02, 0x02, 0x05, 0x09, 0x02, 0x00, 0x02, 0x07, 0x10, 0x09, 0x82, 0x82, 0x89, 0x87, 0x02, 0x02,
	0x87, 0x89, 0x82, 0x82, 0x82, 0x89, 0x8E, 0x82, 0x82, 0x0B, 0x07, 0x09, 0x09, 0x82, 0x00, 0x85, 0x94, 0x8E, 0x89,
	0x00, 0x09, 0x09, 0x02, 0x05, 0x87, 0x00, 0x82, 0x87, 0x02, 0x02, 0x02, 0x05, 0x09, 0x07, 0x0B, 0x09, 0x09, 0x00,
	0x02, 0x82, 0x00, 0x02, 0x02, 0x02, 0x02, 0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x0B, 0x0B,
	0x02, 0x02, 0x02, 0x02, 0x07, 0x12, 0x09, 0x02, 0x02, 0x85, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x09, 0x10,
	0x09, 0x09, 0x02, 0x09, 0x02, 0x82, 0x89, 0x00, 0x02, 0x02, 0x07, 0x12, 0x02, 0x89, 0x00, 0x02, 0x02, 0x82, 0x90,
	0x82, 0x02, 0x00, 0x87, 0x00, 0x02, 0x07, 0x09, 0x09, 0x00, 0x02, 0x02, 0x82, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00,
	0x02, 0x02, 0x02, 0x02, 0x05, 0x09, 0x05, 0x02, 0x00, 0x87, 0x00, 0x07, 0x0B, 0x02, 0x00, 0x89, 0x89, 0x89, 0x85,
	0x82, 0x8C, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x09, 0x05, 0x02, 0x02, 0x05, 0x00, 0x02, 0x02, 0x07,
	0x02, 0x07, 0x0E, 0x07, 0x02, 0x00, 0x8B, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x10, 0x12, 0x00, 0x05, 0x00, 0x8C,
	0x82, 0x85, 0x8B, 0x00, 0x02, 0x02, 0x07, 0x02, 0x82, 0x00, 0x85, 0x07, 0x82, 0x8B, 0x00, 0x02, 0x02, 0x02, 0x02,
	0x07, 0x0B, 0x82, 0x02, 0x02, 0x05, 0x02, 0x02, 0x07, 0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x07, 0x02, 0x02, 0x02,
	0x02, 0x82, 0x05, 0x0B, 0x82, 0x00, 0x02, 0x00, 0x89, 0x00, 0x02, 0x02, 0x00, 0x8B, 0x02, 0x12, 0x07, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x07, 0x02, 0x02, 0x00, 0x8B, 0x00, 0x02, 0x02, 0x02, 0x07, 0x05, 0x07, 0x00, 0x8B, 0x89, 0x8E,
	0x94, 0x89, 0x02, 0x02, 0x00, 0x8B, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x0B, 0x02, 0x89,
	0x00, 0x02, 0x00, 0x89, 0x82, 0x02, 0x07, 0x02, 0x05, 0x09, 0x07, 0x07, 0x05, 0x02, 0x02, 0x85, 0x05, 0x10, 0x05,
	0x82, 0x02, 0x05, 0x09, 0x05, 0x02, 0x89, 0x85, 0x02, 0x07, 0x09, 0x82, 0x05, 0x02, 0x02, 0x02, 0x05, 0x02, 0x02,
	0x00, 0x09, 0x89, 0x00, 0x02, 0x82, 0x02, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02, 0x05, 0x07, 0x82, 0x02, 0x02, 0x09,
	0x19, 0x09, 0x07, 0x12, 0x07, 0x82, 0x02, 0x00, 0x85, 0x09, 0x07, 0x09, 0x02, 0x82, 0x02, 0x07, 0x02, 0x02, 0x02,
	0x02, 0x07, 0x02, 0x09, 0x19, 0x09, 0x05, 0x09, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x85, 0x02, 0x02,
	0x07, 0x07, 0x09, 0x82, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x02, 0x09, 0x09, 0x05, 0x02, 0x07, 0x17, 0x09, 0x02,
	0x02, 0x07, 0x10, 0x09, 0x82, 0x82, 0x02, 0x02, 0x82, 0x02, 0x05, 0x02, 0x00, 0x8B, 0x90, 0x82, 0x02, 0x00, 0x85,
	0x00, 0x85, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x07, 0x02, 0x82, 0x02, 0x00, 0x85, 0x00, 0x02,
	0x00, 0x02, 0x02, 0x02, 0x00, 0x02, 0x05, 0x0B, 0x0E, 0x05, 0x02, 0x07, 0x0B, 0x02, 0x85, 0x82, 0x89, 0x09, 0x07,
	0x02, 0x02, 0x82, 0x87, 0x82, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07, 0x05, 0x02, 0x89, 0x82, 0x82,
	0x09, 0x09, 0x87, 0x00, 0x02, 0x02, 0x07, 0x10, 0x07, 0x07, 0x05, 0x0B, 0x10, 0x05, 0x00, 0x0B, 0x02, 0x09, 0x0B,
	0x05, 0x02, 0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x02, 0x82, 0x10, 0x09, 0x02, 0x02, 0x02,
	0x02, 0x05, 0x09, 0x05, 0x05, 0x09, 0x02, 0x82, 0x10, 0x89, 0x07, 0x02, 0x82, 0x09, 0x02, 0x02, 0x02, 0x07, 0x02,
	0x02, 0x02, 0x82, 0x00, 0x82, 0x82, 0x89, 0x00, 0x07, 0x0B, 0x09, 0x02, 0x02, 0x09, 0x09, 0x07, 0x02, 0x07, 0x02,
	0x05, 0x09, 0x05, 0x05, 0x07, 0x85, 0x00, 0x02, 0x02, 0x00, 0x87, 0x85, 0x00, 0x02, 0x02, 0x02, 0x82, 0x00, 0x00,
	0x02, 0x07, 0x10, 0x10, 0x07, 0x07, 0x19, 0x12, 0x02, 0x89, 0x00, 0x07, 0x82, 0x02, 0x02, 0x02, 0x02, 0x0E, 0x07,
	0x07, 0x02, 0x00, 0x87, 0x00, 0x05, 0x0B, 0x07, 0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x00, 0x00, 0x82, 0x02, 0x87, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x90, 0x85, 0x87, 0x00, 0x00,
	0x82, 0x82, 0x02, 0x02, 0x02, 0x82, 0x02, 0x85, 0x00, 0x00, 0x02, 0x09, 0x05, 0x02, 0x00, 0x89, 0x85, 0x82, 0x02,
	0x07, 0x82, 0x00, 0x02, 0x09, 0x02, 0x09, 0x02, 0x00, 0x85, 0x02, 0x09, 0x02, 0x02, 0x09, 0x17, 0x05, 0x82, 0x10,
	0x10, 0x0E, 0x07, 0x02, 0x82, 0x02, 0x02, 0x85, 0x05, 0x0B, 0x05, 0x02, 0x07, 0x09, 0x02, 0x82, 0x02, 0x02, 0x02,
	0x02, 0x85, 0x00, 0x02, 0x02, 0x82, 0x02, 0x02, 0x07, 0x82, 0x00, 0x00, 0x09, 0x05, 0x02, 0x02, 0x05, 0x02, 0x09,
	0x07, 0x02, 0x02, 0x02, 0x07, 0x0E, 0x82, 0x00, 0x87, 0x8B, 0x87, 0x02, 0x09, 0x19, 0x09, 0x02, 0x02, 0x07, 0x02,
	0x82, 0x90, 0x87, 0x00, 0x89, 0x89, 0x85, 0x02, 0x82, 0x02, 0x02, 0x05, 0x09, 0x05, 0x02, 0x07, 0x0E, 0x05, 0x02,
	0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x07, 0x10, 0x07, 0x02, 0x07, 0x09, 0x05, 0x02, 0x00, 0x02, 0x02, 0x07,
	0x12, 0x00, 0x82, 0x02, 0x02, 0x07, 0x0B, 0x82, 0x02, 0x02, 0x07, 0x05, 0x02, 0x82, 0x02, 0x02, 0x02, 0x07, 0x0B,
	0x07, 0x02, 0x02, 0x85, 0x00, 0x00, 0x02, 0x00, 0x0E, 0x09, 0x00, 0x05, 0x07, 0x02, 0x82, 0x00, 0x00, 0x02, 0x05,
	0x05, 0x89, 0x0E, 0x09, 0x0B, 0x0B, 0x02, 0x07, 0x02, 0x02, 0x82, 0x02, 0x07, 0x10, 0x07, 0x02, 0x02, 0x05, 0x09,
	0x0B, 0x05, 0x02, 0x02, 0x09, 0x0B, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07, 0x0B, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07, 0x10, 0x07, 0x02, 0x02, 0x05, 0x0B, 0x07, 0x02, 0x0E, 0x19, 0x09, 0x00,
	0x89, 0x00, 0x02, 0x02, 0x82, 0x00, 0x00, 0x05, 0x02, 0x02, 0x00, 0x02, 0x02, 0x02, 0x07, 0x10, 0x07, 0x10, 0x07,
	0x82, 0x02, 0x02, 0x02, 0x02, 0x00, 0x85, 0x00, 0x00, 0x89, 0x02, 0x02, 0x02, 0x02, 0x05, 0x07, 0x00, 0x09, 0x10,
	0x0B, 0x1B, 0x09, 0x02, 0x02, 0x02, 0x02, 0x00, 0x85, 0x00, 0x02, 0x05, 0x09, 0x07, 0x0B, 0x07, 0x82, 0x8C, 0x82,
	0x02, 0x02, 0x82, 0x02, 0x02, 0x05, 0x07, 0x82, 0x02, 0x0B, 0x05, 0x07, 0x0B, 0x07, 0x07, 0x07, 0x09, 0x02, 0x0B,
	0x02, 0x09, 0x05, 0x02, 0x02, 0x09, 0x05, 0x82, 0x8E, 0x82, 0x00, 0x89, 0x00, 0x05, 0x00, 0x09, 0x1E, 0x10, 0x05,
	0x02, 0x07, 0x0B, 0x07, 0x0B, 0x19, 0x10, 0x05, 0x82, 0x00, 0x85, 0x89, 0x00, 0x02, 0x02, 0x07, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x05, 0x85, 0x85, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02,
	0x09, 0x02, 0x02, 0x09, 0x05, 0x02, 0x02, 0x02, 0x09, 0x1B, 0x0B, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x07, 0x02, 0x02, 0x02, 0x02, 0x05, 0x09, 0x82, 0x02, 0x02, 0x02, 0x05, 0x0B, 0x07, 0x09, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x00, 0x87, 0x02, 0x07, 0x07, 0x0B, 0x89, 0x00, 0x02, 0x02, 0x09, 0x10, 0x02, 0x85, 0x00, 0x02, 0x02, 0x02,
	0x02, 0x85, 0x00, 0x82, 0x02, 0x02, 0x82, 0x02, 0x05, 0x00, 0x07, 0x02, 0x07, 0x12, 0x02, 0x8C, 0x89, 0x00, 0x02,
	0x02, 0x82, 0x90, 0x85, 0x87, 0x00, 0x05, 0x0E, 0x10, 0x09, 0x12, 0x17, 0x09, 0x00, 0x82, 0x87, 0x00, 0x02, 0x02,
	0x00, 0x89, 0x89, 0x02, 0x0E, 0x09, 0x0B, 0x05, 0x82, 0x07, 0x09, 0x00, 0x02, 0x09, 0x05, 0x02, 0x02, 0x02, 0x02,
	0x09, 0x07, 0x02, 0x07, 0x0E, 0x07, 0x02, 0x02, 0x02, 0x05, 0x07, 0x02, 0x02, 0x02, 0x02, 0x05, 0x09, 0x82, 0x82,
	0x90, 0x8E, 0x02, 0x02, 0x05, 0x0B, 0x09, 0x82, 0x02, 0x02, 0x02, 0x02, 0x87, 0x00, 0x00, 0x82, 0x07, 0x02, 0x82,
	0x05, 0x10, 0x07, 0x07, 0x09, 0x05, 0x07, 0x0E, 0x07, 0x02, 0x02, 0x07, 0x00, 0x8C, 0x82, 0x82, 0x82, 0x00, 0x02,
	0x02, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02, 0x05, 0x02, 0x07, 0x10, 0x07, 0x02, 0x02, 0x05, 0x09, 0x05, 0x00, 0x85,
	0x02, 0x05, 0x07, 0x82, 0x82, 0x8B, 0x85, 0x00, 0x09, 0x05, 0x02, 0x02, 0x02, 0x07, 0x10, 0x09, 0x02, 0x02, 0x02,
	0x07, 0x02, 0x00, 0x89, 0x82, 0x87, 0x82, 0x82, 0x82, 0x02, 0x07, 0x0E, 0x07, 0x02, 0x02, 0x00, 0x85, 0x00, 0x82,
	0x02, 0x02, 0x89, 0x02, 0x00, 0x02, 0x02, 0x82, 0x02, 0x02, 0x07, 0x02, 0x82, 0x02, 0x02, 0x02, 0x00, 0x07, 0x05,
	0x02, 0x07, 0x0E, 0x02, 0x85, 0x00, 0x02, 0x82, 0x82, 0x0B, 0x07, 0x05, 0x09, 0x05, 0x05, 0x02, 0x05, 0x02, 0x02,
	0x07, 0x02, 0x02, 0x12, 0x09, 0x10, 0x07, 0x00, 0x02, 0x02, 0x02, 0x02, 0x07, 0x02, 0x00, 0x89, 0x82, 0x02, 0x02,
	0x82, 0x82, 0x02, 0x02, 0x02, 0x02, 0x82, 0x00, 0x05, 0x0B, 0x07, 0x07, 0x19, 0x10, 0x85, 0x00, 0x02, 0x82, 0x02,
	0x02, 0x02, 0x82, 0x02, 0x07, 0x02, 0x02, 0x02, 0x05, 0x0B, 0x05, 0x07, 0x0B, 0x02, 0x85, 0x00, 0x02, 0x00, 0x8B,
	0x00, 0x07, 0x02, 0x02, 0x00, 0x02, 0x02, 0x85, 0x02, 0x09, 0x05, 0x02, 0x02, 0x02, 0x02, 0x07, 0x07, 0x10, 0x09,
	0x02, 0x07, 0x09, 0x0B, 0x0B, 0x05, 0x05, 0x09, 0x07, 0x09, 0x85, 0x89, 0x82, 0x00, 0x07, 0x02, 0x00, 0x8B, 0x89,
	0x82, 0x02, 0x02, 0x00, 0x87, 0x83, 0x03, 0x02, 0x07, 0x07, 0x02, 0x05, 0x09, 0x19, 0x09, 0x02, 0x85, 0x82, 0x07,
	0x09, 0x82, 0x94, 0x82, 0x02, 0x07, 0x10, 0x02, 0x89, 0x00, 0x07, 0x02, 0x87, 0x82, 0x89, 0x00, 0x02, 0x07, 0x10,
	0x0E, 0x07, 0x09, 0x02, 0x12, 0x07, 0x02, 0x07, 0x0E, 0x07, 0x02, 0x02, 0x02, 0x05, 0x10, 0x1E, 0x0E, 0x85, 0x00,
	0x07, 0x0E, 0x05, 0x85, 0x89, 0x82, 0x82, 0x02, 0x02, 0x02, 0x02, 0x00, 0x85, 0x00, 0x02, 0x02, 0x02, 0x82, 0x02,
	0x02, 0x00, 0x87, 0x82, 0x07, 0x07, 0x09, 0x05, 0x00, 0x89, 0x02, 0x07, 0x0B, 0x82, 0x02, 0x02, 0x00, 0x87, 0x87,
	0x00, 0x02, 0x82, 0x02, 0x02, 0x02, 0x02, 0x00, 0x87, 0x00, 0x02, 0x02, 0x02, 0x07, 0x02, 0x02, 0x02, 0x00, 0x85,
	0x00, 0x00, 0x89, 0x89, 0x00, 0x02, 0x07, 0x02, 0x02, 0x02, 0x05, 0x07, 0x12, 0x07, 0x02, 0x02, 0x00, 0x87, 0x00,
	0x02, 0x02, 0x00, 0x02, 0x02, 0x05, 0x09, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x82, 0x02, 0x02, 0x00, 0x8B, 0x87,
	0x85, 0x02, 0x0B, 0x05, 0x02, 0x02, 0x05, 0x02, 0x02, 0x02, 0x02, 0x07, 0x0B, 0x02, 0x02, 0x09, 0x0E, 0x05, 0x02,
	0x82, 0x02, 0x02, 0x07, 0x00, 0x02, 0x0B, 0x09, 0x07, 0x0E, 0x07, 0x02, 0x02, 0x00, 0x05, 0x09, 0x87, 0x00, 0x00,
	0x05, 0x0B, 0x09, 0x0E, 0x09, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x89, 0x82, 0x86, 0xAD, 0x82, 0x82, 0x03, 0x0F,
	0x82, 0x82, 0x06, 0x03, 0x06, 0x06, 0x1B, 0x06, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x88,
	0x88, 0x0F, 0x06, 0x82, 0x82, 0x03, 0x12, 0x12, 0x12, 0x06, 0x8B, 0x91, 0x88, 0x82, 0x82, 0x82, 0x86, 0x85, 0x82,
	0x03, 0x82, 0x82, 0x82, 0x82, 0x00, 0x0C, 0x03, 0x03, 0x82, 0x82, 0x82, 0x82, 0x06, 0x06, 0x82, 0x82, 0x85, 0x00,
	0x12, 0x00, 0x86, 0x88, 0x83, 0x82, 0x00, 0x83, 0x88, 0x83, 0x82, 0x82, 0x82, 0x00, 0x06, 0x00, 0x83, 0x88, 0x03,
	0x82, 0x86, 0x82, 0x03, 0x0F, 0x03, 0x85, 0x88, 0x12, 0x0F, 0x03, 0x82, 0x86, 0x88, 0x83, 0x82, 0x06, 0x12, 0x85,
	0x83, 0x82, 0x82, 0x82, 0x82, 0x82, 0x85, 0x85, 0x82, 0x00, 0x0F, 0x0F, 0x85, 0x8B, 0x83, 0x83, 0x88, 0x85, 0x85,
	0x06, 0x06, 0x82, 0x82, 0x00, 0x88, 0x88, 0x89, 0x86, 0x82, 0x82, 0x82, 0x03, 0x03, 0x09, 0x00, 0x85, 0x8C, 0x85,
	0x03, 0x0F, 0x03, 0x86, 0x85, 0x86, 0x85, 0x82, 0x82, 0x00, 0x0C, 0x03, 0x83, 0x91, 0x88, 0x82, 0x00, 0x06, 0x00,
	0x82, 0x03, 0x12, 0x06, 0x06, 0x00, 0x06, 0x06, 0x82, 0x83, 0x82, 0x06, 0x06, 0x12, 0x06, 0x82, 0x82, 0x82, 0x82,
	0x82, 0x85, 0x82, 0x82, 0x82, 0x00, 0x06, 0x82, 0x85, 0x85, 0x8C, 0x88, 0x83, 0x85, 0x82, 0x82, 0x82, 0x82, 0x82,
	0x82, 0x82, 0x82, 0xA4, 0x82, 0x82, 0x88, 0x8E, 0x85, 0x82, 0x00, 0x06, 0x00, 0x03, 0x82, 0x82, 0x03, 0x83, 0x82,
	0x85, 0x82, 0x82, 0x82, 0x82, 0x06, 0x12, 0x03, 0x88, 0x82, 0x0C, 0x00, 0x82, 0x83, 0x06, 0x86, 0x83, 0x83, 0x88,
	0x03, 0x0F, 0x0F, 0x00, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x03, 0x09, 0x86, 0x83, 0x82, 0x82,
	0x83, 0x85, 0x06, 0x09, 0x85, 0x03, 0x18, 0x06, 0x03, 0x12, 0x09, 0x82, 0x00, 0x0C, 0x06, 0x82, 0x82, 0x83, 0x88,
	0x85, 0x82, 0x06, 0x82, 0x86, 0x82, 0x82, 0x82, 0x82, 0x86, 0x82, 0x82, 0x00, 0x82, 0x12, 0x06, 0x82, 0x06, 0x0C,
	0x06, 0x09, 0x00, 0x82, 0x06, 0x18, 0x82, 0x82, 0x82, 0x82, 0x85, 0x82, 0x82, 0x82, 0x82, 0x06, 0x82, 0x88, 0x82,
	0x06, 0x82, 0x00, 0x0C, 0x00, 0x82, 0x03, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x83, 0x85, 0x82, 0x82, 0x83,
	0x89, 0x83, 0x82, 0x82, 0x83, 0x00, 0x0F, 0x06, 0x00, 0x03, 0x88, 0x83, 0x82, 0x85, 0x0C, 0x03, 0x82, 0x85, 0x83,
	0x88, 0x86, 0x8C, 0x85, 0x82, 0x82, 0x82, 0x83, 0x88, 0x83, 0x00, 0x09, 0x82, 0x88, 0x85, 0x82, 0x00, 0x0F, 0x0C,
	0x85, 0x85, 0x83, 0x85, 0x0C, 0x06, 0x12, 0x09, 0x06, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x03, 0x00, 0x06,
	0x82, 0x82, 0x82, 0x82, 0x82, 0x85, 0x82, 0x82, 0x85, 0x82, 0x09
};
