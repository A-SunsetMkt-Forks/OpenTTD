/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file win32_s.h Base for Windows sound handling. */

#ifndef SOUND_WIN32_H
#define SOUND_WIN32_H

#include "sound_driver.hpp"

/** Implementation of the sound driver for Windows. */
class SoundDriver_Win32 : public SoundDriver {
public:
	std::optional<std::string_view> Start(const StringList &param) override;

	void Stop() override;
	std::string_view GetName() const override { return "win32"; }
};

/** Factory for the sound driver for Windows. */
class FSoundDriver_Win32 : public DriverFactoryBase {
public:
	FSoundDriver_Win32() : DriverFactoryBase(Driver::DT_SOUND, 9, "win32", "Win32 WaveOut Sound Driver (param hz,samples)") {}
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<SoundDriver_Win32>(); }
};

#endif /* SOUND_WIN32_H */
