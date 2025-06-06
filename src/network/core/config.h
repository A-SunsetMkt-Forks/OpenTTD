/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file config.h Configuration options of the network stuff. It is used even when compiling without network support.
 */

#ifndef NETWORK_CORE_CONFIG_H
#define NETWORK_CORE_CONFIG_H

std::string_view NetworkCoordinatorConnectionString();
std::string_view NetworkStunConnectionString();
std::string_view NetworkContentServerConnectionString();
std::string_view NetworkContentMirrorUriString();
std::string_view NetworkSurveyUriString();

static const uint16_t NETWORK_COORDINATOR_SERVER_PORT = 3976;           ///< The default port of the Game Coordinator server (TCP)
static const uint16_t NETWORK_STUN_SERVER_PORT        = 3975;           ///< The default port of the STUN server (TCP)
static const uint16_t NETWORK_TURN_SERVER_PORT        = 3974;           ///< The default port of the TURN server (TCP)
static const uint16_t NETWORK_CONTENT_SERVER_PORT     = 3978;           ///< The default port of the content server (TCP)
static const uint16_t NETWORK_DEFAULT_PORT            = 3979;           ///< The default port of the game server (TCP & UDP)
static const uint16_t NETWORK_ADMIN_PORT              = 3977;           ///< The default port for admin network

static const size_t UDP_MTU = 1460; ///< Number of bytes we can pack in a single UDP packet

static const std::string NETWORK_SURVEY_DETAILS_LINK = "https://survey.openttd.org/participate"; ///< Link with more details & privacy statement of the survey.
/*
 * Technically a TCP packet could become 64kiB, however the high bit is kept so it becomes possible in the future
 * to go to (significantly) larger packets if needed. This would entail a strategy such as employed for UTF-8.
 *
 * Packets up to 32 KiB have the high bit not set:
 * 00000000 00000000 0bbbbbbb aaaaaaaa -> aaaaaaaa 0bbbbbbb
 * Send_uint16(GB(size, 0, 15)
 *
 * Packets up to 1 GiB, first uint16_t has high bit set so it knows to read a
 * next uint16_t for the remaining bits of the size.
 * 00dddddd cccccccc bbbbbbbb aaaaaaaa -> cccccccc 10dddddd aaaaaaaa bbbbbbbb
 * Send_uint16(GB(size, 16, 14) | 0b10 << 14)
 * Send_uint16(GB(size,  0, 16))
 */
static const size_t TCP_MTU = 32767; ///< Number of bytes we can pack in a single TCP packet
static const size_t COMPAT_MTU = 1460; ///< Number of bytes we can pack in a single packet for backward compatibility

static const uint8_t NETWORK_GAME_ADMIN_VERSION        =    3;           ///< What version of the admin network do we use?
static const uint8_t NETWORK_GAME_INFO_VERSION         =    7;           ///< What version of game-info do we use?
static const uint8_t NETWORK_COORDINATOR_VERSION       =    6;           ///< What version of game-coordinator-protocol do we use?
static const uint8_t NETWORK_SURVEY_VERSION            =    2;           ///< What version of the survey do we use?

static const uint NETWORK_NAME_LENGTH               =   80;           ///< The maximum length of the server name and map name, in bytes including '\0'
static const uint NETWORK_HOSTNAME_LENGTH           =   80;           ///< The maximum length of the host name, in bytes including '\0'
static const uint NETWORK_HOSTNAME_PORT_LENGTH      =   80 + 6;       ///< The maximum length of the host name + port, in bytes including '\0'. The extra six is ":" + port number (with a max of 65536)
static const uint NETWORK_REVISION_LENGTH           =   33;           ///< The maximum length of the revision, in bytes including '\0'
static const uint NETWORK_PASSWORD_LENGTH           =   33;           ///< The maximum length of the password, in bytes including '\0'
static const uint NETWORK_CLIENT_NAME_LENGTH        =   25;           ///< The maximum length of a client's name, in bytes including '\0'
static const uint NETWORK_RCONCOMMAND_LENGTH        =  500;           ///< The maximum length of a rconsole command, in bytes including '\0'
static const uint NETWORK_GAMESCRIPT_JSON_LENGTH    = 9000;           ///< The maximum length of a receiving gamescript json string, in bytes including '\0'.
static const uint NETWORK_CHAT_LENGTH               =  900;           ///< The maximum length of a chat message, in bytes including '\0'
static const uint NETWORK_CONTENT_FILENAME_LENGTH   =   48;           ///< The maximum length of a content's filename, in bytes including '\0'.
static const uint NETWORK_CONTENT_NAME_LENGTH       =   32;           ///< The maximum length of a content's name, in bytes including '\0'.
static const uint NETWORK_CONTENT_VERSION_LENGTH    =   16;           ///< The maximum length of a content's version, in bytes including '\0'.
static const uint NETWORK_CONTENT_URL_LENGTH        =   96;           ///< The maximum length of a content's url, in bytes including '\0'.
static const uint NETWORK_CONTENT_DESC_LENGTH       =  512;           ///< The maximum length of a content's description, in bytes including '\0'.
static const uint NETWORK_CONTENT_TAG_LENGTH        =   32;           ///< The maximum length of a content's tag, in bytes including '\0'.
static const uint NETWORK_ERROR_DETAIL_LENGTH       =  100;           ///< The maximum length of the error detail, in bytes including '\0'.
static const uint NETWORK_INVITE_CODE_LENGTH        =   64;           ///< The maximum length of the invite code, in bytes including '\0'.
static const uint NETWORK_INVITE_CODE_SECRET_LENGTH =   80;           ///< The maximum length of the invite code secret, in bytes including '\0'.
static const uint NETWORK_TOKEN_LENGTH              =   64;           ///< The maximum length of a token, in bytes including '\0'.

static const uint NETWORK_GRF_NAME_LENGTH           =   80;           ///< Maximum length of the name of a GRF

/**
 * Maximum number of GRFs that can be sent.
 *
 * This limit exists to avoid that the SERVER_INFO packet exceeding the
 * maximum MTU. At the time of writing this limit is 32767 (TCP_MTU).
 *
 * In the SERVER_INFO packet is the NetworkGameInfo struct, which is
 * 142 bytes + 100 per NewGRF (under the assumption strings are used to
 * their max). This brings us to roughly 326 possible NewGRFs. Round it
 * down so people don't freak out because they see a weird value, and you
 * get the limit: 255.
 *
 * PS: in case you ever want to raise this number, please be mindful that
 * "amount of NewGRFs" in NetworkGameInfo is currently an uint8.
 */
static const uint NETWORK_MAX_GRF_COUNT             =   255;

/**
 * The maximum length of the hexadecimal encoded secret keys, in bytes including '\0'.
 * This is related to \c X25519_KEY_SIZE in the network crypto internals.
 */
static const uint NETWORK_SECRET_KEY_LENGTH = 32 * 2 + 1;
/**
 * The maximum length of the hexadecimal encoded public keys, in bytes including '\0'.
 * This is related to \c X25519_KEY_SIZE in the network crypto internals.
 */
static const uint NETWORK_PUBLIC_KEY_LENGTH = 32 * 2 + 1;

#endif /* NETWORK_CORE_CONFIG_H */
