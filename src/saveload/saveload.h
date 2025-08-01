/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload.h Functions/types related to saving and loading games. */

#ifndef SAVELOAD_H
#define SAVELOAD_H

#include "saveload_error.hpp"
#include "../fileio_type.h"
#include "../fios.h"

/** SaveLoad versions
 * Previous savegame versions, the trunk revision where they were
 * introduced and the released version that had that particular
 * savegame version.
 * Up to savegame version 18 there is a minor version as well.
 *
 * Older entries keep their original numbering.
 *
 * Newer entries should use a descriptive labels, numeric version
 * and PR can be added to comment.
 *
 * Note that this list must not be reordered.
 */
enum SaveLoadVersion : uint16_t {
	SL_MIN_VERSION,                         ///< First savegame version

	SLV_1,                                  ///<   1.0         0.1.x, 0.2.x
	SLV_2,                                  /**<   2.0         0.3.0
	                                         *     2.1         0.3.1, 0.3.2 */
	SLV_3,                                  ///<   3.x         lost
	SLV_4,                                  /**<   4.0     1
	                                         *     4.1   122   0.3.3, 0.3.4
	                                         *     4.2  1222   0.3.5
	                                         *     4.3  1417
	                                         *     4.4  1426 */

	SLV_5,                                  /**<   5.0  1429
	                                         *     5.1  1440
	                                         *     5.2  1525   0.3.6 */
	SLV_6,                                  /**<   6.0  1721
	                                         *     6.1  1768 */
	SLV_7,                                  ///<   7.0  1770
	SLV_8,                                  ///<   8.0  1786
	SLV_9,                                  ///<   9.0  1909

	SLV_10,                                 ///<  10.0  2030
	SLV_11,                                 /**<  11.0  2033
	                                         *    11.1  2041 */
	SLV_12,                                 ///<  12.1  2046
	SLV_13,                                 ///<  13.1  2080   0.4.0, 0.4.0.1
	SLV_14,                                 ///<  14.0  2441

	SLV_15,                                 ///<  15.0  2499
	SLV_16,                                 /**<  16.0  2817
	                                         *    16.1  3155 */
	SLV_17,                                 /**<  17.0  3212
	                                         *    17.1  3218 */
	SLV_18,                                 ///<  18    3227
	SLV_19,                                 ///<  19    3396

	SLV_20,                                 ///<  20    3403
	SLV_21,                                 ///<  21    3472   0.4.x
	SLV_22,                                 ///<  22    3726
	SLV_23,                                 ///<  23    3915
	SLV_24,                                 ///<  24    4150

	SLV_25,                                 ///<  25    4259
	SLV_26,                                 ///<  26    4466
	SLV_27,                                 ///<  27    4757
	SLV_28,                                 ///<  28    4987
	SLV_29,                                 ///<  29    5070

	SLV_30,                                 ///<  30    5946
	SLV_31,                                 ///<  31    5999
	SLV_32,                                 ///<  32    6001
	SLV_33,                                 ///<  33    6440
	SLV_34,                                 ///<  34    6455

	SLV_35,                                 ///<  35    6602
	SLV_36,                                 ///<  36    6624
	SLV_37,                                 ///<  37    7182
	SLV_38,                                 ///<  38    7195
	SLV_39,                                 ///<  39    7269

	SLV_40,                                 ///<  40    7326
	SLV_41,                                 ///<  41    7348   0.5.x
	SLV_42,                                 ///<  42    7573
	SLV_43,                                 ///<  43    7642
	SLV_44,                                 ///<  44    8144

	SLV_45,                                 ///<  45    8501
	SLV_46,                                 ///<  46    8705
	SLV_47,                                 ///<  47    8735
	SLV_48,                                 ///<  48    8935
	SLV_49,                                 ///<  49    8969

	SLV_50,                                 ///<  50    8973
	SLV_51,                                 ///<  51    8978
	SLV_52,                                 ///<  52    9066
	SLV_53,                                 ///<  53    9316
	SLV_54,                                 ///<  54    9613

	SLV_55,                                 ///<  55    9638
	SLV_56,                                 ///<  56    9667
	SLV_57,                                 ///<  57    9691
	SLV_58,                                 ///<  58    9762
	SLV_59,                                 ///<  59    9779

	SLV_60,                                 ///<  60    9874
	SLV_61,                                 ///<  61    9892
	SLV_62,                                 ///<  62    9905
	SLV_63,                                 ///<  63    9956
	SLV_64,                                 ///<  64   10006

	SLV_65,                                 ///<  65   10210
	SLV_66,                                 ///<  66   10211
	SLV_67,                                 ///<  67   10236
	SLV_68,                                 ///<  68   10266
	SLV_69,                                 ///<  69   10319

	SLV_70,                                 ///<  70   10541
	SLV_71,                                 ///<  71   10567
	SLV_72,                                 ///<  72   10601
	SLV_73,                                 ///<  73   10903
	SLV_74,                                 ///<  74   11030

	SLV_75,                                 ///<  75   11107
	SLV_76,                                 ///<  76   11139
	SLV_77,                                 ///<  77   11172
	SLV_78,                                 ///<  78   11176
	SLV_79,                                 ///<  79   11188

	SLV_80,                                 ///<  80   11228
	SLV_81,                                 ///<  81   11244
	SLV_82,                                 ///<  82   11410
	SLV_83,                                 ///<  83   11589
	SLV_84,                                 ///<  84   11822

	SLV_85,                                 ///<  85   11874
	SLV_86,                                 ///<  86   12042
	SLV_87,                                 ///<  87   12129
	SLV_88,                                 ///<  88   12134
	SLV_89,                                 ///<  89   12160

	SLV_90,                                 ///<  90   12293
	SLV_91,                                 ///<  91   12347
	SLV_92,                                 ///<  92   12381   0.6.x
	SLV_93,                                 ///<  93   12648
	SLV_94,                                 ///<  94   12816

	SLV_95,                                 ///<  95   12924
	SLV_96,                                 ///<  96   13226
	SLV_97,                                 ///<  97   13256
	SLV_98,                                 ///<  98   13375
	SLV_99,                                 ///<  99   13838

	SLV_100,                                ///< 100   13952
	SLV_101,                                ///< 101   14233
	SLV_102,                                ///< 102   14332
	SLV_103,                                ///< 103   14598
	SLV_104,                                ///< 104   14735

	SLV_105,                                ///< 105   14803
	SLV_106,                                ///< 106   14919
	SLV_107,                                ///< 107   15027
	SLV_108,                                ///< 108   15045
	SLV_109,                                ///< 109   15075

	SLV_110,                                ///< 110   15148
	SLV_111,                                ///< 111   15190
	SLV_112,                                ///< 112   15290
	SLV_113,                                ///< 113   15340
	SLV_114,                                ///< 114   15601

	SLV_115,                                ///< 115   15695
	SLV_116,                                ///< 116   15893   0.7.x
	SLV_117,                                ///< 117   16037
	SLV_118,                                ///< 118   16129
	SLV_119,                                ///< 119   16242

	SLV_120,                                ///< 120   16439
	SLV_121,                                ///< 121   16694
	SLV_122,                                ///< 122   16855
	SLV_123,                                ///< 123   16909
	SLV_124,                                ///< 124   16993

	SLV_125,                                ///< 125   17113
	SLV_126,                                ///< 126   17433
	SLV_127,                                ///< 127   17439
	SLV_128,                                ///< 128   18281
	SLV_129,                                ///< 129   18292

	SLV_130,                                ///< 130   18404
	SLV_131,                                ///< 131   18481
	SLV_132,                                ///< 132   18522
	SLV_133,                                ///< 133   18674
	SLV_134,                                ///< 134   18703

	SLV_135,                                ///< 135   18719
	SLV_136,                                ///< 136   18764
	SLV_137,                                ///< 137   18912
	SLV_138,                                ///< 138   18942   1.0.x
	SLV_139,                                ///< 139   19346

	SLV_140,                                ///< 140   19382
	SLV_141,                                ///< 141   19799
	SLV_142,                                ///< 142   20003
	SLV_143,                                ///< 143   20048
	SLV_144,                                ///< 144   20334

	SLV_145,                                ///< 145   20376
	SLV_146,                                ///< 146   20446
	SLV_147,                                ///< 147   20621
	SLV_148,                                ///< 148   20659
	SLV_149,                                ///< 149   20832

	SLV_150,                                ///< 150   20857
	SLV_151,                                ///< 151   20918
	SLV_152,                                ///< 152   21171
	SLV_153,                                ///< 153   21263
	SLV_154,                                ///< 154   21426

	SLV_155,                                ///< 155   21453
	SLV_156,                                ///< 156   21728
	SLV_157,                                ///< 157   21862
	SLV_158,                                ///< 158   21933
	SLV_159,                                ///< 159   21962

	SLV_160,                                ///< 160   21974   1.1.x
	SLV_161,                                ///< 161   22567
	SLV_162,                                ///< 162   22713
	SLV_163,                                ///< 163   22767
	SLV_164,                                ///< 164   23290

	SLV_165,                                ///< 165   23304
	SLV_166,                                ///< 166   23415
	SLV_167,                                ///< 167   23504
	SLV_168,                                ///< 168   23637
	SLV_169,                                ///< 169   23816

	SLV_170,                                ///< 170   23826
	SLV_171,                                ///< 171   23835
	SLV_172,                                ///< 172   23947
	SLV_173,                                ///< 173   23967   1.2.0-RC1
	SLV_174,                                ///< 174   23973   1.2.x

	SLV_175,                                ///< 175   24136
	SLV_176,                                ///< 176   24446
	SLV_177,                                ///< 177   24619
	SLV_178,                                ///< 178   24789
	SLV_179,                                ///< 179   24810

	SLV_180,                                ///< 180   24998   1.3.x
	SLV_181,                                ///< 181   25012
	SLV_182,                                ///< 182   25115 FS#5492, r25259, r25296 Goal status
	SLV_183,                                ///< 183   25363 Cargodist
	SLV_184,                                ///< 184   25508 Unit localisation split

	SLV_185,                                ///< 185   25620 Storybooks
	SLV_186,                                ///< 186   25833 Objects storage
	SLV_187,                                ///< 187   25899 Linkgraph - restricted flows
	SLV_188,                                ///< 188   26169 v1.4  FS#5831 Unify RV travel time
	SLV_189,                                ///< 189   26450 Hierarchical vehicle subgroups

	SLV_190,                                ///< 190   26547 Separate order travel and wait times
	SLV_191,                                ///< 191   26636 FS#6026 Fix disaster vehicle storage (No bump)
	                                        ///< 191   26646 FS#6041 Linkgraph - store locations
	SLV_192,                                ///< 192   26700 FS#6066 Fix saving of order backups
	SLV_193,                                ///< 193   26802
	SLV_194,                                ///< 194   26881 v1.5

	SLV_195,                                ///< 195   27572 v1.6.1
	SLV_196,                                ///< 196   27778 v1.7
	SLV_197,                                ///< 197   27978 v1.8
	SLV_198,                                ///< 198  PR#6763 Switch town growth rate and counter to actual game ticks
	SLV_EXTEND_CARGOTYPES,                  ///< 199  PR#6802 Extend cargotypes to 64

	SLV_EXTEND_RAILTYPES,                   ///< 200  PR#6805 Extend railtypes to 64, adding uint16_t to map array.
	SLV_EXTEND_PERSISTENT_STORAGE,          ///< 201  PR#6885 Extend NewGRF persistent storages.
	SLV_EXTEND_INDUSTRY_CARGO_SLOTS,        ///< 202  PR#6867 Increase industry cargo slots to 16 in, 16 out
	SLV_SHIP_PATH_CACHE,                    ///< 203  PR#7072 Add path cache for ships
	SLV_SHIP_ROTATION,                      ///< 204  PR#7065 Add extra rotation stages for ships.

	SLV_GROUP_LIVERIES,                     ///< 205  PR#7108 Livery storage change and group liveries.
	SLV_SHIPS_STOP_IN_LOCKS,                ///< 206  PR#7150 Ship/lock movement changes.
	SLV_FIX_CARGO_MONITOR,                  ///< 207  PR#7175 v1.9  Cargo monitor data packing fix to support 64 cargotypes.
	SLV_TOWN_CARGOGEN,                      ///< 208  PR#6965 New algorithms for town building cargo generation.
	SLV_SHIP_CURVE_PENALTY,                 ///< 209  PR#7289 Configurable ship curve penalties.

	SLV_SERVE_NEUTRAL_INDUSTRIES,           ///< 210  PR#7234 Company stations can serve industries with attached neutral stations.
	SLV_ROADVEH_PATH_CACHE,                 ///< 211  PR#7261 Add path cache for road vehicles.
	SLV_REMOVE_OPF,                         ///< 212  PR#7245 Remove OPF.
	SLV_TREES_WATER_CLASS,                  ///< 213  PR#7405 WaterClass update for tree tiles.
	SLV_ROAD_TYPES,                         ///< 214  PR#6811 NewGRF road types.

	SLV_SCRIPT_MEMLIMIT,                    ///< 215  PR#7516 Limit on AI/GS memory consumption.
	SLV_MULTITILE_DOCKS,                    ///< 216  PR#7380 Multiple docks per station.
	SLV_TRADING_AGE,                        ///< 217  PR#7780 Configurable company trading age.
	SLV_ENDING_YEAR,                        ///< 218  PR#7747 v1.10  Configurable ending year.
	SLV_REMOVE_TOWN_CARGO_CACHE,            ///< 219  PR#8258 Remove town cargo acceptance and production caches.

	/* Patchpacks for a while considered it a good idea to jump a few versions
	 * above our version for their savegames. But as time continued, this gap
	 * has been closing, up to the point we would start to reuse versions from
	 * their patchpacks. This is not a problem from our perspective: the
	 * savegame will simply fail to load because they all contain chunks we
	 * cannot digest. But, this gives for ugly errors. As we have plenty of
	 * versions anyway, we simply skip the versions we know belong to
	 * patchpacks. This way we can present the user with a clean error
	 * indicate they are loading a savegame from a patchpack.
	 * For future patchpack creators: please follow a system like JGRPP, where
	 * the version is masked with 0x8000, and the true version is stored in
	 * its own chunk with feature toggles.
	 */
	SLV_START_PATCHPACKS,                   ///< 220  First known patchpack to use a version just above ours.
	SLV_END_PATCHPACKS = 286,               ///< 286  Last known patchpack to use a version just above ours.

	SLV_GS_INDUSTRY_CONTROL,                ///< 287  PR#7912 and PR#8115 GS industry control.
	SLV_VEH_MOTION_COUNTER,                 ///< 288  PR#8591 Desync safe motion counter
	SLV_INDUSTRY_TEXT,                      ///< 289  PR#8576 v1.11.0-RC1  Additional GS text for industries.

	SLV_MAPGEN_SETTINGS_REVAMP,             ///< 290  PR#8891 v1.11  Revamp of some mapgen settings (snow coverage, desert coverage, heightmap height, custom terrain type).
	SLV_GROUP_REPLACE_WAGON_REMOVAL,        ///< 291  PR#7441 Per-group wagon removal flag.
	SLV_CUSTOM_SUBSIDY_DURATION,            ///< 292  PR#9081 Configurable subsidy duration.
	SLV_SAVELOAD_LIST_LENGTH,               ///< 293  PR#9374 Consistency in list length with SL_STRUCT / SL_STRUCTLIST / SL_DEQUE / SL_REFLIST.
	SLV_RIFF_TO_ARRAY,                      ///< 294  PR#9375 Changed many CH_RIFF chunks to CH_ARRAY chunks.

	SLV_TABLE_CHUNKS,                       ///< 295  PR#9322 Introduction of CH_TABLE and CH_SPARSE_TABLE.
	SLV_SCRIPT_INT64,                       ///< 296  PR#9415 SQInteger is 64bit but was saved as 32bit.
	SLV_LINKGRAPH_TRAVEL_TIME,              ///< 297  PR#9457 v12.0-RC1  Store travel time in the linkgraph.
	SLV_DOCK_DOCKINGTILES,                  ///< 298  PR#9578 All tiles around docks may be docking tiles.
	SLV_REPAIR_OBJECT_DOCKING_TILES,        ///< 299  PR#9594 v12.0  Fixing issue with docking tiles overlapping objects.

	SLV_U64_TICK_COUNTER,                   ///< 300  PR#10035 Make tick counter 64bit to avoid wrapping.
	SLV_LAST_LOADING_TICK,                  ///< 301  PR#9693 Store tick of last loading for vehicles.
	SLV_MULTITRACK_LEVEL_CROSSINGS,         ///< 302  PR#9931 v13.0  Multi-track level crossings.
	SLV_NEWGRF_ROAD_STOPS,                  ///< 303  PR#10144 NewGRF road stops.
	SLV_LINKGRAPH_EDGES,                    ///< 304  PR#10314 Explicitly store link graph edges destination, PR#10471 int64_t instead of uint64_t league rating

	SLV_VELOCITY_NAUTICAL,                  ///< 305  PR#10594 Separation of land and nautical velocity (knots!)
	SLV_CONSISTENT_PARTIAL_Z,               ///< 306  PR#10570 Conversion from an inconsistent partial Z calculation for slopes, to one that is (more) consistent.
	SLV_MORE_CARGO_AGE,                     ///< 307  PR#10596 Track cargo age for a longer period.
	SLV_LINKGRAPH_SECONDS,                  ///< 308  PR#10610 Store linkgraph update intervals in seconds instead of days.
	SLV_AI_START_DATE,                      ///< 309  PR#10653 Removal of individual AI start dates and added a generic one.

	SLV_EXTEND_VEHICLE_RANDOM,              ///< 310  PR#10701 Extend vehicle random bits.
	SLV_EXTEND_ENTITY_MAPPING,              ///< 311  PR#10672 Extend entity mapping range.
	SLV_DISASTER_VEH_STATE,                 ///< 312  PR#10798 Explicit storage of disaster vehicle state.
	SLV_SAVEGAME_ID,                        ///< 313  PR#10719 Add an unique ID to every savegame (used to deduplicate surveys).
	SLV_STRING_GAMELOG,                     ///< 314  PR#10801 Use std::string in gamelog.

	SLV_INDUSTRY_CARGO_REORGANISE,          ///< 315  PR#10853 Industry accepts/produced data reorganised.
	SLV_PERIODS_IN_TRANSIT_RENAME,          ///< 316  PR#11112 Rename days in transit to (cargo) periods in transit.
	SLV_NEWGRF_LAST_SERVICE,                ///< 317  PR#11124 Added stable date_of_last_service to avoid NewGRF trouble.
	SLV_REMOVE_LOADED_AT_XY,                ///< 318  PR#11276 Remove loaded_at_xy variable from CargoPacket.
	SLV_CARGO_TRAVELLED,                    ///< 319  PR#11283 CargoPacket now tracks how far it travelled inside a vehicle.

	SLV_STATION_RATING_CHEAT,               ///< 320  PR#11346 Add cheat to fix station ratings at 100%.
	SLV_TIMETABLE_START_TICKS,              ///< 321  PR#11468 Convert timetable start from a date to ticks.
	SLV_TIMETABLE_START_TICKS_FIX,          ///< 322  PR#11557 Fix for missing convert timetable start from a date to ticks.
	SLV_TIMETABLE_TICKS_TYPE,               ///< 323  PR#11435 Convert timetable current order time to ticks.
	SLV_WATER_REGIONS,                      ///< 324  PR#10543 Water Regions for ship pathfinder.

	SLV_WATER_REGION_EVAL_SIMPLIFIED,       ///< 325  PR#11750 Simplified Water Region evaluation.
	SLV_ECONOMY_DATE,                       ///< 326  PR#10700 Split calendar and economy timers and dates.
	SLV_ECONOMY_MODE_TIMEKEEPING_UNITS,     ///< 327  PR#11341 Mode to display economy measurements in wallclock units.
	SLV_CALENDAR_SUB_DATE_FRACT,            ///< 328  PR#11428 Add sub_date_fract to measure calendar days.
	SLV_SHIP_ACCELERATION,                  ///< 329  PR#10734 Start using Vehicle's acceleration field for ships too.

	SLV_MAX_LOAN_FOR_COMPANY,               ///< 330  PR#11224 Separate max loan for each company.
	SLV_DEPOT_UNBUNCHING,                   ///< 331  PR#11945 Allow unbunching shared order vehicles at a depot.
	SLV_AI_LOCAL_CONFIG,                    ///< 332  PR#12003 Config of running AI is stored inside Company.
	SLV_SCRIPT_RANDOMIZER,                  ///< 333  PR#12063 v14.0-RC1 Save script randomizers.
	SLV_VEHICLE_ECONOMY_AGE,                ///< 334  PR#12141 v14.0 Add vehicle age in economy year, for profit stats minimum age

	SLV_COMPANY_ALLOW_LIST,                 ///< 335  PR#12337 Saving of list of client keys that are allowed to join this company.
	SLV_GROUP_NUMBERS,                      ///< 336  PR#12297 Add per-company group numbers.
	SLV_INCREASE_STATION_TYPE_FIELD_SIZE,   ///< 337  PR#12572 Increase size of StationType field in map array
	SLV_ROAD_WAYPOINTS,                     ///< 338  PR#12572 Road waypoints
	SLV_COMPANY_INAUGURATED_PERIOD,         ///< 339  PR#12798 Companies show the period inaugurated in wallclock mode.

	SLV_ROAD_STOP_TILE_DATA,                ///< 340  PR#12883 Move storage of road stop tile data, also save for road waypoints.
	SLV_COMPANY_ALLOW_LIST_V2,              ///< 341  PR#12908 Fixed savegame format for saving of list of client keys that are allowed to join this company.
	SLV_WATER_TILE_TYPE,                    ///< 342  PR#13030 Simplify water tile type.
	SLV_PRODUCTION_HISTORY,                 ///< 343  PR#10541 Industry production history.
	SLV_ROAD_TYPE_LABEL_MAP,                ///< 344  PR#13021 Add road type label map to allow upgrade/conversion of road types.

	SLV_NONFLOODING_WATER_TILES,            ///< 345  PR#13013 Store water tile non-flooding state.
	SLV_PATH_CACHE_FORMAT,                  ///< 346  PR#12345 Vehicle path cache format changed.
	SLV_ANIMATED_TILE_STATE_IN_MAP,         ///< 347  PR#13082 Animated tile state saved for improved performance.
	SLV_INCREASE_HOUSE_LIMIT,               ///< 348  PR#12288 Increase house limit to 4096.
	SLV_COMPANY_INAUGURATED_PERIOD_V2,      ///< 349  PR#13448 Fix savegame storage for company inaugurated year in wallclock mode.

	SLV_ENCODED_STRING_FORMAT,              ///< 350  PR#13499 Encoded String format changed.
	SLV_PROTECT_PLACED_HOUSES,              ///< 351  PR#13270 Houses individually placed by players can be protected from town/AI removal.
	SLV_SCRIPT_SAVE_INSTANCES,              ///< 352  PR#13556 Scripts are allowed to save instances.
	SLV_FIX_SCC_ENCODED_NEGATIVE,           ///< 353  PR#14049 Fix encoding of negative parameters.
	SLV_ORDERS_OWNED_BY_ORDERLIST,          ///< 354  PR#13948 Orders stored in OrderList, pool removed.

	SLV_FACE_STYLES,                        ///< 355  PR#14319 Addition of face styles, replacing gender and ethnicity.
	SLV_INDUSTRY_NUM_VALID_HISTORY,         ///< 356  PR#14416 Store number of valid history records for industries.
	SLV_INDUSTRY_ACCEPTED_HISTORY,          ///< 357  PR#14321 Add per-industry history of cargo delivered and waiting.

	SL_MAX_VERSION,                         ///< Highest possible saveload version
};

/** Save or load result codes. */
enum SaveOrLoadResult : uint8_t {
	SL_OK     = 0, ///< completed successfully
	SL_ERROR  = 1, ///< error that was caught before internal structures were modified
	SL_REINIT = 2, ///< error that was caught in the middle of updating game state, need to clear it. (can only happen during load)
};

/** Deals with the type of the savegame, independent of extension */
struct FileToSaveLoad {
	SaveLoadOperation file_op;       ///< File operation to perform.
	FiosType ftype;                  ///< File type.
	std::string name;                ///< Name of the file.
	std::string title;               ///< Internal name of the game.

	void SetMode(const FiosType &ft, SaveLoadOperation fop = SLO_LOAD);
	void Set(const FiosItem &item);
};

/** Types of save games. */
enum SavegameType : uint8_t {
	SGT_TTD,    ///< TTD  savegame (can be detected incorrectly)
	SGT_TTDP1,  ///< TTDP savegame ( -//- ) (data at NW border)
	SGT_TTDP2,  ///< TTDP savegame in new format (data at SE border)
	SGT_OTTD,   ///< OTTD savegame
	SGT_TTO,    ///< TTO savegame
	SGT_INVALID = 0xFF, ///< broken savegame (used internally)
};

extern FileToSaveLoad _file_to_saveload;

std::string GenerateDefaultSaveName();
void SetSaveLoadError(StringID str);
EncodedString GetSaveLoadErrorType();
EncodedString GetSaveLoadErrorMessage();
SaveOrLoadResult SaveOrLoad(std::string_view filename, SaveLoadOperation fop, DetailedFileType dft, Subdirectory sb, bool threaded = true);
void WaitTillSaved();
void ProcessAsyncSaveFinish();
void DoExitSave();

void DoAutoOrNetsave(FiosNumberedSaveName &counter);

SaveOrLoadResult SaveWithFilter(std::shared_ptr<struct SaveFilter> writer, bool threaded);
SaveOrLoadResult LoadWithFilter(std::shared_ptr<struct LoadFilter> reader);

typedef void AutolengthProc(int);

/** Type of a chunk. */
enum ChunkType : uint8_t {
	CH_RIFF = 0,
	CH_ARRAY = 1,
	CH_SPARSE_ARRAY = 2,
	CH_TABLE = 3,
	CH_SPARSE_TABLE = 4,

	CH_TYPE_MASK = 0xf, ///< All ChunkType values have to be within this mask.
	CH_READONLY, ///< Chunk is never saved.
};

/** Handlers and description of chunk. */
struct ChunkHandler {
	uint32_t id;                          ///< Unique ID (4 letters).
	ChunkType type;                     ///< Type of the chunk. @see ChunkType

	ChunkHandler(uint32_t id, ChunkType type) : id(id), type(type) {}

	virtual ~ChunkHandler() = default;

	/**
	 * Save the chunk.
	 * Must be overridden, unless Chunk type is CH_READONLY.
	 */
	virtual void Save() const { NOT_REACHED(); }

	/**
	 * Load the chunk.
	 * Must be overridden.
	 */
	virtual void Load() const = 0;

	/**
	 * Fix the pointers.
	 * Pointers are saved using the index of the pointed object.
	 * On load, pointers are filled with indices and need to be fixed to point to the real object.
	 * Must be overridden if the chunk saves any pointer.
	 */
	virtual void FixPointers() const {}

	/**
	 * Load the chunk for game preview.
	 * Default implementation just skips the data.
	 * @param len Number of bytes to skip.
	 */
	virtual void LoadCheck(size_t len = 0) const;

	std::string GetName() const
	{
		return std::string()
			+ static_cast<char>(this->id >> 24)
			+ static_cast<char>(this->id >> 16)
			+ static_cast<char>(this->id >> 8)
			+ static_cast<char>(this->id);
	}
};

/** A reference to ChunkHandler. */
using ChunkHandlerRef = std::reference_wrapper<const ChunkHandler>;

/** A table of ChunkHandler entries. */
using ChunkHandlerTable = std::span<const ChunkHandlerRef>;

/** A table of SaveLoad entries. */
using SaveLoadTable = std::span<const struct SaveLoad>;

/** A table of SaveLoadCompat entries. */
using SaveLoadCompatTable = std::span<const struct SaveLoadCompat>;

/** Handler for saving/loading an object to/from disk. */
class SaveLoadHandler {
public:
	std::optional<std::vector<SaveLoad>> load_description;

	virtual ~SaveLoadHandler() = default;

	/**
	 * Save the object to disk.
	 * @param object The object to store.
	 */
	virtual void Save([[maybe_unused]] void *object) const {}

	/**
	 * Load the object from disk.
	 * @param object The object to load.
	 */
	virtual void Load([[maybe_unused]] void *object) const {}

	/**
	 * Similar to load, but used only to validate savegames.
	 * @param object The object to load.
	 */
	virtual void LoadCheck([[maybe_unused]] void *object) const {}

	/**
	 * A post-load callback to fix #SL_REF integers into pointers.
	 * @param object The object to fix.
	 */
	virtual void FixPointers([[maybe_unused]] void *object) const {}

	/**
	 * Get the description of the fields in the savegame.
	 */
	virtual SaveLoadTable GetDescription() const = 0;

	/**
	 * Get the pre-header description of the fields in the savegame.
	 */
	virtual SaveLoadCompatTable GetCompatDescription() const = 0;

	/**
	 * Get the description for how to load the chunk. Depending on the
	 * savegame version this can either use the headers in the savegame or
	 * fall back to backwards compatibility and uses hard-coded headers.
	 */
	SaveLoadTable GetLoadDescription() const;
};

/**
 * Default handler for saving/loading an object to/from disk.
 *
 * This handles a few common things for handlers, meaning the actual handler
 * needs less code.
 *
 * Usage: class SlMine : public DefaultSaveLoadHandler<SlMine, MyObject> {}
 *
 * @tparam TImpl The class initializing this template.
 * @tparam TObject The class of the object using this SaveLoadHandler.
 */
template <class TImpl, class TObject>
class DefaultSaveLoadHandler : public SaveLoadHandler {
public:
	SaveLoadTable GetDescription() const override { return static_cast<const TImpl *>(this)->description; }
	SaveLoadCompatTable GetCompatDescription() const override { return static_cast<const TImpl *>(this)->compat_description; }

	virtual void Save([[maybe_unused]] TObject *object) const {}
	void Save(void *object) const override { this->Save(static_cast<TObject *>(object)); }

	virtual void Load([[maybe_unused]] TObject *object) const {}
	void Load(void *object) const override { this->Load(static_cast<TObject *>(object)); }

	virtual void LoadCheck([[maybe_unused]] TObject *object) const {}
	void LoadCheck(void *object) const override { this->LoadCheck(static_cast<TObject *>(object)); }

	virtual void FixPointers([[maybe_unused]] TObject *object) const {}
	void FixPointers(void *object) const override { this->FixPointers(static_cast<TObject *>(object)); }
};

/** Type of reference (#SLE_REF, #SLE_CONDREF). */
enum SLRefType : uint8_t {
	REF_VEHICLE        =  1, ///< Load/save a reference to a vehicle.
	REF_STATION        =  2, ///< Load/save a reference to a station.
	REF_TOWN           =  3, ///< Load/save a reference to a town.
	REF_VEHICLE_OLD    =  4, ///< Load/save an old-style reference to a vehicle (for pre-4.4 savegames).
	REF_ROADSTOPS      =  5, ///< Load/save a reference to a bus/truck stop.
	REF_ENGINE_RENEWS  =  6, ///< Load/save a reference to an engine renewal (autoreplace).
	REF_CARGO_PACKET   =  7, ///< Load/save a reference to a cargo packet.
	REF_ORDERLIST      =  8, ///< Load/save a reference to an orderlist.
	REF_STORAGE        =  9, ///< Load/save a reference to a persistent storage.
	REF_LINK_GRAPH     = 10, ///< Load/save a reference to a link graph.
	REF_LINK_GRAPH_JOB = 11, ///< Load/save a reference to a link graph job.
};

/**
 * VarTypes is the general bitmasked magic type that tells us
 * certain characteristics about the variable it refers to. For example
 * SLE_FILE_* gives the size(type) as it would be in the savegame and
 * SLE_VAR_* the size(type) as it is in memory during runtime. These are
 * the first 8 bits (0-3 SLE_FILE, 4-7 SLE_VAR).
 * Bits 8-15 are reserved for various flags as explained below
 */
enum VarTypes : uint16_t {
	/* 4 bits allocated a maximum of 16 types for NumberType.
	 * NOTE: the SLE_FILE_NNN values are stored in the savegame! */
	SLE_FILE_END      =  0, ///< Used to mark end-of-header in tables.
	SLE_FILE_I8       =  1,
	SLE_FILE_U8       =  2,
	SLE_FILE_I16      =  3,
	SLE_FILE_U16      =  4,
	SLE_FILE_I32      =  5,
	SLE_FILE_U32      =  6,
	SLE_FILE_I64      =  7,
	SLE_FILE_U64      =  8,
	SLE_FILE_STRINGID =  9, ///< StringID offset into strings-array
	SLE_FILE_STRING   = 10,
	SLE_FILE_STRUCT   = 11,
	/* 4 more possible file-primitives */

	SLE_FILE_TYPE_MASK = 0xf, ///< Mask to get the file-type (and not any flags).
	SLE_FILE_HAS_LENGTH_FIELD = 1 << 4, ///< Bit stored in savegame to indicate field has a length field for each entry.

	/* 4 bits allocated a maximum of 16 types for NumberType */
	SLE_VAR_BL    =  0 << 4,
	SLE_VAR_I8    =  1 << 4,
	SLE_VAR_U8    =  2 << 4,
	SLE_VAR_I16   =  3 << 4,
	SLE_VAR_U16   =  4 << 4,
	SLE_VAR_I32   =  5 << 4,
	SLE_VAR_U32   =  6 << 4,
	SLE_VAR_I64   =  7 << 4,
	SLE_VAR_U64   =  8 << 4,
	SLE_VAR_NULL  =  9 << 4, ///< useful to write zeros in savegame.
	SLE_VAR_STR   = 12 << 4, ///< string pointer
	SLE_VAR_STRQ  = 13 << 4, ///< string pointer enclosed in quotes
	SLE_VAR_NAME  = 14 << 4, ///< old custom name to be converted to a string pointer
	/* 1 more possible memory-primitives */

	/* Shortcut values */
	SLE_VAR_CHAR = SLE_VAR_I8,

	/* Default combinations of variables. As savegames change, so can variables
	 * and thus it is possible that the saved value and internal size do not
	 * match and you need to specify custom combo. The defaults are listed here */
	SLE_BOOL         = SLE_FILE_I8  | SLE_VAR_BL,
	SLE_INT8         = SLE_FILE_I8  | SLE_VAR_I8,
	SLE_UINT8        = SLE_FILE_U8  | SLE_VAR_U8,
	SLE_INT16        = SLE_FILE_I16 | SLE_VAR_I16,
	SLE_UINT16       = SLE_FILE_U16 | SLE_VAR_U16,
	SLE_INT32        = SLE_FILE_I32 | SLE_VAR_I32,
	SLE_UINT32       = SLE_FILE_U32 | SLE_VAR_U32,
	SLE_INT64        = SLE_FILE_I64 | SLE_VAR_I64,
	SLE_UINT64       = SLE_FILE_U64 | SLE_VAR_U64,
	SLE_CHAR         = SLE_FILE_I8  | SLE_VAR_CHAR,
	SLE_STRINGID     = SLE_FILE_STRINGID | SLE_VAR_U32,
	SLE_STRING       = SLE_FILE_STRING   | SLE_VAR_STR,
	SLE_STRINGQUOTE  = SLE_FILE_STRING   | SLE_VAR_STRQ,
	SLE_NAME         = SLE_FILE_STRINGID | SLE_VAR_NAME,

	/* Shortcut values */
	SLE_UINT  = SLE_UINT32,
	SLE_INT   = SLE_INT32,
	SLE_STR   = SLE_STRING,
	SLE_STRQ  = SLE_STRINGQUOTE,

	/* 8 bits allocated for a maximum of 8 flags
	 * Flags directing saving/loading of a variable */
	SLF_ALLOW_CONTROL   = 1 << 8, ///< Allow control codes in the strings.
	SLF_ALLOW_NEWLINE   = 1 << 9, ///< Allow new lines in the strings.
	SLF_REPLACE_TABCRLF = 1 << 10, ///< Replace tabs, cr and lf in the string with spaces.
};

typedef uint32_t VarType;

/** Type of data saved. */
enum SaveLoadType : uint8_t {
	SL_VAR         =  0, ///< Save/load a variable.
	SL_REF         =  1, ///< Save/load a reference.
	SL_STRUCT      =  2, ///< Save/load a struct.

	SL_STDSTR      =  4, ///< Save/load a \c std::string.

	SL_ARR         =  5, ///< Save/load a fixed-size array of #SL_VAR elements.
	SL_DEQUE       =  6, ///< Save/load a deque of #SL_VAR elements.
	SL_VECTOR      =  7, ///< Save/load a vector of #SL_VAR elements.
	SL_REFLIST     =  8, ///< Save/load a list of #SL_REF elements.
	SL_STRUCTLIST  =  9, ///< Save/load a list of structs.

	SL_SAVEBYTE    = 10, ///< Save (but not load) a byte.
	SL_NULL        = 11, ///< Save null-bytes and load to nowhere.

	SL_REFVECTOR   = 12, ///< Save/load a vector of #SL_REF elements.
};

typedef void *SaveLoadAddrProc(void *base, size_t extra);

/** SaveLoad type struct. Do NOT use this directly but use the SLE_ macros defined just below! */
struct SaveLoad {
	std::string name;    ///< Name of this field (optional, used for tables).
	SaveLoadType cmd;    ///< The action to take with the saved/loaded type, All types need different action.
	VarType conv;        ///< Type of the variable to be saved; this field combines both FileVarType and MemVarType.
	uint16_t length;       ///< (Conditional) length of the variable (eg. arrays) (max array size is 65536 elements).
	SaveLoadVersion version_from;   ///< Save/load the variable starting from this savegame version.
	SaveLoadVersion version_to;     ///< Save/load the variable before this savegame version.
	SaveLoadAddrProc *address_proc; ///< Callback proc the get the actual variable address in memory.
	size_t extra_data;              ///< Extra data for the callback proc.
	std::shared_ptr<SaveLoadHandler> handler; ///< Custom handler for Save/Load procs.
};

/**
 * SaveLoad information for backwards compatibility.
 *
 * At SLV_SETTINGS_NAME a new method of keeping track of fields in a savegame
 * was added, where the order of fields is no longer important. For older
 * savegames we still need to know the correct order. This struct is the glue
 * to make that happen.
 */
struct SaveLoadCompat {
	std::string name; ///< Name of the field.
	VarTypes null_type; ///< The type associated with the NULL field; defaults to SLE_FILE_U8 to just count bytes.
	uint16_t null_length; ///< Length of the NULL field.
	SaveLoadVersion version_from; ///< Save/load the variable starting from this savegame version.
	SaveLoadVersion version_to; ///< Save/load the variable before this savegame version.
};

/**
 * Get the NumberType of a setting. This describes the integer type
 * as it is represented in memory
 * @param type VarType holding information about the variable-type
 * @return the SLE_VAR_* part of a variable-type description
 */
inline constexpr VarType GetVarMemType(VarType type)
{
	return GB(type, 4, 4) << 4;
}

/**
 * Get the FileType of a setting. This describes the integer type
 * as it is represented in a savegame/file
 * @param type VarType holding information about the file-type
 * @return the SLE_FILE_* part of a variable-type description
 */
inline constexpr VarType GetVarFileType(VarType type)
{
	return GB(type, 0, 4);
}

/**
 * Check if the given saveload type is a numeric type.
 * @param conv the type to check
 * @return True if it's a numeric type.
 */
inline constexpr bool IsNumericType(VarType conv)
{
	return GetVarMemType(conv) <= SLE_VAR_U64;
}

/**
 * Return expect size in bytes of a VarType
 * @param type VarType to get size of.
 * @return size of type in bytes.
 */
inline constexpr size_t SlVarSize(VarType type)
{
	switch (GetVarMemType(type)) {
		case SLE_VAR_BL: return sizeof(bool);
		case SLE_VAR_I8: return sizeof(int8_t);
		case SLE_VAR_U8: return sizeof(uint8_t);
		case SLE_VAR_I16: return sizeof(int16_t);
		case SLE_VAR_U16: return sizeof(uint16_t);
		case SLE_VAR_I32: return sizeof(int32_t);
		case SLE_VAR_U32: return sizeof(uint32_t);
		case SLE_VAR_I64: return sizeof(int64_t);
		case SLE_VAR_U64: return sizeof(uint64_t);
		case SLE_VAR_NULL: return sizeof(void *);
		case SLE_VAR_STR: return sizeof(std::string);
		case SLE_VAR_STRQ: return sizeof(std::string);
		case SLE_VAR_NAME: return sizeof(std::string);
		default: NOT_REACHED();
	}
}

/**
 * Check if a saveload cmd/type/length entry matches the size of the variable.
 * @param cmd SaveLoadType of entry.
 * @param type VarType of entry.
 * @param length Array length of entry.
 * @param size Actual size of variable.
 * @return true iff the sizes match.
 */
inline constexpr bool SlCheckVarSize(SaveLoadType cmd, VarType type, size_t length, size_t size)
{
	switch (cmd) {
		case SL_VAR: return SlVarSize(type) == size;
		case SL_REF: return sizeof(void *) == size;
		case SL_STDSTR: return SlVarSize(type) == size;
		case SL_ARR: return SlVarSize(type) * length <= size; // Partial load of array is permitted.
		case SL_DEQUE: return sizeof(std::deque<void *>) == size;
		case SL_VECTOR: return sizeof(std::vector<void *>) == size;
		case SL_REFLIST: return sizeof(std::list<void *>) == size;
		case SL_REFVECTOR: return sizeof(std::vector<void *>) == size;
		case SL_SAVEBYTE: return true;
		default: NOT_REACHED();
	}
}

/**
 * Storage of simple variables, references (pointers), and arrays.
 * @param cmd      Load/save type. @see SaveLoadType
 * @param name     Field name for table chunks.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 * @param extra    Extra data to pass to the address callback function.
 * @note In general, it is better to use one of the SLE_* macros below.
 */
#define SLE_GENERAL_NAME(cmd, name, base, variable, type, length, from, to, extra) \
	SaveLoad {name, cmd, type, length, from, to, [] (void *b, size_t) -> void * { \
		static_assert(SlCheckVarSize(cmd, type, length, sizeof(static_cast<base *>(b)->variable))); \
		assert(b != nullptr); \
		return const_cast<void *>(static_cast<const void *>(std::addressof(static_cast<base *>(b)->variable))); \
	}, extra, nullptr}

/**
 * Storage of simple variables, references (pointers), and arrays with a custom name.
 * @param cmd      Load/save type. @see SaveLoadType
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 * @param extra    Extra data to pass to the address callback function.
 * @note In general, it is better to use one of the SLE_* macros below.
 */
#define SLE_GENERAL(cmd, base, variable, type, length, from, to, extra) SLE_GENERAL_NAME(cmd, #variable, base, variable, type, length, from, to, extra)

/**
 * Storage of a variable in some savegame versions.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 */
#define SLE_CONDVAR(base, variable, type, from, to) SLE_GENERAL(SL_VAR, base, variable, type, 0, from, to, 0)

/**
 * Storage of a variable in some savegame versions.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param name     Field name for table chunks.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 */
#define SLE_CONDVARNAME(base, variable, name, type, from, to) SLE_GENERAL_NAME(SL_VAR, name, base, variable, type, 0, from, to, 0)

/**
 * Storage of a reference in some savegame versions.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 */
#define SLE_CONDREF(base, variable, type, from, to) SLE_GENERAL(SL_REF, base, variable, type, 0, from, to, 0)

/**
 * Storage of a fixed-size array of #SL_VAR elements in some savegame versions.
 * @param base     Name of the class or struct containing the array.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the array.
 * @param to       Last savegame version that has the array.
 */
#define SLE_CONDARR(base, variable, type, length, from, to) SLE_GENERAL(SL_ARR, base, variable, type, length, from, to, 0)

/**
 * Storage of a fixed-size array of #SL_VAR elements in some savegame versions.
 * @param base     Name of the class or struct containing the array.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param name     Field name for table chunks.
 * @param type     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the array.
 * @param to       Last savegame version that has the array.
 */
#define SLE_CONDARRNAME(base, variable, name, type, length, from, to) SLE_GENERAL_NAME(SL_ARR, name, base, variable, type, length, from, to, 0)

/**
 * Storage of a string in some savegame versions.
 * @param base     Name of the class or struct containing the string.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the string (only used for fixed size buffers).
 * @param from     First savegame version that has the string.
 * @param to       Last savegame version that has the string.
 */
#define SLE_CONDSTR(base, variable, type, length, from, to) SLE_GENERAL(SL_STR, base, variable, type, length, from, to, 0)

/**
 * Storage of a \c std::string in some savegame versions.
 * @param base     Name of the class or struct containing the string.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the string.
 * @param to       Last savegame version that has the string.
 */
#define SLE_CONDSSTR(base, variable, type, from, to) SLE_GENERAL(SL_STDSTR, base, variable, type, 0, from, to, 0)

/**
 * Storage of a \c std::string in some savegame versions.
 * @param base     Name of the class or struct containing the string.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param name     Field name for table chunks.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the string.
 * @param to       Last savegame version that has the string.
 */
#define SLE_CONDSSTRNAME(base, variable, name, type, from, to) SLE_GENERAL_NAME(SL_STDSTR, name, base, variable, type, 0, from, to, 0)

/**
 * Storage of a list of #SL_REF elements in some savegame versions.
 * @param base     Name of the class or struct containing the list.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the list.
 * @param to       Last savegame version that has the list.
 */
#define SLE_CONDREFLIST(base, variable, type, from, to) SLE_GENERAL(SL_REFLIST, base, variable, type, 0, from, to, 0)

/**
 * Storage of a vector of #SL_REF elements in some savegame versions.
 * @param base     Name of the class or struct containing the vector.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the vector.
 * @param to       Last savegame version that has the vector.
 */
#define SLE_CONDREFVECTOR(base, variable, type, from, to) SLE_GENERAL(SL_REFVECTOR, base, variable, type, 0, from, to, 0)

/**
 * Storage of a vector of #SL_VAR elements in some savegame versions.
 * @param base     Name of the class or struct containing the list.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the list.
 * @param to       Last savegame version that has the list.
 */
#define SLE_CONDVECTOR(base, variable, type, from, to) SLE_GENERAL(SL_VECTOR, base, variable, type, 0, from, to, 0)

/**
 * Storage of a deque of #SL_VAR elements in some savegame versions.
 * @param base     Name of the class or struct containing the list.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the list.
 * @param to       Last savegame version that has the list.
 */
#define SLE_CONDDEQUE(base, variable, type, from, to) SLE_GENERAL(SL_DEQUE, base, variable, type, 0, from, to, 0)

/**
 * Storage of a vector of #SL_VAR elements in some savegame versions.
 * @param base     Name of the class or struct containing the list.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the list.
 * @param to       Last savegame version that has the list.
 */
#define SLE_CONDVECTOR(base, variable, type, from, to) SLE_GENERAL(SL_VECTOR, base, variable, type, 0, from, to, 0)

/**
 * Storage of a variable in every version of a savegame.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLE_VAR(base, variable, type) SLE_CONDVAR(base, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a variable in every version of a savegame.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param name     Field name for table chunks.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLE_VARNAME(base, variable, name, type) SLE_CONDVARNAME(base, variable, name, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a reference in every version of a savegame.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Type of the reference, a value from #SLRefType.
 */
#define SLE_REF(base, variable, type) SLE_CONDREF(base, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of fixed-size array of #SL_VAR elements in every version of a savegame.
 * @param base     Name of the class or struct containing the array.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 */
#define SLE_ARR(base, variable, type, length) SLE_CONDARR(base, variable, type, length, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of fixed-size array of #SL_VAR elements in every version of a savegame.
 * @param base     Name of the class or struct containing the array.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param name     Field name for table chunks.
 * @param type     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 */
#define SLE_ARRNAME(base, variable, name, type, length) SLE_CONDARRNAME(base, variable, name, type, length, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a \c std::string in every savegame version.
 * @param base     Name of the class or struct containing the string.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLE_SSTR(base, variable, type) SLE_CONDSSTR(base, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a \c std::string in every savegame version.
 * @param base     Name of the class or struct containing the string.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param name     Field name for table chunks.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLE_SSTRNAME(base, variable, name, type) SLE_CONDSSTRNAME(base, variable, name, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a list of #SL_REF elements in every savegame version.
 * @param base     Name of the class or struct containing the list.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLE_REFLIST(base, variable, type) SLE_CONDREFLIST(base, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a vector of #SL_REF elements in every savegame version.
 * @param base     Name of the class or struct containing the vector.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLE_REFVECTOR(base, variable, type) SLE_CONDREFVECTOR(base, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Only write byte during saving; never read it during loading.
 * When using SLE_SAVEBYTE you will have to read this byte before the table
 * this is in is read. This also means SLE_SAVEBYTE can only be used at the
 * top of a chunk.
 * This is intended to be used to indicate what type of entry this is in a
 * list of entries.
 * @param base     Name of the class or struct containing the variable.
 * @param variable Name of the variable in the class or struct referenced by \a base.
 */
#define SLE_SAVEBYTE(base, variable) SLE_GENERAL(SL_SAVEBYTE, base, variable, 0, 0, SL_MIN_VERSION, SL_MAX_VERSION, 0)

/**
 * Storage of global simple variables, references (pointers), and arrays.
 * @param name     The name of the field.
 * @param cmd      Load/save type. @see SaveLoadType
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 * @param extra    Extra data to pass to the address callback function.
 * @note In general, it is better to use one of the SLEG_* macros below.
 */
#define SLEG_GENERAL(name, cmd, variable, type, length, from, to, extra) \
	SaveLoad {name, cmd, type, length, from, to, [] (void *, size_t) -> void * { \
		static_assert(SlCheckVarSize(cmd, type, length, sizeof(variable))); \
		return static_cast<void *>(std::addressof(variable)); }, extra, nullptr}

/**
 * Storage of a global variable in some savegame versions.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 */
#define SLEG_CONDVAR(name, variable, type, from, to) SLEG_GENERAL(name, SL_VAR, variable, type, 0, from, to, 0)

/**
 * Storage of a global reference in some savegame versions.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field.
 */
#define SLEG_CONDREF(name, variable, type, from, to) SLEG_GENERAL(name, SL_REF, variable, type, 0, from, to, 0)

/**
 * Storage of a global fixed-size array of #SL_VAR elements in some savegame versions.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the array.
 * @param to       Last savegame version that has the array.
 */
#define SLEG_CONDARR(name, variable, type, length, from, to) SLEG_GENERAL(name, SL_ARR, variable, type, length, from, to, 0)

/**
 * Storage of a global \c std::string in some savegame versions.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the string.
 * @param to       Last savegame version that has the string.
 */
#define SLEG_CONDSSTR(name, variable, type, from, to) SLEG_GENERAL(name, SL_STDSTR, variable, type, 0, from, to, 0)

/**
 * Storage of a structs in some savegame versions.
 * @param name     The name of the field.
 * @param handler  SaveLoadHandler for the structs.
 * @param from     First savegame version that has the struct.
 * @param to       Last savegame version that has the struct.
 */
#define SLEG_CONDSTRUCT(name, handler, from, to) SaveLoad {name, SL_STRUCT, 0, 0, from, to, nullptr, 0, std::make_shared<handler>()}

/**
 * Storage of a global reference list in some savegame versions.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the list.
 * @param to       Last savegame version that has the list.
 */
#define SLEG_CONDREFLIST(name, variable, type, from, to) SLEG_GENERAL(name, SL_REFLIST, variable, type, 0, from, to, 0)

/**
 * Storage of a global vector of #SL_VAR elements in some savegame versions.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the list.
 * @param to       Last savegame version that has the list.
 */
#define SLEG_CONDVECTOR(name, variable, type, from, to) SLEG_GENERAL(name, SL_VECTOR, variable, type, 0, from, to, 0)

/**
 * Storage of a list of structs in some savegame versions.
 * @param name     The name of the field.
 * @param handler  SaveLoadHandler for the list of structs.
 * @param from     First savegame version that has the list.
 * @param to       Last savegame version that has the list.
 */
#define SLEG_CONDSTRUCTLIST(name, handler, from, to) SaveLoad {name, SL_STRUCTLIST, 0, 0, from, to, nullptr, 0, std::make_shared<handler>()}

/**
 * Storage of a global variable in every savegame version.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLEG_VAR(name, variable, type) SLEG_CONDVAR(name, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a global reference in every savegame version.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLEG_REF(name, variable, type) SLEG_CONDREF(name, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a global fixed-size array of #SL_VAR elements in every savegame version.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 */
#define SLEG_ARR(name, variable, type, length) SLEG_CONDARR(name, variable, type, length, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a global \c std::string in every savegame version.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLEG_SSTR(name, variable, type) SLEG_CONDSSTR(name, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a structs in every savegame version.
 * @param name     The name of the field.
 * @param handler SaveLoadHandler for the structs.
 */
#define SLEG_STRUCT(name, handler) SLEG_CONDSTRUCT(name, handler, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a global reference list in every savegame version.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLEG_REFLIST(name, variable, type) SLEG_CONDREFLIST(name, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a global vector of #SL_VAR elements in every savegame version.
 * @param name     The name of the field.
 * @param variable Name of the global variable.
 * @param type     Storage of the data in memory and in the savegame.
 */
#define SLEG_VECTOR(name, variable, type) SLEG_CONDVECTOR(name, variable, type, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Storage of a list of structs in every savegame version.
 * @param name    The name of the field.
 * @param handler SaveLoadHandler for the list of structs.
 */
#define SLEG_STRUCTLIST(name, handler) SLEG_CONDSTRUCTLIST(name, handler, SL_MIN_VERSION, SL_MAX_VERSION)

/**
 * Field name where the real SaveLoad can be located.
 * @param name The name of the field.
 */
#define SLC_VAR(name) {name, SLE_FILE_U8, 0, SL_MIN_VERSION, SL_MAX_VERSION}

/**
 * Empty space in every savegame version.
 * @param length Length of the empty space in bytes.
 * @param from   First savegame version that has the empty space.
 * @param to     Last savegame version that has the empty space.
 */
#define SLC_NULL(length, from, to) {{}, SLE_FILE_U8, length, from, to}

/**
 * Checks whether the savegame is below \a major.\a minor.
 * @param major Major number of the version to check against.
 * @param minor Minor number of the version to check against. If \a minor is 0 or not specified, only the major number is checked.
 * @return Savegame version is earlier than the specified version.
 */
inline bool IsSavegameVersionBefore(SaveLoadVersion major, uint8_t minor = 0)
{
	extern SaveLoadVersion _sl_version;
	extern uint8_t            _sl_minor_version;
	return _sl_version < major || (minor > 0 && _sl_version == major && _sl_minor_version < minor);
}

/**
 * Checks whether the savegame is below or at \a major. This should be used to repair data from existing
 * savegames which is no longer corrupted in new savegames, but for which otherwise no savegame
 * bump is required.
 * @param major Major number of the version to check against.
 * @return Savegame version is at most the specified version.
 */
inline bool IsSavegameVersionBeforeOrAt(SaveLoadVersion major)
{
	extern SaveLoadVersion _sl_version;
	return _sl_version <= major;
}

/**
 * Checks if some version from/to combination falls within the range of the
 * active savegame version.
 * @param version_from Inclusive savegame version lower bound.
 * @param version_to   Exclusive savegame version upper bound. SL_MAX_VERSION if no upper bound.
 * @return Active savegame version falls within the given range.
 */
inline bool SlIsObjectCurrentlyValid(SaveLoadVersion version_from, SaveLoadVersion version_to)
{
	extern const SaveLoadVersion SAVEGAME_VERSION;
	return version_from <= SAVEGAME_VERSION && SAVEGAME_VERSION < version_to;
}

/**
 * Get the address of the variable. Null-variables don't have an address,
 * everything else has a callback function that returns the address based
 * on the saveload data and the current object for non-globals.
 */
inline void *GetVariableAddress(const void *object, const SaveLoad &sld)
{
	/* Entry is a null-variable, mostly used to read old savegames etc. */
	if (GetVarMemType(sld.conv) == SLE_VAR_NULL) {
		assert(sld.address_proc == nullptr);
		return nullptr;
	}

	/* Everything else should be a non-null pointer. */
	assert(sld.address_proc != nullptr);
	return sld.address_proc(const_cast<void *>(object), sld.extra_data);
}

int64_t ReadValue(const void *ptr, VarType conv);
void WriteValue(void *ptr, VarType conv, int64_t val);

void SlSetArrayIndex(uint index);
static void SlSetArrayIndex(const ConvertibleThroughBase auto &index) { SlSetArrayIndex(index.base()); }
int SlIterateArray();

void SlSetStructListLength(size_t length);
size_t SlGetStructListLength(size_t limit);

void SlAutolength(AutolengthProc *proc, int arg);
size_t SlGetFieldLength();
void SlSetLength(size_t length);
size_t SlCalcObjMemberLength(const void *object, const SaveLoad &sld);
size_t SlCalcObjLength(const void *object, const SaveLoadTable &slt);

uint8_t SlReadByte();
void SlReadString(std::string &str, size_t length);
void SlWriteByte(uint8_t b);

void SlGlobList(const SaveLoadTable &slt);
void SlCopy(void *object, size_t length, VarType conv);
std::vector<SaveLoad> SlTableHeader(const SaveLoadTable &slt);
std::vector<SaveLoad> SlCompatTableHeader(const SaveLoadTable &slt, const SaveLoadCompatTable &slct);
void SlObject(void *object, const SaveLoadTable &slt);

bool SaveloadCrashWithMissingNewGRFs();

/**
 * Read in bytes from the file/data structure but don't do
 * anything with them, discarding them in effect
 * @param length The amount of bytes that is being treated this way
 */
inline void SlSkipBytes(size_t length)
{
	for (; length != 0; length--) SlReadByte();
}

extern std::string _savegame_format;
extern bool _do_autosave;

/**
 * Default handler for saving/loading a vector to/from disk.
 *
 * This handles a few common things for handlers, meaning the actual handler
 * needs less code.
 *
 * @tparam TImpl The class initializing this template.
 * @tparam TObject The class of the object using this SaveLoadHandler.
 * @tparam TElementType The type of the elements contained within the vector.
 * @tparam MAX_LENGTH maximum number of elements to load.
 */
template <class TImpl, class TObject, class TElementType, size_t MAX_LENGTH = UINT32_MAX>
class VectorSaveLoadHandler : public DefaultSaveLoadHandler<TImpl, TObject> {
public:
	/**
	 * Get instance of vector to load/save.
	 * @param object Object containing vector.
	 * @returns Vector to load/save.
	 */
	virtual std::vector<TElementType> &GetVector(TObject *object) const = 0;

	/**
	 * Get number of elements to load into vector.
	 * @returns Number of elements to load into the vector.
	 * @note This is only overridden if the number of elements comes from a different location due to savegame changes.
	 */
	virtual size_t GetLength() const { return SlGetStructListLength(MAX_LENGTH); }

	void Save(TObject *object) const override
	{
		auto &vector = this->GetVector(object);
		SlSetStructListLength(vector.size());

		for (auto &item : vector) {
			SlObject(&item, this->GetDescription());
		}
	}

	void Load(TObject *object) const override
	{
		auto &vector = this->GetVector(object);
		size_t count = this->GetLength();

		vector.reserve(count);
		while (count-- > 0) {
			auto &item = vector.emplace_back();
			SlObject(&item, this->GetLoadDescription());
		}
	}
};

#endif /* SAVELOAD_H */
