// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#pragma once

#include "include.h"

#define VIEW_MATCH_PARENT (-1)
#define VIEW_WRAP_CONTENT (-2)

typedef struct view_t view_t;
typedef void (*view_measure_t)(view_t *view);
typedef void (*view_layout_t)(view_t *view);
typedef void (*view_draw_t)(SDL_Renderer *renderer, view_t *view);
typedef void (*view_on_event_t)(view_t *view, SDL_Event *event);

typedef enum {
	VIEW_TYPE_BOX	 = 0,
	VIEW_TYPE_TEXT	 = 1,
	VIEW_TYPE_BUTTON = 2,
	VIEW_TYPE_INPUT	 = 3,
} view_type_t;

typedef struct {
	const char *text;	//!< Text to be displayed
	unsigned int color; //!< Color of the text (ARGB8888)
	int font;			//!< Font index
	int size;			//!< Font size
	int align;			//!< Font alignment
} view_text_t;

typedef struct view_t {
	// view type
	const char *id;			//!< Freeform view ID/name
	view_type_t type;		//!< Type of the view
	view_measure_t measure; //!< View bounding box measurement function
	view_layout_t layout;	//!< Layout positioning function (optional)
	view_draw_t draw;		//!< Renderer function

	// positioning
	int w, h;	   //!< Width/height specification (exact in pixels, or VIEW_MATCH_PARENT/VIEW_WRAP_CONTENT)
	int gravity;   //!< Gravity of this view within the parent view
	int weight;	   //!< Weight of this view within the parent view (VIEW_TYPE_BOX only)
	SDL_Rect rect; //!< Bounding box (calculated during layout)

	// common properties
	bool is_gone;	   //!< Whether the view should be ignored in layout
	bool is_invisible; //!< Whether the view should be ignored in drawing
	bool is_disabled;  //!< Whether the view is disabled
	bool is_hovered;   //!< Whether the view is hovered over
	bool is_focused;   //!< Whether the view is focused
	bool is_focusable; //!< Whether the view can be focused

	// view-specific properties
	union {
		struct {
			bool is_horizontal;
			int gravity;
			struct view_t *children;
		} box;

		view_text_t text;
		view_text_t button;

		struct {
			view_text_t label;
			view_text_t placeholder;
			view_text_t value;
			int max_length;
			char *new_value;
		} input;
	} data;

	// event handlers
	struct {
		view_on_event_t accept; //!< On click/Enter keypress
		view_on_event_t hover;	//!< On mouse hover
		view_on_event_t focus;	//!< On focus/mouse click
		view_on_event_t change; //!< On value change
	} event;

	// next sibling
	struct view_t *next;
} view_t;

// view.c
void gfx_view_measure(view_t *views);
void gfx_view_layout(view_t *views);
void gfx_view_draw(SDL_Renderer *renderer, view_t *views);

// view_utils.c
void gfx_view_measure_one(view_t *view, int parent_w, int parent_h);
void gfx_view_layout_one(view_t *view, int x, int y, int parent_w, int parent_h);
void gfx_view_set_text_style(view_text_t *text, unsigned int color, int font, int size, int align);

// view_*.c
view_t *gfx_view_make_box(bool is_horizontal);
view_t *gfx_view_make_text(const char *text);

#define GFX_VIEW_ADD(views, type, width, height, params, ...)                                                          \
	do {                                                                                                               \
		view_t *view = gfx_view_make_##type params;                                                                    \
		view->w		 = width;                                                                                          \
		view->h		 = height;                                                                                         \
		__VA_ARGS__;                                                                                                   \
		LL_APPEND(views, view);                                                                                        \
	} while (0)
