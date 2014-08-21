/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "VUmicro.h"

namespace VUInterface
{
	enum Provider
	{
		PROVIDER_INTERPRETER = 0,
		PROVIDER_SUPERVU = 1,
		PROVIDER_MICROVU = 2,
	};

	enum VUCore
	{
		VUCORE_0 = 0,
		VUCORE_1 = 1,
	};

	void Initialize();

	void ApplyConfig();

	void EmergencyResponse();

	bool HadSomeFailures(const Pcsx2Config::RecompilerOptions& recOpts);

	bool IsProviderAvailable(int provider, int vu_index);

	BaseException* GetException(int provider, int vu_index);

	BaseVUmicroCPU* GetVUProvider(int provider, int vu_index);

	BaseVUmicroCPU* GetCurrentProvider(int vu_index);

}
