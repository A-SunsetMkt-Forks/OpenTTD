/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file game_core.cpp Implementation of Game. */

#include "../stdafx.h"
#include "../core/backup_type.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../network/network.h"
#include "../window_func.h"
#include "../framerate_type.h"
#include "game.hpp"
#include "game_scanner.hpp"
#include "game_config.hpp"
#include "game_instance.hpp"
#include "game_info.hpp"

#include "../safeguards.h"

/* static */ uint Game::frame_counter = 0;
/* static */ GameInfo *Game::info = nullptr;
/* static */ std::unique_ptr<GameInstance> Game::instance = nullptr;
/* static */ std::unique_ptr<GameScannerInfo> Game::scanner_info = nullptr;
/* static */ std::unique_ptr<GameScannerLibrary> Game::scanner_library = nullptr;

/* static */ void Game::GameLoop()
{
	if (_networking && !_network_server) {
		PerformanceMeasurer::SetInactive(PFE_GAMESCRIPT);
		return;
	}
	if (Game::instance == nullptr) {
		PerformanceMeasurer::SetInactive(PFE_GAMESCRIPT);
		return;
	}

	PerformanceMeasurer framerate(PFE_GAMESCRIPT);

	Game::frame_counter++;

	Backup<CompanyID> cur_company(_current_company);
	cur_company.Change(OWNER_DEITY);
	Game::instance->GameLoop();
	cur_company.Restore();

	/* Occasionally collect garbage */
	if ((Game::frame_counter & 255) == 0) {
		Game::instance->CollectGarbage();
	}
}

/* static */ void Game::Initialize()
{
	if (Game::instance != nullptr) Game::Uninitialize(true);

	Game::frame_counter = 0;

	if (Game::scanner_info == nullptr) {
		TarScanner::DoScan(TarScanner::Mode::Game);
		Game::scanner_info = std::make_unique<GameScannerInfo>();
		Game::scanner_info->Initialize();
		Game::scanner_library = std::make_unique<GameScannerLibrary>();
		Game::scanner_library->Initialize();
	}
}

/* static */ void Game::StartNew()
{
	if (Game::instance != nullptr) return;

	/* Don't start GameScripts in intro */
	if (_game_mode == GM_MENU) return;

	/* Clients shouldn't start GameScripts */
	if (_networking && !_network_server) return;

	GameConfig *config = GameConfig::GetConfig(GameConfig::SSS_FORCE_GAME);
	GameInfo *info = config->GetInfo();
	if (info == nullptr) return;

	config->AnchorUnchangeableSettings();

	Backup<CompanyID> cur_company(_current_company);
	cur_company.Change(OWNER_DEITY);

	Game::info = info;
	Game::instance = std::make_unique<GameInstance>();
	Game::instance->Initialize(info);
	Game::instance->LoadOnStack(config->GetToLoadData());
	config->SetToLoadData(nullptr);

	cur_company.Restore();

	InvalidateWindowClassesData(WC_SCRIPT_DEBUG, -1);
}

/* static */ void Game::Uninitialize(bool keepConfig)
{
	Backup<CompanyID> cur_company(_current_company);

	Game::ResetInstance();

	cur_company.Restore();

	if (keepConfig) {
		Rescan();
	} else {
		Game::scanner_info.reset();
		Game::scanner_library.reset();

		_settings_game.script_config.game.reset();
		_settings_newgame.script_config.game.reset();
	}
}

/* static */ void Game::Pause()
{
	if (Game::instance != nullptr) Game::instance->Pause();
}

/* static */ void Game::Unpause()
{
	if (Game::instance != nullptr) Game::instance->Unpause();
}

/* static */ bool Game::IsPaused()
{
	return Game::instance != nullptr? Game::instance->IsPaused() : false;
}

/* static */ void Game::NewEvent(ScriptEvent *event)
{
	ScriptObjectRef counter(event);

	/* Clients should ignore events */
	if (_networking && !_network_server) {
		return;
	}

	/* Check if Game instance is alive */
	if (Game::instance == nullptr) {
		return;
	}

	/* Queue the event */
	Backup<CompanyID> cur_company(_current_company, OWNER_DEITY);
	Game::instance->InsertEvent(event);
	cur_company.Restore();
}

/* static */ void Game::ResetConfig()
{
	/* Check for both newgame as current game if we can reload the GameInfo inside
	 *  the GameConfig. If not, remove the Game from the list. */
	if (_settings_game.script_config.game != nullptr && _settings_game.script_config.game->HasScript()) {
		if (!_settings_game.script_config.game->ResetInfo(true)) {
			Debug(script, 0, "After a reload, the GameScript by the name '{}' was no longer found, and removed from the list.", _settings_game.script_config.game->GetName());
			_settings_game.script_config.game->Change(std::nullopt);
			if (Game::instance != nullptr) Game::ResetInstance();
		} else if (Game::instance != nullptr) {
			Game::info = _settings_game.script_config.game->GetInfo();
		}
	}
	if (_settings_newgame.script_config.game != nullptr && _settings_newgame.script_config.game->HasScript()) {
		if (!_settings_newgame.script_config.game->ResetInfo(false)) {
			Debug(script, 0, "After a reload, the GameScript by the name '{}' was no longer found, and removed from the list.", _settings_newgame.script_config.game->GetName());
			_settings_newgame.script_config.game->Change(std::nullopt);
		}
	}
}

/* static */ void Game::Rescan()
{
	TarScanner::DoScan(TarScanner::Mode::Game);

	Game::scanner_info->RescanDir();
	Game::scanner_library->RescanDir();
	ResetConfig();

	InvalidateWindowData(WC_SCRIPT_LIST, 0, 1);
	SetWindowClassesDirty(WC_SCRIPT_DEBUG);
	InvalidateWindowClassesData(WC_SCRIPT_SETTINGS);
	InvalidateWindowClassesData(WC_GAME_OPTIONS);
}


/* static */ void Game::Save()
{
	if (Game::instance != nullptr && (!_networking || _network_server)) {
		Backup<CompanyID> cur_company(_current_company, OWNER_DEITY);
		Game::instance->Save();
		cur_company.Restore();
	} else {
		GameInstance::SaveEmpty();
	}
}

/* static */ void Game::GetConsoleList(std::back_insert_iterator<std::string> &output_iterator, bool newest_only)
{
	Game::scanner_info->GetConsoleList(output_iterator, newest_only);
}

/* static */ void Game::GetConsoleLibraryList(std::back_insert_iterator<std::string> &output_iterator)
{
	Game::scanner_library->GetConsoleList(output_iterator, true);
}

/* static */ const ScriptInfoList *Game::GetInfoList()
{
	return Game::scanner_info->GetInfoList();
}

/* static */ const ScriptInfoList *Game::GetUniqueInfoList()
{
	return Game::scanner_info->GetUniqueInfoList();
}

/* static */ GameInfo *Game::FindInfo(const std::string &name, int version, bool force_exact_match)
{
	return Game::scanner_info->FindInfo(name, version, force_exact_match);
}

/* static */ GameLibrary *Game::FindLibrary(const std::string &library, int version)
{
	return Game::scanner_library->FindLibrary(library, version);
}

/* static */ void Game::ResetInstance()
{
	Game::instance.reset();
	Game::info = nullptr;
}

/**
 * Check whether we have an Game (library) with the exact characteristics as ci.
 * @param ci the characteristics to search on (shortname and md5sum)
 * @param md5sum whether to check the MD5 checksum
 * @return true iff we have an Game (library) matching.
 */
/* static */ bool Game::HasGame(const ContentInfo &ci, bool md5sum)
{
	return Game::scanner_info->HasScript(ci, md5sum);
}

/* static */ bool Game::HasGameLibrary(const ContentInfo &ci, bool md5sum)
{
	return Game::scanner_library->HasScript(ci, md5sum);
}

/* static */ GameScannerInfo *Game::GetScannerInfo()
{
	return Game::scanner_info.get();
}
/* static */ GameScannerLibrary *Game::GetScannerLibrary()
{
	return Game::scanner_library.get();
}
