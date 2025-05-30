/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ini_type.h Types related to reading/writing '*.ini' files. */

#ifndef INI_TYPE_H
#define INI_TYPE_H

#include "fileio_type.h"

/** Types of groups */
enum IniGroupType : uint8_t {
	IGT_VARIABLES = 0, ///< Values of the form "landscape = hilly".
	IGT_LIST      = 1, ///< A list of values, separated by \n and terminated by the next group block.
	IGT_SEQUENCE  = 2, ///< A list of uninterpreted lines, terminated by the next group block.
};

/** A single "line" in an ini file. */
struct IniItem {
	std::string name;                 ///< The name of this item
	std::optional<std::string> value; ///< The value of this item
	std::string comment;              ///< The comment associated with this item

	IniItem(std::string_view name);

	void SetValue(std::string_view value);
};

/** A group within an ini file. */
struct IniGroup {
	std::list<IniItem> items; ///< all items in the group
	IniGroupType type;   ///< type of group
	std::string name;    ///< name of group
	std::string comment; ///< comment for group

	IniGroup(std::string_view name, IniGroupType type);

	const IniItem *GetItem(std::string_view name) const;
	IniItem &GetOrCreateItem(std::string_view name);
	IniItem &CreateItem(std::string_view name);
	void RemoveItem(std::string_view name);
	void Clear();
};

/** Ini file that only supports loading. */
struct IniLoadFile {
	using IniGroupNameList = std::initializer_list<const std::string_view>;

	std::list<IniGroup> groups; ///< all groups in the ini
	std::string comment;                  ///< last comment in file
	const IniGroupNameList list_group_names; ///< list of group names that are lists
	const IniGroupNameList seq_group_names;  ///< list of group names that are sequences.

	IniLoadFile(const IniGroupNameList &list_group_names = {}, const IniGroupNameList &seq_group_names = {});
	virtual ~IniLoadFile() = default;

	const IniGroup *GetGroup(std::string_view name) const;
	IniGroup *GetGroup(std::string_view name);
	IniGroup &GetOrCreateGroup(std::string_view name);
	IniGroup &CreateGroup(std::string_view name);
	void RemoveGroup(std::string_view name);

	void LoadFromDisk(std::string_view filename, Subdirectory subdir);

	/**
	 * Open the INI file.
	 * @param filename Name of the INI file.
	 * @param subdir The subdir to load the file from.
	 * @param[out] size Size of the opened file.
	 * @return File handle of the opened file, or \c std::nullopt.
	 */
	virtual std::optional<FileHandle> OpenFile(std::string_view filename, Subdirectory subdir, size_t *size) = 0;

	/**
	 * Report an error about the file contents.
	 * @param message The message to show.
	 */
	virtual void ReportFileError(std::string_view message) = 0;
};

/** Ini file that supports both loading and saving. */
struct IniFile : IniLoadFile {
	IniFile(const IniGroupNameList &list_group_names = {});

	bool SaveToDisk(const std::string &filename);

	std::optional<FileHandle> OpenFile(std::string_view filename, Subdirectory subdir, size_t *size) override;
	void ReportFileError(std::string_view message) override;
};

#endif /* INI_TYPE_H */
