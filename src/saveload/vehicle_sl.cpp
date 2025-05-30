/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file vehicle_sl.cpp Code handling saving and loading of vehicles */

#include "../stdafx.h"

#include "saveload.h"
#include "compat/vehicle_sl_compat.h"

#include "../debug.h"
#include "../vehicle_func.h"
#include "../train.h"
#include "../roadveh.h"
#include "../ship.h"
#include "../aircraft.h"
#include "../timetable.h"
#include "../station_base.h"
#include "../effectvehicle_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../disaster_vehicle.h"
#include "../economy_base.h"

#include "../safeguards.h"

/**
 * Link front and rear multiheaded engines to each other
 * This is done when loading a savegame
 */
void ConnectMultiheadedTrains()
{
	for (Train *v : Train::Iterate()) {
		v->other_multiheaded_part = nullptr;
	}

	for (Train *v : Train::Iterate()) {
		if (v->IsFrontEngine() || v->IsFreeWagon()) {
			/* Two ways to associate multiheaded parts to each other:
			 * sequential-matching: Trains shall be arranged to look like <..>..<..>..<..>..
			 * bracket-matching:    Free vehicle chains shall be arranged to look like ..<..<..>..<..>..>..
			 *
			 * Note: Old savegames might contain chains which do not comply with these rules, e.g.
			 *   - the front and read parts have invalid orders
			 *   - different engine types might be combined
			 *   - there might be different amounts of front and rear parts.
			 *
			 * Note: The multiheaded parts need to be matched exactly like they are matched on the server, else desyncs will occur.
			 *   This is why two matching strategies are needed.
			 */

			bool sequential_matching = v->IsFrontEngine();

			for (Train *u = v; u != nullptr; u = u->GetNextVehicle()) {
				if (u->other_multiheaded_part != nullptr) continue; // we already linked this one

				if (u->IsMultiheaded()) {
					if (!u->IsEngine()) {
						/* we got a rear car without a front car. We will convert it to a front one */
						u->SetEngine();
						u->spritenum--;
					}

					/* Find a matching back part */
					EngineID eid = u->engine_type;
					Train *w;
					if (sequential_matching) {
						for (w = u->GetNextVehicle(); w != nullptr; w = w->GetNextVehicle()) {
							if (w->engine_type != eid || w->other_multiheaded_part != nullptr || !w->IsMultiheaded()) continue;

							/* we found a car to partner with this engine. Now we will make sure it face the right way */
							if (w->IsEngine()) {
								w->ClearEngine();
								w->spritenum++;
							}
							break;
						}
					} else {
						uint stack_pos = 0;
						for (w = u->GetNextVehicle(); w != nullptr; w = w->GetNextVehicle()) {
							if (w->engine_type != eid || w->other_multiheaded_part != nullptr || !w->IsMultiheaded()) continue;

							if (w->IsEngine()) {
								stack_pos++;
							} else {
								if (stack_pos == 0) break;
								stack_pos--;
							}
						}
					}

					if (w != nullptr) {
						w->other_multiheaded_part = u;
						u->other_multiheaded_part = w;
					} else {
						/* we got a front car and no rear cars. We will fake this one for forget that it should have been multiheaded */
						u->ClearMultiheaded();
					}
				}
			}
		}
	}
}

/**
 *  Converts all trains to the new subtype format introduced in savegame 16.2
 *  It also links multiheaded engines or make them forget they are multiheaded if no suitable partner is found
 */
void ConvertOldMultiheadToNew()
{
	for (Train *t : Train::Iterate()) SetBit(t->subtype, 7); // indicates that it's the old format and needs to be converted in the next loop

	for (Train *t : Train::Iterate()) {
		if (HasBit(t->subtype, 7) && ((t->subtype & ~0x80) == 0 || (t->subtype & ~0x80) == 4)) {
			for (Train *u = t; u != nullptr; u = u->Next()) {
				const RailVehicleInfo *rvi = RailVehInfo(u->engine_type);

				ClrBit(u->subtype, 7);
				switch (u->subtype) {
					case 0: // TS_Front_Engine
						if (rvi->railveh_type == RAILVEH_MULTIHEAD) u->SetMultiheaded();
						u->SetFrontEngine();
						u->SetEngine();
						break;

					case 1: // TS_Artic_Part
						u->subtype = 0;
						u->SetArticulatedPart();
						break;

					case 2: // TS_Not_First
						u->subtype = 0;
						if (rvi->railveh_type == RAILVEH_WAGON) {
							/* normal wagon */
							u->SetWagon();
							break;
						}
						if (rvi->railveh_type == RAILVEH_MULTIHEAD && rvi->image_index == u->spritenum - 1) {
							/* rear end of a multiheaded engine */
							u->SetMultiheaded();
							break;
						}
						if (rvi->railveh_type == RAILVEH_MULTIHEAD) u->SetMultiheaded();
						u->SetEngine();
						break;

					case 4: // TS_Free_Car
						u->subtype = 0;
						u->SetWagon();
						u->SetFreeWagon();
						break;
					default: SlErrorCorrupt("Invalid train subtype");
				}
			}
		}
	}
}


/** need to be called to load aircraft from old version */
void UpdateOldAircraft()
{
	/* set airport_flags to 0 for all airports just to be sure */
	for (Station *st : Station::Iterate()) {
		st->airport.blocks = {}; // reset airport
	}

	for (Aircraft *a : Aircraft::Iterate()) {
		/* airplane has another vehicle with subtype 4 (shadow), helicopter also has 3 (rotor)
		 * skip those */
		if (a->IsNormalAircraft()) {
			/* airplane in terminal stopped doesn't hurt anyone, so goto next */
			if (a->vehstatus.Test(VehState::Stopped) && a->state == 0) {
				a->state = HANGAR;
				continue;
			}

			AircraftLeaveHangar(a, a->direction); // make airplane visible if it was in a depot for example
			a->vehstatus.Reset(VehState::Stopped); // make airplane moving
			UpdateAircraftCache(a);
			a->cur_speed = a->vcache.cached_max_speed; // so aircraft don't have zero speed while in air
			if (!a->current_order.IsType(OT_GOTO_STATION) && !a->current_order.IsType(OT_GOTO_DEPOT)) {
				/* reset current order so aircraft doesn't have invalid "station-only" order */
				a->current_order.MakeDummy();
			}
			a->state = FLYING;
			AircraftNextAirportPos_and_Order(a); // move it to the entry point of the airport
			GetNewVehiclePosResult gp = GetNewVehiclePos(a);
			a->tile = TileIndex{}; // aircraft in air is tile=0

			/* correct speed of helicopter-rotors */
			if (a->subtype == AIR_HELICOPTER) a->Next()->Next()->cur_speed = 32;

			/* set new position x,y,z */
			GetAircraftFlightLevelBounds(a, &a->z_pos, nullptr);
			SetAircraftPosition(a, gp.x, gp.y, GetAircraftFlightLevel(a));
		}
	}

	/* Clear aircraft from loading vehicles, if we bumped them into the air. */
	for (Station *st : Station::Iterate()) {
		for (auto iter = st->loading_vehicles.begin(); iter != st->loading_vehicles.end(); /* nothing */) {
			Vehicle *v = *iter;
			if (v->type == VEH_AIRCRAFT && !v->current_order.IsType(OT_LOADING)) {
				iter = st->loading_vehicles.erase(iter);
				delete v->cargo_payment;
			} else {
				++iter;
			}
		}
	}
}

/**
 * Check all vehicles to ensure their engine type is valid
 * for the currently loaded NewGRFs (that includes none...)
 * This only makes a difference if NewGRFs are missing, otherwise
 * all vehicles will be valid. This does not make such a game
 * playable, it only prevents crash.
 */
static void CheckValidVehicles()
{
	size_t total_engines = Engine::GetPoolSize();
	EngineID first_engine[4] = { EngineID::Invalid(), EngineID::Invalid(), EngineID::Invalid(), EngineID::Invalid() };

	for (const Engine *e : Engine::IterateType(VEH_TRAIN)) { first_engine[VEH_TRAIN] = e->index; break; }
	for (const Engine *e : Engine::IterateType(VEH_ROAD)) { first_engine[VEH_ROAD] = e->index; break; }
	for (const Engine *e : Engine::IterateType(VEH_SHIP)) { first_engine[VEH_SHIP] = e->index; break; }
	for (const Engine *e : Engine::IterateType(VEH_AIRCRAFT)) { first_engine[VEH_AIRCRAFT] = e->index; break; }

	for (Vehicle *v : Vehicle::Iterate()) {
		/* Test if engine types match */
		switch (v->type) {
			case VEH_TRAIN:
			case VEH_ROAD:
			case VEH_SHIP:
			case VEH_AIRCRAFT:
				if (v->engine_type >= total_engines || v->type != v->GetEngine()->type) {
					v->engine_type = first_engine[v->type];
				}
				break;

			default:
				break;
		}
	}
}

extern uint8_t _age_cargo_skip_counter; // From misc_sl.cpp

/** Called after load for phase 1 of vehicle initialisation */
void AfterLoadVehiclesPhase1(bool part_of_load)
{
	for (Vehicle *v : Vehicle::Iterate()) {
		/* Reinstate the previous pointer */
		if (v->Next() != nullptr) v->Next()->previous = v;
		if (v->NextShared() != nullptr) v->NextShared()->previous_shared = v;

		if (part_of_load) v->fill_percent_te_id = INVALID_TE_ID;
		v->first = nullptr;
		if (v->IsGroundVehicle()) v->GetGroundVehicleCache()->first_engine = EngineID::Invalid();
	}

	/* AfterLoadVehicles may also be called in case of NewGRF reload, in this
	 * case we may not convert orders again. */
	if (part_of_load) {
		/* Create shared vehicle chain for very old games (pre 5,2) and create
		 * OrderList from shared vehicle chains. For this to work correctly, the
		 * following conditions must be fulfilled:
		 * a) both next_shared and previous_shared are not set for pre 5,2 games
		 * b) both next_shared and previous_shared are set for later games
		 */
		std::map<uint32_t, OrderList *> mapping;

		for (Vehicle *v : Vehicle::Iterate()) {
			if (v->orders != nullptr) {
				if (IsSavegameVersionBefore(SLV_105)) { // Pre-105 didn't save an OrderList
					if (mapping[v->old_orders] == nullptr) {
						/* This adds the whole shared vehicle chain for case b */

						/* Creating an OrderList here is safe because the number of vehicles
						 * allowed in these savegames matches the number of OrderLists. As
						 * such each vehicle can get an OrderList and it will (still) fit. */
						assert(OrderList::CanAllocateItem());
						std::vector<Order> orders;
						for (const OldOrderSaveLoadItem *old_order = GetOldOrder(v->old_orders); old_order != nullptr; old_order = GetOldOrder(old_order->next)) {
							orders.push_back(std::move(old_order->order));
						}
						v->orders = mapping[v->old_orders] = new OrderList(std::move(orders), v);
					} else {
						v->orders = mapping[v->old_orders];
						/* For old games (case a) we must create the shared vehicle chain */
						if (IsSavegameVersionBefore(SLV_5, 2)) {
							v->AddToShared(v->orders->GetFirstSharedVehicle());
						}
					}
				} else { // OrderList was saved as such, only recalculate not saved values
					if (v->PreviousShared() == nullptr) {
						v->orders->Initialize(v);
					}
				}
			}
		}
	}

	for (Vehicle *v : Vehicle::Iterate()) {
		/* Fill the first pointers */
		if (v->Previous() == nullptr) {
			for (Vehicle *u = v; u != nullptr; u = u->Next()) {
				u->first = v;
			}
		}
	}

	if (part_of_load) {
		if (IsSavegameVersionBefore(SLV_105)) {
			/* Before 105 there was no order for shared orders, thus it messed up horribly */
			for (Vehicle *v : Vehicle::Iterate()) {
				if (v->First() != v || v->orders != nullptr || v->previous_shared != nullptr || v->next_shared == nullptr) continue;

				/* As above, allocating OrderList here is safe. */
				assert(OrderList::CanAllocateItem());
				v->orders = new OrderList();
				v->orders->first_shared = v;
				for (Vehicle *u = v; u != nullptr; u = u->next_shared) {
					u->orders = v->orders;
				}
			}
		}

		if (IsSavegameVersionBefore(SLV_157)) {
			/* The road vehicle subtype was converted to a flag. */
			for (RoadVehicle *rv : RoadVehicle::Iterate()) {
				if (rv->subtype == 0) {
					/* The road vehicle is at the front. */
					rv->SetFrontEngine();
				} else if (rv->subtype == 1) {
					/* The road vehicle is an articulated part. */
					rv->subtype = 0;
					rv->SetArticulatedPart();
				} else {
					SlErrorCorrupt("Invalid road vehicle subtype");
				}
			}
		}

		if (IsSavegameVersionBefore(SLV_160)) {
			/* In some old savegames there might be some "crap" stored. */
			for (Vehicle *v : Vehicle::Iterate()) {
				if (!v->IsPrimaryVehicle() && v->type != VEH_DISASTER) {
					v->current_order.Free();
					v->unitnumber = 0;
				}
			}
		}

		if (IsSavegameVersionBefore(SLV_162)) {
			/* Set the vehicle-local cargo age counter from the old global counter. */
			for (Vehicle *v : Vehicle::Iterate()) {
				v->cargo_age_counter = _age_cargo_skip_counter;
			}
		}

		if (IsSavegameVersionBefore(SLV_180)) {
			/* Set service interval flags */
			for (Vehicle *v : Vehicle::Iterate()) {
				if (!v->IsPrimaryVehicle()) continue;

				const Company *c = Company::Get(v->owner);
				int interval = CompanyServiceInterval(c, v->type);

				v->SetServiceIntervalIsCustom(v->GetServiceInterval() != interval);
				v->SetServiceIntervalIsPercent(c->settings.vehicle.servint_ispercent);
			}
		}

		if (IsSavegameVersionBefore(SLV_SHIP_ROTATION)) {
			/* Ship rotation added */
			for (Ship *s : Ship::Iterate()) {
				s->rotation = s->direction;
			}
		} else {
			for (Ship *s : Ship::Iterate()) {
				if (s->rotation == s->direction) continue;
				/* In case we are rotating on gameload, set the rotation position to
				 * the current position, otherwise the applied workaround offset would
				 * be with respect to 0,0.
				 */
				s->rotation_x_pos = s->x_pos;
				s->rotation_y_pos = s->y_pos;
			}
		}

		if (IsSavegameVersionBefore(SLV_TIMETABLE_START_TICKS)) {
			/* Convert timetable start from a date to an absolute tick in TimerGameTick::counter. */
			for (Vehicle *v : Vehicle::Iterate()) {
				/* If the start date is 0, the vehicle is not waiting to start and can be ignored. */
				if (v->timetable_start == 0) continue;

				v->timetable_start = GetStartTickFromDate(TimerGameEconomy::Date(v->timetable_start));
			}
		}

		if (IsSavegameVersionBefore(SLV_VEHICLE_ECONOMY_AGE)) {
			/* Set vehicle economy age based on calendar age. */
			for (Vehicle *v : Vehicle::Iterate()) {
				v->economy_age = TimerGameEconomy::Date{v->age.base()};
			}
		}
	}

	CheckValidVehicles();
}

/** Called after load for phase 2 of vehicle initialisation */
void AfterLoadVehiclesPhase2(bool part_of_load)
{
	for (Vehicle *v : Vehicle::Iterate()) {
		assert(v->First() != nullptr);

		v->trip_occupancy = CalcPercentVehicleFilled(v, nullptr);

		switch (v->type) {
			case VEH_TRAIN: {
				Train *t = Train::From(v);
				if (t->IsFrontEngine() || t->IsFreeWagon()) {
					t->gcache.last_speed = t->cur_speed; // update displayed train speed
					t->ConsistChanged(CCF_SAVELOAD);
				}
				break;
			}

			case VEH_ROAD: {
				RoadVehicle *rv = RoadVehicle::From(v);
				if (rv->IsFrontEngine()) {
					rv->gcache.last_speed = rv->cur_speed; // update displayed road vehicle speed

					rv->roadtype = Engine::Get(rv->engine_type)->u.road.roadtype;
					rv->compatible_roadtypes = GetRoadTypeInfo(rv->roadtype)->powered_roadtypes;
					RoadTramType rtt = GetRoadTramType(rv->roadtype);
					for (RoadVehicle *u = rv; u != nullptr; u = u->Next()) {
						u->roadtype = rv->roadtype;
						u->compatible_roadtypes = rv->compatible_roadtypes;
						if (GetRoadType(u->tile, rtt) == INVALID_ROADTYPE) SlErrorCorrupt("Road vehicle on invalid road type");
					}

					RoadVehUpdateCache(rv);
					if (_settings_game.vehicle.roadveh_acceleration_model != AM_ORIGINAL) {
						rv->CargoChanged();
					}
				}
				break;
			}

			case VEH_SHIP:
				Ship::From(v)->UpdateCache();
				break;

			default: break;
		}
	}

	/* Stop non-front engines */
	if (part_of_load && IsSavegameVersionBefore(SLV_112)) {
		for (Vehicle *v : Vehicle::Iterate()) {
			if (v->type == VEH_TRAIN) {
				Train *t = Train::From(v);
				if (!t->IsFrontEngine()) {
					if (t->IsEngine()) t->vehstatus.Set(VehState::Stopped);
					/* cur_speed is now relevant for non-front parts - nonzero breaks
					 * moving-wagons-inside-depot- and autoreplace- code */
					t->cur_speed = 0;
				}
			}
			/* trains weren't stopping gradually in old OTTD versions (and TTO/TTD)
			 * other vehicle types didn't have zero speed while stopped (even in 'recent' OTTD versions) */
			if (v->vehstatus.Test(VehState::Stopped) && (v->type != VEH_TRAIN || IsSavegameVersionBefore(SLV_2, 1))) {
				v->cur_speed = 0;
			}
		}
	}

	for (Vehicle *v : Vehicle::Iterate()) {
		switch (v->type) {
			case VEH_ROAD:
			case VEH_TRAIN:
			case VEH_SHIP:
				v->GetImage(v->direction, EIT_ON_MAP, &v->sprite_cache.sprite_seq);
				break;

			case VEH_AIRCRAFT:
				if (Aircraft::From(v)->IsNormalAircraft()) {
					v->GetImage(v->direction, EIT_ON_MAP, &v->sprite_cache.sprite_seq);

					/* The aircraft's shadow will have the same image as the aircraft, but no colour */
					Vehicle *shadow = v->Next();
					if (shadow == nullptr) SlErrorCorrupt("Missing shadow for aircraft");

					shadow->sprite_cache.sprite_seq.CopyWithoutPalette(v->sprite_cache.sprite_seq);

					/* In the case of a helicopter we will update the rotor sprites */
					if (v->subtype == AIR_HELICOPTER) {
						Vehicle *rotor = shadow->Next();
						if (rotor == nullptr) SlErrorCorrupt("Missing rotor for helicopter");

						GetRotorImage(Aircraft::From(v), EIT_ON_MAP, &rotor->sprite_cache.sprite_seq);
					}

					UpdateAircraftCache(Aircraft::From(v), true);
				}
				break;

			case VEH_DISASTER: {
				auto *dv = DisasterVehicle::From(v);
				if (dv->subtype == ST_SMALL_UFO && dv->state != 0) {
					RoadVehicle *u = RoadVehicle::GetIfValid(v->dest_tile.base());
					if (u != nullptr && u->IsFrontEngine()) {
						/* Delete UFO targetting a vehicle which is already a target. */
						if (u->disaster_vehicle != VehicleID::Invalid() && u->disaster_vehicle != dv->index) {
							delete v;
							continue;
						} else {
							u->disaster_vehicle = dv->index;
						}
					}
				}
				break;
			}

			default: break;
		}

		if (part_of_load && v->unitnumber != 0) {
			Company::Get(v->owner)->freeunits[v->type].UseID(v->unitnumber);
		}

		v->UpdateDeltaXY();
		v->coord.left = INVALID_COORD;
		v->sprite_cache.old_coord.left = INVALID_COORD;
		if (v->type != VEH_EFFECT) v->UpdatePosition();
		v->UpdateViewport(false);
	}
}

bool TrainController(Train *v, Vehicle *nomove, bool reverse = true); // From train_cmd.cpp
void ReverseTrainDirection(Train *v);
void ReverseTrainSwapVeh(Train *v, int l, int r);

/** Fixup old train spacing. */
void FixupTrainLengths()
{
	/* Vehicle center was moved from 4 units behind the front to half the length
	 * behind the front. Move vehicles so they end up on the same spot. */
	for (Vehicle *v : Vehicle::Iterate()) {
		if (v->type == VEH_TRAIN && v->IsPrimaryVehicle()) {
			/* The vehicle center is now more to the front depending on vehicle length,
			 * so we need to move all vehicles forward to cover the difference to the
			 * old center, otherwise wagon spacing in trains would be broken upon load. */
			for (Train *u = Train::From(v); u != nullptr; u = u->Next()) {
				if (u->track == TRACK_BIT_DEPOT || u->vehstatus.Test(VehState::Crashed)) continue;

				Train *next = u->Next();

				/* Try to pull the vehicle half its length forward. */
				int diff = (VEHICLE_LENGTH - u->gcache.cached_veh_length) / 2;
				int done;
				for (done = 0; done < diff; done++) {
					if (!TrainController(u, next, false)) break;
				}

				if (next != nullptr && done < diff && u->IsFrontEngine()) {
					/* Pulling the front vehicle forwards failed, we either encountered a dead-end
					 * or a red signal. To fix this, we try to move the whole train the required
					 * space backwards and re-do the fix up of the front vehicle. */

					/* Ignore any signals when backtracking. */
					TrainForceProceeding old_tfp = u->force_proceed;
					u->force_proceed = TFP_SIGNAL;

					/* Swap start<>end, start+1<>end-1, ... */
					int r = CountVehiclesInChain(u) - 1; // number of vehicles - 1
					int l = 0;
					do ReverseTrainSwapVeh(u, l++, r--); while (l <= r);

					/* We moved the first vehicle which is now the last. Move it back to the
					 * original position as we will fix up the last vehicle later in the loop. */
					for (int i = 0; i < done; i++) TrainController(u->Last(), nullptr);

					/* Move the train backwards to get space for the first vehicle. As the stopping
					 * distance from a line end is rounded up, move the train one unit more to cater
					 * for front vehicles with odd lengths. */
					int moved;
					for (moved = 0; moved < diff + 1; moved++) {
						if (!TrainController(u, nullptr, false)) break;
					}

					/* Swap start<>end, start+1<>end-1, ... again. */
					r = CountVehiclesInChain(u) - 1; // number of vehicles - 1
					l = 0;
					do ReverseTrainSwapVeh(u, l++, r--); while (l <= r);

					u->force_proceed = old_tfp;

					/* Tracks are too short to fix the train length. The player has to fix the
					 * train in a depot. Bail out so we don't damage the vehicle chain any more. */
					if (moved < diff + 1) break;

					/* Re-do the correction for the first vehicle. */
					for (done = 0; done < diff; done++) TrainController(u, next, false);

					/* We moved one unit more backwards than needed for even-length front vehicles,
					 * try to move that unit forward again. We don't care if this step fails. */
					TrainController(u, nullptr, false);
				}

				/* If the next wagon is still in a depot, check if it shouldn't be outside already. */
				if (next != nullptr && next->track == TRACK_BIT_DEPOT) {
					int d = TicksToLeaveDepot(u);
					if (d <= 0) {
						/* Next vehicle should have left the depot already, show it and pull forward. */
						next->vehstatus.Reset(VehState::Hidden);
						next->track = TrackToTrackBits(GetRailDepotTrack(next->tile));
						for (int i = 0; i >= d; i--) TrainController(next, nullptr);
					}
				}
			}

			/* Update all cached properties after moving the vehicle chain around. */
			Train::From(v)->ConsistChanged(CCF_TRACK);
		}
	}
}

static uint8_t  _cargo_periods;
static StationID _cargo_source;
static TileIndex _cargo_source_xy;
static uint16_t _cargo_count;
static uint16_t _cargo_paid_for;
static Money  _cargo_feeder_share;

class SlVehicleCommon : public DefaultSaveLoadHandler<SlVehicleCommon, Vehicle> {
public:
	static inline const SaveLoad description[] = {
		    SLE_VAR(Vehicle, subtype,               SLE_UINT8),

		    SLE_REF(Vehicle, next,                  REF_VEHICLE_OLD),
		SLE_CONDVAR(Vehicle, name,                  SLE_NAME,                     SL_MIN_VERSION,  SLV_84),
		SLE_CONDSSTR(Vehicle, name,                  SLE_STR | SLF_ALLOW_CONTROL,  SLV_84, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, unitnumber,            SLE_FILE_U8  | SLE_VAR_U16,   SL_MIN_VERSION,   SLV_8),
		SLE_CONDVAR(Vehicle, unitnumber,            SLE_UINT16,                   SLV_8, SL_MAX_VERSION),
		    SLE_VAR(Vehicle, owner,                 SLE_UINT8),
		SLE_CONDVAR(Vehicle, tile,                  SLE_FILE_U16 | SLE_VAR_U32,   SL_MIN_VERSION,   SLV_6),
		SLE_CONDVAR(Vehicle, tile,                  SLE_UINT32,                   SLV_6, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, dest_tile,             SLE_FILE_U16 | SLE_VAR_U32,   SL_MIN_VERSION,   SLV_6),
		SLE_CONDVAR(Vehicle, dest_tile,             SLE_UINT32,                   SLV_6, SL_MAX_VERSION),

		SLE_CONDVAR(Vehicle, x_pos,                 SLE_FILE_U16 | SLE_VAR_U32,   SL_MIN_VERSION,   SLV_6),
		SLE_CONDVAR(Vehicle, x_pos,                 SLE_UINT32,                   SLV_6, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, y_pos,                 SLE_FILE_U16 | SLE_VAR_U32,   SL_MIN_VERSION,   SLV_6),
		SLE_CONDVAR(Vehicle, y_pos,                 SLE_UINT32,                   SLV_6, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, z_pos,                 SLE_FILE_U8  | SLE_VAR_I32,   SL_MIN_VERSION, SLV_164),
		SLE_CONDVAR(Vehicle, z_pos,                 SLE_INT32,                  SLV_164, SL_MAX_VERSION),
		    SLE_VAR(Vehicle, direction,             SLE_UINT8),

		    SLE_VAR(Vehicle, spritenum,             SLE_UINT8),
		    SLE_VAR(Vehicle, engine_type,           SLE_UINT16),
		    SLE_VAR(Vehicle, cur_speed,             SLE_UINT16),
		    SLE_VAR(Vehicle, subspeed,              SLE_UINT8),
		    SLE_VAR(Vehicle, acceleration,          SLE_UINT8),
		SLE_CONDVAR(Vehicle, motion_counter,        SLE_UINT32,                   SLV_VEH_MOTION_COUNTER, SL_MAX_VERSION),
		    SLE_VAR(Vehicle, progress,              SLE_UINT8),

		    SLE_VAR(Vehicle, vehstatus,             SLE_UINT8),
		SLE_CONDVAR(Vehicle, last_station_visited,  SLE_FILE_U8  | SLE_VAR_U16,   SL_MIN_VERSION,   SLV_5),
		SLE_CONDVAR(Vehicle, last_station_visited,  SLE_UINT16,                   SLV_5, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, last_loading_station,  SLE_UINT16,                 SLV_182, SL_MAX_VERSION),

		    SLE_VAR(Vehicle, cargo_type,            SLE_UINT8),
		SLE_CONDVAR(Vehicle, cargo_subtype,         SLE_UINT8,                   SLV_35, SL_MAX_VERSION),
		SLEG_CONDVAR("cargo_days", _cargo_periods,  SLE_UINT8,                    SL_MIN_VERSION,  SLV_68),
		SLEG_CONDVAR("cargo_source", _cargo_source, SLE_FILE_U8  | SLE_VAR_U16,   SL_MIN_VERSION,   SLV_7),
		SLEG_CONDVAR("cargo_source", _cargo_source, SLE_UINT16,                   SLV_7,  SLV_68),
		SLEG_CONDVAR("cargo_source_xy", _cargo_source_xy, SLE_UINT32,             SLV_44,  SLV_68),
		    SLE_VAR(Vehicle, cargo_cap,             SLE_UINT16),
		SLE_CONDVAR(Vehicle, refit_cap,             SLE_UINT16,                 SLV_182, SL_MAX_VERSION),
		SLEG_CONDVAR("cargo_count", _cargo_count,   SLE_UINT16,                   SL_MIN_VERSION,  SLV_68),
		SLE_CONDREFLIST(Vehicle, cargo.packets,     REF_CARGO_PACKET,            SLV_68, SL_MAX_VERSION),
		SLE_CONDARR(Vehicle, cargo.action_counts,   SLE_UINT, VehicleCargoList::NUM_MOVE_TO_ACTION, SLV_181, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, cargo_age_counter,     SLE_UINT16,                 SLV_162, SL_MAX_VERSION),

		    SLE_VAR(Vehicle, day_counter,           SLE_UINT8),
		    SLE_VAR(Vehicle, tick_counter,          SLE_UINT8),
		SLE_CONDVAR(Vehicle, running_ticks,         SLE_UINT8,                   SLV_88, SL_MAX_VERSION),

		    SLE_VAR(Vehicle, cur_implicit_order_index,  SLE_UINT8),
		SLE_CONDVAR(Vehicle, cur_real_order_index,  SLE_UINT8,                  SLV_158, SL_MAX_VERSION),

		/* This next line is for version 4 and prior compatibility.. it temporarily reads
		type and flags (which were both 4 bits) into type. Later on this is
		converted correctly */
		SLE_CONDVAR(Vehicle, current_order.type,    SLE_UINT8,                    SL_MIN_VERSION,   SLV_5),
		SLE_CONDVAR(Vehicle, current_order.dest,    SLE_FILE_U8  | SLE_VAR_U16,   SL_MIN_VERSION,   SLV_5),

		/* Orders for version 5 and on */
		SLE_CONDVAR(Vehicle, current_order.type,    SLE_UINT8,                    SLV_5, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, current_order.flags,   SLE_UINT8,                    SLV_5, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, current_order.dest,    SLE_UINT16,                   SLV_5, SL_MAX_VERSION),

		/* Refit in current order */
		SLE_CONDVAR(Vehicle, current_order.refit_cargo,   SLE_UINT8,             SLV_36, SL_MAX_VERSION),

		/* Timetable in current order */
		SLE_CONDVAR(Vehicle, current_order.wait_time,     SLE_UINT16,            SLV_67, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, current_order.travel_time,   SLE_UINT16,            SLV_67, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, current_order.max_speed,     SLE_UINT16,           SLV_174, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, timetable_start,       SLE_FILE_I32 | SLE_VAR_U64, SLV_129, SLV_TIMETABLE_START_TICKS),
		SLE_CONDVAR(Vehicle, timetable_start,       SLE_UINT64,                 SLV_TIMETABLE_START_TICKS, SL_MAX_VERSION),

		SLE_CONDVARNAME(Vehicle, old_orders, "orders", SLE_FILE_U16 | SLE_VAR_U32, SL_MIN_VERSION, SLV_69),
		SLE_CONDVARNAME(Vehicle, old_orders, "orders", SLE_UINT32,                 SLV_69, SLV_105),
		SLE_CONDREF(Vehicle, orders,                REF_ORDERLIST,              SLV_105, SL_MAX_VERSION),

		SLE_CONDVAR(Vehicle, age,                   SLE_FILE_U16 | SLE_VAR_I32,   SL_MIN_VERSION,  SLV_31),
		SLE_CONDVAR(Vehicle, age,                   SLE_INT32,                   SLV_31, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, economy_age,           SLE_INT32,  SLV_VEHICLE_ECONOMY_AGE, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, max_age,               SLE_FILE_U16 | SLE_VAR_I32,   SL_MIN_VERSION,  SLV_31),
		SLE_CONDVAR(Vehicle, max_age,               SLE_INT32,                   SLV_31, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, date_of_last_service,  SLE_FILE_U16 | SLE_VAR_I32,   SL_MIN_VERSION,  SLV_31),
		SLE_CONDVAR(Vehicle, date_of_last_service,  SLE_INT32,                   SLV_31, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, date_of_last_service_newgrf, SLE_INT32,             SLV_NEWGRF_LAST_SERVICE, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, service_interval,      SLE_UINT16,                   SL_MIN_VERSION,  SLV_31),
		SLE_CONDVAR(Vehicle, service_interval,      SLE_FILE_U32 | SLE_VAR_U16,  SLV_31, SLV_180),
		SLE_CONDVAR(Vehicle, service_interval,      SLE_UINT16,                 SLV_180, SL_MAX_VERSION),
		    SLE_VAR(Vehicle, reliability,           SLE_UINT16),
		    SLE_VAR(Vehicle, reliability_spd_dec,   SLE_UINT16),
		    SLE_VAR(Vehicle, breakdown_ctr,         SLE_UINT8),
		    SLE_VAR(Vehicle, breakdown_delay,       SLE_UINT8),
		    SLE_VAR(Vehicle, breakdowns_since_last_service, SLE_UINT8),
		    SLE_VAR(Vehicle, breakdown_chance,      SLE_UINT8),
		SLE_CONDVAR(Vehicle, build_year,            SLE_FILE_U8 | SLE_VAR_I32,    SL_MIN_VERSION,  SLV_31),
		SLE_CONDVAR(Vehicle, build_year,            SLE_INT32,                   SLV_31, SL_MAX_VERSION),

		    SLE_VAR(Vehicle, load_unload_ticks,     SLE_UINT16),
		SLEG_CONDVAR("cargo_paid_for", _cargo_paid_for, SLE_UINT16,              SLV_45, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, vehicle_flags,         SLE_FILE_U8 | SLE_VAR_U16,   SLV_40, SLV_180),
		SLE_CONDVAR(Vehicle, vehicle_flags,         SLE_UINT16,                 SLV_180, SL_MAX_VERSION),

		SLE_CONDVAR(Vehicle, profit_this_year,      SLE_FILE_I32 | SLE_VAR_I64,   SL_MIN_VERSION,  SLV_65),
		SLE_CONDVAR(Vehicle, profit_this_year,      SLE_INT64,                   SLV_65, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, profit_last_year,      SLE_FILE_I32 | SLE_VAR_I64,   SL_MIN_VERSION,  SLV_65),
		SLE_CONDVAR(Vehicle, profit_last_year,      SLE_INT64,                   SLV_65, SL_MAX_VERSION),
		SLEG_CONDVAR("cargo_feeder_share", _cargo_feeder_share, SLE_FILE_I32 | SLE_VAR_I64,  SLV_51,  SLV_65),
		SLEG_CONDVAR("cargo_feeder_share", _cargo_feeder_share, SLE_INT64,                   SLV_65,  SLV_68),
		SLE_CONDVAR(Vehicle, value,                 SLE_FILE_I32 | SLE_VAR_I64,   SL_MIN_VERSION,  SLV_65),
		SLE_CONDVAR(Vehicle, value,                 SLE_INT64,                   SLV_65, SL_MAX_VERSION),

		SLE_CONDVAR(Vehicle, random_bits,           SLE_FILE_U8 | SLE_VAR_U16,    SLV_2, SLV_EXTEND_VEHICLE_RANDOM),
		SLE_CONDVAR(Vehicle, random_bits,           SLE_UINT16,                   SLV_EXTEND_VEHICLE_RANDOM, SL_MAX_VERSION),
		SLE_CONDVARNAME(Vehicle, waiting_random_triggers, "waiting_triggers", SLE_UINT8, SLV_2, SL_MAX_VERSION),

		SLE_CONDREF(Vehicle, next_shared,           REF_VEHICLE,                  SLV_2, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, group_id,              SLE_UINT16,                  SLV_60, SL_MAX_VERSION),

		SLE_CONDVAR(Vehicle, current_order_time,    SLE_FILE_U32 | SLE_VAR_I32,  SLV_67, SLV_TIMETABLE_TICKS_TYPE),
		SLE_CONDVAR(Vehicle, current_order_time,    SLE_INT32,                   SLV_TIMETABLE_TICKS_TYPE, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, last_loading_tick,     SLE_UINT64,                   SLV_LAST_LOADING_TICK, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, lateness_counter,      SLE_INT32,                   SLV_67, SL_MAX_VERSION),

		SLE_CONDVAR(Vehicle, depot_unbunching_last_departure, SLE_UINT64,        SLV_DEPOT_UNBUNCHING, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, depot_unbunching_next_departure, SLE_UINT64,        SLV_DEPOT_UNBUNCHING, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, round_trip_time,       SLE_INT32,                   SLV_DEPOT_UNBUNCHING, SL_MAX_VERSION),
	};

	static inline const SaveLoadCompatTable compat_description = _vehicle_common_sl_compat;

	void Save(Vehicle *v) const override
	{
		SlObject(v, this->GetDescription());
	}

	void Load(Vehicle *v) const override
	{
		SlObject(v, this->GetLoadDescription());
	}

	void FixPointers(Vehicle *v) const override
	{
		SlObject(v, this->GetDescription());
	}
};

class SlVehicleTrain : public DefaultSaveLoadHandler<SlVehicleTrain, Vehicle> {
public:
	static inline const SaveLoad description[] = {
		 SLEG_STRUCT("common", SlVehicleCommon),
		     SLE_VAR(Train, crash_anim_pos,      SLE_UINT16),
		     SLE_VAR(Train, force_proceed,       SLE_UINT8),
		     SLE_VAR(Train, railtype,            SLE_UINT8),
		     SLE_VAR(Train, track,               SLE_UINT8),

		 SLE_CONDVAR(Train, flags,               SLE_FILE_U8  | SLE_VAR_U16,   SLV_2,  SLV_100),
		 SLE_CONDVAR(Train, flags,               SLE_UINT16,                 SLV_100, SL_MAX_VERSION),
		 SLE_CONDVAR(Train, wait_counter,        SLE_UINT16,                 SLV_136, SL_MAX_VERSION),
		 SLE_CONDVAR(Train, gv_flags,            SLE_UINT16,                 SLV_139, SL_MAX_VERSION),
	};
	static inline const SaveLoadCompatTable compat_description = _vehicle_train_sl_compat;

	void Save(Vehicle *v) const override
	{
		if (v->type != VEH_TRAIN) return;
		SlObject(v, this->GetDescription());
	}

	void Load(Vehicle *v) const override
	{
		if (v->type != VEH_TRAIN) return;
		SlObject(v, this->GetLoadDescription());
	}

	void FixPointers(Vehicle *v) const override
	{
		if (v->type != VEH_TRAIN) return;
		SlObject(v, this->GetDescription());
	}
};

class SlVehicleRoadVehPath : public VectorSaveLoadHandler<SlVehicleRoadVehPath, RoadVehicle, RoadVehPathElement> {
public:
	static inline const SaveLoad description[] = {
		SLE_VAR(RoadVehPathElement, trackdir, SLE_UINT8),
		SLE_VAR(RoadVehPathElement, tile, SLE_UINT32),
	};
	static inline const SaveLoadCompatTable compat_description = {};

	std::vector<RoadVehPathElement> &GetVector(RoadVehicle *rv) const override { return rv->path; }
};

class SlVehicleRoadVeh : public DefaultSaveLoadHandler<SlVehicleRoadVeh, Vehicle> {
public:
	/* RoadVehicle path is stored in std::pair which cannot be directly saved. */
	static inline std::vector<Trackdir> rv_path_td;
	static inline std::vector<TileIndex> rv_path_tile;

	static inline const SaveLoad description[] = {
		  SLEG_STRUCT("common", SlVehicleCommon),
		      SLE_VAR(RoadVehicle, state,                SLE_UINT8),
		      SLE_VAR(RoadVehicle, frame,                SLE_UINT8),
		      SLE_VAR(RoadVehicle, blocked_ctr,          SLE_UINT16),
		      SLE_VAR(RoadVehicle, overtaking,           SLE_UINT8),
		      SLE_VAR(RoadVehicle, overtaking_ctr,       SLE_UINT8),
		      SLE_VAR(RoadVehicle, crashed_ctr,          SLE_UINT16),
		      SLE_VAR(RoadVehicle, reverse_ctr,          SLE_UINT8),
		SLEG_CONDVECTOR("path.td",   rv_path_td,         SLE_UINT8,                  SLV_ROADVEH_PATH_CACHE, SLV_PATH_CACHE_FORMAT),
		SLEG_CONDVECTOR("path.tile", rv_path_tile,       SLE_UINT32,                 SLV_ROADVEH_PATH_CACHE, SLV_PATH_CACHE_FORMAT),
		SLEG_CONDSTRUCTLIST("path", SlVehicleRoadVehPath, SLV_PATH_CACHE_FORMAT, SL_MAX_VERSION),
		  SLE_CONDVAR(RoadVehicle, gv_flags,             SLE_UINT16,                 SLV_139, SL_MAX_VERSION),
	};
	static inline const SaveLoadCompatTable compat_description = _vehicle_roadveh_sl_compat;

	static void ConvertPathCache(RoadVehicle &rv)
	{
		/* The two vectors should be the same size, but if not we can just ignore the cache and not cause more issues. */
		if (rv_path_td.size() != rv_path_tile.size()) {
			Debug(sl, 1, "Found RoadVehicle {} with invalid path cache, ignoring.", rv.index);
			return;
		}
		size_t n = std::min(rv_path_td.size(), rv_path_tile.size());
		if (n == 0) return;

		rv.path.reserve(n);
		for (size_t c = 0; c < n; ++c) {
			rv.path.emplace_back(rv_path_td[c], rv_path_tile[c]);
		}

		/* Path cache is now taken from back instead of front, so needs reversing. */
		std::reverse(std::begin(rv.path), std::end(rv.path));
	}

	void Save(Vehicle *v) const override
	{
		if (v->type != VEH_ROAD) return;
		SlObject(v, this->GetDescription());
	}

	void Load(Vehicle *v) const override
	{
		if (v->type != VEH_ROAD) return;
		SlObject(v, this->GetLoadDescription());
		if (!IsSavegameVersionBefore(SLV_ROADVEH_PATH_CACHE) && IsSavegameVersionBefore(SLV_PATH_CACHE_FORMAT)) {
			ConvertPathCache(*static_cast<RoadVehicle *>(v));
		}
	}

	void FixPointers(Vehicle *v) const override
	{
		if (v->type != VEH_ROAD) return;
		SlObject(v, this->GetDescription());
	}
};

class SlVehicleShipPath : public VectorSaveLoadHandler<SlVehicleShipPath, Ship, ShipPathElement> {
public:
	static inline const SaveLoad description[] = {
		SLE_VAR(ShipPathElement, trackdir, SLE_UINT8),
	};
	static inline const SaveLoadCompatTable compat_description = {};

	std::vector<ShipPathElement> &GetVector(Ship *s) const override { return s->path; }
};

class SlVehicleShip : public DefaultSaveLoadHandler<SlVehicleShip, Vehicle> {
public:
	static inline std::vector<Trackdir> ship_path_td;

	static inline const SaveLoad description[] = {
		  SLEG_STRUCT("common", SlVehicleCommon),
		      SLE_VAR(Ship, state,                     SLE_UINT8),
		SLEG_CONDVECTOR("path", ship_path_td, SLE_UINT8, SLV_SHIP_PATH_CACHE, SLV_PATH_CACHE_FORMAT),
		SLEG_CONDSTRUCTLIST("path", SlVehicleShipPath, SLV_PATH_CACHE_FORMAT, SL_MAX_VERSION),
		  SLE_CONDVAR(Ship, rotation,                  SLE_UINT8,                  SLV_SHIP_ROTATION, SL_MAX_VERSION),
	};
	static inline const SaveLoadCompatTable compat_description = _vehicle_ship_sl_compat;

	void Save(Vehicle *v) const override
	{
		if (v->type != VEH_SHIP) return;
		SlObject(v, this->GetDescription());
	}

	void Load(Vehicle *v) const override
	{
		if (v->type != VEH_SHIP) return;
		SlObject(v, this->GetLoadDescription());

		if (IsSavegameVersionBefore(SLV_PATH_CACHE_FORMAT)) {
			/* Path cache is now taken from back instead of front, so needs reversing. */
			Ship *s = static_cast<Ship *>(v);
			std::transform(std::rbegin(ship_path_td), std::rend(ship_path_td), std::back_inserter(s->path), [](Trackdir trackdir) { return trackdir; });
		}
	}

	void FixPointers(Vehicle *v) const override
	{
		if (v->type != VEH_SHIP) return;
		SlObject(v, this->GetDescription());
	}
};

class SlVehicleAircraft : public DefaultSaveLoadHandler<SlVehicleAircraft, Vehicle> {
public:
	static inline const SaveLoad description[] = {
		 SLEG_STRUCT("common", SlVehicleCommon),
		     SLE_VAR(Aircraft, crashed_counter,       SLE_UINT16),
		     SLE_VAR(Aircraft, pos,                   SLE_UINT8),

		 SLE_CONDVAR(Aircraft, targetairport,         SLE_FILE_U8  | SLE_VAR_U16,   SL_MIN_VERSION, SLV_5),
		 SLE_CONDVAR(Aircraft, targetairport,         SLE_UINT16,                   SLV_5, SL_MAX_VERSION),

		     SLE_VAR(Aircraft, state,                 SLE_UINT8),

		 SLE_CONDVAR(Aircraft, previous_pos,          SLE_UINT8,                    SLV_2, SL_MAX_VERSION),
		 SLE_CONDVAR(Aircraft, last_direction,        SLE_UINT8,                    SLV_2, SL_MAX_VERSION),
		 SLE_CONDVAR(Aircraft, number_consecutive_turns, SLE_UINT8,                 SLV_2, SL_MAX_VERSION),

		 SLE_CONDVAR(Aircraft, turn_counter,          SLE_UINT8,                  SLV_136, SL_MAX_VERSION),
		 SLE_CONDVAR(Aircraft, flags,                 SLE_UINT8,                  SLV_167, SL_MAX_VERSION),
	};
	static inline const SaveLoadCompatTable compat_description = _vehicle_aircraft_sl_compat;

	void Save(Vehicle *v) const override
	{
		if (v->type != VEH_AIRCRAFT) return;
		SlObject(v, this->GetDescription());
	}

	void Load(Vehicle *v) const override
	{
		if (v->type != VEH_AIRCRAFT) return;
		SlObject(v, this->GetLoadDescription());
	}

	void FixPointers(Vehicle *v) const override
	{
		if (v->type != VEH_AIRCRAFT) return;
		SlObject(v, this->GetDescription());
	}
};

class SlVehicleEffect : public DefaultSaveLoadHandler<SlVehicleEffect, Vehicle> {
public:
	static inline const SaveLoad description[] = {
		     SLE_VAR(Vehicle, subtype,               SLE_UINT8),

		 SLE_CONDVAR(Vehicle, tile,                  SLE_FILE_U16 | SLE_VAR_U32,   SL_MIN_VERSION,   SLV_6),
		 SLE_CONDVAR(Vehicle, tile,                  SLE_UINT32,                   SLV_6, SL_MAX_VERSION),

		 SLE_CONDVAR(Vehicle, x_pos,                 SLE_FILE_I16 | SLE_VAR_I32,   SL_MIN_VERSION,   SLV_6),
		 SLE_CONDVAR(Vehicle, x_pos,                 SLE_INT32,                    SLV_6, SL_MAX_VERSION),
		 SLE_CONDVAR(Vehicle, y_pos,                 SLE_FILE_I16 | SLE_VAR_I32,   SL_MIN_VERSION,   SLV_6),
		 SLE_CONDVAR(Vehicle, y_pos,                 SLE_INT32,                    SLV_6, SL_MAX_VERSION),
		 SLE_CONDVAR(Vehicle, z_pos,                 SLE_FILE_U8  | SLE_VAR_I32,   SL_MIN_VERSION, SLV_164),
		 SLE_CONDVAR(Vehicle, z_pos,                 SLE_INT32,                  SLV_164, SL_MAX_VERSION),

		     SLE_VAR(Vehicle, sprite_cache.sprite_seq.seq[0].sprite, SLE_FILE_U16 | SLE_VAR_U32),
		     SLE_VAR(Vehicle, progress,              SLE_UINT8),
		     SLE_VAR(Vehicle, vehstatus,             SLE_UINT8),

		     SLE_VAR(EffectVehicle, animation_state,    SLE_UINT16),
		     SLE_VAR(EffectVehicle, animation_substate, SLE_UINT8),

		 SLE_CONDVAR(Vehicle, spritenum,             SLE_UINT8,                    SLV_2, SL_MAX_VERSION),
	};
	static inline const SaveLoadCompatTable compat_description = _vehicle_effect_sl_compat;

	void Save(Vehicle *v) const override
	{
		if (v->type != VEH_EFFECT) return;
		SlObject(v, this->GetDescription());
	}

	void Load(Vehicle *v) const override
	{
		if (v->type != VEH_EFFECT) return;
		SlObject(v, this->GetLoadDescription());
	}

	void FixPointers(Vehicle *v) const override
	{
		if (v->type != VEH_EFFECT) return;
		SlObject(v, this->GetDescription());
	}
};

class SlVehicleDisaster : public DefaultSaveLoadHandler<SlVehicleDisaster, Vehicle> {
public:
	static inline const SaveLoad description[] = {
		    SLE_REF(Vehicle, next,                  REF_VEHICLE_OLD),

		    SLE_VAR(Vehicle, subtype,               SLE_UINT8),
		SLE_CONDVAR(Vehicle, tile,                  SLE_FILE_U16 | SLE_VAR_U32,   SL_MIN_VERSION,   SLV_6),
		SLE_CONDVAR(Vehicle, tile,                  SLE_UINT32,                   SLV_6, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, dest_tile,             SLE_FILE_U16 | SLE_VAR_U32,   SL_MIN_VERSION,   SLV_6),
		SLE_CONDVAR(Vehicle, dest_tile,             SLE_UINT32,                   SLV_6, SL_MAX_VERSION),

		SLE_CONDVAR(Vehicle, x_pos,                 SLE_FILE_I16 | SLE_VAR_I32,   SL_MIN_VERSION,   SLV_6),
		SLE_CONDVAR(Vehicle, x_pos,                 SLE_INT32,                    SLV_6, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, y_pos,                 SLE_FILE_I16 | SLE_VAR_I32,   SL_MIN_VERSION,   SLV_6),
		SLE_CONDVAR(Vehicle, y_pos,                 SLE_INT32,                    SLV_6, SL_MAX_VERSION),
		SLE_CONDVAR(Vehicle, z_pos,                 SLE_FILE_U8  | SLE_VAR_I32,   SL_MIN_VERSION, SLV_164),
		SLE_CONDVAR(Vehicle, z_pos,                 SLE_INT32,                  SLV_164, SL_MAX_VERSION),
		    SLE_VAR(Vehicle, direction,             SLE_UINT8),

		    SLE_VAR(Vehicle, owner,                 SLE_UINT8),
		    SLE_VAR(Vehicle, vehstatus,             SLE_UINT8),
		SLE_CONDVARNAME(DisasterVehicle, state, "current_order.dest", SLE_FILE_U8 | SLE_VAR_U16, SL_MIN_VERSION,         SLV_5),
		SLE_CONDVARNAME(DisasterVehicle, state, "current_order.dest", SLE_UINT16,                SLV_5,                  SLV_DISASTER_VEH_STATE),
		SLE_CONDVAR(DisasterVehicle,     state,                       SLE_UINT16,                SLV_DISASTER_VEH_STATE, SL_MAX_VERSION),

		    SLE_VAR(Vehicle, sprite_cache.sprite_seq.seq[0].sprite, SLE_FILE_U16 | SLE_VAR_U32),
		SLE_CONDVAR(Vehicle, age,                   SLE_FILE_U16 | SLE_VAR_I32,   SL_MIN_VERSION,  SLV_31),
		SLE_CONDVAR(Vehicle, age,                   SLE_INT32,                   SLV_31, SL_MAX_VERSION),
		    SLE_VAR(Vehicle, tick_counter,          SLE_UINT8),

		SLE_CONDVAR(DisasterVehicle, image_override,            SLE_FILE_U16 | SLE_VAR_U32,   SL_MIN_VERSION, SLV_191),
		SLE_CONDVAR(DisasterVehicle, image_override,            SLE_UINT32,                 SLV_191, SL_MAX_VERSION),
		SLE_CONDVAR(DisasterVehicle, big_ufo_destroyer_target,  SLE_FILE_U16 | SLE_VAR_U32,   SL_MIN_VERSION, SLV_191),
		SLE_CONDVAR(DisasterVehicle, big_ufo_destroyer_target,  SLE_UINT32,                 SLV_191, SL_MAX_VERSION),
		SLE_CONDVAR(DisasterVehicle, flags,                     SLE_UINT8,                  SLV_194, SL_MAX_VERSION),
	};

	static inline const SaveLoadCompatTable compat_description = _vehicle_disaster_sl_compat;

	void Save(Vehicle *v) const override
	{
		if (v->type != VEH_DISASTER) return;
		SlObject(v, this->GetDescription());
	}

	void Load(Vehicle *v) const override
	{
		if (v->type != VEH_DISASTER) return;
		SlObject(v, this->GetLoadDescription());
	}

	void FixPointers(Vehicle *v) const override
	{
		if (v->type != VEH_DISASTER) return;
		SlObject(v, this->GetDescription());
	}
};

static const SaveLoad _vehicle_desc[] = {
	SLE_SAVEBYTE(Vehicle, type),
	SLEG_STRUCT("train", SlVehicleTrain),
	SLEG_STRUCT("roadveh", SlVehicleRoadVeh),
	SLEG_STRUCT("ship", SlVehicleShip),
	SLEG_STRUCT("aircraft", SlVehicleAircraft),
	SLEG_STRUCT("effect", SlVehicleEffect),
	SLEG_STRUCT("disaster", SlVehicleDisaster),
};

struct VEHSChunkHandler : ChunkHandler {
	VEHSChunkHandler() : ChunkHandler('VEHS', CH_SPARSE_TABLE) {}

	void Save() const override
	{
		SlTableHeader(_vehicle_desc);

		/* Write the vehicles */
		for (Vehicle *v : Vehicle::Iterate()) {
			SlSetArrayIndex(v->index);
			SlObject(v, _vehicle_desc);
		}
	}

	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(_vehicle_desc, _vehicle_sl_compat);

		int index;

		_cargo_count = 0;

		while ((index = SlIterateArray()) != -1) {
			Vehicle *v;
			VehicleType vtype = (VehicleType)SlReadByte();

			switch (vtype) {
				case VEH_TRAIN:    v = new (VehicleID(index)) Train();           break;
				case VEH_ROAD:     v = new (VehicleID(index)) RoadVehicle();     break;
				case VEH_SHIP:     v = new (VehicleID(index)) Ship();            break;
				case VEH_AIRCRAFT: v = new (VehicleID(index)) Aircraft();        break;
				case VEH_EFFECT:   v = new (VehicleID(index)) EffectVehicle();   break;
				case VEH_DISASTER: v = new (VehicleID(index)) DisasterVehicle(); break;
				case VEH_INVALID: // Savegame shouldn't contain invalid vehicles
				default: SlErrorCorrupt("Invalid vehicle type");
			}

			SlObject(v, slt);

			if (_cargo_count != 0 && IsCompanyBuildableVehicleType(v) && CargoPacket::CanAllocateItem()) {
				/* Don't construct the packet with station here, because that'll fail with old savegames */
				CargoPacket *cp = new CargoPacket(_cargo_count, _cargo_periods, _cargo_source, _cargo_source_xy, _cargo_feeder_share);
				v->cargo.Append(cp);
			}

			/* Old savegames used 'last_station_visited = 0xFF' */
			if (IsSavegameVersionBefore(SLV_5) && v->last_station_visited == 0xFF) {
				v->last_station_visited = StationID::Invalid();
			}

			if (IsSavegameVersionBefore(SLV_182)) v->last_loading_station = StationID::Invalid();

			if (IsSavegameVersionBefore(SLV_5)) {
				/* Convert the current_order.type (which is a mix of type and flags, because
				 *  in those versions, they both were 4 bits big) to type and flags */
				v->current_order.flags = GB(v->current_order.type, 4, 4);
				v->current_order.type &= 0x0F;
			}

			/* Advanced vehicle lists got added */
			if (IsSavegameVersionBefore(SLV_60)) v->group_id = DEFAULT_GROUP;
		}
	}

	void FixPointers() const override
	{
		for (Vehicle *v : Vehicle::Iterate()) {
			SlObject(v, _vehicle_desc);
		}
	}
};

static const VEHSChunkHandler VEHS;
static const ChunkHandlerRef veh_chunk_handlers[] = {
	VEHS,
};

extern const ChunkHandlerTable _veh_chunk_handlers(veh_chunk_handlers);
