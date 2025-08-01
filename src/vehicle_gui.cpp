/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file vehicle_gui.cpp The base GUI for all vehicles. */

#include "stdafx.h"
#include "debug.h"
#include "company_func.h"
#include "gui.h"
#include "textbuf_gui.h"
#include "command_func.h"
#include "vehicle_gui_base.h"
#include "viewport_func.h"
#include "newgrf_text.h"
#include "newgrf_debug.h"
#include "roadveh.h"
#include "train.h"
#include "aircraft.h"
#include "depot_map.h"
#include "group_gui.h"
#include "strings_func.h"
#include "vehicle_func.h"
#include "autoreplace_gui.h"
#include "string_func.h"
#include "dropdown_type.h"
#include "dropdown_func.h"
#include "timetable.h"
#include "articulated_vehicles.h"
#include "spritecache.h"
#include "core/geometry_func.hpp"
#include "core/container_func.hpp"
#include "company_base.h"
#include "engine_func.h"
#include "station_base.h"
#include "tilehighlight_func.h"
#include "zoom_func.h"
#include "depot_cmd.h"
#include "vehicle_cmd.h"
#include "order_cmd.h"
#include "roadveh_cmd.h"
#include "train_cmd.h"
#include "hotkeys.h"
#include "group_cmd.h"

#include "table/strings.h"

#include "safeguards.h"


static std::array<std::array<BaseVehicleListWindow::GroupBy, VEH_COMPANY_END>, VLT_END> _grouping{};
static std::array<Sorting, BaseVehicleListWindow::GB_END> _sorting{};

static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleNumberSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleNameSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleAgeSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleProfitThisYearSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleProfitLastYearSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleCargoSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleReliabilitySorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleMaxSpeedSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleModelSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleValueSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleLengthSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleTimeToLiveSorter;
static BaseVehicleListWindow::VehicleIndividualSortFunction VehicleTimetableDelaySorter;
static BaseVehicleListWindow::VehicleGroupSortFunction VehicleGroupLengthSorter;
static BaseVehicleListWindow::VehicleGroupSortFunction VehicleGroupTotalProfitThisYearSorter;
static BaseVehicleListWindow::VehicleGroupSortFunction VehicleGroupTotalProfitLastYearSorter;
static BaseVehicleListWindow::VehicleGroupSortFunction VehicleGroupAverageProfitThisYearSorter;
static BaseVehicleListWindow::VehicleGroupSortFunction VehicleGroupAverageProfitLastYearSorter;

/** Wrapper to convert a VehicleIndividualSortFunction to a VehicleGroupSortFunction */
template <BaseVehicleListWindow::VehicleIndividualSortFunction func>
static bool VehicleIndividualToGroupSorterWrapper(GUIVehicleGroup const &a, GUIVehicleGroup const &b)
{
	return func(*(a.vehicles_begin), *(b.vehicles_begin));
}

const std::initializer_list<BaseVehicleListWindow::VehicleGroupSortFunction * const> BaseVehicleListWindow::vehicle_group_none_sorter_funcs = {
	&VehicleIndividualToGroupSorterWrapper<VehicleNumberSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleNameSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleAgeSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleProfitThisYearSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleProfitLastYearSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleCargoSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleReliabilitySorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleMaxSpeedSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleModelSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleValueSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleLengthSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleTimeToLiveSorter>,
	&VehicleIndividualToGroupSorterWrapper<VehicleTimetableDelaySorter>,
};

const std::initializer_list<const StringID> BaseVehicleListWindow::vehicle_group_none_sorter_names_calendar = {
	STR_SORT_BY_NUMBER,
	STR_SORT_BY_NAME,
	STR_SORT_BY_AGE,
	STR_SORT_BY_PROFIT_THIS_YEAR,
	STR_SORT_BY_PROFIT_LAST_YEAR,
	STR_SORT_BY_TOTAL_CAPACITY_PER_CARGOTYPE,
	STR_SORT_BY_RELIABILITY,
	STR_SORT_BY_MAX_SPEED,
	STR_SORT_BY_MODEL,
	STR_SORT_BY_VALUE,
	STR_SORT_BY_LENGTH,
	STR_SORT_BY_LIFE_TIME,
	STR_SORT_BY_TIMETABLE_DELAY,
};

const std::initializer_list<const StringID> BaseVehicleListWindow::vehicle_group_none_sorter_names_wallclock = {
	STR_SORT_BY_NUMBER,
	STR_SORT_BY_NAME,
	STR_SORT_BY_AGE,
	STR_SORT_BY_PROFIT_THIS_PERIOD,
	STR_SORT_BY_PROFIT_LAST_PERIOD,
	STR_SORT_BY_TOTAL_CAPACITY_PER_CARGOTYPE,
	STR_SORT_BY_RELIABILITY,
	STR_SORT_BY_MAX_SPEED,
	STR_SORT_BY_MODEL,
	STR_SORT_BY_VALUE,
	STR_SORT_BY_LENGTH,
	STR_SORT_BY_LIFE_TIME,
	STR_SORT_BY_TIMETABLE_DELAY,
};

const std::initializer_list<BaseVehicleListWindow::VehicleGroupSortFunction * const> BaseVehicleListWindow::vehicle_group_shared_orders_sorter_funcs = {
	&VehicleGroupLengthSorter,
	&VehicleGroupTotalProfitThisYearSorter,
	&VehicleGroupTotalProfitLastYearSorter,
	&VehicleGroupAverageProfitThisYearSorter,
	&VehicleGroupAverageProfitLastYearSorter,
};

const std::initializer_list<const StringID> BaseVehicleListWindow::vehicle_group_shared_orders_sorter_names_calendar = {
	STR_SORT_BY_NUM_VEHICLES,
	STR_SORT_BY_TOTAL_PROFIT_THIS_YEAR,
	STR_SORT_BY_TOTAL_PROFIT_LAST_YEAR,
	STR_SORT_BY_AVERAGE_PROFIT_THIS_YEAR,
	STR_SORT_BY_AVERAGE_PROFIT_LAST_YEAR,
};

const std::initializer_list<const StringID> BaseVehicleListWindow::vehicle_group_shared_orders_sorter_names_wallclock = {
	STR_SORT_BY_NUM_VEHICLES,
	STR_SORT_BY_TOTAL_PROFIT_THIS_PERIOD,
	STR_SORT_BY_TOTAL_PROFIT_LAST_PERIOD,
	STR_SORT_BY_AVERAGE_PROFIT_THIS_PERIOD,
	STR_SORT_BY_AVERAGE_PROFIT_LAST_PERIOD,
};

const std::initializer_list<const StringID> BaseVehicleListWindow::vehicle_group_by_names = {
	STR_GROUP_BY_NONE,
	STR_GROUP_BY_SHARED_ORDERS,
};

const StringID BaseVehicleListWindow::vehicle_depot_name[] = {
	STR_VEHICLE_LIST_SEND_TRAIN_TO_DEPOT,
	STR_VEHICLE_LIST_SEND_ROAD_VEHICLE_TO_DEPOT,
	STR_VEHICLE_LIST_SEND_SHIP_TO_DEPOT,
	STR_VEHICLE_LIST_SEND_AIRCRAFT_TO_HANGAR
};

BaseVehicleListWindow::BaseVehicleListWindow(WindowDesc &desc, const VehicleListIdentifier &vli) : Window(desc), vli(vli)
{
	this->vehicle_sel = VehicleID::Invalid();
	this->grouping = _grouping[vli.type][vli.vtype];
	this->UpdateSortingFromGrouping();
}

std::span<const StringID> BaseVehicleListWindow::GetVehicleSorterNames() const
{
	switch (this->grouping) {
		case GB_NONE:
			return TimerGameEconomy::UsingWallclockUnits() ? vehicle_group_none_sorter_names_wallclock : vehicle_group_none_sorter_names_calendar;
		case GB_SHARED_ORDERS:
			return TimerGameEconomy::UsingWallclockUnits() ? vehicle_group_shared_orders_sorter_names_wallclock : vehicle_group_shared_orders_sorter_names_calendar;
		default:
			NOT_REACHED();
	}
}

/**
 * Get the number of digits of space required for the given number.
 * @param number The number.
 * @return The number of digits to allocate space for.
 */
uint CountDigitsForAllocatingSpace(uint number)
{
	if (number >= 10000) return 5;
	if (number >= 1000) return 4;
	if (number >= 100) return 3;

	/*
	 * When the smallest unit number is less than 10, it is
	 * quite likely that it will expand to become more than
	 * 10 quite soon.
	 */
	return 2;
}

/**
 * Get the number of digits the biggest unit number of a set of vehicles has.
 * @param vehicles The list of vehicles.
 * @return The number of digits to allocate space for.
 */
uint GetUnitNumberDigits(VehicleList &vehicles)
{
	uint unitnumber = 0;
	for (const Vehicle *v : vehicles) {
		unitnumber = std::max<uint>(unitnumber, v->unitnumber);
	}

	return CountDigitsForAllocatingSpace(unitnumber);
}

void BaseVehicleListWindow::BuildVehicleList()
{
	if (!this->vehgroups.NeedRebuild()) return;

	Debug(misc, 3, "Building vehicle list type {} for company {} given index {}", this->vli.type, this->vli.company, this->vli.index);

	this->vehgroups.clear();

	GenerateVehicleSortList(&this->vehicles, this->vli);

	CargoTypes used = 0;
	for (const Vehicle *v : this->vehicles) {
		for (const Vehicle *u = v; u != nullptr; u = u->Next()) {
			if (u->cargo_cap > 0) SetBit(used, u->cargo_type);
		}
	}
	this->used_cargoes = used;

	if (this->grouping == GB_NONE) {
		uint max_unitnumber = 0;
		for (auto it = this->vehicles.begin(); it != this->vehicles.end(); ++it) {
			this->vehgroups.emplace_back(it, it + 1);

			max_unitnumber = std::max<uint>(max_unitnumber, (*it)->unitnumber);
		}
		this->unitnumber_digits = CountDigitsForAllocatingSpace(max_unitnumber);
	} else {
		/* Sort by the primary vehicle; we just want all vehicles that share the same orders to form a contiguous range. */
		std::stable_sort(this->vehicles.begin(), this->vehicles.end(), [](const Vehicle * const &u, const Vehicle * const &v) {
			return u->FirstShared() < v->FirstShared();
		});

		uint max_num_vehicles = 0;

		VehicleList::const_iterator begin = this->vehicles.begin();
		while (begin != this->vehicles.end()) {
			VehicleList::const_iterator end = std::find_if_not(begin, this->vehicles.cend(), [first_shared = (*begin)->FirstShared()](const Vehicle * const &v) {
				return v->FirstShared() == first_shared;
			});

			this->vehgroups.emplace_back(begin, end);

			max_num_vehicles = std::max<uint>(max_num_vehicles, static_cast<uint>(end - begin));

			begin = end;
		}

		this->unitnumber_digits = CountDigitsForAllocatingSpace(max_num_vehicles);
	}
	this->FilterVehicleList();

	this->vehgroups.RebuildDone();
	this->vscroll->SetCount(this->vehgroups.size());
}

/**
 * Check whether a single vehicle should pass the filter.
 *
 * @param v The vehicle to check.
 * @param cargo_type The cargo to filter for.
 * @return true iff the vehicle carries the cargo.
 */
static bool CargoFilterSingle(const Vehicle *v, const CargoType cargo_type)
{
	if (cargo_type == CargoFilterCriteria::CF_ANY) {
		return true;
	} else if (cargo_type == CargoFilterCriteria::CF_NONE) {
		for (const Vehicle *w = v; w != nullptr; w = w->Next()) {
			if (w->cargo_cap > 0) {
				return false;
			}
		}
		return true;
	} else if (cargo_type == CargoFilterCriteria::CF_FREIGHT) {
		bool have_capacity = false;
		for (const Vehicle *w = v; w != nullptr; w = w->Next()) {
			if (w->cargo_cap > 0) {
				if (IsCargoInClass(w->cargo_type, CargoClass::Passengers)) {
					return false;
				} else {
					have_capacity = true;
				}
			}
		}
		return have_capacity;
	} else {
		for (const Vehicle *w = v; w != nullptr; w = w->Next()) {
			if (w->cargo_cap > 0 && w->cargo_type == cargo_type) {
				return true;
			}
		}
		return false;
	}
}

/**
 * Check whether a vehicle can carry a specific cargo.
 *
 * @param vehgroup The vehicle group which contains the vehicle to be checked
 * @param cargo_type The cargo what we are looking for
 * @return Whether the vehicle can carry the specified cargo or not
 */
static bool CargoFilter(const GUIVehicleGroup *vehgroup, const CargoType cargo_type)
{
	auto it = vehgroup->vehicles_begin;

	/* Check if any vehicle in the group matches; if so, the whole group does. */
	for (; it != vehgroup->vehicles_end; it++) {
		if (CargoFilterSingle(*it, cargo_type)) return true;
	}

	return false;
}

/**
 * Test if cargo icon overlays should be drawn.
 * @returns true iff cargo icon overlays should be drawn.
 */
bool ShowCargoIconOverlay()
{
	return _shift_pressed && _ctrl_pressed;
}

/**
 * Add a cargo icon to the list of overlays.
 * @param overlays List of overlays.
 * @param x Horizontal position.
 * @param width Width available.
 * @param v Vehicle to add.
 */
void AddCargoIconOverlay(std::vector<CargoIconOverlay> &overlays, int x, int width, const Vehicle *v)
{
	bool rtl = _current_text_dir == TD_RTL;
	if (!v->IsArticulatedPart() || v->cargo_type != v->Previous()->cargo_type) {
		/* Add new overlay slot. */
		overlays.emplace_back(rtl ? x - width : x, rtl ? x : x + width, v->cargo_type, v->cargo_cap);
	} else {
		/* This is an articulated part with the same cargo type, adjust left or right of last overlay slot. */
		if (rtl) {
			overlays.back().left -= width;
		} else {
			overlays.back().right += width;
		}
		overlays.back().cargo_cap += v->cargo_cap;
	}
}

/**
 * Draw a cargo icon overlaying an existing sprite, with a black contrast outline.
 * @param x Horizontal position from left.
 * @param y Vertical position from top.
 * @param cargo_type Cargo type to draw icon for.
 */
void DrawCargoIconOverlay(int x, int y, CargoType cargo_type)
{
	if (!ShowCargoIconOverlay()) return;
	if (!IsValidCargoType(cargo_type)) return;

	const CargoSpec *cs = CargoSpec::Get(cargo_type);

	SpriteID spr = cs->GetCargoIcon();
	if (spr == 0) return;

	Dimension d = GetSpriteSize(spr);
	d.width /= 2;
	d.height /= 2;
	int one = ScaleGUITrad(1);

	/* Draw the cargo icon in black shifted 4 times to create the outline. */
	DrawSprite(spr, PALETTE_ALL_BLACK, x - d.width - one, y - d.height);
	DrawSprite(spr, PALETTE_ALL_BLACK, x - d.width + one, y - d.height);
	DrawSprite(spr, PALETTE_ALL_BLACK, x - d.width, y - d.height - one);
	DrawSprite(spr, PALETTE_ALL_BLACK, x - d.width, y - d.height + one);
	/* Draw the cargo icon normally. */
	DrawSprite(spr, PAL_NONE, x - d.width, y - d.height);
}

/**
 * Draw a list of cargo icon overlays.
 * @param overlays List of overlays.
 * @param y Vertical position.
 */
void DrawCargoIconOverlays(std::span<const CargoIconOverlay> overlays, int y)
{
	for (const auto &cio : overlays) {
		if (cio.cargo_cap == 0) continue;
		DrawCargoIconOverlay((cio.left + cio.right) / 2, y, cio.cargo_type);
	}
}

static GUIVehicleGroupList::FilterFunction * const _vehicle_group_filter_funcs[] = {
	&CargoFilter,
};

/**
 * Set cargo filter for the vehicle group list.
 * @param cargo_type The cargo to be set.
 */
void BaseVehicleListWindow::SetCargoFilter(CargoType cargo_type)
{
	if (this->cargo_filter_criteria != cargo_type) {
		this->cargo_filter_criteria = cargo_type;
		/* Deactivate filter if criteria is 'Show All', activate it otherwise. */
		this->vehgroups.SetFilterState(this->cargo_filter_criteria != CargoFilterCriteria::CF_ANY);
		this->vehgroups.SetFilterType(0);
		this->vehgroups.ForceRebuild();
	}
}

/**
 *Populate the filter list and set the cargo filter criteria.
 */
void BaseVehicleListWindow::SetCargoFilterArray()
{
	this->cargo_filter_criteria = CargoFilterCriteria::CF_ANY;
	this->vehgroups.SetFilterFuncs(_vehicle_group_filter_funcs);
	this->vehgroups.SetFilterState(this->cargo_filter_criteria != CargoFilterCriteria::CF_ANY);
}

/**
 *Filter the engine list against the currently selected cargo filter.
 */
void BaseVehicleListWindow::FilterVehicleList()
{
	this->vehgroups.Filter(this->cargo_filter_criteria);
	if (this->vehicles.empty()) {
		/* No vehicle passed through the filter, invalidate the previously selected vehicle */
		this->vehicle_sel = VehicleID::Invalid();
	} else if (this->vehicle_sel != VehicleID::Invalid() && std::ranges::find(this->vehicles, Vehicle::Get(this->vehicle_sel)) == this->vehicles.end()) { // previously selected engine didn't pass the filter, remove selection
		this->vehicle_sel = VehicleID::Invalid();
	}
}

/**
 * Compute the size for the Action dropdown.
 * @param show_autoreplace If true include the autoreplace item.
 * @param show_group If true include group-related stuff.
 * @param show_create If true include group-create item.
 * @return Required size.
 */
Dimension BaseVehicleListWindow::GetActionDropdownSize(bool show_autoreplace, bool show_group, bool show_create)
{
	Dimension d = {0, 0};

	if (show_autoreplace) d = maxdim(d, GetStringBoundingBox(STR_VEHICLE_LIST_REPLACE_VEHICLES));
	d = maxdim(d, GetStringBoundingBox(STR_VEHICLE_LIST_SEND_FOR_SERVICING));
	d = maxdim(d, GetStringBoundingBox(this->vehicle_depot_name[this->vli.vtype]));

	if (show_group) {
		d = maxdim(d, GetStringBoundingBox(STR_GROUP_ADD_SHARED_VEHICLE));
		d = maxdim(d, GetStringBoundingBox(STR_GROUP_REMOVE_ALL_VEHICLES));
	} else if (show_create) {
		d = maxdim(d, GetStringBoundingBox(STR_VEHICLE_LIST_CREATE_GROUP));
	}

	return d;
}

void BaseVehicleListWindow::OnInit()
{
	this->order_arrow_width = std::max(GetStringBoundingBox(STR_JUST_LEFT_ARROW, FS_SMALL).width, GetStringBoundingBox(STR_JUST_RIGHT_ARROW, FS_SMALL).width);
	this->SetCargoFilterArray();
}

StringID BaseVehicleListWindow::GetCargoFilterLabel(CargoType cargo_type) const
{
	switch (cargo_type) {
		case CargoFilterCriteria::CF_ANY: return STR_CARGO_TYPE_FILTER_ALL;
		case CargoFilterCriteria::CF_FREIGHT: return STR_CARGO_TYPE_FILTER_FREIGHT;
		case CargoFilterCriteria::CF_NONE: return STR_CARGO_TYPE_FILTER_NONE;
		default: return CargoSpec::Get(cargo_type)->name;
	}
}

/**
 * Build drop down list for cargo filter selection.
 * @param full If true, build list with all cargo types, instead of only used cargo types.
 * @return Drop down list for cargo filter.
 */
DropDownList BaseVehicleListWindow::BuildCargoDropDownList(bool full) const
{
	DropDownList list;

	/* Add item for disabling filtering. */
	list.push_back(MakeDropDownListStringItem(this->GetCargoFilterLabel(CargoFilterCriteria::CF_ANY), CargoFilterCriteria::CF_ANY));
	/* Add item for freight (i.e. vehicles with cargo capacity and with no passenger capacity). */
	list.push_back(MakeDropDownListStringItem(this->GetCargoFilterLabel(CargoFilterCriteria::CF_FREIGHT), CargoFilterCriteria::CF_FREIGHT));
	/* Add item for vehicles not carrying anything, e.g. train engines. */
	list.push_back(MakeDropDownListStringItem(this->GetCargoFilterLabel(CargoFilterCriteria::CF_NONE), CargoFilterCriteria::CF_NONE));

	/* Add cargos */
	Dimension d = GetLargestCargoIconSize();
	for (const CargoSpec *cs : _sorted_cargo_specs) {
		if (!full && !HasBit(this->used_cargoes, cs->Index())) continue;
		list.push_back(MakeDropDownListIconItem(d, cs->GetCargoIcon(), PAL_NONE, cs->name, cs->Index(), false, !HasBit(this->used_cargoes, cs->Index())));
	}

	return list;
}

/**
 * Display the Action dropdown window.
 * @param show_autoreplace If true include the autoreplace item.
 * @param show_group If true include group-related stuff.
 * @param show_create If true include group-create item.
 * @return Itemlist for dropdown
 */
DropDownList BaseVehicleListWindow::BuildActionDropdownList(bool show_autoreplace, bool show_group, bool show_create)
{
	DropDownList list;

	/* Autoreplace actions. */
	if (show_autoreplace) {
		list.push_back(MakeDropDownListStringItem(STR_VEHICLE_LIST_REPLACE_VEHICLES, ADI_REPLACE));
		list.push_back(MakeDropDownListDividerItem());
	}

	/* Group actions. */
	if (show_group) {
		list.push_back(MakeDropDownListStringItem(STR_GROUP_ADD_SHARED_VEHICLE, ADI_ADD_SHARED));
		list.push_back(MakeDropDownListStringItem(STR_GROUP_REMOVE_ALL_VEHICLES, ADI_REMOVE_ALL));
		list.push_back(MakeDropDownListDividerItem());
	} else if (show_create) {
		list.push_back(MakeDropDownListStringItem(STR_VEHICLE_LIST_CREATE_GROUP, ADI_CREATE_GROUP));
		list.push_back(MakeDropDownListDividerItem());
	}

	/* Depot actions. */
	list.push_back(MakeDropDownListStringItem(STR_VEHICLE_LIST_SEND_FOR_SERVICING, ADI_SERVICE));
	list.push_back(MakeDropDownListStringItem(this->vehicle_depot_name[this->vli.vtype], ADI_DEPOT));

	return list;
}

/* cached values for VehicleNameSorter to spare many GetString() calls */
static const Vehicle *_last_vehicle[2] = { nullptr, nullptr };

void BaseVehicleListWindow::SortVehicleList()
{
	if (this->vehgroups.Sort()) return;

	/* invalidate cached values for name sorter - vehicle names could change */
	_last_vehicle[0] = _last_vehicle[1] = nullptr;
}

void DepotSortList(VehicleList *list)
{
	if (list->size() < 2) return;
	std::sort(list->begin(), list->end(), &VehicleNumberSorter);
}

/** draw the vehicle profit button in the vehicle list window. */
static void DrawVehicleProfitButton(TimerGameEconomy::Date age, Money display_profit_last_year, uint num_vehicles, int x, int y)
{
	SpriteID spr;

	/* draw profit-based coloured icons */
	if (age <= VEHICLE_PROFIT_MIN_AGE) {
		spr = SPR_PROFIT_NA;
	} else if (display_profit_last_year < 0) {
		spr = SPR_PROFIT_NEGATIVE;
	} else if (display_profit_last_year < VEHICLE_PROFIT_THRESHOLD * num_vehicles) {
		spr = SPR_PROFIT_SOME;
	} else {
		spr = SPR_PROFIT_LOT;
	}
	DrawSprite(spr, PAL_NONE, x, y);
}

/** Maximum number of refit cycles we try, to prevent infinite loops. And we store only a byte anyway */
static const uint MAX_REFIT_CYCLE = 256;

/**
 * Get the best fitting subtype when 'cloning'/'replacing' \a v_from with \a v_for.
 * All articulated parts of both vehicles are tested to find a possibly shared subtype.
 * For \a v_for only vehicle refittable to \a dest_cargo_type are considered.
 * @param v_from the vehicle to match the subtype from
 * @param v_for  the vehicle to get the subtype for
 * @param dest_cargo_type Destination cargo type.
 * @return the best sub type
 */
uint8_t GetBestFittingSubType(Vehicle *v_from, Vehicle *v_for, CargoType dest_cargo_type)
{
	v_from = v_from->GetFirstEnginePart();
	v_for = v_for->GetFirstEnginePart();

	/* Create a list of subtypes used by the various parts of v_for */
	static std::vector<StringID> subtypes;
	subtypes.clear();
	for (; v_from != nullptr; v_from = v_from->HasArticulatedPart() ? v_from->GetNextArticulatedPart() : nullptr) {
		const Engine *e_from = v_from->GetEngine();
		if (!e_from->CanCarryCargo() || !e_from->info.callback_mask.Test(VehicleCallbackMask::CargoSuffix)) continue;
		include(subtypes, GetCargoSubtypeText(v_from));
	}

	uint8_t ret_refit_cyc = 0;
	bool success = false;
	if (!subtypes.empty()) {
		/* Check whether any articulated part is refittable to 'dest_cargo_type' with a subtype listed in 'subtypes' */
		for (Vehicle *v = v_for; v != nullptr; v = v->HasArticulatedPart() ? v->GetNextArticulatedPart() : nullptr) {
			const Engine *e = v->GetEngine();
			if (!e->CanCarryCargo() || !e->info.callback_mask.Test(VehicleCallbackMask::CargoSuffix)) continue;
			if (!HasBit(e->info.refit_mask, dest_cargo_type) && v->cargo_type != dest_cargo_type) continue;

			CargoType old_cargo_type = v->cargo_type;
			uint8_t old_cargo_subtype = v->cargo_subtype;

			/* Set the 'destination' cargo */
			v->cargo_type = dest_cargo_type;

			/* Cycle through the refits */
			for (uint refit_cyc = 0; refit_cyc < MAX_REFIT_CYCLE; refit_cyc++) {
				v->cargo_subtype = refit_cyc;

				/* Make sure we don't pick up anything cached. */
				v->First()->InvalidateNewGRFCache();
				v->InvalidateNewGRFCache();

				StringID subtype = GetCargoSubtypeText(v);
				if (subtype == STR_EMPTY) break;

				if (std::ranges::find(subtypes, subtype) == subtypes.end()) continue;

				/* We found something matching. */
				ret_refit_cyc = refit_cyc;
				success = true;
				break;
			}

			/* Reset the vehicle's cargo type */
			v->cargo_type    = old_cargo_type;
			v->cargo_subtype = old_cargo_subtype;

			/* Make sure we don't taint the vehicle. */
			v->First()->InvalidateNewGRFCache();
			v->InvalidateNewGRFCache();

			if (success) break;
		}
	}

	return ret_refit_cyc;
}

/** Option to refit a vehicle chain */
struct RefitOption {
	CargoType cargo;    ///< Cargo to refit to
	uint8_t subtype;     ///< Subcargo to use
	StringID string;  ///< GRF-local String to display for the cargo

	/**
	 * Equality operator for #RefitOption.
	 * @param other Compare to this #RefitOption.
	 * @return True if both #RefitOption are equal.
	 */
	inline bool operator == (const RefitOption &other) const
	{
		return other.cargo == this->cargo && other.string == this->string;
	}
};

using RefitOptions = std::map<CargoType, std::vector<RefitOption>, CargoTypeComparator>; ///< Available refit options (subtype and string) associated with each cargo type.

/**
 * Draw the list of available refit options for a consist and highlight the selected refit option (if any).
 * @param refits Available refit options for each (sorted) cargo.
 * @param sel   Selected refit option in the window
 * @param pos   Position of the selected item in caller widow
 * @param rows  Number of rows(capacity) in caller window
 * @param delta Step height in caller window
 * @param r     Rectangle of the matrix widget.
 */
static void DrawVehicleRefitWindow(const RefitOptions &refits, const RefitOption *sel, uint pos, uint rows, uint delta, const Rect &r)
{
	Rect ir = r.Shrink(WidgetDimensions::scaled.matrix);
	uint current = 0;

	bool rtl = _current_text_dir == TD_RTL;
	uint iconwidth = std::max(GetSpriteSize(SPR_CIRCLE_FOLDED).width, GetSpriteSize(SPR_CIRCLE_UNFOLDED).width);
	uint iconheight = GetSpriteSize(SPR_CIRCLE_FOLDED).height;
	PixelColour linecolour = GetColourGradient(COLOUR_ORANGE, SHADE_NORMAL);

	int iconleft   = rtl ? ir.right - iconwidth     : ir.left;
	int iconcenter = rtl ? ir.right - iconwidth / 2 : ir.left + iconwidth / 2;
	int iconinner  = rtl ? ir.right - iconwidth     : ir.left + iconwidth;

	Rect tr = ir.Indent(iconwidth + WidgetDimensions::scaled.hsep_wide, rtl);

	/* Draw the list of subtypes for each cargo, and find the selected refit option (by its position). */
	for (const auto &pair : refits) {
		bool has_subtypes = pair.second.size() > 1;
		for (const RefitOption &refit : pair.second) {
			if (current >= pos + rows) break;

			/* Hide subtypes if selected cargo type does not match */
			if ((sel == nullptr || sel->cargo != refit.cargo) && refit.subtype != UINT8_MAX) continue;

			/* Refit options with a position smaller than pos don't have to be drawn. */
			if (current < pos) {
				current++;
				continue;
			}

			if (has_subtypes) {
				if (refit.subtype != UINT8_MAX) {
					/* Draw tree lines */
					int ycenter = tr.top + GetCharacterHeight(FS_NORMAL) / 2;
					GfxDrawLine(iconcenter, tr.top - WidgetDimensions::scaled.matrix.top, iconcenter, (&refit == &pair.second.back()) ? ycenter : tr.top - WidgetDimensions::scaled.matrix.top + delta - 1, linecolour);
					GfxDrawLine(iconcenter, ycenter, iconinner, ycenter, linecolour);
				} else {
					/* Draw expand/collapse icon */
					DrawSprite((sel != nullptr && sel->cargo == refit.cargo) ? SPR_CIRCLE_UNFOLDED : SPR_CIRCLE_FOLDED, PAL_NONE, iconleft, tr.top + (GetCharacterHeight(FS_NORMAL) - iconheight) / 2);
				}
			}

			TextColour colour = (sel != nullptr && sel->cargo == refit.cargo && sel->subtype == refit.subtype) ? TC_WHITE : TC_BLACK;
			/* Get the cargo name. */
			DrawString(tr, GetString(STR_JUST_STRING_STRING, CargoSpec::Get(refit.cargo)->name, refit.string), colour);

			tr.top += delta;
			current++;
		}
	}
}

/** Refit cargo window. */
struct RefitWindow : public Window {
	const RefitOption *selected_refit = nullptr; ///< Selected refit option.
	RefitOptions refit_list{}; ///< List of refit subtypes available for each sorted cargo.
	VehicleOrderID order = INVALID_VEH_ORDER_ID; ///< If not #INVALID_VEH_ORDER_ID, selection is part of a refit order (rather than execute directly).
	uint information_width = 0; ///< Width required for correctly displaying all cargoes in the information panel.
	Scrollbar *vscroll = nullptr; ///< The main scrollbar.
	Scrollbar *hscroll = nullptr; ///< Only used for long vehicles.
	int vehicle_width = 0; ///< Width of the vehicle being drawn.
	int sprite_left = 0; ///< Left position of the vehicle sprite.
	int sprite_right = 0; ///< Right position of the vehicle sprite.
	uint vehicle_margin = 0; ///< Margin to use while selecting vehicles when the vehicle image is centered.
	int click_x = 0; ///< Position of the first click while dragging.
	VehicleID selected_vehicle{}; ///< First vehicle in the current selection.
	uint8_t num_vehicles = 0; ///< Number of selected vehicles.
	bool auto_refit = false; ///< Select cargo for auto-refitting.

	/**
	 * Collects all (cargo, subcargo) refit options of a vehicle chain.
	 */
	void BuildRefitList()
	{
		/* Store the currently selected RefitOption. */
		std::optional<RefitOption> current_refit_option;
		if (this->selected_refit != nullptr) current_refit_option = *(this->selected_refit);
		this->selected_refit = nullptr;

		this->refit_list.clear();
		Vehicle *v = Vehicle::Get(this->window_number);

		/* Check only the selected vehicles. */
		VehicleSet vehicles_to_refit;
		GetVehicleSet(vehicles_to_refit, Vehicle::Get(this->selected_vehicle), this->num_vehicles);

		do {
			if (v->type == VEH_TRAIN && std::ranges::find(vehicles_to_refit, v->index) == vehicles_to_refit.end()) continue;
			const Engine *e = v->GetEngine();
			CargoTypes cmask = e->info.refit_mask;
			VehicleCallbackMasks callback_mask = e->info.callback_mask;

			/* Skip this engine if it does not carry anything */
			if (!e->CanCarryCargo()) continue;
			/* Skip this engine if we build the list for auto-refitting and engine doesn't allow it. */
			if (this->auto_refit && !e->info.misc_flags.Test(EngineMiscFlag::AutoRefit)) continue;

			/* Loop through all cargoes in the refit mask */
			for (const auto &cs : _sorted_cargo_specs) {
				CargoType cargo_type = cs->Index();
				/* Skip cargo type if it's not listed */
				if (!HasBit(cmask, cargo_type)) continue;

				auto &list = this->refit_list[cargo_type];
				bool first_vehicle = list.empty();
				if (first_vehicle) {
					/* Keeping the current subtype is always an option. It also serves as the option in case of no subtypes */
					list.emplace_back(cargo_type, UINT8_MAX, STR_EMPTY);
				}

				/* Check the vehicle's callback mask for cargo suffixes.
				 * This is not supported for ordered refits, since subtypes only have a meaning
				 * for a specific vehicle at a specific point in time, which conflicts with shared orders,
				 * autoreplace, autorenew, clone, order restoration, ... */
				if (this->order == INVALID_VEH_ORDER_ID && callback_mask.Test(VehicleCallbackMask::CargoSuffix)) {
					/* Make a note of the original cargo type. It has to be
					 * changed to test the cargo & subtype... */
					CargoType temp_cargo = v->cargo_type;
					uint8_t temp_subtype  = v->cargo_subtype;

					v->cargo_type = cargo_type;

					for (uint refit_cyc = 0; refit_cyc < MAX_REFIT_CYCLE; refit_cyc++) {
						v->cargo_subtype = refit_cyc;

						/* Make sure we don't pick up anything cached. */
						v->First()->InvalidateNewGRFCache();
						v->InvalidateNewGRFCache();

						StringID subtype = GetCargoSubtypeText(v);

						if (first_vehicle) {
							/* Append new subtype (don't add duplicates though) */
							if (subtype == STR_EMPTY) break;

							RefitOption option{cargo_type, static_cast<uint8_t>(refit_cyc), subtype};
							include(list, option);
						} else {
							/* Intersect the subtypes of earlier vehicles with the subtypes of this vehicle */
							if (subtype == STR_EMPTY) {
								/* No more subtypes for this vehicle, delete all subtypes >= refit_cyc */
								/* UINT8_MAX item is in front, other subtypes are sorted. So just truncate the list in the right spot */
								for (uint i = 1; i < list.size(); i++) {
									if (list[i].subtype >= refit_cyc) {
										list.erase(list.begin() + i, list.end());
										break;
									}
								}
								break;
							} else {
								/* Check whether the subtype matches with the subtype of earlier vehicles. */
								uint pos = 1;
								while (pos < list.size() && list[pos].subtype != refit_cyc) pos++;
								if (pos < list.size() && list[pos].string != subtype) {
									/* String mismatch, remove item keeping the order */
									list.erase(list.begin() + pos);
								}
							}
						}
					}

					/* Reset the vehicle's cargo type */
					v->cargo_type    = temp_cargo;
					v->cargo_subtype = temp_subtype;

					/* And make sure we haven't tainted the cache */
					v->First()->InvalidateNewGRFCache();
					v->InvalidateNewGRFCache();
				}
			}
		} while (v->IsGroundVehicle() && (v = v->Next()) != nullptr);

		/* Restore the previously selected RefitOption. */
		if (current_refit_option.has_value()) {
			for (const auto &pair : this->refit_list) {
				for (const auto &refit : pair.second) {
					if (refit.cargo == current_refit_option->cargo && refit.subtype == current_refit_option->subtype) {
						this->selected_refit = &refit;
						break;
					}
				}
				if (this->selected_refit != nullptr) break;
			}
		}

		this->SetWidgetDisabledState(WID_VR_REFIT, this->selected_refit == nullptr);
	}

	/**
	 * Refresh scrollbar after selection changed
	 */
	void RefreshScrollbar()
	{
		size_t scroll_row = 0;
		size_t rows = 0;
		CargoType cargo = this->selected_refit == nullptr ? INVALID_CARGO : this->selected_refit->cargo;

		for (const auto &pair : this->refit_list) {
			if (pair.first == cargo) {
				/* selected_refit points to an element in the vector so no need to search for it. */
				scroll_row = rows + (this->selected_refit - pair.second.data());
				rows += pair.second.size();
			} else {
				rows++; /* Unselected cargo type is collapsed into one row. */
			}
		}

		this->vscroll->SetCount(rows);
		this->vscroll->ScrollTowards(static_cast<int>(scroll_row));
	}

	/**
	 * Select a row.
	 * @param click_row Clicked row
	 */
	void SetSelection(uint click_row)
	{
		uint row = 0;

		for (const auto &pair : refit_list) {
			for (const RefitOption &refit : pair.second) {
				if (row == click_row) {
					this->selected_refit = &refit;
					return;
				}
				row++;
				/* If this cargo type is not already selected then its subtypes are not visible, so skip the rest. */
				if (this->selected_refit == nullptr || this->selected_refit->cargo != refit.cargo) break;
			}
		}

		/* No selection made */
		this->selected_refit = nullptr;
	}

	RefitWindow(WindowDesc &desc, const Vehicle *v, VehicleOrderID order, bool auto_refit) : Window(desc)
	{
		this->auto_refit = auto_refit;
		this->order = order;
		this->CreateNestedTree();

		this->vscroll = this->GetScrollbar(WID_VR_SCROLLBAR);
		this->hscroll = (v->IsGroundVehicle() ? this->GetScrollbar(WID_VR_HSCROLLBAR) : nullptr);
		this->GetWidget<NWidgetCore>(WID_VR_SELECT_HEADER)->SetToolTip(STR_REFIT_TRAIN_LIST_TOOLTIP + v->type);
		this->GetWidget<NWidgetCore>(WID_VR_MATRIX)->SetToolTip(STR_REFIT_TRAIN_LIST_TOOLTIP + v->type);
		NWidgetCore *nwi = this->GetWidget<NWidgetCore>(WID_VR_REFIT);
		nwi->SetStringTip(STR_REFIT_TRAIN_REFIT_BUTTON + v->type, STR_REFIT_TRAIN_REFIT_TOOLTIP + v->type);
		this->GetWidget<NWidgetStacked>(WID_VR_SHOW_HSCROLLBAR)->SetDisplayedPlane(v->IsGroundVehicle() ? 0 : SZSP_HORIZONTAL);
		this->GetWidget<NWidgetCore>(WID_VR_VEHICLE_PANEL_DISPLAY)->SetToolTip((v->type == VEH_TRAIN) ? STR_REFIT_SELECT_VEHICLES_TOOLTIP : STR_NULL);

		this->FinishInitNested(v->index);
		this->owner = v->owner;

		this->SetWidgetDisabledState(WID_VR_REFIT, this->selected_refit == nullptr);
	}

	void OnInit() override
	{
		/* (Re)build the refit list */
		this->OnInvalidateData(VIWD_CONSIST_CHANGED);
	}

	void OnPaint() override
	{
		/* Determine amount of items for scroller. */
		if (this->hscroll != nullptr) this->hscroll->SetCount(this->vehicle_width);

		/* Calculate sprite position. */
		NWidgetCore *vehicle_panel_display = this->GetWidget<NWidgetCore>(WID_VR_VEHICLE_PANEL_DISPLAY);
		int sprite_width = std::max(0, ((int)vehicle_panel_display->current_x - this->vehicle_width) / 2);
		this->sprite_left = vehicle_panel_display->pos_x;
		this->sprite_right = vehicle_panel_display->pos_x + vehicle_panel_display->current_x - 1;
		if (_current_text_dir == TD_RTL) {
			this->sprite_right -= sprite_width;
			this->vehicle_margin = vehicle_panel_display->current_x - sprite_right;
		} else {
			this->sprite_left += sprite_width;
			this->vehicle_margin = sprite_left;
		}

		this->DrawWidgets();
	}

	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		switch (widget) {
			case WID_VR_MATRIX:
				fill.height = resize.height = GetCharacterHeight(FS_NORMAL) + padding.height;
				size.height = resize.height * 8;
				break;

			case WID_VR_VEHICLE_PANEL_DISPLAY:
				size.height = ScaleGUITrad(GetVehicleHeight(Vehicle::Get(this->window_number)->type));
				break;

			case WID_VR_INFO:
				size.width = this->information_width + padding.height;
				break;
		}
	}

	std::string GetWidgetString(WidgetID widget, StringID stringid) const override
	{
		if (widget == WID_VR_CAPTION) return GetString(STR_REFIT_CAPTION, Vehicle::Get(this->window_number)->index);

		return this->Window::GetWidgetString(widget, stringid);
	}

	/**
	 * Gets the #StringID to use for displaying capacity.
	 * @param option Cargo and cargo subtype to check for capacity.
	 * @return INVALID_STRING_ID if there is no capacity. StringID to use in any other case.
	 * @post String parameters have been set.
	 */
	std::string GetCapacityString(const RefitOption &option) const
	{
		assert(_current_company == _local_company);
		auto [cost, refit_capacity, mail_capacity, cargo_capacities] = Command<CMD_REFIT_VEHICLE>::Do(DoCommandFlag::QueryCost, this->selected_vehicle, option.cargo, option.subtype, this->auto_refit, false, this->num_vehicles);

		if (cost.Failed()) return {};

		Money money = cost.GetCost();
		if (mail_capacity > 0) {
			if (this->order != INVALID_VEH_ORDER_ID) {
				/* No predictable cost */
				return GetString(STR_PURCHASE_INFO_AIRCRAFT_CAPACITY, option.cargo, refit_capacity, GetCargoTypeByLabel(CT_MAIL), mail_capacity);
			}

			if (money <= 0) {
				return GetString(STR_REFIT_NEW_CAPACITY_INCOME_FROM_AIRCRAFT_REFIT, option.cargo, refit_capacity, GetCargoTypeByLabel(CT_MAIL), mail_capacity, -money);
			}

			return GetString(STR_REFIT_NEW_CAPACITY_COST_OF_AIRCRAFT_REFIT, option.cargo, refit_capacity, GetCargoTypeByLabel(CT_MAIL), mail_capacity, money);
		}

		if (this->order != INVALID_VEH_ORDER_ID) {
			/* No predictable cost */
			return GetString(STR_PURCHASE_INFO_CAPACITY, option.cargo, refit_capacity, STR_EMPTY);
		}

		if (money <= 0) {
			return GetString(STR_REFIT_NEW_CAPACITY_INCOME_FROM_REFIT, option.cargo, refit_capacity, -money);
		}

		return GetString(STR_REFIT_NEW_CAPACITY_COST_OF_REFIT, option.cargo, refit_capacity, money);
	}

	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		switch (widget) {
			case WID_VR_VEHICLE_PANEL_DISPLAY: {
				Vehicle *v = Vehicle::Get(this->window_number);
				DrawVehicleImage(v, {this->sprite_left, r.top, this->sprite_right, r.bottom},
					VehicleID::Invalid(), EIT_IN_DETAILS, this->hscroll != nullptr ? this->hscroll->GetPosition() : 0);

				/* Highlight selected vehicles. */
				if (this->order != INVALID_VEH_ORDER_ID) break;
				int x = 0;
				switch (v->type) {
					case VEH_TRAIN: {
						VehicleSet vehicles_to_refit;
						GetVehicleSet(vehicles_to_refit, Vehicle::Get(this->selected_vehicle), this->num_vehicles);

						int left = INT32_MIN;
						int width = 0;

						/* Determine top & bottom position of the highlight.*/
						const int height = ScaleSpriteTrad(12);
						const int highlight_top = CentreBounds(r.top, r.bottom, height);
						const int highlight_bottom = highlight_top + height - 1;

						for (Train *u = Train::From(v); u != nullptr; u = u->Next()) {
							/* Start checking. */
							const bool contained = std::ranges::find(vehicles_to_refit, u->index) != vehicles_to_refit.end();
							if (contained && left == INT32_MIN) {
								left = x - this->hscroll->GetPosition() + r.left + this->vehicle_margin;
								width = 0;
							}

							/* Draw a selection. */
							if ((!contained || u->Next() == nullptr) && left != INT32_MIN) {
								if (u->Next() == nullptr && contained) {
									int current_width = u->GetDisplayImageWidth();
									width += current_width;
									x += current_width;
								}

								int right = Clamp(left + width, 0, r.right);
								left = std::max(0, left);

								if (_current_text_dir == TD_RTL) {
									right = r.Width() - left;
									left = right - width;
								}

								if (left != right) {
									Rect hr = {left, highlight_top, right, highlight_bottom};
									DrawFrameRect(hr.Expand(WidgetDimensions::scaled.bevel), COLOUR_WHITE, FrameFlag::BorderOnly);
								}

								left = INT32_MIN;
							}

							int current_width = u->GetDisplayImageWidth();
							width += current_width;
							x += current_width;
						}
						break;
					}

					default: break;
				}
				break;
			}

			case WID_VR_MATRIX:
				DrawVehicleRefitWindow(this->refit_list, this->selected_refit, this->vscroll->GetPosition(), this->vscroll->GetCapacity(), this->resize.step_height, r);
				break;

			case WID_VR_INFO:
				if (this->selected_refit != nullptr) {
					std::string string = this->GetCapacityString(*this->selected_refit);
					if (!string.empty()) {
						DrawStringMultiLine(r.Shrink(WidgetDimensions::scaled.framerect), string);
					}
				}
				break;
		}
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	void OnInvalidateData([[maybe_unused]] int data = 0, [[maybe_unused]] bool gui_scope = true) override
	{
		switch (data) {
			case VIWD_AUTOREPLACE: // Autoreplace replaced the vehicle; selected_vehicle became invalid.
			case VIWD_CONSIST_CHANGED: { // The consist has changed; rebuild the entire list.
				/* Clear the selection. */
				Vehicle *v = Vehicle::Get(this->window_number);
				this->selected_vehicle = v->index;
				this->num_vehicles = UINT8_MAX;
				[[fallthrough]];
			}

			case 2: { // The vehicle selection has changed; rebuild the entire list.
				if (!gui_scope) break;
				this->BuildRefitList();

				/* The vehicle width has changed too. */
				this->vehicle_width = GetVehicleWidth(Vehicle::Get(this->window_number), EIT_IN_DETAILS);
				uint max_width = 0;

				/* Check the width of all cargo information strings. */
				for (const auto &list : this->refit_list) {
					for (const RefitOption &refit : list.second) {
						std::string string = this->GetCapacityString(refit);
						if (!string.empty()) {
							Dimension dim = GetStringBoundingBox(string);
							max_width = std::max(dim.width, max_width);
						}
					}
				}

				if (this->information_width < max_width) {
					this->information_width = max_width;
					this->ReInit();
				}
				[[fallthrough]];
			}

			case 1: // A new cargo has been selected.
				if (!gui_scope) break;
				this->RefreshScrollbar();
				break;
		}
	}

	int GetClickPosition(int click_x)
	{
		const NWidgetCore *matrix_widget = this->GetWidget<NWidgetCore>(WID_VR_VEHICLE_PANEL_DISPLAY);
		if (_current_text_dir == TD_RTL) click_x = matrix_widget->current_x - click_x;
		click_x -= this->vehicle_margin;
		if (this->hscroll != nullptr) click_x += this->hscroll->GetPosition();

		return click_x;
	}

	void SetSelectedVehicles(int drag_x)
	{
		drag_x = GetClickPosition(drag_x);

		int left_x  = std::min(this->click_x, drag_x);
		int right_x = std::max(this->click_x, drag_x);
		this->num_vehicles = 0;

		Vehicle *v = Vehicle::Get(this->window_number);
		/* Find the vehicle part that was clicked. */
		switch (v->type) {
			case VEH_TRAIN: {
				/* Don't select anything if we are not clicking in the vehicle. */
				if (left_x >= 0) {
					const Train *u = Train::From(v);
					bool start_counting = false;
					for (; u != nullptr; u = u->Next()) {
						int current_width = u->GetDisplayImageWidth();
						left_x  -= current_width;
						right_x -= current_width;

						if (left_x < 0 && !start_counting) {
							this->selected_vehicle = u->index;
							start_counting = true;

							/* Count the first vehicle, even if articulated part */
							this->num_vehicles++;
						} else if (start_counting && !u->IsArticulatedPart()) {
							/* Do not count articulated parts */
							this->num_vehicles++;
						}

						if (right_x < 0) break;
					}
				}

				/* If the selection is not correct, clear it. */
				if (this->num_vehicles != 0) {
					if (_ctrl_pressed) this->num_vehicles = UINT8_MAX;
					break;
				}
				[[fallthrough]];
			}

			default:
				/* Clear the selection. */
				this->selected_vehicle = v->index;
				this->num_vehicles = UINT8_MAX;
				break;
		}
	}

	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		switch (widget) {
			case WID_VR_VEHICLE_PANEL_DISPLAY: { // Vehicle image.
				if (this->order != INVALID_VEH_ORDER_ID) break;
				NWidgetBase *nwi = this->GetWidget<NWidgetBase>(WID_VR_VEHICLE_PANEL_DISPLAY);
				this->click_x = GetClickPosition(pt.x - nwi->pos_x);
				this->SetSelectedVehicles(pt.x - nwi->pos_x);
				this->SetWidgetDirty(WID_VR_VEHICLE_PANEL_DISPLAY);
				if (!_ctrl_pressed) {
					SetObjectToPlaceWnd(SPR_CURSOR_MOUSE, PAL_NONE, HT_DRAG, this);
				} else {
					/* The vehicle selection has changed. */
					this->InvalidateData(2);
				}
				break;
			}

			case WID_VR_MATRIX: { // listbox
				this->SetSelection(this->vscroll->GetScrolledRowFromWidget(pt.y, this, WID_VR_MATRIX));
				this->SetWidgetDisabledState(WID_VR_REFIT, this->selected_refit == nullptr);
				this->InvalidateData(1);

				if (click_count == 1) break;
				[[fallthrough]];
			}

			case WID_VR_REFIT: // refit button
				if (this->selected_refit != nullptr) {
					const Vehicle *v = Vehicle::Get(this->window_number);

					if (this->order == INVALID_VEH_ORDER_ID) {
						bool delete_window = this->selected_vehicle == v->index && this->num_vehicles == UINT8_MAX;
						if (Command<CMD_REFIT_VEHICLE>::Post(GetCmdRefitVehMsg(v), v->tile, this->selected_vehicle, this->selected_refit->cargo, this->selected_refit->subtype, false, false, this->num_vehicles) && delete_window) this->Close();
					} else {
						if (Command<CMD_ORDER_REFIT>::Post(v->tile, v->index, this->order, this->selected_refit->cargo)) this->Close();
					}
				}
				break;
		}
	}

	void OnMouseDrag(Point pt, WidgetID widget) override
	{
		switch (widget) {
			case WID_VR_VEHICLE_PANEL_DISPLAY: { // Vehicle image.
				if (this->order != INVALID_VEH_ORDER_ID) break;
				NWidgetBase *nwi = this->GetWidget<NWidgetBase>(WID_VR_VEHICLE_PANEL_DISPLAY);
				this->SetSelectedVehicles(pt.x - nwi->pos_x);
				this->SetWidgetDirty(WID_VR_VEHICLE_PANEL_DISPLAY);
				break;
			}
		}
	}

	void OnDragDrop(Point pt, WidgetID widget) override
	{
		switch (widget) {
			case WID_VR_VEHICLE_PANEL_DISPLAY: { // Vehicle image.
				if (this->order != INVALID_VEH_ORDER_ID) break;
				NWidgetBase *nwi = this->GetWidget<NWidgetBase>(WID_VR_VEHICLE_PANEL_DISPLAY);
				this->SetSelectedVehicles(pt.x - nwi->pos_x);
				this->InvalidateData(2);
				break;
			}
		}
	}

	void OnResize() override
	{
		this->vehicle_width = GetVehicleWidth(Vehicle::Get(this->window_number), EIT_IN_DETAILS);
		this->vscroll->SetCapacityFromWidget(this, WID_VR_MATRIX);
		if (this->hscroll != nullptr) this->hscroll->SetCapacityFromWidget(this, WID_VR_VEHICLE_PANEL_DISPLAY);
	}
};

static constexpr NWidgetPart _nested_vehicle_refit_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_VR_CAPTION),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
	EndContainer(),
	/* Vehicle display + scrollbar. */
	NWidget(NWID_VERTICAL),
		NWidget(WWT_PANEL, COLOUR_GREY, WID_VR_VEHICLE_PANEL_DISPLAY), SetMinimalSize(228, 14), SetResize(1, 0), SetScrollbar(WID_VR_HSCROLLBAR), EndContainer(),
		NWidget(NWID_SELECTION, INVALID_COLOUR, WID_VR_SHOW_HSCROLLBAR),
			NWidget(NWID_HSCROLLBAR, COLOUR_GREY, WID_VR_HSCROLLBAR),
		EndContainer(),
	EndContainer(),
	NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_VR_SELECT_HEADER), SetStringTip(STR_REFIT_TITLE), SetResize(1, 0),
	/* Matrix + scrollbar. */
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_MATRIX, COLOUR_GREY, WID_VR_MATRIX), SetMinimalSize(228, 112), SetResize(1, 14), SetFill(1, 1), SetMatrixDataTip(1, 0), SetScrollbar(WID_VR_SCROLLBAR),
		NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_VR_SCROLLBAR),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY, WID_VR_INFO), SetMinimalTextLines(2, WidgetDimensions::unscaled.framerect.Vertical()), SetResize(1, 0), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VR_REFIT), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_RESIZEBOX, COLOUR_GREY),
	EndContainer(),
};

static WindowDesc _vehicle_refit_desc(
	WDP_AUTO, "view_vehicle_refit", 240, 174,
	WC_VEHICLE_REFIT, WC_VEHICLE_VIEW,
	WindowDefaultFlag::Construction,
	_nested_vehicle_refit_widgets
);

/**
 * Show the refit window for a vehicle
 * @param *v The vehicle to show the refit window for
 * @param order of the vehicle to assign refit to, or INVALID_VEH_ORDER_ID to refit the vehicle now
 * @param parent the parent window of the refit window
 * @param auto_refit Choose cargo for auto-refitting
 */
void ShowVehicleRefitWindow(const Vehicle *v, VehicleOrderID order, Window *parent, bool auto_refit)
{
	CloseWindowById(WC_VEHICLE_REFIT, v->index);
	RefitWindow *w = new RefitWindow(_vehicle_refit_desc, v, order, auto_refit);
	w->parent = parent;
}

/** Display list of cargo types of the engine, for the purchase information window */
uint ShowRefitOptionsList(int left, int right, int y, EngineID engine)
{
	/* List of cargo types of this engine */
	CargoTypes cmask = GetUnionOfArticulatedRefitMasks(engine, false);
	/* List of cargo types available in this climate */
	CargoTypes lmask = _cargo_mask;

	/* Draw nothing if the engine is not refittable */
	if (HasAtMostOneBit(cmask)) return y;

	std::string str;
	if (cmask == lmask) {
		/* Engine can be refitted to all types in this climate */
		str = GetString(STR_PURCHASE_INFO_REFITTABLE_TO, STR_PURCHASE_INFO_ALL_TYPES, std::monostate{});
	} else {
		/* Check if we are able to refit to more cargo types and unable to. If
		 * so, invert the cargo types to list those that we can't refit to. */
		if (CountBits(cmask ^ lmask) < CountBits(cmask) && CountBits(cmask ^ lmask) <= 7) {
			cmask ^= lmask;
			str = GetString(STR_PURCHASE_INFO_REFITTABLE_TO, STR_PURCHASE_INFO_ALL_BUT, cmask);
		} else {
			str = GetString(STR_PURCHASE_INFO_REFITTABLE_TO, STR_JUST_CARGO_LIST, cmask);
		}
	}

	return DrawStringMultiLine(left, right, y, INT32_MAX, str);
}

/** Get the cargo subtype text from NewGRF for the vehicle details window. */
StringID GetCargoSubtypeText(const Vehicle *v)
{
	if (!EngInfo(v->engine_type)->callback_mask.Test(VehicleCallbackMask::CargoSuffix)) return STR_EMPTY;
	std::array<int32_t, 1> regs100;
	uint16_t cb = GetVehicleCallback(CBID_VEHICLE_CARGO_SUFFIX, 0, 0, v->engine_type, v, regs100);
	if (v->GetGRF()->grf_version < 8 && cb == 0xFF) return STR_EMPTY;
	if (cb == CALLBACK_FAILED || cb == 0x400) return STR_EMPTY;
	if (cb == 0x40F) {
		return GetGRFStringID(v->GetGRFID(), static_cast<GRFStringID>(regs100[0]));
	}
	if (cb > 0x400) {
		ErrorUnknownCallbackResult(v->GetGRFID(), CBID_VEHICLE_CARGO_SUFFIX, cb);
		return STR_EMPTY;
	}
	return GetGRFStringID(v->GetGRFID(), GRFSTR_MISC_GRF_TEXT + cb);
}

/** Sort vehicle groups by the number of vehicles in the group */
static bool VehicleGroupLengthSorter(const GUIVehicleGroup &a, const GUIVehicleGroup &b)
{
	return a.NumVehicles() < b.NumVehicles();
}

/** Sort vehicle groups by the total profit this year */
static bool VehicleGroupTotalProfitThisYearSorter(const GUIVehicleGroup &a, const GUIVehicleGroup &b)
{
	return a.GetDisplayProfitThisYear() < b.GetDisplayProfitThisYear();
}

/** Sort vehicle groups by the total profit last year */
static bool VehicleGroupTotalProfitLastYearSorter(const GUIVehicleGroup &a, const GUIVehicleGroup &b)
{
	return a.GetDisplayProfitLastYear() < b.GetDisplayProfitLastYear();
}

/** Sort vehicle groups by the average profit this year */
static bool VehicleGroupAverageProfitThisYearSorter(const GUIVehicleGroup &a, const GUIVehicleGroup &b)
{
	return a.GetDisplayProfitThisYear() * static_cast<uint>(b.NumVehicles()) < b.GetDisplayProfitThisYear() * static_cast<uint>(a.NumVehicles());
}

/** Sort vehicle groups by the average profit last year */
static bool VehicleGroupAverageProfitLastYearSorter(const GUIVehicleGroup &a, const GUIVehicleGroup &b)
{
	return a.GetDisplayProfitLastYear() * static_cast<uint>(b.NumVehicles()) < b.GetDisplayProfitLastYear() * static_cast<uint>(a.NumVehicles());
}

/** Sort vehicles by their number */
static bool VehicleNumberSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	return a->unitnumber < b->unitnumber;
}

/** Sort vehicles by their name */
static bool VehicleNameSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	static std::string last_name[2] = { {}, {} };

	if (a != _last_vehicle[0]) {
		_last_vehicle[0] = a;
		last_name[0] = GetString(STR_VEHICLE_NAME, a->index);
	}

	if (b != _last_vehicle[1]) {
		_last_vehicle[1] = b;
		last_name[1] = GetString(STR_VEHICLE_NAME, b->index);
	}

	int r = StrNaturalCompare(last_name[0], last_name[1]); // Sort by name (natural sorting).
	return (r != 0) ? r < 0: VehicleNumberSorter(a, b);
}

/** Sort vehicles by their age */
static bool VehicleAgeSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	auto r = a->age - b->age;
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by this year profit */
static bool VehicleProfitThisYearSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	int r = ClampTo<int32_t>(a->GetDisplayProfitThisYear() - b->GetDisplayProfitThisYear());
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by last year profit */
static bool VehicleProfitLastYearSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	int r = ClampTo<int32_t>(a->GetDisplayProfitLastYear() - b->GetDisplayProfitLastYear());
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by their cargo */
static bool VehicleCargoSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	const Vehicle *v;
	CargoArray diff{};

	/* Append the cargo of the connected waggons */
	for (v = a; v != nullptr; v = v->Next()) diff[v->cargo_type] += v->cargo_cap;
	for (v = b; v != nullptr; v = v->Next()) diff[v->cargo_type] -= v->cargo_cap;

	int r = 0;
	for (uint d : diff) {
		r = d;
		if (r != 0) break;
	}

	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by their reliability */
static bool VehicleReliabilitySorter(const Vehicle * const &a, const Vehicle * const &b)
{
	int r = a->reliability - b->reliability;
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by their max speed */
static bool VehicleMaxSpeedSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	int r = a->vcache.cached_max_speed - b->vcache.cached_max_speed;
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by model */
static bool VehicleModelSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	int r = a->engine_type.base() - b->engine_type.base();
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by their value */
static bool VehicleValueSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	const Vehicle *u;
	Money diff = 0;

	for (u = a; u != nullptr; u = u->Next()) diff += u->value;
	for (u = b; u != nullptr; u = u->Next()) diff -= u->value;

	int r = ClampTo<int32_t>(diff);
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by their length */
static bool VehicleLengthSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	int r = a->GetGroundVehicleCache()->cached_total_length - b->GetGroundVehicleCache()->cached_total_length;
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by the time they can still live */
static bool VehicleTimeToLiveSorter(const Vehicle * const &a, const Vehicle * const &b)
{
	int r = ClampTo<int32_t>((a->max_age - a->age) - (b->max_age - b->age));
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

/** Sort vehicles by the timetable delay */
static bool VehicleTimetableDelaySorter(const Vehicle * const &a, const Vehicle * const &b)
{
	int r = a->lateness_counter - b->lateness_counter;
	return (r != 0) ? r < 0 : VehicleNumberSorter(a, b);
}

void InitializeGUI()
{
	_grouping = {};
	_sorting = {};
}

/**
 * Assign a vehicle window a new vehicle
 * @param window_class WindowClass to search for
 * @param from_index the old vehicle ID
 * @param to_index the new vehicle ID
 */
static inline void ChangeVehicleWindow(WindowClass window_class, VehicleID from_index, VehicleID to_index)
{
	Window *w = FindWindowById(window_class, from_index);
	if (w != nullptr) {
		/* Update window_number */
		w->window_number = to_index;
		if (w->viewport != nullptr) w->viewport->follow_vehicle = to_index;

		/* Update vehicle drag data */
		if (_thd.window_class == window_class && _thd.window_number == from_index) {
			_thd.window_number = to_index;
		}

		/* Notify the window. */
		w->InvalidateData(VIWD_AUTOREPLACE, false);
	}
}

/**
 * Report a change in vehicle IDs (due to autoreplace) to affected vehicle windows.
 * @param from_index the old vehicle ID
 * @param to_index the new vehicle ID
 */
void ChangeVehicleViewWindow(VehicleID from_index, VehicleID to_index)
{
	ChangeVehicleWindow(WC_VEHICLE_VIEW,      from_index, to_index);
	ChangeVehicleWindow(WC_VEHICLE_ORDERS,    from_index, to_index);
	ChangeVehicleWindow(WC_VEHICLE_REFIT,     from_index, to_index);
	ChangeVehicleWindow(WC_VEHICLE_DETAILS,   from_index, to_index);
	ChangeVehicleWindow(WC_VEHICLE_TIMETABLE, from_index, to_index);
}

static constexpr NWidgetPart _nested_vehicle_list[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(NWID_SELECTION, INVALID_COLOUR, WID_VL_CAPTION_SELECTION),
			NWidget(WWT_CAPTION, COLOUR_GREY, WID_VL_CAPTION),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_CAPTION, COLOUR_GREY, WID_VL_CAPTION_SHARED_ORDERS),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VL_ORDER_VIEW), SetMinimalSize(61, 14), SetStringTip(STR_GOTO_ORDER_VIEW, STR_GOTO_ORDER_VIEW_TOOLTIP),
			EndContainer(),
		EndContainer(),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),

	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL, NWidContainerFlag::EqualSize),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_VL_GROUP_ORDER), SetMinimalSize(0, 12), SetFill(1, 1), SetStringTip(STR_STATION_VIEW_GROUP, STR_TOOLTIP_GROUP_ORDER),
			NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VL_SORT_ORDER), SetMinimalSize(0, 12), SetFill(1, 1), SetStringTip(STR_BUTTON_SORT_BY, STR_TOOLTIP_SORT_ORDER),
		EndContainer(),
		NWidget(NWID_VERTICAL, NWidContainerFlag::EqualSize),
			NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_VL_GROUP_BY_PULLDOWN), SetMinimalSize(0, 12), SetFill(1, 1), SetToolTip(STR_TOOLTIP_GROUP_ORDER),
			NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_VL_SORT_BY_PULLDOWN), SetMinimalSize(0, 12), SetFill(1, 1), SetToolTip(STR_TOOLTIP_SORT_CRITERIA),
		EndContainer(),
		NWidget(NWID_VERTICAL, NWidContainerFlag::EqualSize),
			NWidget(WWT_PANEL, COLOUR_GREY), SetMinimalSize(0, 12), SetFill(1, 1), SetResize(1, 0), EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(NWID_SELECTION, INVALID_COLOUR, WID_VL_FILTER_BY_CARGO_SEL),
					NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_VL_FILTER_BY_CARGO), SetMinimalSize(0, 12), SetFill(0, 1), SetToolTip(STR_TOOLTIP_FILTER_CRITERIA),
				EndContainer(),
				NWidget(WWT_PANEL, COLOUR_GREY), SetMinimalSize(0, 12), SetFill(1, 1), SetResize(1, 0), EndContainer(),
			EndContainer(),
		EndContainer(),
	EndContainer(),

	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_MATRIX, COLOUR_GREY, WID_VL_LIST), SetMinimalSize(248, 0), SetFill(1, 0), SetResize(1, 1), SetMatrixDataTip(1, 0), SetScrollbar(WID_VL_SCROLLBAR),
		NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_VL_SCROLLBAR),
	EndContainer(),

	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_SELECTION, INVALID_COLOUR, WID_VL_HIDE_BUTTONS),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VL_AVAILABLE_VEHICLES), SetMinimalSize(106, 12), SetFill(0, 1),
								SetToolTip(STR_VEHICLE_LIST_AVAILABLE_ENGINES_TOOLTIP),
				NWidget(WWT_PANEL, COLOUR_GREY), SetMinimalSize(0, 12), SetResize(1, 0), SetFill(1, 1), EndContainer(),
				NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_VL_MANAGE_VEHICLES_DROPDOWN), SetMinimalSize(118, 12), SetFill(0, 1),
								SetStringTip(STR_VEHICLE_LIST_MANAGE_LIST, STR_VEHICLE_LIST_MANAGE_LIST_TOOLTIP),
				NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VL_STOP_ALL), SetAspect(WidgetDimensions::ASPECT_VEHICLE_FLAG), SetFill(0, 1),
								SetSpriteTip(SPR_FLAG_VEH_STOPPED, STR_VEHICLE_LIST_MASS_STOP_LIST_TOOLTIP),
				NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VL_START_ALL), SetAspect(WidgetDimensions::ASPECT_VEHICLE_FLAG), SetFill(0, 1),
								SetSpriteTip(SPR_FLAG_VEH_RUNNING, STR_VEHICLE_LIST_MASS_START_LIST_TOOLTIP),
			EndContainer(),
			/* Widget to be shown for other companies hiding the previous 5 widgets. */
			NWidget(WWT_PANEL, COLOUR_GREY), SetFill(1, 1), SetResize(1, 0), EndContainer(),
		EndContainer(),
		NWidget(WWT_RESIZEBOX, COLOUR_GREY),
	EndContainer(),
};

static void DrawSmallOrderList(const Vehicle *v, int left, int right, int y, uint order_arrow_width, VehicleOrderID start)
{
	auto orders = v->Orders();
	if (orders.empty()) return;

	bool rtl = _current_text_dir == TD_RTL;
	int l_offset = rtl ? 0 : order_arrow_width;
	int r_offset = rtl ? order_arrow_width : 0;
	int i = 0;
	VehicleOrderID oid = start;

	do {
		if (oid == v->cur_real_order_index) DrawString(left, right, y, rtl ? STR_JUST_LEFT_ARROW : STR_JUST_RIGHT_ARROW, TC_BLACK, SA_LEFT, false, FS_SMALL);

		if (orders[oid].IsType(OT_GOTO_STATION)) {
			DrawString(left + l_offset, right - r_offset, y, GetString(STR_STATION_NAME, orders[oid].GetDestination()), TC_BLACK, SA_LEFT, false, FS_SMALL);

			y += GetCharacterHeight(FS_SMALL);
			if (++i == 4) break;
		}

		oid = v->orders->GetNext(oid);
	} while (oid != start);
}

/** Draw small order list in the vehicle GUI, but without the little black arrow.  This is used for shared order groups. */
static void DrawSmallOrderList(const OrderList &orderlist, int left, int right, int y, uint order_arrow_width)
{
	bool rtl = _current_text_dir == TD_RTL;
	int l_offset = rtl ? 0 : order_arrow_width;
	int r_offset = rtl ? order_arrow_width : 0;
	int i = 0;

	for (const Order &order : orderlist.GetOrders()) {
		if (order.IsType(OT_GOTO_STATION)) {
			DrawString(left + l_offset, right - r_offset, y, GetString(STR_STATION_NAME, order.GetDestination()), TC_BLACK, SA_LEFT, false, FS_SMALL);

			y += GetCharacterHeight(FS_SMALL);
			if (++i == 4) break;
		}
	}
}

/**
 * Draws an image of a vehicle chain
 * @param v         Front vehicle
 * @param r         Rect to draw at
 * @param selection Selected vehicle to draw a frame around
 * @param skip      Number of pixels to skip at the front (for scrolling)
 */
void DrawVehicleImage(const Vehicle *v, const Rect &r, VehicleID selection, EngineImageType image_type, int skip)
{
	switch (v->type) {
		case VEH_TRAIN:    DrawTrainImage(Train::From(v), r, selection, image_type, skip); break;
		case VEH_ROAD:     DrawRoadVehImage(v, r, selection, image_type, skip);  break;
		case VEH_SHIP:     DrawShipImage(v, r, selection, image_type);     break;
		case VEH_AIRCRAFT: DrawAircraftImage(v, r, selection, image_type); break;
		default: NOT_REACHED();
	}
}

/**
 * Get the height of a vehicle in the vehicle list GUIs.
 * @param type    the vehicle type to look at
 * @param divisor the resulting height must be dividable by this
 * @return the height
 */
uint GetVehicleListHeight(VehicleType type, uint divisor)
{
	/* Name + vehicle + profit */
	uint base = ScaleGUITrad(GetVehicleHeight(type)) + 2 * GetCharacterHeight(FS_SMALL) + WidgetDimensions::scaled.matrix.Vertical();
	/* Drawing of the 4 small orders + profit*/
	if (type >= VEH_SHIP) base = std::max(base, 6U * GetCharacterHeight(FS_SMALL) + WidgetDimensions::scaled.matrix.Vertical());

	if (divisor == 1) return base;

	/* Make sure the height is dividable by divisor */
	uint rem = base % divisor;
	return base + (rem == 0 ? 0 : divisor - rem);
}

/**
 * Get width required for the formatted unit number display.
 * @param digits Number of digits required for unit number.
 * @return Required width in pixels.
 */
static int GetUnitNumberWidth(int digits)
{
	return GetStringBoundingBox(GetString(STR_JUST_COMMA, GetParamMaxDigits(digits))).width;
}

/**
 * Draw all the vehicle list items.
 * @param selected_vehicle The vehicle that is to be highlighted.
 * @param line_height      Height of a single item line.
 * @param r                Rectangle with edge positions of the matrix widget.
 */
void BaseVehicleListWindow::DrawVehicleListItems(VehicleID selected_vehicle, int line_height, const Rect &r) const
{
	Rect ir = r.WithHeight(line_height).Shrink(WidgetDimensions::scaled.matrix, RectPadding::zero);
	bool rtl = _current_text_dir == TD_RTL;

	Dimension profit = GetSpriteSize(SPR_PROFIT_LOT);
	int text_offset = std::max<int>(profit.width, GetUnitNumberWidth(this->unitnumber_digits)) + WidgetDimensions::scaled.hsep_normal;
	Rect tr = ir.Indent(text_offset, rtl);

	bool show_orderlist = this->vli.vtype >= VEH_SHIP;
	Rect olr = ir.Indent(std::max(ScaleGUITrad(100) + text_offset, ir.Width() / 2), rtl);

	int image_left  = (rtl && show_orderlist) ? olr.right : tr.left;
	int image_right = (!rtl && show_orderlist) ? olr.left : tr.right;

	int vehicle_button_x = rtl ? ir.right - profit.width : ir.left;

	auto [first, last] = this->vscroll->GetVisibleRangeIterators(this->vehgroups);
	for (auto it = first; it != last; ++it) {
		const GUIVehicleGroup &vehgroup = *it;

		DrawString(tr.left, tr.right, ir.bottom - GetCharacterHeight(FS_SMALL) - WidgetDimensions::scaled.framerect.bottom,
				GetString(TimerGameEconomy::UsingWallclockUnits() ? STR_VEHICLE_LIST_PROFIT_THIS_PERIOD_LAST_PERIOD : STR_VEHICLE_LIST_PROFIT_THIS_YEAR_LAST_YEAR,
						vehgroup.GetDisplayProfitThisYear(),
						vehgroup.GetDisplayProfitLastYear()));

		DrawVehicleProfitButton(vehgroup.GetOldestVehicleAge(), vehgroup.GetDisplayProfitLastYear(), vehgroup.NumVehicles(), vehicle_button_x, ir.top + GetCharacterHeight(FS_NORMAL) + WidgetDimensions::scaled.vsep_normal);

		switch (this->grouping) {
			case GB_NONE: {
				const Vehicle *v = vehgroup.GetSingleVehicle();

				if (v->vehicle_flags.Test(VehicleFlag::PathfinderLost)) {
					DrawSprite(SPR_WARNING_SIGN, PAL_NONE, vehicle_button_x, ir.top + GetCharacterHeight(FS_NORMAL) + WidgetDimensions::scaled.vsep_normal + profit.height);
				}

				DrawVehicleImage(v, {image_left, ir.top, image_right, ir.bottom}, selected_vehicle, EIT_IN_LIST, 0);

				if (_settings_client.gui.show_cargo_in_vehicle_lists) {
					/* Get the cargoes the vehicle can carry */
					CargoTypes vehicle_cargoes = 0;

					for (auto u = v; u != nullptr; u = u->Next()) {
						if (u->cargo_cap == 0) continue;

						SetBit(vehicle_cargoes, u->cargo_type);
					}

					if (!v->name.empty()) {
						/* The vehicle got a name so we will print it and the cargoes */
						DrawString(tr.left, tr.right, ir.top,
								GetString(STR_VEHICLE_LIST_NAME_AND_CARGO, STR_VEHICLE_NAME, v->index, STR_VEHICLE_LIST_CARGO, vehicle_cargoes),
								TC_BLACK, SA_LEFT, false, FS_SMALL);
					} else if (v->group_id != DEFAULT_GROUP) {
						/* The vehicle has no name, but is member of a group, so print group name and the cargoes */
						DrawString(tr.left, tr.right, ir.top,
								GetString(STR_VEHICLE_LIST_NAME_AND_CARGO, STR_GROUP_NAME, v->group_id, STR_VEHICLE_LIST_CARGO, vehicle_cargoes),
								TC_BLACK, SA_LEFT, false, FS_SMALL);
					} else {
						/* The vehicle has no name, and is not a member of a group, so just print the cargoes */
						DrawString(tr.left, tr.right, ir.top, GetString(STR_VEHICLE_LIST_CARGO, vehicle_cargoes), TC_BLACK, SA_LEFT, false, FS_SMALL);
					}
				} else if (!v->name.empty()) {
					/* The vehicle got a name so we will print it */
					DrawString(tr.left, tr.right, ir.top, GetString(STR_VEHICLE_NAME, v->index), TC_BLACK, SA_LEFT, false, FS_SMALL);
				} else if (v->group_id != DEFAULT_GROUP) {
					/* The vehicle has no name, but is member of a group, so print group name */
					DrawString(tr.left, tr.right, ir.top, GetString(STR_GROUP_NAME, v->group_id), TC_BLACK, SA_LEFT, false, FS_SMALL);
				}

				if (show_orderlist) DrawSmallOrderList(v, olr.left, olr.right, ir.top + GetCharacterHeight(FS_SMALL), this->order_arrow_width, v->cur_real_order_index);

				TextColour tc;
				if (v->IsChainInDepot()) {
					tc = TC_BLUE;
				} else {
					tc = (v->age > v->max_age - CalendarTime::DAYS_IN_LEAP_YEAR) ? TC_RED : TC_BLACK;
				}

				DrawString(ir.left, ir.right, ir.top + WidgetDimensions::scaled.framerect.top, GetString(STR_JUST_COMMA, v->unitnumber), tc);
				break;
			}

			case GB_SHARED_ORDERS:
				assert(vehgroup.NumVehicles() > 0);

				for (int i = 0; i < static_cast<int>(vehgroup.NumVehicles()); ++i) {
					if (image_left + WidgetDimensions::scaled.hsep_wide * i >= image_right) break; // Break if there is no more space to draw any more vehicles anyway.
					DrawVehicleImage(vehgroup.vehicles_begin[i], {image_left + WidgetDimensions::scaled.hsep_wide * i, ir.top, image_right, ir.bottom}, selected_vehicle, EIT_IN_LIST, 0);
				}

				if (show_orderlist) DrawSmallOrderList(*(vehgroup.vehicles_begin[0])->orders, olr.left, olr.right, ir.top + GetCharacterHeight(FS_SMALL), this->order_arrow_width);

				DrawString(ir.left, ir.right, ir.top + WidgetDimensions::scaled.framerect.top, GetString(STR_JUST_COMMA, vehgroup.NumVehicles()), TC_BLACK);
				break;

			default:
				NOT_REACHED();
		}

		ir = ir.Translate(0, line_height);
	}
}

void BaseVehicleListWindow::UpdateSortingFromGrouping()
{
	/* Set up sorting. Make the window-specific _sorting variable
	 * point to the correct global _sorting struct so we are freed
	 * from having conditionals during window operation */
	switch (this->vli.vtype) {
		case VEH_TRAIN:    this->sorting = &_sorting[this->grouping].train; break;
		case VEH_ROAD:     this->sorting = &_sorting[this->grouping].roadveh; break;
		case VEH_SHIP:     this->sorting = &_sorting[this->grouping].ship; break;
		case VEH_AIRCRAFT: this->sorting = &_sorting[this->grouping].aircraft; break;
		default: NOT_REACHED();
	}
	this->vehgroups.SetSortFuncs(this->GetVehicleSorterFuncs());
	this->vehgroups.SetListing(*this->sorting);
	this->vehgroups.ForceRebuild();
	this->vehgroups.NeedResort();
}

void BaseVehicleListWindow::UpdateVehicleGroupBy(GroupBy group_by)
{
	if (this->grouping != group_by) {
		/* Save the old sorting option, so that if we change the grouping option back later on,
		 * UpdateSortingFromGrouping() will automatically restore the saved sorting option. */
		*this->sorting = this->vehgroups.GetListing();

		this->grouping = group_by;
		_grouping[this->vli.type][this->vli.vtype] = group_by;
		this->UpdateSortingFromGrouping();
	}
}

/**
 * Window for the (old) vehicle listing.
 * See #VehicleListIdentifier::Pack for the contents of the window number.
 */
struct VehicleListWindow : public BaseVehicleListWindow {
private:
	/** Enumeration of planes of the button row at the bottom. */
	enum ButtonPlanes : uint8_t {
		BP_SHOW_BUTTONS, ///< Show the buttons.
		BP_HIDE_BUTTONS, ///< Show the empty panel.
	};

	/** Enumeration of planes of the title row at the top. */
	enum CaptionPlanes : uint8_t {
		BP_NORMAL,        ///< Show shared orders caption and buttons.
		BP_SHARED_ORDERS, ///< Show the normal caption.
	};

public:
	VehicleListWindow(WindowDesc &desc, WindowNumber window_number, const VehicleListIdentifier &vli) : BaseVehicleListWindow(desc, vli)
	{
		this->CreateNestedTree();

		this->GetWidget<NWidgetStacked>(WID_VL_FILTER_BY_CARGO_SEL)->SetDisplayedPlane((this->vli.type == VL_SHARED_ORDERS) ? SZSP_NONE : 0);

		this->vscroll = this->GetScrollbar(WID_VL_SCROLLBAR);

		/* Set up the window widgets */
		this->GetWidget<NWidgetCore>(WID_VL_LIST)->SetToolTip(STR_VEHICLE_LIST_TRAIN_LIST_TOOLTIP + this->vli.vtype);

		NWidgetStacked *nwi = this->GetWidget<NWidgetStacked>(WID_VL_CAPTION_SELECTION);
		if (this->vli.type == VL_SHARED_ORDERS) {
			this->GetWidget<NWidgetCore>(WID_VL_CAPTION_SHARED_ORDERS)->SetString(STR_VEHICLE_LIST_SHARED_ORDERS_LIST_CAPTION);
			/* If we are in the shared orders window, then disable the group-by dropdown menu.
			 * Remove this when the group-by dropdown menu has another option apart from grouping by shared orders. */
			this->SetWidgetDisabledState(WID_VL_GROUP_ORDER, true);
			this->SetWidgetDisabledState(WID_VL_GROUP_BY_PULLDOWN, true);
			nwi->SetDisplayedPlane(BP_SHARED_ORDERS);
		} else {
			this->GetWidget<NWidgetCore>(WID_VL_CAPTION)->SetString(STR_VEHICLE_LIST_TRAIN_CAPTION + this->vli.vtype);
			nwi->SetDisplayedPlane(BP_NORMAL);
		}

		this->FinishInitNested(window_number);
		if (this->vli.company != OWNER_NONE) this->owner = this->vli.company;

		this->BuildVehicleList();
		this->SortVehicleList();
	}

	~VehicleListWindow()
	{
		*this->sorting = this->vehgroups.GetListing();
	}

	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		switch (widget) {
			case WID_VL_LIST:
				fill.height = resize.height = GetVehicleListHeight(this->vli.vtype, 1);

				switch (this->vli.vtype) {
					case VEH_TRAIN:
					case VEH_ROAD:
						size.height = 6 * resize.height;
						break;
					case VEH_SHIP:
					case VEH_AIRCRAFT:
						size.height = 4 * resize.height;
						break;
					default: NOT_REACHED();
				}
				break;

			case WID_VL_SORT_ORDER: {
				Dimension d = GetStringBoundingBox(this->GetWidget<NWidgetCore>(widget)->GetString());
				d.width += padding.width + Window::SortButtonWidth() * 2; // Doubled since the string is centred and it also looks better.
				d.height += padding.height;
				size = maxdim(size, d);
				break;
			}

			case WID_VL_GROUP_BY_PULLDOWN:
				size.width = GetStringListWidth(this->vehicle_group_by_names) + padding.width;
				break;

			case WID_VL_SORT_BY_PULLDOWN:
				size.width = GetStringListWidth(this->vehicle_group_none_sorter_names_calendar);
				size.width = std::max(size.width, GetStringListWidth(this->vehicle_group_none_sorter_names_wallclock));
				size.width = std::max(size.width, GetStringListWidth(this->vehicle_group_shared_orders_sorter_names_calendar));
				size.width = std::max(size.width, GetStringListWidth(this->vehicle_group_shared_orders_sorter_names_wallclock));
				size.width += padding.width;
				break;

			case WID_VL_FILTER_BY_CARGO:
				size.width = std::max(size.width, GetDropDownListDimension(this->BuildCargoDropDownList(true)).width + padding.width);
				break;

			case WID_VL_MANAGE_VEHICLES_DROPDOWN: {
				Dimension d = this->GetActionDropdownSize(this->vli.type == VL_STANDARD, false, true);
				d.height += padding.height;
				d.width  += padding.width;
				size = maxdim(size, d);
				break;
			}
		}
	}

	std::string GetWidgetString(WidgetID widget, StringID stringid) const override
	{
		switch (widget) {
			case WID_VL_AVAILABLE_VEHICLES:
				return GetString(STR_VEHICLE_LIST_AVAILABLE_TRAINS + this->vli.vtype);

			case WID_VL_GROUP_BY_PULLDOWN:
				return GetString(std::data(this->vehicle_group_by_names)[this->grouping]);

			case WID_VL_SORT_BY_PULLDOWN:
				return GetString(this->GetVehicleSorterNames()[this->vehgroups.SortType()]);

			case WID_VL_FILTER_BY_CARGO:
				return GetString(this->GetCargoFilterLabel(this->cargo_filter_criteria));

			case WID_VL_CAPTION:
			case WID_VL_CAPTION_SHARED_ORDERS: {
				switch (this->vli.type) {
					case VL_SHARED_ORDERS: // Shared Orders
						return GetString(stringid, this->vehicles.size());

					case VL_STANDARD: // Company Name
						return GetString(stringid, STR_COMPANY_NAME, this->vli.ToCompanyID(), std::monostate{}, this->vehicles.size());

					case VL_STATION_LIST: // Station/Waypoint Name
						return GetString(stringid, Station::IsExpected(BaseStation::Get(this->vli.ToStationID())) ? STR_STATION_NAME : STR_WAYPOINT_NAME, this->vli.ToStationID(), std::monostate{}, this->vehicles.size());

					case VL_DEPOT_LIST:
						return GetString(stringid, STR_DEPOT_CAPTION, this->vli.vtype, this->vli.ToDestinationID(), this->vehicles.size());

					default: NOT_REACHED();
				}
			}

			default:
				return this->Window::GetWidgetString(widget, stringid);
		}
	}

	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		switch (widget) {
			case WID_VL_SORT_ORDER:
				/* draw arrow pointing up/down for ascending/descending sorting */
				this->DrawSortButtonState(widget, this->vehgroups.IsDescSortOrder() ? SBS_DOWN : SBS_UP);
				break;

			case WID_VL_LIST:
				this->DrawVehicleListItems(VehicleID::Invalid(), this->resize.step_height, r);
				break;
		}
	}

	void OnPaint() override
	{
		this->BuildVehicleList();
		this->SortVehicleList();

		if (this->vehicles.empty() && this->IsWidgetLowered(WID_VL_MANAGE_VEHICLES_DROPDOWN)) {
			this->CloseChildWindows(WC_DROPDOWN_MENU);
		}

		/* Hide the widgets that we will not use in this window
		 * Some windows contains actions only fit for the owner */
		int plane_to_show = (this->owner == _local_company) ? BP_SHOW_BUTTONS : BP_HIDE_BUTTONS;
		NWidgetStacked *nwi = this->GetWidget<NWidgetStacked>(WID_VL_HIDE_BUTTONS);
		if (plane_to_show != nwi->shown_plane) {
			nwi->SetDisplayedPlane(plane_to_show);
			nwi->SetDirty(this);
		}
		if (this->owner == _local_company) {
			this->SetWidgetDisabledState(WID_VL_AVAILABLE_VEHICLES, this->vli.type != VL_STANDARD);
			this->SetWidgetsDisabledState(this->vehicles.empty(),
				WID_VL_MANAGE_VEHICLES_DROPDOWN,
				WID_VL_STOP_ALL,
				WID_VL_START_ALL);
		}

		this->DrawWidgets();
	}

	bool last_overlay_state = false;
	void OnMouseLoop() override
	{
		if (last_overlay_state != ShowCargoIconOverlay()) {
			last_overlay_state = ShowCargoIconOverlay();
			this->SetDirty();
		}
	}

	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		switch (widget) {
		    case WID_VL_ORDER_VIEW: // Open the shared orders window
				assert(this->vli.type == VL_SHARED_ORDERS);
				assert(!this->vehicles.empty());
				ShowOrdersWindow(this->vehicles[0]);
				break;

			case WID_VL_SORT_ORDER: // Flip sorting method ascending/descending
				this->vehgroups.ToggleSortOrder();
				this->SetDirty();
				break;

			case WID_VL_GROUP_BY_PULLDOWN: // Select sorting criteria dropdown menu
				ShowDropDownMenu(this, this->vehicle_group_by_names, this->grouping, WID_VL_GROUP_BY_PULLDOWN, 0, 0);
				return;

			case WID_VL_SORT_BY_PULLDOWN: // Select sorting criteria dropdown menu
				ShowDropDownMenu(this, this->GetVehicleSorterNames(), this->vehgroups.SortType(), WID_VL_SORT_BY_PULLDOWN, 0,
						(this->vli.vtype == VEH_TRAIN || this->vli.vtype == VEH_ROAD) ? 0 : (1 << 10));
				return;

			case WID_VL_FILTER_BY_CARGO: // Cargo filter dropdown
				ShowDropDownList(this, this->BuildCargoDropDownList(false), this->cargo_filter_criteria, widget);
				break;

			case WID_VL_LIST: { // Matrix to show vehicles
				auto it = this->vscroll->GetScrolledItemFromWidget(this->vehgroups, pt.y, this, WID_VL_LIST);
				if (it == this->vehgroups.end()) return; // click out of list bound

				const GUIVehicleGroup &vehgroup = *it;
				switch (this->grouping) {
					case GB_NONE: {
						const Vehicle *v = vehgroup.GetSingleVehicle();
						if (!VehicleClicked(v)) {
							if (_ctrl_pressed) {
								ShowCompanyGroupForVehicle(v);
							} else {
								ShowVehicleViewWindow(v);
							}
						}
						break;
					}

					case GB_SHARED_ORDERS: {
						assert(vehgroup.NumVehicles() > 0);
						if (!VehicleClicked(vehgroup)) {
							const Vehicle *v = vehgroup.vehicles_begin[0];
							if (_ctrl_pressed) {
								ShowOrdersWindow(v);
							} else {
								if (vehgroup.NumVehicles() == 1) {
									ShowVehicleViewWindow(v);
								} else {
									ShowVehicleListWindow(v);
								}
							}
						}
						break;
					}

					default: NOT_REACHED();
				}

				break;
			}

			case WID_VL_AVAILABLE_VEHICLES:
				ShowBuildVehicleWindow(INVALID_TILE, this->vli.vtype);
				break;

			case WID_VL_MANAGE_VEHICLES_DROPDOWN: {
				ShowDropDownList(this, this->BuildActionDropdownList(this->vli.type == VL_STANDARD, false, true), 0, WID_VL_MANAGE_VEHICLES_DROPDOWN);
				break;
			}

			case WID_VL_STOP_ALL:
			case WID_VL_START_ALL:
				Command<CMD_MASS_START_STOP>::Post(TileIndex{}, widget == WID_VL_START_ALL, true, this->vli);
				break;
		}
	}

	void OnDropdownSelect(WidgetID widget, int index, int) override
	{
		switch (widget) {
			case WID_VL_GROUP_BY_PULLDOWN:
				this->UpdateVehicleGroupBy(static_cast<GroupBy>(index));
				break;

			case WID_VL_SORT_BY_PULLDOWN:
				this->vehgroups.SetSortType(index);
				break;

			case WID_VL_FILTER_BY_CARGO:
				this->SetCargoFilter(index);
				break;

			case WID_VL_MANAGE_VEHICLES_DROPDOWN:
				assert(!this->vehicles.empty());

				switch (index) {
					case ADI_REPLACE: // Replace window
						ShowReplaceGroupVehicleWindow(ALL_GROUP, this->vli.vtype);
						break;
					case ADI_SERVICE: // Send for servicing
					case ADI_DEPOT: // Send to Depots
						Command<CMD_SEND_VEHICLE_TO_DEPOT>::Post(GetCmdSendToDepotMsg(this->vli.vtype), VehicleID::Invalid(), (index == ADI_SERVICE ? DepotCommandFlag::Service : DepotCommandFlags{}) | DepotCommandFlag::MassSend, this->vli);
						break;

					case ADI_CREATE_GROUP: // Create group
						Command<CMD_ADD_VEHICLE_GROUP>::Post(CcAddVehicleNewGroup, NEW_GROUP, VehicleID::Invalid(), false, this->vli);
						break;

					default: NOT_REACHED();
				}
				break;

			default: NOT_REACHED();
		}
		this->SetDirty();
	}

	void OnGameTick() override
	{
		if (this->vehgroups.NeedResort()) {
			StationID station = (this->vli.type == VL_STATION_LIST) ? this->vli.ToStationID() : StationID::Invalid();

			Debug(misc, 3, "Periodic resort {} list company {} at station {}", this->vli.vtype, this->owner, station);
			this->SetDirty();
		}
	}

	void OnResize() override
	{
		this->vscroll->SetCapacityFromWidget(this, WID_VL_LIST);
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	void OnInvalidateData([[maybe_unused]] int data = 0, [[maybe_unused]] bool gui_scope = true) override
	{
		if (!gui_scope && HasBit(data, 31) && this->vli.type == VL_SHARED_ORDERS) {
			/* Needs to be done in command-scope, so everything stays valid */
			this->vli.SetIndex(GB(data, 0, 20));
			this->window_number = this->vli.ToWindowNumber();
			this->vehgroups.ForceRebuild();
			return;
		}

		if (data == 0) {
			/* This needs to be done in command-scope to enforce rebuilding before resorting invalid data */
			this->vehgroups.ForceRebuild();
		} else {
			this->vehgroups.ForceResort();
		}
	}
};

static WindowDesc _vehicle_list_desc[] = {
	{
		WDP_AUTO, "list_vehicles_train", 325, 246,
		WC_TRAINS_LIST, WC_NONE,
		{},
		_nested_vehicle_list
	},
	{
		WDP_AUTO, "list_vehicles_roadveh", 260, 246,
		WC_ROADVEH_LIST, WC_NONE,
		{},
		_nested_vehicle_list
	},
	{
		WDP_AUTO, "list_vehicles_ship", 260, 246,
		WC_SHIPS_LIST, WC_NONE,
		{},
		_nested_vehicle_list
	},
	{
		WDP_AUTO, "list_vehicles_aircraft", 260, 246,
		WC_AIRCRAFT_LIST, WC_NONE,
		{},
		_nested_vehicle_list
	}
};

static void ShowVehicleListWindowLocal(CompanyID company, VehicleListType vlt, VehicleType vehicle_type, uint32_t unique_number)
{
	if (!Company::IsValidID(company) && company != OWNER_NONE) return;

	assert(vehicle_type < std::size(_vehicle_list_desc));
	VehicleListIdentifier vli(vlt, vehicle_type, company, unique_number);
	AllocateWindowDescFront<VehicleListWindow>(_vehicle_list_desc[vehicle_type], vli.ToWindowNumber(), vli);
}

void ShowVehicleListWindow(CompanyID company, VehicleType vehicle_type)
{
	/* If _settings_client.gui.advanced_vehicle_list > 1, display the Advanced list
	 * if _settings_client.gui.advanced_vehicle_list == 1, display Advanced list only for local company
	 * if _ctrl_pressed, do the opposite action (Advanced list x Normal list)
	 */

	if ((_settings_client.gui.advanced_vehicle_list > (uint)(company != _local_company)) != _ctrl_pressed) {
		ShowCompanyGroup(company, vehicle_type);
	} else {
		ShowVehicleListWindowLocal(company, VL_STANDARD, vehicle_type, company.base());
	}
}

void ShowVehicleListWindow(const Vehicle *v)
{
	ShowVehicleListWindowLocal(v->owner, VL_SHARED_ORDERS, v->type, v->FirstShared()->index.base());
}

void ShowVehicleListWindow(CompanyID company, VehicleType vehicle_type, StationID station)
{
	ShowVehicleListWindowLocal(company, VL_STATION_LIST, vehicle_type, station.base());
}

void ShowVehicleListWindow(CompanyID company, VehicleType vehicle_type, TileIndex depot_tile)
{
	ShowVehicleListWindowLocal(company, VL_DEPOT_LIST, vehicle_type, GetDepotDestinationIndex(depot_tile).base());
}


/* Unified vehicle GUI - Vehicle Details Window */

static_assert(WID_VD_DETAILS_CARGO_CARRIED    == WID_VD_DETAILS_CARGO_CARRIED + TDW_TAB_CARGO   );
static_assert(WID_VD_DETAILS_TRAIN_VEHICLES   == WID_VD_DETAILS_CARGO_CARRIED + TDW_TAB_INFO    );
static_assert(WID_VD_DETAILS_CAPACITY_OF_EACH == WID_VD_DETAILS_CARGO_CARRIED + TDW_TAB_CAPACITY);
static_assert(WID_VD_DETAILS_TOTAL_CARGO      == WID_VD_DETAILS_CARGO_CARRIED + TDW_TAB_TOTALS  );

/** Vehicle details widgets (other than train). */
static constexpr NWidgetPart _nested_nontrain_vehicle_details_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_VD_CAPTION),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY, WID_VD_TOP_DETAILS), SetMinimalSize(405, 42), SetResize(1, 0), EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY, WID_VD_MIDDLE_DETAILS), SetMinimalSize(405, 45), SetResize(1, 0), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_VD_DECREASE_SERVICING_INTERVAL), SetFill(0, 1),
				SetArrowWidgetTypeTip(AWV_DECREASE),
		NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_VD_INCREASE_SERVICING_INTERVAL), SetFill(0, 1),
				SetArrowWidgetTypeTip(AWV_INCREASE),
		NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_VD_SERVICE_INTERVAL_DROPDOWN), SetFill(0, 1),
				SetStringTip(STR_EMPTY, STR_SERVICE_INTERVAL_DROPDOWN_TOOLTIP),
		NWidget(WWT_PANEL, COLOUR_GREY, WID_VD_SERVICING_INTERVAL), SetFill(1, 1), SetResize(1, 0), EndContainer(),
		NWidget(WWT_RESIZEBOX, COLOUR_GREY),
	EndContainer(),
};

/** Train details widgets. */
static constexpr NWidgetPart _nested_train_vehicle_details_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_VD_CAPTION), SetStringTip(STR_VEHICLE_DETAILS_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY, WID_VD_TOP_DETAILS), SetResize(1, 0), SetMinimalSize(405, 42), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_MATRIX, COLOUR_GREY, WID_VD_MATRIX), SetResize(1, 1), SetMinimalSize(393, 45), SetMatrixDataTip(1, 0), SetFill(1, 0), SetScrollbar(WID_VD_SCROLLBAR),
		NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_VD_SCROLLBAR),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_VD_DECREASE_SERVICING_INTERVAL), SetFill(0, 1),
				SetArrowWidgetTypeTip(AWV_DECREASE),
		NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_VD_INCREASE_SERVICING_INTERVAL), SetFill(0, 1),
				SetArrowWidgetTypeTip(AWV_INCREASE),
		NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_VD_SERVICE_INTERVAL_DROPDOWN), SetFill(0, 1),
				SetStringTip(STR_EMPTY, STR_SERVICE_INTERVAL_DROPDOWN_TOOLTIP),
		NWidget(WWT_PANEL, COLOUR_GREY, WID_VD_SERVICING_INTERVAL), SetFill(1, 1), SetResize(1, 0), EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VD_DETAILS_CARGO_CARRIED), SetMinimalSize(96, 12),
				SetStringTip(STR_VEHICLE_DETAIL_TAB_CARGO, STR_VEHICLE_DETAILS_TRAIN_CARGO_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VD_DETAILS_TRAIN_VEHICLES), SetMinimalSize(99, 12),
				SetStringTip(STR_VEHICLE_DETAIL_TAB_INFORMATION, STR_VEHICLE_DETAILS_TRAIN_INFORMATION_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VD_DETAILS_CAPACITY_OF_EACH), SetMinimalSize(99, 12),
				SetStringTip(STR_VEHICLE_DETAIL_TAB_CAPACITIES, STR_VEHICLE_DETAILS_TRAIN_CAPACITIES_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VD_DETAILS_TOTAL_CARGO), SetMinimalSize(99, 12),
				SetStringTip(STR_VEHICLE_DETAIL_TAB_TOTAL_CARGO, STR_VEHICLE_DETAILS_TRAIN_TOTAL_CARGO_TOOLTIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_RESIZEBOX, COLOUR_GREY),
	EndContainer(),
};


extern int GetTrainDetailsWndVScroll(VehicleID veh_id, TrainDetailsWindowTabs det_tab);
extern void DrawTrainDetails(const Train *v, const Rect &r, int vscroll_pos, uint16_t vscroll_cap, TrainDetailsWindowTabs det_tab);
extern void DrawRoadVehDetails(const Vehicle *v, const Rect &r);
extern void DrawShipDetails(const Vehicle *v, const Rect &r);
extern void DrawAircraftDetails(const Aircraft *v, const Rect &r);

static const StringID _service_interval_dropdown_calendar[] = {
	STR_VEHICLE_DETAILS_DEFAULT,
	STR_VEHICLE_DETAILS_DAYS,
	STR_VEHICLE_DETAILS_PERCENT,
};

static const StringID _service_interval_dropdown_wallclock[] = {
	STR_VEHICLE_DETAILS_DEFAULT,
	STR_VEHICLE_DETAILS_MINUTES,
	STR_VEHICLE_DETAILS_PERCENT,
};

/** Class for managing the vehicle details window. */
struct VehicleDetailsWindow : Window {
	TrainDetailsWindowTabs tab = TDW_TAB_CARGO; ///< For train vehicles: which tab is displayed.
	Scrollbar *vscroll = nullptr;

	/** Initialize a newly created vehicle details window */
	VehicleDetailsWindow(WindowDesc &desc, WindowNumber window_number) : Window(desc)
	{
		const Vehicle *v = Vehicle::Get(window_number);

		this->CreateNestedTree();
		this->vscroll = (v->type == VEH_TRAIN ? this->GetScrollbar(WID_VD_SCROLLBAR) : nullptr);
		this->FinishInitNested(window_number);

		this->owner = v->owner;
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	void OnInvalidateData([[maybe_unused]] int data = 0, [[maybe_unused]] bool gui_scope = true) override
	{
		if (data == VIWD_AUTOREPLACE) {
			/* Autoreplace replaced the vehicle.
			 * Nothing to do for this window. */
			return;
		}
		if (!gui_scope) return;
		const Vehicle *v = Vehicle::Get(this->window_number);
		if (v->type == VEH_ROAD) {
			const NWidgetBase *nwid_info = this->GetWidget<NWidgetBase>(WID_VD_MIDDLE_DETAILS);
			uint aimed_height = this->GetRoadVehDetailsHeight(v);
			/* If the number of articulated parts changes, the size of the window must change too. */
			if (aimed_height != nwid_info->current_y) {
				this->ReInit();
			}
		}
	}

	/**
	 * Gets the desired height for the road vehicle details panel.
	 * @param v Road vehicle being shown.
	 * @return Desired height in pixels.
	 */
	uint GetRoadVehDetailsHeight(const Vehicle *v)
	{
		uint desired_height;
		if (v->HasArticulatedPart()) {
			/* An articulated RV has its text drawn under the sprite instead of after it, hence 15 pixels extra. */
			desired_height = ScaleGUITrad(15) + 3 * GetCharacterHeight(FS_NORMAL) + WidgetDimensions::scaled.vsep_normal * 2;
			/* Add space for the cargo amount for each part. */
			for (const Vehicle *u = v; u != nullptr; u = u->Next()) {
				if (u->cargo_cap != 0) desired_height += GetCharacterHeight(FS_NORMAL);
			}
		} else {
			desired_height = 4 * GetCharacterHeight(FS_NORMAL) + WidgetDimensions::scaled.vsep_normal * 2;
		}
		return desired_height;
	}

	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		switch (widget) {
			case WID_VD_TOP_DETAILS: {
				Dimension dim = { 0, 0 };
				size.height = 4 * GetCharacterHeight(FS_NORMAL) + padding.height;

				uint64_t max_value = GetParamMaxValue(INT16_MAX);
				dim = maxdim(dim, GetStringBoundingBox(GetString(STR_VEHICLE_INFO_MAX_SPEED, max_value)));
				dim = maxdim(dim, GetStringBoundingBox(GetString(STR_VEHICLE_INFO_WEIGHT_POWER_MAX_SPEED, max_value, max_value, max_value)));
				dim = maxdim(dim, GetStringBoundingBox(GetString(STR_VEHICLE_INFO_WEIGHT_POWER_MAX_SPEED_MAX_TE, max_value, max_value, max_value, max_value)));
				dim = maxdim(dim, GetStringBoundingBox(GetString(STR_VEHICLE_INFO_PROFIT_THIS_YEAR_LAST_YEAR_MIN_PERFORMANCE, max_value, max_value, max_value)));
				dim = maxdim(dim, GetStringBoundingBox(GetString(STR_VEHICLE_INFO_PROFIT_THIS_PERIOD_LAST_PERIOD_MIN_PERFORMANCE, max_value, max_value, max_value)));
				dim = maxdim(dim, GetStringBoundingBox(GetString(STR_VEHICLE_INFO_RELIABILITY_BREAKDOWNS, max_value, max_value)));
				dim = maxdim(dim, GetStringBoundingBox(GetString(TimerGameEconomy::UsingWallclockUnits() ? STR_VEHICLE_INFO_AGE_RUNNING_COST_PERIOD : STR_VEHICLE_INFO_AGE_RUNNING_COST_YR, STR_VEHICLE_INFO_AGE, max_value, max_value, max_value)));
				size.width = dim.width + padding.width;
				break;
			}

			case WID_VD_MIDDLE_DETAILS: {
				const Vehicle *v = Vehicle::Get(this->window_number);
				switch (v->type) {
					case VEH_ROAD:
						size.height = this->GetRoadVehDetailsHeight(v) + padding.height;
						break;

					case VEH_SHIP:
						size.height = 4 * GetCharacterHeight(FS_NORMAL) + WidgetDimensions::scaled.vsep_normal * 2 + padding.height;
						break;

					case VEH_AIRCRAFT:
						size.height = 5 * GetCharacterHeight(FS_NORMAL) + WidgetDimensions::scaled.vsep_normal * 2 + padding.height;
						break;

					default:
						NOT_REACHED(); // Train uses WID_VD_MATRIX instead.
				}
				break;
			}

			case WID_VD_MATRIX:
				fill.height = resize.height = std::max<uint>(ScaleGUITrad(14), GetCharacterHeight(FS_NORMAL) + padding.height);
				size.height = 4 * resize.height;
				break;

			case WID_VD_SERVICE_INTERVAL_DROPDOWN: {
				Dimension d = maxdim(GetStringListBoundingBox(_service_interval_dropdown_calendar), GetStringListBoundingBox(_service_interval_dropdown_wallclock));
				d.width += padding.width;
				d.height += padding.height;
				size = maxdim(size, d);
				break;
			}

			case WID_VD_SERVICING_INTERVAL:
				/* Do we show the last serviced value as a date or minutes since service? */
				auto params = TimerGameEconomy::UsingWallclockUnits()
					? MakeParameters(GetParamMaxValue(MAX_SERVINT_DAYS), STR_VEHICLE_DETAILS_LAST_SERVICE_MINUTES_AGO, EconomyTime::MAX_DATE)
					: MakeParameters(GetParamMaxValue(MAX_SERVINT_DAYS), STR_VEHICLE_DETAILS_LAST_SERVICE_DATE, TimerGameEconomy::DateAtStartOfYear(EconomyTime::MAX_YEAR));

				size.width = std::max(size.width, GetStringBoundingBox(GetStringWithArgs(STR_VEHICLE_DETAILS_SERVICING_INTERVAL_PERCENT, params)).width);
				PrepareArgsForNextRun(params);
				size.width = std::max(size.width, GetStringBoundingBox(GetStringWithArgs(STR_VEHICLE_DETAILS_SERVICING_INTERVAL_DAYS, params)).width);

				size.width += padding.width;
				size.height = GetCharacterHeight(FS_NORMAL) + padding.height;
				break;
		}
	}

	/** Checks whether service interval is enabled for the vehicle. */
	static bool IsVehicleServiceIntervalEnabled(const VehicleType vehicle_type, CompanyID company_id)
	{
		if (_local_company != company_id) return false;

		const VehicleDefaultSettings *vds = &Company::Get(company_id)->settings.vehicle;
		switch (vehicle_type) {
			default: NOT_REACHED();
			case VEH_TRAIN:    return vds->servint_trains   != 0;
			case VEH_ROAD:     return vds->servint_roadveh  != 0;
			case VEH_SHIP:     return vds->servint_ships    != 0;
			case VEH_AIRCRAFT: return vds->servint_aircraft != 0;
		}
	}

	/**
	 * Draw the details for the given vehicle at the position of the Details windows
	 *
	 * @param v     current vehicle
	 * @param r     the Rect to draw within
	 * @param vscroll_pos Position of scrollbar (train only)
	 * @param vscroll_cap Number of lines currently displayed (train only)
	 * @param det_tab Selected details tab (train only)
	 */
	static void DrawVehicleDetails(const Vehicle *v, const Rect &r, int vscroll_pos, uint vscroll_cap, TrainDetailsWindowTabs det_tab)
	{
		switch (v->type) {
			case VEH_TRAIN:    DrawTrainDetails(Train::From(v), r, vscroll_pos, vscroll_cap, det_tab);  break;
			case VEH_ROAD:     DrawRoadVehDetails(v, r);  break;
			case VEH_SHIP:     DrawShipDetails(v, r);     break;
			case VEH_AIRCRAFT: DrawAircraftDetails(Aircraft::From(v), r); break;
			default: NOT_REACHED();
		}
	}

	std::string GetWidgetString(WidgetID widget, StringID stringid) const override
	{
		if (widget == WID_VD_CAPTION) return GetString(STR_VEHICLE_DETAILS_CAPTION, Vehicle::Get(this->window_number)->index);

		return this->Window::GetWidgetString(widget, stringid);
	}

	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		const Vehicle *v = Vehicle::Get(this->window_number);

		switch (widget) {
			case WID_VD_TOP_DETAILS: {
				Rect tr = r.Shrink(WidgetDimensions::scaled.framerect);

				/* Draw running cost */
				DrawString(tr,
					GetString(TimerGameEconomy::UsingWallclockUnits() ? STR_VEHICLE_INFO_AGE_RUNNING_COST_PERIOD : STR_VEHICLE_INFO_AGE_RUNNING_COST_YR,
						(v->age + CalendarTime::DAYS_IN_YEAR < v->max_age) ? STR_VEHICLE_INFO_AGE : STR_VEHICLE_INFO_AGE_RED,
						TimerGameCalendar::DateToYear(v->age),
						TimerGameCalendar::DateToYear(v->max_age),
						v->GetDisplayRunningCost()));
				tr.top += GetCharacterHeight(FS_NORMAL);

				/* Draw max speed */
				uint64_t max_speed = PackVelocity(v->GetDisplayMaxSpeed(), v->type);
				if (v->type == VEH_TRAIN ||
						(v->type == VEH_ROAD && _settings_game.vehicle.roadveh_acceleration_model != AM_ORIGINAL)) {
					const GroundVehicleCache *gcache = v->GetGroundVehicleCache();
					if (v->type == VEH_TRAIN && (_settings_game.vehicle.train_acceleration_model == AM_ORIGINAL ||
							GetRailTypeInfo(Train::From(v)->railtype)->acceleration_type == 2)) {
						DrawString(tr, GetString(STR_VEHICLE_INFO_WEIGHT_POWER_MAX_SPEED, gcache->cached_weight, gcache->cached_power, max_speed));
					} else {
						DrawString(tr, GetString(STR_VEHICLE_INFO_WEIGHT_POWER_MAX_SPEED_MAX_TE, gcache->cached_weight, gcache->cached_power, max_speed, gcache->cached_max_te));
					}
				} else if (v->type == VEH_AIRCRAFT) {
					StringID type = v->GetEngine()->GetAircraftTypeText();
					if (Aircraft::From(v)->GetRange() > 0) {
						DrawString(tr, GetString(STR_VEHICLE_INFO_MAX_SPEED_TYPE_RANGE, max_speed, type, Aircraft::From(v)->GetRange()));
					} else {
						DrawString(tr, GetString(STR_VEHICLE_INFO_MAX_SPEED_TYPE, max_speed, type));
					}
				} else {
					DrawString(tr, GetString(STR_VEHICLE_INFO_MAX_SPEED, max_speed));
				}
				tr.top += GetCharacterHeight(FS_NORMAL);

				/* Draw profit */
				if (v->IsGroundVehicle()) {
					DrawString(tr,
						GetString(TimerGameEconomy::UsingWallclockUnits() ? STR_VEHICLE_INFO_PROFIT_THIS_PERIOD_LAST_PERIOD_MIN_PERFORMANCE : STR_VEHICLE_INFO_PROFIT_THIS_YEAR_LAST_YEAR_MIN_PERFORMANCE,
							v->GetDisplayProfitThisYear(),
							v->GetDisplayProfitLastYear(),
							v->GetDisplayMinPowerToWeight()));
				} else {
					DrawString(tr,
						GetString(TimerGameEconomy::UsingWallclockUnits() ? STR_VEHICLE_INFO_PROFIT_THIS_PERIOD_LAST_PERIOD : STR_VEHICLE_INFO_PROFIT_THIS_YEAR_LAST_YEAR,
							v->GetDisplayProfitThisYear(),
							v->GetDisplayProfitLastYear()));
				}
				tr.top += GetCharacterHeight(FS_NORMAL);

				/* Draw breakdown & reliability */
				DrawString(tr, GetString(STR_VEHICLE_INFO_RELIABILITY_BREAKDOWNS, ToPercent16(v->reliability), v->breakdowns_since_last_service));
				break;
			}

			case WID_VD_MATRIX: {
				/* For trains only. */
				DrawVehicleDetails(v, r.Shrink(WidgetDimensions::scaled.matrix, RectPadding::zero).WithHeight(this->resize.step_height), this->vscroll->GetPosition(), this->vscroll->GetCapacity(), this->tab);
				break;
			}

			case WID_VD_MIDDLE_DETAILS: {
				/* For other vehicles, at the place of the matrix. */
				bool rtl = _current_text_dir == TD_RTL;
				uint sprite_width = GetSingleVehicleWidth(v, EIT_IN_DETAILS) + WidgetDimensions::scaled.framerect.Horizontal();
				Rect tr = r.Shrink(WidgetDimensions::scaled.framerect);

				/* Articulated road vehicles use a complete line. */
				if (v->type == VEH_ROAD && v->HasArticulatedPart()) {
					DrawVehicleImage(v, tr.WithHeight(ScaleGUITrad(GetVehicleHeight(v->type)), false), VehicleID::Invalid(), EIT_IN_DETAILS, 0);
				} else {
					Rect sr = tr.WithWidth(sprite_width, rtl);
					DrawVehicleImage(v, sr.WithHeight(ScaleGUITrad(GetVehicleHeight(v->type)), false), VehicleID::Invalid(), EIT_IN_DETAILS, 0);
				}

				DrawVehicleDetails(v, tr.Indent(sprite_width, rtl), 0, 0, this->tab);
				break;
			}

			case WID_VD_SERVICING_INTERVAL: {
				/* Draw service interval text */
				Rect tr = r.Shrink(WidgetDimensions::scaled.framerect);

				/* We're using wallclock units. Show minutes since last serviced. */
				if (TimerGameEconomy::UsingWallclockUnits()) {
					int minutes_since_serviced = (TimerGameEconomy::date - v->date_of_last_service).base() / EconomyTime::DAYS_IN_ECONOMY_MONTH;
					DrawString(tr.left, tr.right, CentreBounds(r.top, r.bottom, GetCharacterHeight(FS_NORMAL)),
							GetString(v->ServiceIntervalIsPercent() ? STR_VEHICLE_DETAILS_SERVICING_INTERVAL_PERCENT : STR_VEHICLE_DETAILS_SERVICING_INTERVAL_MINUTES,
									v->GetServiceInterval(), STR_VEHICLE_DETAILS_LAST_SERVICE_MINUTES_AGO, minutes_since_serviced));
					break;
				}

				/* We're using calendar dates. Show the date of last service. */
				DrawString(tr.left, tr.right, CentreBounds(r.top, r.bottom, GetCharacterHeight(FS_NORMAL)),
						GetString(v->ServiceIntervalIsPercent() ? STR_VEHICLE_DETAILS_SERVICING_INTERVAL_PERCENT : STR_VEHICLE_DETAILS_SERVICING_INTERVAL_DAYS,
								v->GetServiceInterval(), STR_VEHICLE_DETAILS_LAST_SERVICE_DATE, v->date_of_last_service));
				break;
			}
		}
	}

	/** Repaint vehicle details window. */
	void OnPaint() override
	{
		const Vehicle *v = Vehicle::Get(this->window_number);

		if (v->type == VEH_TRAIN) {
			this->LowerWidget(WID_VD_DETAILS_CARGO_CARRIED + this->tab);
			this->vscroll->SetCount(GetTrainDetailsWndVScroll(v->index, this->tab));
		}

		/* Disable service-scroller when interval is set to disabled */
		this->SetWidgetsDisabledState(!IsVehicleServiceIntervalEnabled(v->type, v->owner),
			WID_VD_INCREASE_SERVICING_INTERVAL,
			WID_VD_DECREASE_SERVICING_INTERVAL);

		StringID str =
			!v->ServiceIntervalIsCustom() ? STR_VEHICLE_DETAILS_DEFAULT :
			v->ServiceIntervalIsPercent() ? STR_VEHICLE_DETAILS_PERCENT :
			TimerGameEconomy::UsingWallclockUnits() ? STR_VEHICLE_DETAILS_MINUTES : STR_VEHICLE_DETAILS_DAYS;
		this->GetWidget<NWidgetCore>(WID_VD_SERVICE_INTERVAL_DROPDOWN)->SetString(str);
		this->SetWidgetDisabledState(WID_VD_SERVICE_INTERVAL_DROPDOWN, v->owner != _local_company);

		this->DrawWidgets();
	}

	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		switch (widget) {
			case WID_VD_INCREASE_SERVICING_INTERVAL:   // increase int
			case WID_VD_DECREASE_SERVICING_INTERVAL: { // decrease int
				const Vehicle *v = Vehicle::Get(this->window_number);
				int mod;
				if (!v->ServiceIntervalIsPercent() && TimerGameEconomy::UsingWallclockUnits()) {
					mod = _ctrl_pressed ? 1 : 5;
				} else {
					mod = _ctrl_pressed ? 5 : 10;
				}

				mod = (widget == WID_VD_DECREASE_SERVICING_INTERVAL) ? -mod : mod;
				mod = GetServiceIntervalClamped(mod + v->GetServiceInterval(), v->ServiceIntervalIsPercent());
				if (mod == v->GetServiceInterval()) return;

				Command<CMD_CHANGE_SERVICE_INT>::Post(STR_ERROR_CAN_T_CHANGE_SERVICING, v->index, mod, true, v->ServiceIntervalIsPercent());
				break;
			}

			case WID_VD_SERVICE_INTERVAL_DROPDOWN: {
				const Vehicle *v = Vehicle::Get(this->window_number);
				ShowDropDownMenu(this,
					TimerGameEconomy::UsingWallclockUnits() ? _service_interval_dropdown_wallclock : _service_interval_dropdown_calendar,
					v->ServiceIntervalIsCustom() ? (v->ServiceIntervalIsPercent() ? 2 : 1) : 0, widget, 0, 0);
				break;
			}

			case WID_VD_DETAILS_CARGO_CARRIED:
			case WID_VD_DETAILS_TRAIN_VEHICLES:
			case WID_VD_DETAILS_CAPACITY_OF_EACH:
			case WID_VD_DETAILS_TOTAL_CARGO:
				this->SetWidgetsLoweredState(false,
					WID_VD_DETAILS_CARGO_CARRIED,
					WID_VD_DETAILS_TRAIN_VEHICLES,
					WID_VD_DETAILS_CAPACITY_OF_EACH,
					WID_VD_DETAILS_TOTAL_CARGO);

				this->tab = (TrainDetailsWindowTabs)(widget - WID_VD_DETAILS_CARGO_CARRIED);
				this->SetDirty();
				break;
		}
	}

	bool OnTooltip([[maybe_unused]] Point pt, WidgetID widget, TooltipCloseCondition close_cond) override
	{
		if (widget == WID_VD_INCREASE_SERVICING_INTERVAL || widget == WID_VD_DECREASE_SERVICING_INTERVAL) {
			const Vehicle *v = Vehicle::Get(this->window_number);
			StringID tool_tip;
			if (v->ServiceIntervalIsPercent()) {
				tool_tip = widget == WID_VD_INCREASE_SERVICING_INTERVAL ? STR_VEHICLE_DETAILS_INCREASE_SERVICING_INTERVAL_TOOLTIP_PERCENT : STR_VEHICLE_DETAILS_DECREASE_SERVICING_INTERVAL_TOOLTIP_PERCENT;
			} else if (TimerGameEconomy::UsingWallclockUnits()) {
				tool_tip = widget == WID_VD_INCREASE_SERVICING_INTERVAL ? STR_VEHICLE_DETAILS_INCREASE_SERVICING_INTERVAL_TOOLTIP_MINUTES : STR_VEHICLE_DETAILS_DECREASE_SERVICING_INTERVAL_TOOLTIP_MINUTES;
			} else {
				tool_tip = widget == WID_VD_INCREASE_SERVICING_INTERVAL ? STR_VEHICLE_DETAILS_INCREASE_SERVICING_INTERVAL_TOOLTIP_DAYS : STR_VEHICLE_DETAILS_DECREASE_SERVICING_INTERVAL_TOOLTIP_DAYS;
			}
			GuiShowTooltips(this, GetEncodedString(tool_tip), close_cond);
			return true;
		}

		return false;
	}

	void OnDropdownSelect(WidgetID widget, int index, int) override
	{
		switch (widget) {
			case WID_VD_SERVICE_INTERVAL_DROPDOWN: {
				const Vehicle *v = Vehicle::Get(this->window_number);
				bool iscustom = index != 0;
				bool ispercent = iscustom ? (index == 2) : Company::Get(v->owner)->settings.vehicle.servint_ispercent;
				uint16_t interval = GetServiceIntervalClamped(v->GetServiceInterval(), ispercent);
				Command<CMD_CHANGE_SERVICE_INT>::Post(STR_ERROR_CAN_T_CHANGE_SERVICING, v->index, interval, iscustom, ispercent);
				break;
			}
		}
	}

	void OnResize() override
	{
		NWidgetCore *nwi = this->GetWidget<NWidgetCore>(WID_VD_MATRIX);
		if (nwi != nullptr) {
			this->vscroll->SetCapacityFromWidget(this, WID_VD_MATRIX);
		}
	}
};

/** Vehicle details window descriptor. */
static WindowDesc _train_vehicle_details_desc(
	WDP_AUTO, "view_vehicle_details_train", 405, 178,
	WC_VEHICLE_DETAILS, WC_VEHICLE_VIEW,
	{},
	_nested_train_vehicle_details_widgets
);

/** Vehicle details window descriptor for other vehicles than a train. */
static WindowDesc _nontrain_vehicle_details_desc(
	WDP_AUTO, "view_vehicle_details", 405, 113,
	WC_VEHICLE_DETAILS, WC_VEHICLE_VIEW,
	{},
	_nested_nontrain_vehicle_details_widgets
);

/** Shows the vehicle details window of the given vehicle. */
static void ShowVehicleDetailsWindow(const Vehicle *v)
{
	CloseWindowById(WC_VEHICLE_ORDERS, v->index, false);
	CloseWindowById(WC_VEHICLE_TIMETABLE, v->index, false);
	AllocateWindowDescFront<VehicleDetailsWindow>((v->type == VEH_TRAIN) ? _train_vehicle_details_desc : _nontrain_vehicle_details_desc, v->index);
}


/* Unified vehicle GUI - Vehicle View Window */

/** Vehicle view widgets. */
static constexpr NWidgetPart _nested_vehicle_view_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_RENAME), SetAspect(WidgetDimensions::ASPECT_RENAME), SetSpriteTip(SPR_RENAME),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_VV_CAPTION),
		NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_LOCATION), SetAspect(WidgetDimensions::ASPECT_LOCATION), SetSpriteTip(SPR_GOTO_LOCATION),
		NWidget(WWT_DEBUGBOX, COLOUR_GREY),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_GREY),
			NWidget(WWT_INSET, COLOUR_GREY), SetPadding(2, 2, 2, 2),
				NWidget(NWID_VIEWPORT, INVALID_COLOUR, WID_VV_VIEWPORT), SetMinimalSize(226, 84), SetResize(1, 1),
			EndContainer(),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_SELECTION, INVALID_COLOUR, WID_VV_SELECT_DEPOT_CLONE),
				NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_GOTO_DEPOT), SetMinimalSize(18, 18), SetSpriteTip(SPR_EMPTY /* filled later */),
				NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_CLONE), SetMinimalSize(18, 18), SetSpriteTip(SPR_EMPTY /* filled later */),
			EndContainer(),
			/* For trains only, 'ignore signal' button. */
			NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_FORCE_PROCEED), SetMinimalSize(18, 18),
											SetSpriteTip(SPR_IGNORE_SIGNALS, STR_VEHICLE_VIEW_TRAIN_IGNORE_SIGNAL_TOOLTIP),
			NWidget(NWID_SELECTION, INVALID_COLOUR, WID_VV_SELECT_REFIT_TURN),
				NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_REFIT), SetMinimalSize(18, 18), SetSpriteTip(SPR_REFIT_VEHICLE),
				NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_TURN_AROUND), SetMinimalSize(18, 18),
												SetSpriteTip(SPR_FORCE_VEHICLE_TURN, STR_VEHICLE_VIEW_ROAD_VEHICLE_REVERSE_TOOLTIP),
			EndContainer(),
			NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_SHOW_ORDERS), SetMinimalSize(18, 18), SetSpriteTip(SPR_SHOW_ORDERS),
			NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_SHOW_DETAILS), SetMinimalSize(18, 18), SetSpriteTip(SPR_SHOW_VEHICLE_DETAILS),
			NWidget(WWT_PANEL, COLOUR_GREY), SetMinimalSize(18, 0), SetResize(0, 1), EndContainer(),
		EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHBTN, COLOUR_GREY, WID_VV_START_STOP), SetResize(1, 0), SetFill(1, 0),
		NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VV_ORDER_LOCATION), SetAspect(WidgetDimensions::ASPECT_LOCATION), SetSpriteTip(SPR_GOTO_LOCATION, STR_VEHICLE_VIEW_ORDER_LOCATION_TOOLTIP),
		NWidget(WWT_RESIZEBOX, COLOUR_GREY),
	EndContainer(),
};

/* Just to make sure, nobody has changed the vehicle type constants, as we are
	 using them for array indexing in a number of places here. */
static_assert(VEH_TRAIN == 0);
static_assert(VEH_ROAD == 1);
static_assert(VEH_SHIP == 2);
static_assert(VEH_AIRCRAFT == 3);

/** Zoom levels for vehicle views indexed by vehicle type. */
static const ZoomLevel _vehicle_view_zoom_levels[] = {
	ZoomLevel::Train,
	ZoomLevel::RoadVehicle,
	ZoomLevel::Ship,
	ZoomLevel::Aircraft,
};

/* Constants for geometry of vehicle view viewport */
static const int VV_INITIAL_VIEWPORT_WIDTH = 226;
static const int VV_INITIAL_VIEWPORT_HEIGHT = 84;
static const int VV_INITIAL_VIEWPORT_HEIGHT_TRAIN = 102;

/** Command indices for the _vehicle_command_translation_table. */
enum VehicleCommandTranslation : uint8_t {
	VCT_CMD_START_STOP = 0,
	VCT_CMD_CLONE_VEH,
	VCT_CMD_TURN_AROUND,
};

/** Command codes for the shared buttons indexed by VehicleCommandTranslation and vehicle type. */
static const StringID _vehicle_msg_translation_table[][4] = {
	{ // VCT_CMD_START_STOP
		STR_ERROR_CAN_T_STOP_START_TRAIN,
		STR_ERROR_CAN_T_STOP_START_ROAD_VEHICLE,
		STR_ERROR_CAN_T_STOP_START_SHIP,
		STR_ERROR_CAN_T_STOP_START_AIRCRAFT
	},
	{ // VCT_CMD_CLONE_VEH
		STR_ERROR_CAN_T_BUY_TRAIN,
		STR_ERROR_CAN_T_BUY_ROAD_VEHICLE,
		STR_ERROR_CAN_T_BUY_SHIP,
		STR_ERROR_CAN_T_BUY_AIRCRAFT
	},
	{ // VCT_CMD_TURN_AROUND
		STR_ERROR_CAN_T_REVERSE_DIRECTION_TRAIN,
		STR_ERROR_CAN_T_MAKE_ROAD_VEHICLE_TURN,
		INVALID_STRING_ID, // invalid for ships
		INVALID_STRING_ID  // invalid for aircraft
	},
};

/**
 * This is the Callback method after attempting to start/stop a vehicle
 * @param result the result of the start/stop command
 * @param veh_id Vehicle ID.
 */
void CcStartStopVehicle(Commands, const CommandCost &result, VehicleID veh_id, bool)
{
	if (result.Failed()) return;

	const Vehicle *v = Vehicle::GetIfValid(veh_id);
	if (v == nullptr || !v->IsPrimaryVehicle() || v->owner != _local_company) return;

	StringID msg = v->vehstatus.Test(VehState::Stopped) ? STR_VEHICLE_COMMAND_STOPPED : STR_VEHICLE_COMMAND_STARTED;
	Point pt = RemapCoords(v->x_pos, v->y_pos, v->z_pos);
	AddTextEffect(GetEncodedString(msg), pt.x, pt.y, Ticks::DAY_TICKS, TE_RISING);
}

/**
 * Executes #CMD_START_STOP_VEHICLE for given vehicle.
 * @param v Vehicle to start/stop
 * @param texteffect Should a texteffect be shown?
 */
void StartStopVehicle(const Vehicle *v, bool texteffect)
{
	assert(v->IsPrimaryVehicle());
	Command<CMD_START_STOP_VEHICLE>::Post(_vehicle_msg_translation_table[VCT_CMD_START_STOP][v->type], texteffect ? CcStartStopVehicle : nullptr, v->tile, v->index, false);
}

/** Checks whether the vehicle may be refitted at the moment.*/
static bool IsVehicleRefitable(const Vehicle *v)
{
	if (!v->IsStoppedInDepot()) return false;

	do {
		if (IsEngineRefittable(v->engine_type)) return true;
	} while (v->IsGroundVehicle() && (v = v->Next()) != nullptr);

	return false;
}

/** Window manager class for viewing a vehicle. */
struct VehicleViewWindow : Window {
private:
	/** Display planes available in the vehicle view window. */
	enum PlaneSelections : uint8_t {
		SEL_DC_GOTO_DEPOT,  ///< Display 'goto depot' button in #WID_VV_SELECT_DEPOT_CLONE stacked widget.
		SEL_DC_CLONE,       ///< Display 'clone vehicle' button in #WID_VV_SELECT_DEPOT_CLONE stacked widget.

		SEL_RT_REFIT,       ///< Display 'refit' button in #WID_VV_SELECT_REFIT_TURN stacked widget.
		SEL_RT_TURN_AROUND, ///< Display 'turn around' button in #WID_VV_SELECT_REFIT_TURN stacked widget.

		SEL_DC_BASEPLANE = SEL_DC_GOTO_DEPOT, ///< First plane of the #WID_VV_SELECT_DEPOT_CLONE stacked widget.
		SEL_RT_BASEPLANE = SEL_RT_REFIT,      ///< First plane of the #WID_VV_SELECT_REFIT_TURN stacked widget.
	};
	bool mouse_over_start_stop = false;

	/**
	 * Display a plane in the window.
	 * @param plane Plane to show.
	 */
	void SelectPlane(PlaneSelections plane)
	{
		switch (plane) {
			case SEL_DC_GOTO_DEPOT:
			case SEL_DC_CLONE:
				this->GetWidget<NWidgetStacked>(WID_VV_SELECT_DEPOT_CLONE)->SetDisplayedPlane(plane - SEL_DC_BASEPLANE);
				break;

			case SEL_RT_REFIT:
			case SEL_RT_TURN_AROUND:
				this->GetWidget<NWidgetStacked>(WID_VV_SELECT_REFIT_TURN)->SetDisplayedPlane(plane - SEL_RT_BASEPLANE);
				break;

			default:
				NOT_REACHED();
		}
	}

public:
	VehicleViewWindow(WindowDesc &desc, WindowNumber window_number) : Window(desc)
	{
		this->flags.Set(WindowFlag::DisableVpScroll);
		this->CreateNestedTree();

		/* Sprites for the 'send to depot' button indexed by vehicle type. */
		static const SpriteID vehicle_view_goto_depot_sprites[] = {
			SPR_SEND_TRAIN_TODEPOT,
			SPR_SEND_ROADVEH_TODEPOT,
			SPR_SEND_SHIP_TODEPOT,
			SPR_SEND_AIRCRAFT_TODEPOT,
		};
		const Vehicle *v = Vehicle::Get(window_number);
		this->GetWidget<NWidgetCore>(WID_VV_GOTO_DEPOT)->SetSprite(vehicle_view_goto_depot_sprites[v->type]);

		/* Sprites for the 'clone vehicle' button indexed by vehicle type. */
		static const SpriteID vehicle_view_clone_sprites[] = {
			SPR_CLONE_TRAIN,
			SPR_CLONE_ROADVEH,
			SPR_CLONE_SHIP,
			SPR_CLONE_AIRCRAFT,
		};
		this->GetWidget<NWidgetCore>(WID_VV_CLONE)->SetSprite(vehicle_view_clone_sprites[v->type]);

		switch (v->type) {
			case VEH_TRAIN:
				this->GetWidget<NWidgetCore>(WID_VV_TURN_AROUND)->SetToolTip(STR_VEHICLE_VIEW_TRAIN_REVERSE_TOOLTIP);
				break;

			case VEH_ROAD:
				break;

			case VEH_SHIP:
			case VEH_AIRCRAFT:
				this->SelectPlane(SEL_RT_REFIT);
				break;

			default: NOT_REACHED();
		}
		this->FinishInitNested(window_number);
		this->owner = v->owner;
		this->GetWidget<NWidgetViewport>(WID_VV_VIEWPORT)->InitializeViewport(this, static_cast<VehicleID>(this->window_number), ScaleZoomGUI(_vehicle_view_zoom_levels[v->type]));

		this->GetWidget<NWidgetCore>(WID_VV_START_STOP)->SetToolTip(STR_VEHICLE_VIEW_TRAIN_STATUS_START_STOP_TOOLTIP + v->type);
		this->GetWidget<NWidgetCore>(WID_VV_RENAME)->SetToolTip(STR_VEHICLE_DETAILS_TRAIN_RENAME + v->type);
		this->GetWidget<NWidgetCore>(WID_VV_LOCATION)->SetToolTip(STR_VEHICLE_VIEW_TRAIN_CENTER_TOOLTIP + v->type);
		this->GetWidget<NWidgetCore>(WID_VV_REFIT)->SetToolTip(STR_VEHICLE_VIEW_TRAIN_REFIT_TOOLTIP + v->type);
		this->GetWidget<NWidgetCore>(WID_VV_GOTO_DEPOT)->SetToolTip(STR_VEHICLE_VIEW_TRAIN_SEND_TO_DEPOT_TOOLTIP + v->type);
		this->GetWidget<NWidgetCore>(WID_VV_SHOW_ORDERS)->SetToolTip(STR_VEHICLE_VIEW_TRAIN_ORDERS_TOOLTIP + v->type);
		this->GetWidget<NWidgetCore>(WID_VV_SHOW_DETAILS)->SetToolTip(STR_VEHICLE_VIEW_TRAIN_SHOW_DETAILS_TOOLTIP + v->type);
		this->GetWidget<NWidgetCore>(WID_VV_CLONE)->SetToolTip(STR_VEHICLE_VIEW_CLONE_TRAIN_INFO + v->type);

		this->UpdateButtonStatus();
	}

	void Close([[maybe_unused]] int data = 0) override
	{
		CloseWindowById(WC_VEHICLE_ORDERS, this->window_number, false);
		CloseWindowById(WC_VEHICLE_REFIT, this->window_number, false);
		CloseWindowById(WC_VEHICLE_DETAILS, this->window_number, false);
		CloseWindowById(WC_VEHICLE_TIMETABLE, this->window_number, false);
		this->Window::Close();
	}

	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		const Vehicle *v = Vehicle::Get(this->window_number);
		switch (widget) {
			case WID_VV_START_STOP:
				size.height = std::max<uint>({size.height, (uint)GetCharacterHeight(FS_NORMAL), GetScaledSpriteSize(SPR_WARNING_SIGN).height, GetScaledSpriteSize(SPR_FLAG_VEH_STOPPED).height, GetScaledSpriteSize(SPR_FLAG_VEH_RUNNING).height}) + padding.height;
				break;

			case WID_VV_FORCE_PROCEED:
				if (v->type != VEH_TRAIN) {
					size.height = 0;
					size.width = 0;
				}
				break;

			case WID_VV_VIEWPORT:
				size.width = VV_INITIAL_VIEWPORT_WIDTH;
				size.height = (v->type == VEH_TRAIN) ? VV_INITIAL_VIEWPORT_HEIGHT_TRAIN : VV_INITIAL_VIEWPORT_HEIGHT;
				break;
		}
	}

	void OnPaint() override
	{
		const Vehicle *v = Vehicle::Get(this->window_number);
		bool is_localcompany = v->owner == _local_company;
		bool refitable_and_stopped_in_depot = IsVehicleRefitable(v);

		this->SetWidgetDisabledState(WID_VV_RENAME, !is_localcompany);
		this->SetWidgetDisabledState(WID_VV_GOTO_DEPOT, !is_localcompany);
		this->SetWidgetDisabledState(WID_VV_REFIT, !refitable_and_stopped_in_depot || !is_localcompany);
		this->SetWidgetDisabledState(WID_VV_CLONE, !is_localcompany);

		if (v->type == VEH_TRAIN) {
			this->SetWidgetLoweredState(WID_VV_FORCE_PROCEED, Train::From(v)->force_proceed == TFP_SIGNAL);
			this->SetWidgetDisabledState(WID_VV_FORCE_PROCEED, !is_localcompany);
		}

		if (v->type == VEH_TRAIN || v->type == VEH_ROAD) {
			this->SetWidgetDisabledState(WID_VV_TURN_AROUND, !is_localcompany);
		}

		this->SetWidgetDisabledState(WID_VV_ORDER_LOCATION, v->current_order.GetLocation(v) == INVALID_TILE);

		const Window *mainwindow = GetMainWindow();
		if (mainwindow->viewport->follow_vehicle == v->index) {
			this->LowerWidget(WID_VV_LOCATION);
		}

		this->DrawWidgets();
	}

	std::string GetWidgetString(WidgetID widget, StringID stringid) const override
	{
		if (widget != WID_VV_CAPTION) return this->Window::GetWidgetString(widget, stringid);

		const Vehicle *v = Vehicle::Get(this->window_number);
		return GetString(STR_VEHICLE_VIEW_CAPTION, v->index);
	}

	std::string GetVehicleStatusString(const Vehicle *v, TextColour &text_colour) const
	{
		text_colour = TC_BLACK;

		if (v->vehstatus.Test(VehState::Crashed)) return GetString(STR_VEHICLE_STATUS_CRASHED);

		if (v->type != VEH_AIRCRAFT && v->breakdown_ctr == 1) return GetString(STR_VEHICLE_STATUS_BROKEN_DOWN);

		if (v->vehstatus.Test(VehState::Stopped) && (!mouse_over_start_stop || v->IsStoppedInDepot())) {
			if (v->type != VEH_TRAIN) return GetString(STR_VEHICLE_STATUS_STOPPED);
			if (v->cur_speed != 0) return GetString(STR_VEHICLE_STATUS_TRAIN_STOPPING_VEL, PackVelocity(v->GetDisplaySpeed(), v->type));
			if (Train::From(v)->gcache.cached_power == 0) return GetString(STR_VEHICLE_STATUS_TRAIN_NO_POWER);
			return GetString(STR_VEHICLE_STATUS_STOPPED);
		}

		if (v->IsInDepot() && v->IsWaitingForUnbunching()) return GetString(STR_VEHICLE_STATUS_WAITING_UNBUNCHING);

		if (v->type == VEH_TRAIN && Train::From(v)->flags.Test(VehicleRailFlag::Stuck) && !v->current_order.IsType(OT_LOADING)) return GetString(STR_VEHICLE_STATUS_TRAIN_STUCK);

		if (v->type == VEH_AIRCRAFT && HasBit(Aircraft::From(v)->flags, VAF_DEST_TOO_FAR) && !v->current_order.IsType(OT_LOADING)) return GetString(STR_VEHICLE_STATUS_AIRCRAFT_TOO_FAR);

		/* Vehicle is in a "normal" state, show current order. */
		if (mouse_over_start_stop) {
			if (v->vehstatus.Test(VehState::Stopped)) {
				text_colour = TC_RED | TC_FORCED;
			} else if (v->type == VEH_TRAIN && Train::From(v)->flags.Test(VehicleRailFlag::Stuck) && !v->current_order.IsType(OT_LOADING)) {
				text_colour = TC_ORANGE | TC_FORCED;
			}
		}

		switch (v->current_order.GetType()) {
			case OT_GOTO_STATION:
				return GetString(v->vehicle_flags.Test(VehicleFlag::PathfinderLost) ? STR_VEHICLE_STATUS_CANNOT_REACH_STATION_VEL : STR_VEHICLE_STATUS_HEADING_FOR_STATION_VEL,
					v->current_order.GetDestination(), PackVelocity(v->GetDisplaySpeed(), v->type));

			case OT_GOTO_DEPOT: {
				/* This case *only* happens when multiple nearest depot orders
				 * follow each other (including an order list only one order: a
				 * nearest depot order) and there are no reachable depots.
				 * It is primarily to guard for the case that there is no
				 * depot with index 0, which would be used as fallback for
				 * evaluating the string in the status bar. */
				if (v->current_order.GetDestination() == DepotID::Invalid()) return {};

				auto params = MakeParameters(v->type, v->current_order.GetDestination(), PackVelocity(v->GetDisplaySpeed(), v->type));
				if (v->current_order.GetDepotActionType() & ODATFB_HALT) {
					return GetStringWithArgs(v->vehicle_flags.Test(VehicleFlag::PathfinderLost) ? STR_VEHICLE_STATUS_CANNOT_REACH_DEPOT_VEL : STR_VEHICLE_STATUS_HEADING_FOR_DEPOT_VEL, params);
				}

				if (v->current_order.GetDepotActionType() & ODATFB_UNBUNCH) {
					return GetStringWithArgs(v->vehicle_flags.Test(VehicleFlag::PathfinderLost) ? STR_VEHICLE_STATUS_CANNOT_REACH_DEPOT_SERVICE_VEL : STR_VEHICLE_STATUS_HEADING_FOR_DEPOT_UNBUNCH_VEL, params);
				}

				return GetStringWithArgs(v->vehicle_flags.Test(VehicleFlag::PathfinderLost) ? STR_VEHICLE_STATUS_CANNOT_REACH_DEPOT_SERVICE_VEL : STR_VEHICLE_STATUS_HEADING_FOR_DEPOT_SERVICE_VEL, params);
			}

			case OT_LOADING:
				return GetString(STR_VEHICLE_STATUS_LOADING_UNLOADING);

			case OT_GOTO_WAYPOINT:
				assert(v->type == VEH_TRAIN || v->type == VEH_ROAD || v->type == VEH_SHIP);
				return GetString(v->vehicle_flags.Test(VehicleFlag::PathfinderLost) ? STR_VEHICLE_STATUS_CANNOT_REACH_WAYPOINT_VEL : STR_VEHICLE_STATUS_HEADING_FOR_WAYPOINT_VEL,
					v->current_order.GetDestination(),PackVelocity(v->GetDisplaySpeed(), v->type));

			case OT_LEAVESTATION:
				if (v->type != VEH_AIRCRAFT) {
					return GetString(STR_VEHICLE_STATUS_LEAVING);
				}
				[[fallthrough]];

			default:
				if (v->GetNumManualOrders() == 0) {
					return GetString(STR_VEHICLE_STATUS_NO_ORDERS_VEL, PackVelocity(v->GetDisplaySpeed(), v->type));
				}

				return {};
		}
	}

	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		if (widget != WID_VV_START_STOP) return;

		/* Draw the flag plus orders. */
		bool rtl = (_current_text_dir == TD_RTL);
		uint icon_width = std::max({GetScaledSpriteSize(SPR_WARNING_SIGN).width, GetScaledSpriteSize(SPR_FLAG_VEH_STOPPED).width, GetScaledSpriteSize(SPR_FLAG_VEH_RUNNING).width});
		Rect tr = r.Shrink(WidgetDimensions::scaled.framerect);

		const Vehicle *v = Vehicle::Get(this->window_number);
		SpriteID image = v->vehstatus.Test(VehState::Stopped) ? SPR_FLAG_VEH_STOPPED : (v->vehicle_flags.Test(VehicleFlag::PathfinderLost)) ? SPR_WARNING_SIGN : SPR_FLAG_VEH_RUNNING;
		DrawSpriteIgnorePadding(image, PAL_NONE, tr.WithWidth(icon_width, rtl), SA_CENTER);

		tr = tr.Indent(icon_width + WidgetDimensions::scaled.imgbtn.Horizontal(), rtl);

		TextColour text_colour = TC_FROMSTRING;
		std::string str = GetVehicleStatusString(v, text_colour);
		DrawString(tr.left, tr.right, CentreBounds(tr.top, tr.bottom, GetCharacterHeight(FS_NORMAL)), str, text_colour, SA_HOR_CENTER);
	}

	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		const Vehicle *v = Vehicle::Get(this->window_number);

		switch (widget) {
			case WID_VV_RENAME: { // rename
				ShowQueryString(GetString(STR_VEHICLE_NAME, v->index), STR_QUERY_RENAME_TRAIN_CAPTION + v->type,
						MAX_LENGTH_VEHICLE_NAME_CHARS, this, CS_ALPHANUMERAL, {QueryStringFlag::EnableDefault, QueryStringFlag::LengthIsInChars});
				break;
			}

			case WID_VV_START_STOP: // start stop
				StartStopVehicle(v, false);
				break;

			case WID_VV_ORDER_LOCATION: {
				/* Scroll to current order destination */
				TileIndex tile = v->current_order.GetLocation(v);
				if (tile == INVALID_TILE) break;

				if (_ctrl_pressed) {
					ShowExtraViewportWindow(tile);
				} else {
					ScrollMainWindowToTile(tile);
				}
				break;
			}

			case WID_VV_LOCATION: // center main view
				if (_ctrl_pressed) {
					ShowExtraViewportWindow(TileVirtXY(v->x_pos, v->y_pos));
				} else {
					const Window *mainwindow = GetMainWindow();
					if (click_count > 1) {
						/* main window 'follows' vehicle */
						mainwindow->viewport->follow_vehicle = v->index;
					} else {
						if (mainwindow->viewport->follow_vehicle == v->index) mainwindow->viewport->follow_vehicle = VehicleID::Invalid();
						ScrollMainWindowTo(v->x_pos, v->y_pos, v->z_pos);
					}
				}
				break;

			case WID_VV_GOTO_DEPOT: // goto hangar
				Command<CMD_SEND_VEHICLE_TO_DEPOT>::Post(GetCmdSendToDepotMsg(v), v->index, _ctrl_pressed ? DepotCommandFlag::Service : DepotCommandFlags{}, {});
				break;
			case WID_VV_REFIT: // refit
				ShowVehicleRefitWindow(v, INVALID_VEH_ORDER_ID, this);
				break;
			case WID_VV_SHOW_ORDERS: // show orders
				if (_ctrl_pressed) {
					ShowTimetableWindow(v);
				} else {
					ShowOrdersWindow(v);
				}
				break;
			case WID_VV_SHOW_DETAILS: // show details
				if (_ctrl_pressed) {
					ShowCompanyGroupForVehicle(v);
				} else {
					ShowVehicleDetailsWindow(v);
				}
				break;
			case WID_VV_CLONE: // clone vehicle
				/* Suppress the vehicle GUI when share-cloning.
				 * There is no point to it except for starting the vehicle.
				 * For starting the vehicle the player has to open the depot GUI, which is
				 * most likely already open, but is also visible in the vehicle viewport. */
				Command<CMD_CLONE_VEHICLE>::Post(_vehicle_msg_translation_table[VCT_CMD_CLONE_VEH][v->type],
										_ctrl_pressed ? nullptr : CcCloneVehicle,
										v->tile, v->index, _ctrl_pressed);
				break;
			case WID_VV_TURN_AROUND: // turn around
				assert(v->IsGroundVehicle());
				if (v->type == VEH_ROAD) {
					Command<CMD_TURN_ROADVEH>::Post(_vehicle_msg_translation_table[VCT_CMD_TURN_AROUND][v->type], v->tile, v->index);
				} else {
					Command<CMD_REVERSE_TRAIN_DIRECTION>::Post(_vehicle_msg_translation_table[VCT_CMD_TURN_AROUND][v->type], v->tile, v->index, false);
				}
				break;
			case WID_VV_FORCE_PROCEED: // force proceed
				assert(v->type == VEH_TRAIN);
				Command<CMD_FORCE_TRAIN_PROCEED>::Post(STR_ERROR_CAN_T_MAKE_TRAIN_PASS_SIGNAL, v->tile, v->index);
				break;
		}
	}

	EventState OnHotkey(int hotkey) override
	{
		/* If the hotkey is not for any widget in the UI (i.e. for honking) */
		if (hotkey == WID_VV_HONK_HORN) {
			const Window *mainwindow = GetMainWindow();
			const Vehicle *v = Vehicle::Get(window_number);
			/* Only play the sound if we're following this vehicle */
			if (mainwindow->viewport->follow_vehicle == v->index) {
				v->PlayLeaveStationSound(true);
			}
		}
		return Window::OnHotkey(hotkey);
	}

	void OnQueryTextFinished(std::optional<std::string> str) override
	{
		if (!str.has_value()) return;

		Command<CMD_RENAME_VEHICLE>::Post(STR_ERROR_CAN_T_RENAME_TRAIN + Vehicle::Get(this->window_number)->type, static_cast<VehicleID>(this->window_number), *str);
	}

	void OnMouseOver([[maybe_unused]] Point pt, WidgetID widget) override
	{
		bool start_stop = widget == WID_VV_START_STOP;
		if (start_stop != mouse_over_start_stop) {
			mouse_over_start_stop = start_stop;
			this->SetWidgetDirty(WID_VV_START_STOP);
		}
	}

	void OnMouseWheel(int wheel, WidgetID widget) override
	{
		if (widget != WID_VV_VIEWPORT) return;
		if (_settings_client.gui.scrollwheel_scrolling != SWS_OFF) {
			DoZoomInOutWindow(wheel < 0 ? ZOOM_IN : ZOOM_OUT, this);
		}
	}

	void OnResize() override
	{
		if (this->viewport != nullptr) {
			NWidgetViewport *nvp = this->GetWidget<NWidgetViewport>(WID_VV_VIEWPORT);
			nvp->UpdateViewportCoordinates(this);
		}
	}

	void UpdateButtonStatus()
	{
		const Vehicle *v = Vehicle::Get(this->window_number);
		bool veh_stopped = v->IsStoppedInDepot();

		/* Widget WID_VV_GOTO_DEPOT must be hidden if the vehicle is already stopped in depot.
		 * Widget WID_VV_CLONE_VEH should then be shown, since cloning is allowed only while in depot and stopped.
		 */
		PlaneSelections plane = veh_stopped ? SEL_DC_CLONE : SEL_DC_GOTO_DEPOT;
		NWidgetStacked *nwi = this->GetWidget<NWidgetStacked>(WID_VV_SELECT_DEPOT_CLONE); // Selection widget 'send to depot' / 'clone'.
		if (nwi->shown_plane + SEL_DC_BASEPLANE != plane) {
			this->SelectPlane(plane);
			this->SetWidgetDirty(WID_VV_SELECT_DEPOT_CLONE);
		}
		/* The same system applies to widget WID_VV_REFIT_VEH and VVW_WIDGET_TURN_AROUND.*/
		if (v->IsGroundVehicle()) {
			plane = veh_stopped ? SEL_RT_REFIT : SEL_RT_TURN_AROUND;
			nwi = this->GetWidget<NWidgetStacked>(WID_VV_SELECT_REFIT_TURN);
			if (nwi->shown_plane + SEL_RT_BASEPLANE != plane) {
				this->SelectPlane(plane);
				this->SetWidgetDirty(WID_VV_SELECT_REFIT_TURN);
			}
		}
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	void OnInvalidateData([[maybe_unused]] int data = 0, [[maybe_unused]] bool gui_scope = true) override
	{
		if (data == VIWD_AUTOREPLACE) {
			/* Autoreplace replaced the vehicle.
			 * Nothing to do for this window. */
			return;
		}

		this->UpdateButtonStatus();
	}

	bool IsNewGRFInspectable() const override
	{
		return ::IsNewGRFInspectable(GetGrfSpecFeature(Vehicle::Get(this->window_number)->type), this->window_number);
	}

	void ShowNewGRFInspectWindow() const override
	{
		::ShowNewGRFInspectWindow(GetGrfSpecFeature(Vehicle::Get(this->window_number)->type), this->window_number);
	}

	static inline HotkeyList hotkeys{"vehicleview", {
		Hotkey('H', "honk", WID_VV_HONK_HORN),
	}};
};

/** Vehicle view window descriptor for all vehicles but trains. */
static WindowDesc _vehicle_view_desc(
	WDP_AUTO, "view_vehicle", 250, 116,
	WC_VEHICLE_VIEW, WC_NONE,
	{},
	_nested_vehicle_view_widgets,
	&VehicleViewWindow::hotkeys
);

/**
 * Vehicle view window descriptor for trains. Only minimum_height and
 *  default_height are different for train view.
 */
static WindowDesc _train_view_desc(
	WDP_AUTO, "view_vehicle_train", 250, 134,
	WC_VEHICLE_VIEW, WC_NONE,
	{},
	_nested_vehicle_view_widgets,
	&VehicleViewWindow::hotkeys
);

/** Shows the vehicle view window of the given vehicle. */
void ShowVehicleViewWindow(const Vehicle *v)
{
	AllocateWindowDescFront<VehicleViewWindow>((v->type == VEH_TRAIN) ? _train_view_desc : _vehicle_view_desc, v->index);
}

/**
 * Dispatch a "vehicle selected" event if any window waits for it.
 * @param v selected vehicle;
 * @return did any window accept vehicle selection?
 */
bool VehicleClicked(const Vehicle *v)
{
	assert(v != nullptr);
	if (!(_thd.place_mode & HT_VEHICLE)) return false;

	v = v->First();
	if (!v->IsPrimaryVehicle()) return false;

	return _thd.GetCallbackWnd()->OnVehicleSelect(v);
}

/**
 * Dispatch a "vehicle group selected" event if any window waits for it.
 * @param begin iterator to the start of the range of vehicles
 * @param end iterator to the end of the range of vehicles
 * @return did any window accept vehicle group selection?
 */
bool VehicleClicked(VehicleList::const_iterator begin, VehicleList::const_iterator end)
{
	assert(begin != end);
	if (!(_thd.place_mode & HT_VEHICLE)) return false;

	/* If there is only one vehicle in the group, act as if we clicked a single vehicle */
	if (begin + 1 == end) return _thd.GetCallbackWnd()->OnVehicleSelect(*begin);

	return _thd.GetCallbackWnd()->OnVehicleSelect(begin, end);
}

/**
 * Dispatch a "vehicle group selected" event if any window waits for it.
 * @param vehgroup the GUIVehicleGroup representing the vehicle group
 * @return did any window accept vehicle group selection?
 */
bool VehicleClicked(const GUIVehicleGroup &vehgroup)
{
	return VehicleClicked(vehgroup.vehicles_begin, vehgroup.vehicles_end);
}

void StopGlobalFollowVehicle(const Vehicle *v)
{
	Window *w = GetMainWindow();
	if (w->viewport->follow_vehicle == v->index) {
		ScrollMainWindowTo(v->x_pos, v->y_pos, v->z_pos, true); // lock the main view on the vehicle's last position
		w->viewport->CancelFollow(*w);
	}
}


/**
 * This is the Callback method after the construction attempt of a primary vehicle
 * @param result indicates completion (or not) of the operation
 * @param new_veh_id ID of the new vehicle.
 */
void CcBuildPrimaryVehicle(Commands, const CommandCost &result, VehicleID new_veh_id, uint, uint16_t, CargoArray)
{
	if (result.Failed()) return;

	const Vehicle *v = Vehicle::Get(new_veh_id);
	ShowVehicleViewWindow(v);
}

/**
 * Get the width of a vehicle (part) in pixels.
 * @param v Vehicle to get the width for.
 * @return Width of the vehicle.
 */
int GetSingleVehicleWidth(const Vehicle *v, EngineImageType image_type)
{
	switch (v->type) {
		case VEH_TRAIN:
			return Train::From(v)->GetDisplayImageWidth();

		case VEH_ROAD:
			return RoadVehicle::From(v)->GetDisplayImageWidth();

		default:
			bool rtl = _current_text_dir == TD_RTL;
			VehicleSpriteSeq seq;
			v->GetImage(rtl ? DIR_E : DIR_W, image_type, &seq);
			Rect rec;
			seq.GetBounds(&rec);
			return UnScaleGUI(rec.Width());
	}
}

/**
 * Get the width of a vehicle (including all parts of the consist) in pixels.
 * @param v Vehicle to get the width for.
 * @return Width of the vehicle.
 */
int GetVehicleWidth(const Vehicle *v, EngineImageType image_type)
{
	if (v->type == VEH_TRAIN || v->type == VEH_ROAD) {
		int vehicle_width = 0;
		for (const Vehicle *u = v; u != nullptr; u = u->Next()) {
			vehicle_width += GetSingleVehicleWidth(u, image_type);
		}
		return vehicle_width;
	} else {
		return GetSingleVehicleWidth(v, image_type);
	}
}

/**
 * Set the mouse cursor to look like a vehicle.
 * @param v Vehicle
 * @param image_type Type of vehicle image to use.
 */
void SetMouseCursorVehicle(const Vehicle *v, EngineImageType image_type)
{
	bool rtl = _current_text_dir == TD_RTL;

	_cursor.sprites.clear();
	int total_width = 0;
	int y_offset = 0;
	bool rotor_seq = false; // Whether to draw the rotor of the vehicle in this step.
	bool is_ground_vehicle = v->IsGroundVehicle();

	while (v != nullptr) {
		if (total_width >= ScaleSpriteTrad(2 * (int)VEHICLEINFO_FULL_VEHICLE_WIDTH)) break;

		PaletteID pal = v->vehstatus.Test(VehState::Crashed) ? PALETTE_CRASH : GetVehiclePalette(v);
		VehicleSpriteSeq seq;

		if (rotor_seq) {
			GetCustomRotorSprite(Aircraft::From(v), image_type, &seq);
			if (!seq.IsValid()) seq.Set(SPR_ROTOR_STOPPED);
			y_offset = -ScaleSpriteTrad(5);
		} else {
			v->GetImage(rtl ? DIR_E : DIR_W, image_type, &seq);
		}

		int x_offs = 0;
		if (v->type == VEH_TRAIN) x_offs = Train::From(v)->GetCursorImageOffset();

		for (uint i = 0; i < seq.count; ++i) {
			PaletteID pal2 = v->vehstatus.Test(VehState::Crashed) || !seq.seq[i].pal ? pal : seq.seq[i].pal;
			_cursor.sprites.emplace_back(seq.seq[i].sprite, pal2, rtl ? (-total_width + x_offs) : (total_width + x_offs), y_offset);
		}

		if (v->type == VEH_AIRCRAFT && v->subtype == AIR_HELICOPTER && !rotor_seq) {
			/* Draw rotor part in the next step. */
			rotor_seq = true;
		} else {
			total_width += GetSingleVehicleWidth(v, image_type);
			v = v->HasArticulatedPart() ? v->GetNextArticulatedPart() : nullptr;
		}
	}

	if (is_ground_vehicle) {
		/* Center trains and road vehicles on the front vehicle */
		int offs = (ScaleSpriteTrad(VEHICLEINFO_FULL_VEHICLE_WIDTH) - total_width) / 2;
		if (rtl) offs = -offs;
		for (auto &cs : _cursor.sprites) {
			cs.pos.x += offs;
		}
	}

	UpdateCursorSize();
}
