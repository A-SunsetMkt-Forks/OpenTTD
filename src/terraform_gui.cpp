/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file terraform_gui.cpp GUI related to terraforming the map. */

#include "stdafx.h"
#include "core/backup_type.hpp"
#include "clear_map.h"
#include "company_func.h"
#include "company_base.h"
#include "house.h"
#include "gui.h"
#include "window_gui.h"
#include "window_func.h"
#include "viewport_func.h"
#include "command_func.h"
#include "signs_func.h"
#include "sound_func.h"
#include "base_station_base.h"
#include "textbuf_gui.h"
#include "genworld.h"
#include "tree_map.h"
#include "landscape_type.h"
#include "tilehighlight_func.h"
#include "strings_func.h"
#include "newgrf_object.h"
#include "object.h"
#include "hotkeys.h"
#include "engine_base.h"
#include "terraform_gui.h"
#include "terraform_cmd.h"
#include "zoom_func.h"
#include "rail_cmd.h"
#include "landscape_cmd.h"
#include "terraform_cmd.h"
#include "object_cmd.h"

#include "widgets/terraform_widget.h"

#include "table/strings.h"

#include "safeguards.h"

void CcTerraform(Commands, const CommandCost &result, Money, TileIndex tile)
{
	if (result.Succeeded()) {
		if (_settings_client.sound.confirm) SndPlayTileFx(SND_1F_CONSTRUCTION_OTHER, tile);
	} else {
		SetRedErrorSquare(tile);
	}
}


/** Scenario editor command that generates desert areas */
static void GenerateDesertArea(TileIndex end, TileIndex start)
{
	if (_game_mode != GM_EDITOR) return;

	Backup<bool> old_generating_world(_generating_world, true);

	TileArea ta(start, end);
	for (TileIndex tile : ta) {
		SetTropicZone(tile, (_ctrl_pressed) ? TROPICZONE_NORMAL : TROPICZONE_DESERT);
		Command<CMD_LANDSCAPE_CLEAR>::Post(tile);
		MarkTileDirtyByTile(tile);
	}
	old_generating_world.Restore();
	InvalidateWindowClassesData(WC_TOWN_VIEW, 0);
}

/** Scenario editor command that generates rocky areas */
static void GenerateRockyArea(TileIndex end, TileIndex start)
{
	if (_game_mode != GM_EDITOR) return;

	bool success = false;
	TileArea ta(start, end);

	for (TileIndex tile : ta) {
		switch (GetTileType(tile)) {
			case MP_TREES:
				if (GetTreeGround(tile) == TREE_GROUND_SHORE) continue;
				[[fallthrough]];

			case MP_CLEAR:
				MakeClear(tile, CLEAR_ROCKS, 3);
				break;

			default:
				continue;
		}
		MarkTileDirtyByTile(tile);
		success = true;
	}

	if (success && _settings_client.sound.confirm) SndPlayTileFx(SND_1F_CONSTRUCTION_OTHER, end);
}

/**
 * A central place to handle all X_AND_Y dragged GUI functions.
 * @param proc       Procedure related to the dragging
 * @param start_tile Begin of the dragging
 * @param end_tile   End of the dragging
 * @return Returns true if the action was found and handled, and false otherwise. This
 * allows for additional implements that are more local. For example X_Y drag
 * of convertrail which belongs in rail_gui.cpp and not terraform_gui.cpp
 */
bool GUIPlaceProcDragXY(ViewportDragDropSelectionProcess proc, TileIndex start_tile, TileIndex end_tile)
{
	if (!_settings_game.construction.freeform_edges) {
		/* When end_tile is MP_VOID, the error tile will not be visible to the
		 * user. This happens when terraforming at the southern border. */
		if (TileX(end_tile) == Map::MaxX()) end_tile += TileDiffXY(-1, 0);
		if (TileY(end_tile) == Map::MaxY()) end_tile += TileDiffXY(0, -1);
	}

	switch (proc) {
		case DDSP_DEMOLISH_AREA:
			Command<CMD_CLEAR_AREA>::Post(STR_ERROR_CAN_T_CLEAR_THIS_AREA, CcPlaySound_EXPLOSION, end_tile, start_tile, _ctrl_pressed);
			break;
		case DDSP_RAISE_AND_LEVEL_AREA:
			Command<CMD_LEVEL_LAND>::Post(STR_ERROR_CAN_T_RAISE_LAND_HERE, CcTerraform, end_tile, start_tile, _ctrl_pressed, LM_RAISE);
			break;
		case DDSP_LOWER_AND_LEVEL_AREA:
			Command<CMD_LEVEL_LAND>::Post(STR_ERROR_CAN_T_LOWER_LAND_HERE, CcTerraform, end_tile, start_tile, _ctrl_pressed, LM_LOWER);
			break;
		case DDSP_LEVEL_AREA:
			Command<CMD_LEVEL_LAND>::Post(STR_ERROR_CAN_T_LEVEL_LAND_HERE, CcTerraform, end_tile, start_tile, _ctrl_pressed, LM_LEVEL);
			break;
		case DDSP_CREATE_ROCKS:
			GenerateRockyArea(end_tile, start_tile);
			break;
		case DDSP_CREATE_DESERT:
			GenerateDesertArea(end_tile, start_tile);
			break;
		default:
			return false;
	}

	return true;
}

/**
 * Start a drag for demolishing an area.
 * @param tile Position of one corner.
 */
void PlaceProc_DemolishArea(TileIndex tile)
{
	VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_DEMOLISH_AREA);
}

/** Terra form toolbar managing class. */
struct TerraformToolbarWindow : Window {
	int last_user_action = INVALID_WID_TT; ///< Last started user action.

	TerraformToolbarWindow(WindowDesc &desc, WindowNumber window_number) : Window(desc)
	{
		/* This is needed as we like to have the tree available on OnInit. */
		this->CreateNestedTree();
		this->FinishInitNested(window_number);
	}

	void OnInit() override
	{
		/* Don't show the place object button when there are no objects to place. */
		NWidgetStacked *show_object = this->GetWidget<NWidgetStacked>(WID_TT_SHOW_PLACE_OBJECT);
		show_object->SetDisplayedPlane(ObjectClass::GetUIClassCount() != 0 ? 0 : SZSP_NONE);
	}

	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		if (widget < WID_TT_BUTTONS_START) return;

		switch (widget) {
			case WID_TT_LOWER_LAND: // Lower land button
				HandlePlacePushButton(this, WID_TT_LOWER_LAND, ANIMCURSOR_LOWERLAND, HT_POINT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_TT_RAISE_LAND: // Raise land button
				HandlePlacePushButton(this, WID_TT_RAISE_LAND, ANIMCURSOR_RAISELAND, HT_POINT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_TT_LEVEL_LAND: // Level land button
				HandlePlacePushButton(this, WID_TT_LEVEL_LAND, SPR_CURSOR_LEVEL_LAND, HT_POINT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_TT_DEMOLISH: // Demolish aka dynamite button
				HandlePlacePushButton(this, WID_TT_DEMOLISH, ANIMCURSOR_DEMOLISH, HT_RECT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_TT_BUY_LAND: // Buy land button
				HandlePlacePushButton(this, WID_TT_BUY_LAND, SPR_CURSOR_BUY_LAND, HT_RECT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_TT_PLANT_TREES: // Plant trees button
				ShowBuildTreesToolbar();
				break;

			case WID_TT_PLACE_SIGN: // Place sign button
				HandlePlacePushButton(this, WID_TT_PLACE_SIGN, SPR_CURSOR_SIGN, HT_RECT);
				this->last_user_action = widget;
				break;

			case WID_TT_PLACE_OBJECT: // Place object button
				ShowBuildObjectPicker();
				break;

			default: NOT_REACHED();
		}
	}

	void OnPlaceObject([[maybe_unused]] Point pt, TileIndex tile) override
	{
		switch (this->last_user_action) {
			case WID_TT_LOWER_LAND: // Lower land button
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_LOWER_AND_LEVEL_AREA);
				break;

			case WID_TT_RAISE_LAND: // Raise land button
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_RAISE_AND_LEVEL_AREA);
				break;

			case WID_TT_LEVEL_LAND: // Level land button
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_LEVEL_AREA);
				break;

			case WID_TT_DEMOLISH: // Demolish aka dynamite button
				PlaceProc_DemolishArea(tile);
				break;

			case WID_TT_BUY_LAND: // Buy land button
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_BUILD_OBJECT);
				break;

			case WID_TT_PLACE_SIGN: // Place sign button
				PlaceProc_Sign(tile);
				break;

			default: NOT_REACHED();
		}
	}

	void OnPlaceDrag(ViewportPlaceMethod select_method, [[maybe_unused]] ViewportDragDropSelectionProcess select_proc, [[maybe_unused]] Point pt) override
	{
		VpSelectTilesWithMethod(pt.x, pt.y, select_method);
	}

	Point OnInitialPosition([[maybe_unused]] int16_t sm_width, [[maybe_unused]] int16_t sm_height, [[maybe_unused]] int window_number) override
	{
		Point pt = GetToolbarAlignedWindowPosition(sm_width);
		pt.y += sm_height;
		return pt;
	}

	void OnPlaceMouseUp([[maybe_unused]] ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, [[maybe_unused]] Point pt, TileIndex start_tile, TileIndex end_tile) override
	{
		if (pt.x != -1) {
			switch (select_proc) {
				default: NOT_REACHED();
				case DDSP_DEMOLISH_AREA:
				case DDSP_RAISE_AND_LEVEL_AREA:
				case DDSP_LOWER_AND_LEVEL_AREA:
				case DDSP_LEVEL_AREA:
					GUIPlaceProcDragXY(select_proc, start_tile, end_tile);
					break;
				case DDSP_BUILD_OBJECT:
					if (!_settings_game.construction.freeform_edges) {
						/* When end_tile is MP_VOID, the error tile will not be visible to the
						 * user. This happens when terraforming at the southern border. */
						if (TileX(end_tile) == Map::MaxX()) end_tile += TileDiffXY(-1, 0);
						if (TileY(end_tile) == Map::MaxY()) end_tile += TileDiffXY(0, -1);
					}
					Command<CMD_BUILD_OBJECT_AREA>::Post(STR_ERROR_CAN_T_PURCHASE_THIS_LAND, CcPlaySound_CONSTRUCTION_RAIL,
						end_tile, start_tile, OBJECT_OWNED_LAND, 0, (_ctrl_pressed ? true : false));
					break;
			}
		}
	}

	void OnPlaceObjectAbort() override
	{
		this->RaiseButtons();
	}

	/**
	 * Handler for global hotkeys of the TerraformToolbarWindow.
	 * @param hotkey Hotkey
	 * @return ES_HANDLED if hotkey was accepted.
	 */
	static EventState TerraformToolbarGlobalHotkeys(int hotkey)
	{
		if (_game_mode != GM_NORMAL) return ES_NOT_HANDLED;
		Window *w = ShowTerraformToolbar(nullptr);
		if (w == nullptr) return ES_NOT_HANDLED;
		return w->OnHotkey(hotkey);
	}

	static inline HotkeyList hotkeys{"terraform", {
		Hotkey('Q' | WKC_GLOBAL_HOTKEY, "lower", WID_TT_LOWER_LAND),
		Hotkey('W' | WKC_GLOBAL_HOTKEY, "raise", WID_TT_RAISE_LAND),
		Hotkey('E' | WKC_GLOBAL_HOTKEY, "level", WID_TT_LEVEL_LAND),
		Hotkey('D' | WKC_GLOBAL_HOTKEY, "dynamite", WID_TT_DEMOLISH),
		Hotkey('U', "buyland", WID_TT_BUY_LAND),
		Hotkey('I', "trees", WID_TT_PLANT_TREES),
		Hotkey('O', "placesign", WID_TT_PLACE_SIGN),
		Hotkey('P', "placeobject", WID_TT_PLACE_OBJECT),
	}, TerraformToolbarGlobalHotkeys};
};

static constexpr NWidgetPart _nested_terraform_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetStringTip(STR_LANDSCAPING_TOOLBAR, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_LOWER_LAND), SetToolbarMinimalSize(1),
								SetFill(0, 1), SetSpriteTip(SPR_IMG_TERRAFORM_DOWN, STR_LANDSCAPING_TOOLTIP_LOWER_A_CORNER_OF_LAND),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_RAISE_LAND), SetToolbarMinimalSize(1),
								SetFill(0, 1), SetSpriteTip(SPR_IMG_TERRAFORM_UP, STR_LANDSCAPING_TOOLTIP_RAISE_A_CORNER_OF_LAND),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_LEVEL_LAND), SetToolbarMinimalSize(1),
								SetFill(0, 1), SetSpriteTip(SPR_IMG_LEVEL_LAND, STR_LANDSCAPING_LEVEL_LAND_TOOLTIP),

		NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetToolbarSpacerMinimalSize(), EndContainer(),

		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_DEMOLISH), SetToolbarMinimalSize(1),
								SetFill(0, 1), SetSpriteTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_BUY_LAND), SetToolbarMinimalSize(1),
								SetFill(0, 1), SetSpriteTip(SPR_IMG_BUY_LAND, STR_LANDSCAPING_TOOLTIP_PURCHASE_LAND),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_TT_PLANT_TREES), SetToolbarMinimalSize(1),
								SetFill(0, 1), SetSpriteTip(SPR_IMG_PLANTTREES, STR_SCENEDIT_TOOLBAR_PLANT_TREES_TOOLTIP),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_PLACE_SIGN), SetToolbarMinimalSize(1),
								SetFill(0, 1), SetSpriteTip(SPR_IMG_SIGN, STR_SCENEDIT_TOOLBAR_PLACE_SIGN_TOOLTIP),
		NWidget(NWID_SELECTION, INVALID_COLOUR, WID_TT_SHOW_PLACE_OBJECT),
			NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_TT_PLACE_OBJECT), SetToolbarMinimalSize(1),
								SetFill(0, 1), SetSpriteTip(SPR_IMG_TRANSMITTER, STR_SCENEDIT_TOOLBAR_PLACE_OBJECT_TOOLTIP),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _terraform_desc(
	WDP_MANUAL, "toolbar_landscape", 0, 0,
	WC_SCEN_LAND_GEN, WC_NONE,
	WindowDefaultFlag::Construction,
	_nested_terraform_widgets,
	&TerraformToolbarWindow::hotkeys
);

/**
 * Show the toolbar for terraforming in the game.
 * @param link The toolbar we might want to link to.
 * @return The allocated toolbar if the window was newly opened, else \c nullptr.
 */
Window *ShowTerraformToolbar(Window *link)
{
	if (!Company::IsValidID(_local_company)) return nullptr;

	Window *w;
	if (link == nullptr) {
		w = AllocateWindowDescFront<TerraformToolbarWindow>(_terraform_desc, 0);
		return w;
	}

	/* Delete the terraform toolbar to place it again. */
	CloseWindowById(WC_SCEN_LAND_GEN, 0, true);
	w = AllocateWindowDescFront<TerraformToolbarWindow>(_terraform_desc, 0);
	/* Align the terraform toolbar under the main toolbar. */
	w->top -= w->height;
	w->SetDirty();
	/* Put the linked toolbar to the left / right of it. */
	link->left = w->left + (_current_text_dir == TD_RTL ? w->width : -link->width);
	link->top  = w->top;
	link->SetDirty();

	return w;
}

static uint8_t _terraform_size = 1;

/**
 * Raise/Lower a bigger chunk of land at the same time in the editor. When
 * raising get the lowest point, when lowering the highest point, and set all
 * tiles in the selection to that height.
 * @todo : Incorporate into game itself to allow for ingame raising/lowering of
 *         larger chunks at the same time OR remove altogether, as we have 'level land' ?
 * @param tile The top-left tile where the terraforming will start
 * @param mode true for raising, false for lowering land
 */
static void CommonRaiseLowerBigLand(TileIndex tile, bool mode)
{
	if (_terraform_size == 1) {
		StringID msg =
			mode ? STR_ERROR_CAN_T_RAISE_LAND_HERE : STR_ERROR_CAN_T_LOWER_LAND_HERE;

		Command<CMD_TERRAFORM_LAND>::Post(msg, CcTerraform, tile, SLOPE_N, mode);
	} else {
		assert(_terraform_size != 0);
		TileArea ta(tile, _terraform_size, _terraform_size);
		ta.ClampToMap();

		if (ta.w == 0 || ta.h == 0) return;

		if (_settings_client.sound.confirm) SndPlayTileFx(SND_1F_CONSTRUCTION_OTHER, tile);

		uint h;
		if (mode != 0) {
			/* Raise land */
			h = MAX_TILE_HEIGHT;
			for (TileIndex tile2 : ta) {
				h = std::min(h, TileHeight(tile2));
			}
		} else {
			/* Lower land */
			h = 0;
			for (TileIndex tile2 : ta) {
				h = std::max(h, TileHeight(tile2));
			}
		}

		for (TileIndex tile2 : ta) {
			if (TileHeight(tile2) == h) {
				Command<CMD_TERRAFORM_LAND>::Post(tile2, SLOPE_N, mode);
			}
		}
	}
}

static const int8_t _multi_terraform_coords[][2] = {
	{  0, -2},
	{  4,  0}, { -4,  0}, {  0,  2},
	{ -8,  2}, { -4,  4}, {  0,  6}, {  4,  4}, {  8,  2},
	{-12,  0}, { -8, -2}, { -4, -4}, {  0, -6}, {  4, -4}, {  8, -2}, { 12,  0},
	{-16,  2}, {-12,  4}, { -8,  6}, { -4,  8}, {  0, 10}, {  4,  8}, {  8,  6}, { 12,  4}, { 16,  2},
	{-20,  0}, {-16, -2}, {-12, -4}, { -8, -6}, { -4, -8}, {  0,-10}, {  4, -8}, {  8, -6}, { 12, -4}, { 16, -2}, { 20,  0},
	{-24,  2}, {-20,  4}, {-16,  6}, {-12,  8}, { -8, 10}, { -4, 12}, {  0, 14}, {  4, 12}, {  8, 10}, { 12,  8}, { 16,  6}, { 20,  4}, { 24,  2},
	{-28,  0}, {-24, -2}, {-20, -4}, {-16, -6}, {-12, -8}, { -8,-10}, { -4,-12}, {  0,-14}, {  4,-12}, {  8,-10}, { 12, -8}, { 16, -6}, { 20, -4}, { 24, -2}, { 28,  0},
};

static constexpr NWidgetPart _nested_scen_edit_land_gen_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetStringTip(STR_TERRAFORM_TOOLBAR_LAND_GENERATION_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
		NWidget(NWID_HORIZONTAL), SetPadding(2, 2, 7, 2),
			NWidget(NWID_SPACER), SetFill(1, 0),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_DEMOLISH), SetToolbarMinimalSize(1),
										SetFill(0, 1), SetSpriteTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_LOWER_LAND), SetToolbarMinimalSize(1),
										SetFill(0, 1), SetSpriteTip(SPR_IMG_TERRAFORM_DOWN, STR_LANDSCAPING_TOOLTIP_LOWER_A_CORNER_OF_LAND),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_RAISE_LAND), SetToolbarMinimalSize(1),
										SetFill(0, 1), SetSpriteTip(SPR_IMG_TERRAFORM_UP, STR_LANDSCAPING_TOOLTIP_RAISE_A_CORNER_OF_LAND),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_LEVEL_LAND), SetToolbarMinimalSize(1),
										SetFill(0, 1), SetSpriteTip(SPR_IMG_LEVEL_LAND, STR_LANDSCAPING_LEVEL_LAND_TOOLTIP),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_PLACE_ROCKS), SetToolbarMinimalSize(1),
										SetFill(0, 1), SetSpriteTip(SPR_IMG_ROCKS, STR_TERRAFORM_TOOLTIP_PLACE_ROCKY_AREAS_ON_LANDSCAPE),
			NWidget(NWID_SELECTION, INVALID_COLOUR, WID_ETT_SHOW_PLACE_DESERT),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_PLACE_DESERT), SetToolbarMinimalSize(1),
											SetFill(0, 1), SetSpriteTip(SPR_IMG_DESERT, STR_TERRAFORM_TOOLTIP_DEFINE_DESERT_AREA),
			EndContainer(),
			NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_ETT_PLACE_OBJECT), SetToolbarMinimalSize(1),
										SetFill(0, 1), SetSpriteTip(SPR_IMG_TRANSMITTER, STR_SCENEDIT_TOOLBAR_PLACE_OBJECT_TOOLTIP),
			NWidget(NWID_SPACER), SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_HORIZONTAL),
			NWidget(NWID_SPACER), SetFill(1, 0),
			NWidget(WWT_EMPTY, INVALID_COLOUR, WID_ETT_DOTS), SetMinimalSize(59, 31), SetStringTip(STR_EMPTY),
			NWidget(NWID_SPACER), SetFill(1, 0),
			NWidget(NWID_VERTICAL),
				NWidget(NWID_SPACER), SetFill(0, 1),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_INCREASE_SIZE), SetMinimalSize(12, 12), SetSpriteTip(SPR_ARROW_UP, STR_TERRAFORM_TOOLTIP_INCREASE_SIZE_OF_LAND_AREA),
				NWidget(NWID_SPACER), SetMinimalSize(0, 1),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_DECREASE_SIZE), SetMinimalSize(12, 12), SetSpriteTip(SPR_ARROW_DOWN, STR_TERRAFORM_TOOLTIP_DECREASE_SIZE_OF_LAND_AREA),
				NWidget(NWID_SPACER), SetFill(0, 1),
			EndContainer(),
			NWidget(NWID_SPACER), SetMinimalSize(2, 0),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 6),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_ETT_NEW_SCENARIO), SetMinimalSize(160, 12),
								SetFill(1, 0), SetStringTip(STR_TERRAFORM_SE_NEW_WORLD, STR_TERRAFORM_TOOLTIP_GENERATE_RANDOM_LAND), SetPadding(0, 2, 0, 2),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_ETT_RESET_LANDSCAPE), SetMinimalSize(160, 12),
								SetFill(1, 0), SetStringTip(STR_TERRAFORM_RESET_LANDSCAPE, STR_TERRAFORM_RESET_LANDSCAPE_TOOLTIP), SetPadding(1, 2, 2, 2),
	EndContainer(),
};

/**
 * Callback function for the scenario editor 'reset landscape' confirmation window
 * @param confirmed boolean value, true when yes was clicked, false otherwise
 */
static void ResetLandscapeConfirmationCallback(Window *, bool confirmed)
{
	if (confirmed) {
		/* Set generating_world to true to get instant-green grass after removing
		 * company property. */
		Backup<bool> old_generating_world(_generating_world, true);

		/* Delete all companies */
		for (Company *c : Company::Iterate()) {
			ChangeOwnershipOfCompanyItems(c->index, INVALID_OWNER);
			delete c;
		}

		old_generating_world.Restore();

		/* Delete all station signs */
		for (BaseStation *st : BaseStation::Iterate()) {
			/* There can be buoys, remove them */
			if (IsBuoyTile(st->xy)) Command<CMD_LANDSCAPE_CLEAR>::Do({DoCommandFlag::Execute, DoCommandFlag::Bankrupt}, st->xy);
			if (!st->IsInUse()) delete st;
		}

		/* Now that all vehicles are gone, we can reset the engine pool. Maybe it reduces some NewGRF changing-mess */
		EngineOverrideManager::ResetToCurrentNewGRFConfig();

		MarkWholeScreenDirty();
	}
}

/** Landscape generation window handler in the scenario editor. */
struct ScenarioEditorLandscapeGenerationWindow : Window {
	int last_user_action = INVALID_WID_ETT; ///< Last started user action.

	ScenarioEditorLandscapeGenerationWindow(WindowDesc &desc, WindowNumber window_number) : Window(desc)
	{
		this->CreateNestedTree();
		NWidgetStacked *show_desert = this->GetWidget<NWidgetStacked>(WID_ETT_SHOW_PLACE_DESERT);
		show_desert->SetDisplayedPlane(_settings_game.game_creation.landscape == LandscapeType::Tropic ? 0 : SZSP_NONE);
		this->FinishInitNested(window_number);
	}

	void OnPaint() override
	{
		this->DrawWidgets();

		if (this->IsWidgetLowered(WID_ETT_LOWER_LAND) || this->IsWidgetLowered(WID_ETT_RAISE_LAND)) { // change area-size if raise/lower corner is selected
			SetTileSelectSize(_terraform_size, _terraform_size);
		}
	}

	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		if (widget != WID_ETT_DOTS) return;

		size.width  = std::max<uint>(size.width,  ScaleGUITrad(59));
		size.height = std::max<uint>(size.height, ScaleGUITrad(31));
	}

	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		if (widget != WID_ETT_DOTS) return;

		int center_x = RoundDivSU(r.left + r.right, 2);
		int center_y = RoundDivSU(r.top + r.bottom, 2);

		int n = _terraform_size * _terraform_size;
		const int8_t *coords = &_multi_terraform_coords[0][0];

		assert(n != 0);
		do {
			DrawSprite(SPR_WHITE_POINT, PAL_NONE, center_x + ScaleGUITrad(coords[0]), center_y + ScaleGUITrad(coords[1]));
			coords += 2;
		} while (--n);
	}

	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		if (widget < WID_ETT_BUTTONS_START) return;

		switch (widget) {
			case WID_ETT_DEMOLISH: // Demolish aka dynamite button
				HandlePlacePushButton(this, WID_ETT_DEMOLISH, ANIMCURSOR_DEMOLISH, HT_RECT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_ETT_LOWER_LAND: // Lower land button
				HandlePlacePushButton(this, WID_ETT_LOWER_LAND, ANIMCURSOR_LOWERLAND, HT_POINT);
				this->last_user_action = widget;
				break;

			case WID_ETT_RAISE_LAND: // Raise land button
				HandlePlacePushButton(this, WID_ETT_RAISE_LAND, ANIMCURSOR_RAISELAND, HT_POINT);
				this->last_user_action = widget;
				break;

			case WID_ETT_LEVEL_LAND: // Level land button
				HandlePlacePushButton(this, WID_ETT_LEVEL_LAND, SPR_CURSOR_LEVEL_LAND, HT_POINT | HT_DIAGONAL);
				this->last_user_action = widget;
				break;

			case WID_ETT_PLACE_ROCKS: // Place rocks button
				HandlePlacePushButton(this, WID_ETT_PLACE_ROCKS, SPR_CURSOR_ROCKY_AREA, HT_RECT);
				this->last_user_action = widget;
				break;

			case WID_ETT_PLACE_DESERT: // Place desert button (in tropical climate)
				HandlePlacePushButton(this, WID_ETT_PLACE_DESERT, SPR_CURSOR_DESERT, HT_RECT);
				this->last_user_action = widget;
				break;

			case WID_ETT_PLACE_OBJECT: // Place transmitter button
				ShowBuildObjectPicker();
				break;

			case WID_ETT_INCREASE_SIZE:
			case WID_ETT_DECREASE_SIZE: { // Increase/Decrease terraform size
				int size = (widget == WID_ETT_INCREASE_SIZE) ? 1 : -1;
				this->HandleButtonClick(widget);
				size += _terraform_size;

				if (!IsInsideMM(size, 1, 8 + 1)) return;
				_terraform_size = size;

				SndClickBeep();
				this->SetDirty();
				break;
			}

			case WID_ETT_NEW_SCENARIO: // gen random land
				this->HandleButtonClick(widget);
				ShowCreateScenario();
				break;

			case WID_ETT_RESET_LANDSCAPE: // Reset landscape
				ShowQuery(
					GetEncodedString(STR_QUERY_RESET_LANDSCAPE_CAPTION),
					GetEncodedString(STR_RESET_LANDSCAPE_CONFIRMATION_TEXT),
					nullptr, ResetLandscapeConfirmationCallback);
				break;

			default: NOT_REACHED();
		}
	}

	void OnTimeout() override
	{
		for (const auto &pair : this->widget_lookup) {
			if (pair.first < WID_ETT_START || (pair.first >= WID_ETT_BUTTONS_START && pair.first < WID_ETT_BUTTONS_END)) continue; // skip the buttons
			this->RaiseWidgetWhenLowered(pair.first);
		}
	}

	void OnPlaceObject([[maybe_unused]] Point pt, TileIndex tile) override
	{
		switch (this->last_user_action) {
			case WID_ETT_DEMOLISH: // Demolish aka dynamite button
				PlaceProc_DemolishArea(tile);
				break;

			case WID_ETT_LOWER_LAND: // Lower land button
				CommonRaiseLowerBigLand(tile, false);
				break;

			case WID_ETT_RAISE_LAND: // Raise land button
				CommonRaiseLowerBigLand(tile, true);
				break;

			case WID_ETT_LEVEL_LAND: // Level land button
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_LEVEL_AREA);
				break;

			case WID_ETT_PLACE_ROCKS: // Place rocks button
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_CREATE_ROCKS);
				break;

			case WID_ETT_PLACE_DESERT: // Place desert button (in tropical climate)
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_CREATE_DESERT);
				break;

			default: NOT_REACHED();
		}
	}

	void OnPlaceDrag(ViewportPlaceMethod select_method, [[maybe_unused]] ViewportDragDropSelectionProcess select_proc, [[maybe_unused]] Point pt) override
	{
		VpSelectTilesWithMethod(pt.x, pt.y, select_method);
	}

	void OnPlaceMouseUp([[maybe_unused]] ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, [[maybe_unused]] Point pt, TileIndex start_tile, TileIndex end_tile) override
	{
		if (pt.x != -1) {
			switch (select_proc) {
				default: NOT_REACHED();
				case DDSP_CREATE_ROCKS:
				case DDSP_CREATE_DESERT:
				case DDSP_RAISE_AND_LEVEL_AREA:
				case DDSP_LOWER_AND_LEVEL_AREA:
				case DDSP_LEVEL_AREA:
				case DDSP_DEMOLISH_AREA:
					GUIPlaceProcDragXY(select_proc, start_tile, end_tile);
					break;
			}
		}
	}

	void OnPlaceObjectAbort() override
	{
		this->RaiseButtons();
		this->SetDirty();
	}

	/**
	 * Handler for global hotkeys of the ScenarioEditorLandscapeGenerationWindow.
	 * @param hotkey Hotkey
	 * @return ES_HANDLED if hotkey was accepted.
	 */
	static EventState TerraformToolbarEditorGlobalHotkeys(int hotkey)
	{
		if (_game_mode != GM_EDITOR) return ES_NOT_HANDLED;
		Window *w = ShowEditorTerraformToolbar();
		if (w == nullptr) return ES_NOT_HANDLED;
		return w->OnHotkey(hotkey);
	}

	static inline HotkeyList hotkeys{"terraform_editor", {
		Hotkey('D' | WKC_GLOBAL_HOTKEY, "dynamite", WID_ETT_DEMOLISH),
		Hotkey('Q' | WKC_GLOBAL_HOTKEY, "lower", WID_ETT_LOWER_LAND),
		Hotkey('W' | WKC_GLOBAL_HOTKEY, "raise", WID_ETT_RAISE_LAND),
		Hotkey('E' | WKC_GLOBAL_HOTKEY, "level", WID_ETT_LEVEL_LAND),
		Hotkey('R', "rocky", WID_ETT_PLACE_ROCKS),
		Hotkey('T', "desert", WID_ETT_PLACE_DESERT),
		Hotkey('O', "object", WID_ETT_PLACE_OBJECT),
	}, TerraformToolbarEditorGlobalHotkeys};
};

static WindowDesc _scen_edit_land_gen_desc(
	WDP_AUTO, "toolbar_landscape_scen", 0, 0,
	WC_SCEN_LAND_GEN, WC_NONE,
	WindowDefaultFlag::Construction,
	_nested_scen_edit_land_gen_widgets,
	&ScenarioEditorLandscapeGenerationWindow::hotkeys
);

/**
 * Show the toolbar for terraforming in the scenario editor.
 * @return The allocated toolbar if the window was newly opened, else \c nullptr.
 */
Window *ShowEditorTerraformToolbar()
{
	return AllocateWindowDescFront<ScenarioEditorLandscapeGenerationWindow>(_scen_edit_land_gen_desc, 0);
}
