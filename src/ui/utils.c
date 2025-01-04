// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#include "ui.h"

void ui_state_set(ui_t *ui, ui_state_t state) {
	ui->prev_state = ui->state;
	ui->state	   = state;
	ui->next_state = UI_STATE_MAX;
}

void ui_state_set_via(ui_t *ui, ui_state_t state, ui_state_t via) {
	ui->prev_state = ui->state;
	ui->state	   = via;
	ui->next_state = state;
}

void ui_state_prev(ui_t *ui) {
	if (ui->prev_state == UI_STATE_MAX)
		return;
	ui->next_state = ui->state;
	ui->state	   = ui->prev_state;
	ui->prev_state = UI_STATE_MAX;
}

void ui_state_next(ui_t *ui) {
	if (ui->next_state == UI_STATE_MAX)
		return;
	ui->state	   = ui->next_state;
	ui->next_state = UI_STATE_MAX;
}

void ui_state_error(ui_t *ui) {
	ui_state_prev(ui);
	ui_state_set(ui, UI_STATE_ERROR);
}
