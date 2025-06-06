/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_station.h Header file for NewGRF stations */

#ifndef NEWGRF_STATION_H
#define NEWGRF_STATION_H

#include "core/enum_type.hpp"
#include "newgrf_animation_type.h"
#include "newgrf_badge_type.h"
#include "newgrf_callbacks.h"
#include "newgrf_class.h"
#include "newgrf_commons.h"
#include "cargo_type.h"
#include "station_type.h"
#include "rail_type.h"
#include "newgrf_spritegroup.h"
#include "newgrf_town.h"

/** Scope resolver for stations. */
struct StationScopeResolver : public ScopeResolver {
	TileIndex tile;                     ///< %Tile of the station.
	struct BaseStation *st;             ///< Instance of the station.
	const struct StationSpec *statspec; ///< Station (type) specification.
	CargoType cargo_type;                 ///< Type of cargo of the station.
	Axis axis;                          ///< Station axis, used only for the slope check callback.

	/**
	 * Station variable cache
	 * This caches 'expensive' station variable lookups which iterate over
	 * several tiles that may be called multiple times per Resolve().
	 */
	struct Cache {
		std::optional<uint32_t> v40;
		std::optional<uint32_t> v41;
		std::optional<uint32_t> v45;
		std::optional<uint32_t> v46;
		std::optional<uint32_t> v47;
		std::optional<uint32_t> v49;
	};
	mutable Cache cache;

	/**
	 * Constructor for station scopes.
	 * @param ro Surrounding resolver.
	 * @param statspec Station (type) specification.
	 * @param st Instance of the station.
	 * @param tile %Tile of the station.
	 */
	StationScopeResolver(ResolverObject &ro, const StationSpec *statspec, BaseStation *st, TileIndex tile)
		: ScopeResolver(ro), tile(tile), st(st), statspec(statspec), cargo_type(INVALID_CARGO), axis(INVALID_AXIS)
	{
	}

	uint32_t GetRandomBits() const override;
	uint32_t GetRandomTriggers() const override;

	uint32_t GetVariable(uint8_t variable, [[maybe_unused]] uint32_t parameter, bool &available) const override;
};

/** Station resolver. */
struct StationResolverObject : public SpecializedResolverObject<StationRandomTriggers> {
	StationScopeResolver station_scope; ///< The station scope resolver.
	std::optional<TownScopeResolver> town_scope = std::nullopt; ///< The town scope resolver (created on the first call).

	StationResolverObject(const StationSpec *statspec, BaseStation *st, TileIndex tile,
			CallbackID callback = CBID_NO_CALLBACK, uint32_t callback_param1 = 0, uint32_t callback_param2 = 0);

	TownScopeResolver *GetTown();

	ScopeResolver *GetScope(VarSpriteGroupScope scope = VSG_SCOPE_SELF, uint8_t relative = 0) override
	{
		switch (scope) {
			case VSG_SCOPE_SELF:
				return &this->station_scope;

			case VSG_SCOPE_PARENT: {
				TownScopeResolver *tsr = this->GetTown();
				if (tsr != nullptr) return tsr;
				[[fallthrough]];
			}

			default:
				return ResolverObject::GetScope(scope, relative);
		}
	}

	const SpriteGroup *ResolveReal(const RealSpriteGroup &group) const override;

	GrfSpecFeature GetFeature() const override;
	uint32_t GetDebugID() const override;
};

static const uint32_t STATION_CLASS_LABEL_DEFAULT = 'DFLT';
static const uint32_t STATION_CLASS_LABEL_WAYPOINT = 'WAYP';

enum StationClassID : uint16_t {
	STAT_CLASS_BEGIN = 0, ///< the lowest valid value
	STAT_CLASS_DFLT = 0, ///< Default station class.
	STAT_CLASS_WAYP, ///< Waypoint class.
	STAT_CLASS_MAX = UINT16_MAX, ///< Maximum number of classes.
};

/** Allow incrementing of StationClassID variables */
DECLARE_INCREMENT_DECREMENT_OPERATORS(StationClassID)

enum class StationSpecFlag : uint8_t {
	SeparateGround = 0, ///< Use different sprite set for ground sprites.
	DivByStationSize = 1, ///< Divide cargo amount by station size.
	Cb141RandomBits = 2, ///< Callback 141 needs random bits.
	CustomFoundations = 3, ///< Draw custom foundations.
	ExtendedFoundations = 4, ///< Extended foundation block instead of simple.
};
using StationSpecFlags = EnumBitSet<StationSpecFlag, uint8_t>;

/** Station specification. */
struct StationSpec : NewGRFSpecBase<StationClassID> {
	StationSpec() : name(0),
		disallowed_platforms(0), disallowed_lengths(0),
		cargo_threshold(0), cargo_triggers(0),
		callback_mask(0), flags(0)
	{}

	CargoGRFFileProps grf_prop; ///< Link to NewGRF
	StringID name;             ///< Name of this station.

	/**
	 * Bitmask of number of platforms available for the station.
	 * 0..6 correspond to 1..7, while bit 7 corresponds to >7 platforms.
	 */
	uint8_t disallowed_platforms;
	/**
	 * Bitmask of platform lengths available for the station.
	 * 0..6 correspond to 1..7, while bit 7 corresponds to >7 tiles long.
	 */
	uint8_t disallowed_lengths;

	/**
	 * Number of tile layouts.
	 * A minimum of 8 is required is required for stations.
	 * 0-1 = plain platform
	 * 2-3 = platform with building
	 * 4-5 = platform with roof, left side
	 * 6-7 = platform with roof, right side
	 */
	std::vector<NewGRFSpriteLayout> renderdata; ///< Array of tile layouts.

	/**
	 * Cargo threshold for choosing between little and lots of cargo
	 * @note little/lots are equivalent to the moving/loading states for vehicles
	 */
	uint16_t cargo_threshold;

	CargoTypes cargo_triggers; ///< Bitmask of cargo types which cause trigger re-randomizing

	StationCallbackMasks callback_mask; ///< Bitmask of station callbacks that have to be called

	StationSpecFlags flags; ///< Bitmask of flags, bit 0: use different sprite set; bit 1: divide cargo about by station size

	enum class TileFlag : uint8_t {
		Pylons = 0, ///< Tile should contain catenary pylons.
		NoWires = 1, ///< Tile should NOT contain catenary wires.
		Blocked = 2, ///< Tile is blocked to vehicles.
	};
	using TileFlags = EnumBitSet<TileFlag, uint8_t>;
	std::vector<TileFlags> tileflags; ///< List of tile flags.

	AnimationInfo<StationAnimationTriggers> animation;

	/** Custom platform layouts, keyed by platform and length combined. */
	std::unordered_map<uint16_t, std::vector<uint8_t>> layouts;

	std::vector<BadgeID> badges;
};

/** Class containing information relating to station classes. */
using StationClass = NewGRFClass<StationSpec, StationClassID, STAT_CLASS_MAX>;

const StationSpec *GetStationSpec(TileIndex t);

/**
 * Get the station layout key for a given station layout size.
 * @param platforms Number of platforms.
 * @param length Length of platforms.
 * @returns Key of station layout.
 */
inline uint16_t GetStationLayoutKey(uint8_t platforms, uint8_t length)
{
	return (length << 8U) | platforms;
}

/**
 * Test if a StationClass is the waypoint class.
 * @param cls StationClass to test.
 * @return true if the class is the waypoint class.
 */
inline bool IsWaypointClass(const StationClass &cls)
{
	return cls.global_id == STATION_CLASS_LABEL_WAYPOINT || GB(cls.global_id, 24, 8) == UINT8_MAX;
}

/* Evaluate a tile's position within a station, and return the result a bitstuffed format. */
uint32_t GetPlatformInfo(Axis axis, uint8_t tile, int platforms, int length, int x, int y, bool centred);

SpriteID GetCustomStationRelocation(const StationSpec *statspec, BaseStation *st, TileIndex tile, uint32_t var10 = 0);
void GetCustomStationRelocation(SpriteLayoutProcessor &processor, const StationSpec *statspec, BaseStation *st, TileIndex tile);
SpriteID GetCustomStationFoundationRelocation(const StationSpec *statspec, BaseStation *st, TileIndex tile, uint layout, uint edge_info);
uint16_t GetStationCallback(CallbackID callback, uint32_t param1, uint32_t param2, const StationSpec *statspec, BaseStation *st, TileIndex tile, std::span<int32_t> regs100 = {});
CommandCost PerformStationTileSlopeCheck(TileIndex north_tile, TileIndex cur_tile, const StationSpec *statspec, Axis axis, uint8_t plat_len, uint8_t numtracks);

/* Allocate a StationSpec to a Station. This is called once per build operation. */
int AllocateSpecToStation(const StationSpec *statspec, BaseStation *st, bool exec);

/* Deallocate a StationSpec from a Station. Called when removing a single station tile. */
void DeallocateSpecFromStation(BaseStation *st, uint8_t specindex);

/* Draw representation of a station tile for GUI purposes. */
bool DrawStationTile(int x, int y, RailType railtype, Axis axis, StationClassID sclass, uint station);

void AnimateStationTile(TileIndex tile);
void TriggerStationAnimation(BaseStation *st, TileIndex tile, StationAnimationTrigger trigger, CargoType cargo_type = INVALID_CARGO);
void TriggerStationRandomisation(BaseStation *st, TileIndex tile, StationRandomTrigger trigger, CargoType cargo_type = INVALID_CARGO);
void StationUpdateCachedTriggers(BaseStation *st);

#endif /* NEWGRF_STATION_H */
