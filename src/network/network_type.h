/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file network_type.h Types used for networking. */

#ifndef NETWORK_TYPE_H
#define NETWORK_TYPE_H

#include "../core/enum_type.hpp"
#include "../core/pool_type.hpp"

/** How many clients can we have */
static const uint MAX_CLIENTS = 255;

/**
 * Vehicletypes in the order they are send in info packets.
 */
enum NetworkVehicleType : uint8_t {
	NETWORK_VEH_TRAIN = 0,
	NETWORK_VEH_LORRY,
	NETWORK_VEH_BUS,
	NETWORK_VEH_PLANE,
	NETWORK_VEH_SHIP,

	NETWORK_VEH_END
};

/**
 * Game type the server can be using.
 * Used on the network protocol to communicate with Game Coordinator.
 */
enum ServerGameType : uint8_t {
	SERVER_GAME_TYPE_LOCAL = 0,
	SERVER_GAME_TYPE_PUBLIC,
	SERVER_GAME_TYPE_INVITE_ONLY,
};

/** 'Unique' identifier to be given to clients */
enum ClientID : uint32_t {
	INVALID_CLIENT_ID = 0, ///< Client is not part of anything
	CLIENT_ID_SERVER  = 1, ///< Servers always have this ID
	CLIENT_ID_FIRST   = 2, ///< The first client ID
};

/** Indices into the client related pools */
using ClientPoolID = PoolID<uint16_t, struct ClientPoolIDTag, MAX_CLIENTS + 1 /* dedicated server. */, 0xFFFF>;

/** Indices into the admin tables. */
using AdminID = PoolID<uint8_t, struct AdminIDTag, 16, 0xFF>;

/** Simple calculated statistics of a company */
struct NetworkCompanyStats {
	uint16_t num_vehicle[NETWORK_VEH_END];            ///< How many vehicles are there of this type?
	uint16_t num_station[NETWORK_VEH_END];            ///< How many stations are there of this type?
	bool ai;                                        ///< Is this company an AI
};

struct NetworkClientInfo;

/**
 * Destination of our chat messages.
 * @warning The values of the enum items are part of the admin network API. Only append at the end.
 */
enum DestType : uint8_t {
	DESTTYPE_BROADCAST, ///< Send message/notice to all clients (All)
	DESTTYPE_TEAM,      ///< Send message/notice to everyone playing the same company (Team)
	DESTTYPE_CLIENT,    ///< Send message/notice to only a certain client (Private)
};
DECLARE_ENUM_AS_ADDABLE(DestType)

/**
 * Actions that can be used for NetworkTextMessage.
 * @warning The values of the enum items are part of the admin network API. Only append at the end.
 */
enum NetworkAction : uint8_t {
	NETWORK_ACTION_JOIN,
	NETWORK_ACTION_LEAVE,
	NETWORK_ACTION_SERVER_MESSAGE,
	NETWORK_ACTION_CHAT,
	NETWORK_ACTION_CHAT_COMPANY,
	NETWORK_ACTION_CHAT_CLIENT,
	NETWORK_ACTION_GIVE_MONEY,
	NETWORK_ACTION_NAME_CHANGE,
	NETWORK_ACTION_COMPANY_SPECTATOR,
	NETWORK_ACTION_COMPANY_JOIN,
	NETWORK_ACTION_COMPANY_NEW,
	NETWORK_ACTION_KICKED,
	NETWORK_ACTION_EXTERNAL_CHAT,
};

/**
 * The error codes we send around in the protocols.
 * @warning The values of the enum items are part of the admin network API. Only append at the end.
 */
enum NetworkErrorCode : uint8_t {
	NETWORK_ERROR_GENERAL, // Try to use this one like never

	/* Signals from clients */
	NETWORK_ERROR_DESYNC,
	NETWORK_ERROR_SAVEGAME_FAILED,
	NETWORK_ERROR_CONNECTION_LOST,
	NETWORK_ERROR_ILLEGAL_PACKET,
	NETWORK_ERROR_NEWGRF_MISMATCH,

	/* Signals from servers */
	NETWORK_ERROR_NOT_AUTHORIZED,
	NETWORK_ERROR_NOT_EXPECTED,
	NETWORK_ERROR_WRONG_REVISION,
	NETWORK_ERROR_NAME_IN_USE,
	NETWORK_ERROR_WRONG_PASSWORD,
	NETWORK_ERROR_COMPANY_MISMATCH, // Happens in CLIENT_COMMAND
	NETWORK_ERROR_KICKED,
	NETWORK_ERROR_CHEATER,
	NETWORK_ERROR_FULL,
	NETWORK_ERROR_TOO_MANY_COMMANDS,
	NETWORK_ERROR_TIMEOUT_PASSWORD,
	NETWORK_ERROR_TIMEOUT_COMPUTER,
	NETWORK_ERROR_TIMEOUT_MAP,
	NETWORK_ERROR_TIMEOUT_JOIN,
	NETWORK_ERROR_INVALID_CLIENT_NAME,
	NETWORK_ERROR_NOT_ON_ALLOW_LIST,
	NETWORK_ERROR_NO_AUTHENTICATION_METHOD_AVAILABLE,

	NETWORK_ERROR_END,
};

/**
 * Simple helper to (more easily) manage authorized keys.
 *
 * The authorized keys are hexadecimal representations of their binary form.
 * The authorized keys are case insensitive.
 */
class NetworkAuthorizedKeys : public std::vector<std::string> {
public:
	bool Contains(std::string_view key) const;
	bool Add(std::string_view key);
	bool Remove(std::string_view key);
};

#endif /* NETWORK_TYPE_H */
