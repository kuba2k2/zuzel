// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#pragma once

#include "include.h"

#define VIEW_WRAP_CONTENT (0)
#define VIEW_MATCH_PARENT (-1)

typedef struct view_t view_t;
typedef void (*view_inflate_t)(view_t *view, cJSON *json);
typedef void (*view_measure_t)(view_t *view);
typedef void (*view_layout_t)(view_t *view);
typedef void (*view_draw_t)(SDL_Renderer *renderer, view_t *view);
typedef bool (*view_on_event_t)(view_t *view, SDL_Event *event);

typedef enum {
	VIEW_TYPE_FRAME	 = 0,
	VIEW_TYPE_BOX	 = 1,
	VIEW_TYPE_TEXT	 = 2,
	VIEW_TYPE_BUTTON = 3,
	VIEW_TYPE_INPUT	 = 4,
} view_type_t;

typedef struct view_text_t {
	char *text;			//!< Text to be displayed (view-owned)
	unsigned int color; //!< Color of the text (ARGB8888)
	int font;			//!< Font index
	int size;			//!< Font size
	int align;			//!< Font alignment
} view_text_t;

typedef struct view_t {
	// view type
	char *id;				  //!< Freeform view ID/name (view-owned)
	view_type_t type;		  //!< Type of the view
	view_inflate_t inflate;	  //!< View inflater (parameter deserialization)
	view_measure_t measure;	  //!< View bounding box measurement function
	view_layout_t layout;	  //!< Layout positioning function (optional)
	view_draw_t draw;		  //!< Renderer function
	view_on_event_t on_event; //!< Event processor function

	// positioning
	int w, h;			//!< Width/height specification (exact in pixels, or VIEW_MATCH_PARENT/VIEW_WRAP_CONTENT)
	int ml, mr, mt, mb; //!< Margins: left/right/top/bottom
	int gravity;		//!< Gravity of this view within the parent view
	int weight;			//!< Weight of this view within the parent view (VIEW_TYPE_BOX only)
	SDL_Rect rect;		//!< Bounding box (calculated during layout)

	// common properties
	bool is_gone;	   //!< Whether the view should be ignored in layout
	bool is_invisible; //!< Whether the view should be ignored in drawing
	bool is_disabled;  //!< Whether the view is disabled
	bool is_focused;   //!< Whether the view is focused
	bool is_focusable; //!< Whether the view can be focused

	// view-specific properties
	union {
		struct {
			bool is_horizontal;
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
		// sent by view core
		view_on_event_t focus; //!< On focus change
		// sent by specific views
		view_on_event_t accept; //!< On click/Enter keypress
		view_on_event_t change; //!< On value change
	} event;

	// child views (if supported)
	struct view_t *children;
	// parent view
	struct view_t *parent;

	// doubly-linked list siblings
	// note: head's 'prev' points to the last item
	struct view_t *prev, *next;
} view_t;

// view.c
view_t *gfx_view_inflate(cJSON *json, view_t *parent);
void gfx_view_measure(view_t *views);
void gfx_view_layout(view_t *views);
void gfx_view_draw(SDL_Renderer *renderer, view_t *views);
bool gfx_view_on_event(view_t *views, SDL_Event *e);

// view_utils.c
void gfx_view_measure_one(view_t *view, int parent_w, int parent_h);
void gfx_view_layout_one(view_t *view, int x, int y, int parent_w, int parent_h);
void gfx_view_set_text_style(view_text_t *text, unsigned int color, int font, int size, int align);
view_t *gfx_view_find_prev(view_t *view);
view_t *gfx_view_find_next(view_t *view);

// view_*.c
view_t *gfx_view_make_frame(view_t *parent);
view_t *gfx_view_make_box(view_t *parent);
view_t *gfx_view_make_text(view_t *parent);

#define GFX_VIEW_FIND(start, item, direction, same, check)                                                             \
	do {                                                                                                               \
		item = start;                                                                                                  \
		if (!(same) || !(check)) {                                                                                     \
			do {                                                                                                       \
				item = gfx_view_find_##direction(item);                                                                \
			} while (item != NULL && !(check));                                                                        \
		}                                                                                                              \
	} while (0)

#define GFX_VIEW_IS_ACTIVE(view) (!view->is_gone && !view->is_invisible && !view->is_disabled && view->is_focusable)
#define GFX_VIEW_IN_BOX(view, x, y)                                                                                    \
	(x >= view->rect.x && x <= view->rect.x + view->rect.w && y >= view->rect.y && y <= view->rect.y + view->rect.h)
