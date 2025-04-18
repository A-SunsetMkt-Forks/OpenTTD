/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ai.hpp Base functions for all AIs. */

#ifndef AI_HPP
#define AI_HPP

#include "../script/api/script_event_types.hpp"
#include "ai_scanner.hpp"

/**
 * Main AI class. Contains all functions needed to start, stop, save and load AIs.
 */
class AI {
public:
	/**
	 * Is it possible to start a new AI company?
	 * @return True if a new AI company can be started.
	 */
	static bool CanStartNew();

	/**
	 * Start a new AI company.
	 * @param company At which slot the AI company should start.
	 */
	static void StartNew(CompanyID company);

	/**
	 * Called every game-tick to let AIs do something.
	 */
	static void GameLoop();

	/**
	 * Get the current AI tick.
	 */
	static uint GetTick();

	/**
	 * Stop a company to be controlled by an AI.
	 * @param company The company from which the AI needs to detach.
	 * @pre Company::IsValidAiID(company)
	 */
	static void Stop(CompanyID company);

	/**
	 * Suspend the AI and then pause execution of the script. The script
	 * will not be resumed from its suspended state until the script has
	 * been unpaused.
	 * @param company The company for which the AI should be paused.
	 * @pre Company::IsValidAiID(company)
	 */
	static void Pause(CompanyID company);

	/**
	 * Resume execution of the AI. This function will not actually execute
	 * the script, but set a flag so that the script is executed my the usual
	 * mechanism that executes the script.
	 * @param company The company for which the AI should be unpaused.
	 * @pre Company::IsValidAiID(company)
	 */
	static void Unpause(CompanyID company);

	/**
	 * Checks if the AI is paused.
	 * @param company The company for which to check if the AI is paused.
	 * @pre Company::IsValidAiID(company)
	 * @return true if the AI is paused, otherwise false.
	 */
	static bool IsPaused(CompanyID company);

	/**
	 * Kill any and all AIs we manage.
	 */
	static void KillAll();

	/**
	 * Initialize the AI system.
	 */
	static void Initialize();

	/**
	 * Uninitialize the AI system
	 * @param keepConfig Should we keep AIConfigs, or can we free that memory?
	 */
	static void Uninitialize(bool keepConfig);

	/**
	 * Reset all AIConfigs, and make them reload their AIInfo.
	 * If the AIInfo could no longer be found, an error is reported to the user.
	 */
	static void ResetConfig();

	/**
	 * Queue a new event for an AI.
	 */
	static void NewEvent(CompanyID company, ScriptEvent *event);

	/**
	 * Broadcast a new event to all active AIs.
	 */
	static void BroadcastNewEvent(ScriptEvent *event, CompanyID skip_company = CompanyID::Invalid());

	/**
	 * Save data from an AI to a savegame.
	 */
	static void Save(CompanyID company);

	/** Wrapper function for AIScanner::GetAIConsoleList */
	static void GetConsoleList(std::back_insert_iterator<std::string> &output_iterator, bool newest_only);
	/** Wrapper function for AIScanner::GetAIConsoleLibraryList */
	static void GetConsoleLibraryList(std::back_insert_iterator<std::string> &output_iterator);
	/** Wrapper function for AIScanner::GetAIInfoList */
	static const ScriptInfoList *GetInfoList();
	/** Wrapper function for AIScanner::GetUniqueAIInfoList */
	static const ScriptInfoList *GetUniqueInfoList();
	/** Wrapper function for AIScanner::FindInfo */
	static class AIInfo *FindInfo(const std::string &name, int version, bool force_exact_match);
	/** Wrapper function for AIScanner::FindLibrary */
	static class AILibrary *FindLibrary(const std::string &library, int version);

	/**
	 * Rescans all searchpaths for available AIs. If a used AI is no longer
	 * found it is removed from the config.
	 */
	static void Rescan();

	/** Gets the ScriptScanner instance that is used to find AIs */
	static AIScannerInfo *GetScannerInfo();
	/** Gets the ScriptScanner instance that is used to find AI Libraries */
	static AIScannerLibrary *GetScannerLibrary();

	/** Wrapper function for AIScanner::HasAI */
	static bool HasAI(const ContentInfo &ci, bool md5sum);
	static bool HasAILibrary(const ContentInfo &ci, bool md5sum);
private:
	static uint frame_counter; ///< Tick counter for the AI code
	static std::unique_ptr<AIScannerInfo> scanner_info; ///< ScriptScanner instance that is used to find AIs
	static std::unique_ptr<AIScannerLibrary> scanner_library; ///< ScriptScanner instance that is used to find AI Libraries
};

#endif /* AI_HPP */
