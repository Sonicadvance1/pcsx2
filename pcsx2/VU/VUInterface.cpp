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

#include <map>
#include <vector>

#include "PrecompiledHeader.h"
#include "Common.h"
#include "VUmicro.h"

#include "VU/VUInterface.h"

extern const Pcsx2Config EmuConfig;

namespace VUInterface
{
// Private Members
class CpuInitializerBase
{
public:
	CpuInitializerBase() {}
	virtual ~CpuInitializerBase() {}

	virtual bool IsAvailable() const = 0;
	virtual BaseVUmicroCPU* GetPtr() = 0;

	ScopedExcept       ExThrown;
};

template< typename CpuType >
class CpuInitializer : public CpuInitializerBase
{
public:
	ScopedPtr<CpuType> MyCpu;

	CpuInitializer();
	virtual ~CpuInitializer() throw();

	bool IsAvailable() const
	{
		return !!MyCpu;
	}

	BaseVUmicroCPU* GetPtr() { return MyCpu.GetPtr(); }
};

// --------------------------------------------------------------------------------------
//  CpuInitializer Template
// --------------------------------------------------------------------------------------
// Helper for initializing various PCSX2 CPU providers, and handing errors and cleanup.
//
template< typename CpuType >
CpuInitializer< CpuType >::CpuInitializer()
{
	try
	{
		MyCpu = new CpuType();
		MyCpu->Reserve();
	}
	catch( Exception::RuntimeError& ex )
	{
		Console.Error( L"CPU provider error:\n\t" + ex.FormatDiagnosticMessage() );
		MyCpu = NULL;
		ExThrown = ex.Clone();
	}
	catch( std::runtime_error& ex )
	{
		Console.Error( L"CPU provider error (STL Exception)\n\tDetails:" + fromUTF8( ex.what() ) );
		MyCpu = NULL;
		ExThrown = new Exception::RuntimeError(ex);
	}
}

template< typename CpuType >
CpuInitializer< CpuType >::~CpuInitializer() throw()
{
	if (MyCpu)
		MyCpu->Shutdown();
}

// --------------------------------------------------------------------------------------
//  CpuInitializerSet
// --------------------------------------------------------------------------------------
class CpuInitializerSet
{
public:
	std::map<int, std::vector<CpuInitializerBase*> > m_cores;

	// Note: Allocate sVU first -- it's the most picky.
	CpuInitializerSet()
	{
		m_cores[PROVIDER_SUPERVU] = { new CpuInitializer<recSuperVU0>(), new CpuInitializer<recSuperVU1>() };
		m_cores[PROVIDER_MICROVU] = { new CpuInitializer<recMicroVU0>(), new CpuInitializer<recMicroVU1>() };
		m_cores[PROVIDER_INTERPRETER] = { new CpuInitializer<InterpVU0>(), new CpuInitializer<InterpVU1>() };
	}

	virtual ~CpuInitializerSet() throw()
	{

		for (auto& it : m_cores)
		{
			for (int i = 0; i < 2; ++i)
			{
				if (it.second[i])
					delete it.second[i];
			}
		}
	}
};

BaseVUmicroCPU* cpu_vu[2] = {};
ScopedPtr<CpuInitializerSet> CpuProviders;

// Public functions
void Initialize()
{
	CpuProviders = new CpuInitializerSet();
}

void ApplyConfig()
{
	cpu_vu[VUCORE_0] = CpuProviders->m_cores[PROVIDER_INTERPRETER][0]->GetPtr();
	cpu_vu[VUCORE_1] = CpuProviders->m_cores[PROVIDER_INTERPRETER][1]->GetPtr();

	if( EmuConfig.Cpu.Recompiler.EnableVU0 )
	{
		cpu_vu[VUCORE_0] = EmuConfig.Cpu.Recompiler.UseMicroVU0 ?
			CpuProviders->m_cores[PROVIDER_MICROVU][0]->GetPtr() : CpuProviders->m_cores[PROVIDER_SUPERVU][0]->GetPtr();
	}

	if( EmuConfig.Cpu.Recompiler.EnableVU1 )
	{
		cpu_vu[VUCORE_1] = EmuConfig.Cpu.Recompiler.UseMicroVU1 ?
			CpuProviders->m_cores[PROVIDER_MICROVU][1]->GetPtr() : CpuProviders->m_cores[PROVIDER_SUPERVU][1]->GetPtr();
	}
}

void EmergencyResponse()
{
	if (cpu_vu[VUCORE_0])
	{
		cpu_vu[VUCORE_0]->SetCacheReserve( (cpu_vu[VUCORE_0]->GetCacheReserve() * 2) / 3 );
		cpu_vu[VUCORE_0]->Reset();
	}

	if (cpu_vu[VUCORE_1])
	{
		cpu_vu[VUCORE_1]->SetCacheReserve( (cpu_vu[VUCORE_1]->GetCacheReserve() * 2) / 3 );
		cpu_vu[VUCORE_1]->Reset();
	}
}

bool HadSomeFailures( const Pcsx2Config::RecompilerOptions& recOpts )
{
	return (recOpts.EnableVU0 && recOpts.UseMicroVU0 && !IsProviderAvailable(PROVIDER_MICROVU, VUCORE_0)) ||
	       (recOpts.EnableVU1 && recOpts.UseMicroVU0 && !IsProviderAvailable(PROVIDER_MICROVU, VUCORE_1)) ||
	       (recOpts.EnableVU0 && !recOpts.UseMicroVU0 && !IsProviderAvailable(PROVIDER_SUPERVU, VUCORE_0)) ||
	       (recOpts.EnableVU1 && !recOpts.UseMicroVU1 && !IsProviderAvailable(PROVIDER_SUPERVU, VUCORE_1));
}

bool IsProviderAvailable(int provider, int vu_index)
{
	return CpuProviders->m_cores[provider][vu_index]->IsAvailable();
}

BaseException* GetException(int provider, int vu_index)
{
	return CpuProviders->m_cores[provider][vu_index]->ExThrown;
}

BaseVUmicroCPU* GetCurrentProvider(int vu_index)
{
	return cpu_vu[vu_index];
}

// This is a semi-hacky function for convenience
BaseVUmicroCPU* GetVUProvider(int provider, int vu_index)
{
	return CpuProviders->m_cores[provider][vu_index]->GetPtr();
}

}
