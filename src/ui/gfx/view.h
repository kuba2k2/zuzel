// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#pragma once

#include "include.h"

#define VIEW_WRAP_CONTENT (0)
#define VIEW_MATCH_PARENT (-1)

typedef struct view_t view_t;
typedef struct view_inflate_on_event_t view_inflate_on_event_t;

typedef void (*view_inflate_t)(view_t *view, cJSON *json, const view_inflate_on_event_t *on_event);
typedef void (*view_free_t)(view_t *views);
typedef void (*view_measure_t)(view_t *view);
typedef void (*view_layout_t)(view_t *view);
typedef void (*view_draw_t)(SDL_Renderer *renderer, view_t *view);
typedef bool (*view_on_event_t)(view_t *view, SDL_Event *e);

typedef enum {
	VIEW_TYPE_FRAME	 = 0,
	VIEW_TYPE_BOX	 = 1,
	VIEW_TYPE_TEXT	 = 2,
	VIEW_TYPE_BUTTON = 3,
	VIEW_TYPE_SLIDER = 4,
	VIEW_TYPE_INPUT	 = 5,
} view_type_t;

typedef struct view_text_t {
	char *text;			//!< Text to be displayed (view-owned)
	unsigned int color; //!< Color of the text (ARGB8888)
	int font;			//!< Font index
	int size;			//!< Font size
	int align;			//!< Font alignment
} view_text_t;

typedef struct view_inflate_on_event_t {
	const char *name;
	view_on_event_t func;
} view_inflate_on_event_t;

typedef struct view_t {
	// view type
	char *id;				  //!< Freeform view ID/name (view-owned)
	view_type_t type;		  //!< Type of the view
	view_inflate_t inflate;	  //!< View inflater (parameter deserialization)
	view_free_t free;		  //!< Function to free/release the views (recursively)
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
	bool in_event;	   //!< Whether the view should capture all events, regardless of its focus state

	// view-specific properties
	union {
		struct {
			bool is_horizontal;
		} box;

		view_text_t text;

		struct {
			view_text_t text;
			unsigned int bg_color;
			unsigned int bg_disabled;
			unsigned int bg_focused;
			unsigned int fg_shadow;
			unsigned int fg_disabled;
			unsigned int fg_focused;
		} button;

		struct {
			view_text_t text;
			int value;
			int min;
			int max;
			view_t *button;
		} slider;

		struct {
			view_text_t text;
			view_text_t placeholder;
			int max_length;
			char *value;
			int pos;
		} input;
	} data;

	// event handlers
	struct {
		// sent by view core
		view_on_event_t focus; //!< On focus change
		// sent by specific views
		view_on_event_t press;	//!< On press (mouse click/Enter key)
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
extern bool gfx_view_bounding_box;
view_t *gfx_view_inflate(cJSON *json, view_t *parent, const view_inflate_on_event_t *on_event);
void gfx_view_free(view_t *views);
void gfx_view_measure(view_t *views);
void gfx_view_layout(view_t *views);
void gfx_view_draw(SDL_Renderer *renderer, view_t *views);
bool gfx_view_on_event(view_t *views, SDL_Event *e);

// view_utils.c
view_t *gfx_view_inflate_from_file(const char *filename, view_t *parent, const view_inflate_on_event_t *on_event);
void gfx_view_measure_one(view_t *view, int parent_w, int parent_h);
void gfx_view_layout_one(view_t *view, int x, int y, int parent_w, int parent_h);
void gfx_view_set_text_style(view_text_t *text, unsigned int color, int font, int size, int align);
view_t *gfx_view_find_prev(view_t *view);
view_t *gfx_view_find_next(view_t *view);
view_t *gfx_view_find_by_id(view_t *views, const char *id);
char *gfx_view_make_id(view_t *view);

// view_*.c
view_t *gfx_view_make_frame(view_t *parent);
view_t *gfx_view_make_box(view_t *parent);
view_t *gfx_view_make_text(view_t *parent);
view_t *gfx_view_make_button(view_t *parent);
view_t *gfx_view_make_slider(view_t *parent);
view_t *gfx_view_make_input(view_t *parent);

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
#define GFX_VIEW_IN_BOX(view, _x, _y)                                                                                  \
	(_x >= view->rect.x && _x <= view->rect.x + view->rect.w && _y >= view->rect.y && _y <= view->rect.y + view->rect.h)
