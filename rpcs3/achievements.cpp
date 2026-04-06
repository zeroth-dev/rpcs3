
#include "achievements.h"
#include "RA_Consoles.h"
#include <cstdlib>
#include <cstdint>
#include "Emu/System.h"
#include "Emu/Memory/vm.h"
#include "Utilities/StrFmt.h"
#include "Utilities/StrUtil.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/cell/PPUThread.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include <fstream>


#define MAIN_MEMORY_START_ADDRESS	(0x3000'0000)
#define MAIN_MEMORY_SIZE			(0x1000'0000)


class CPUDisAsm;
std::shared_ptr<CPUDisAsm> make_basic_ppu_disasm();
static bool s_raintegration_initialized = false;

namespace Achievements
{

}


void Achievements::InitializeRAIntegration(void* main_window_handle)
{
	RA_InitClient((HWND)main_window_handle, "RPCS3", "ABCDABCD");
	RA_SetUserAgentDetail("Windows 10+");

	RA_InstallSharedFunctions(RACallbackIsActive, RACallbackCauseUnpause, RACallbackCausePause, RACallbackRebuildMenu,
		RACallbackEstimateTitle, RACallbackResetEmulator, RACallbackLoadROM);
	RA_SetConsoleID(PlayStation3);

	// EE physical memory and scratchpad are currently exposed (matching direct rcheevos implementation).
	RA_InstallMemoryBank(0, RACallbackReadMemory, RACallbackWriteMemory, GetExposedEEMemorySize());
	RA_InstallMemoryBankBlockReader(0, RACallbackReadBlock);

	// Fire off a login anyway. Saves going into the menu and doing it.
	RA_AttemptLogin(0);

	s_raintegration_initialized = true;

	// this is pretty lame, but we may as well persist until we exit anyway
	std::atexit(RA_Shutdown);
}

void Achievements::RACallbackCausePause()
{
	Emu.CallFromMainThread([]()
	{
		if (Emu.GetStatus() == system_state::running)
			Emu.Pause();
	});
}

void Achievements::RACallbackRebuildMenu()
{
	// unused, we build the menu on demand
}


int Achievements::RACallbackIsActive()
{
	return Emu.GetStatus() == system_state::running;
}

void Achievements::RACallbackCauseUnpause()
{
	Emu.CallFromMainThread([]()
		{
			if (Emu.GetStatus() == system_state::paused)
				Emu.Resume();
		});
}


void Achievements::RACallbackEstimateTitle(char* buf)
{
	std::string title = "Testing";
	std::strncpy(buf, title.c_str(), title.size());
}

void Achievements::RACallbackResetEmulator()
{
	if (Emu.GetStatus() != system_state::stopped)
		Emu.Restart();
	
}

void Achievements::RACallbackLoadROM(const char* unused)
{
	// unused
	UNREFERENCED_PARAMETER(unused);
}


unsigned char Achievements::RACallbackReadMemory(unsigned int address)
{
	// Offset the address to get to the main memory

	address = MAIN_MEMORY_START_ADDRESS + address;
	if (vm::check_addr(address, 0, sizeof(unsigned char)))
	{
		return (unsigned char)(vm::get_super_ptr(address));
	}
	else
		return 0u;

}

unsigned int Achievements::RACallbackReadBlock(unsigned int address, unsigned char* buffer, unsigned int bytes)
{

	// Offset the address to get to the main memory

	address = MAIN_MEMORY_START_ADDRESS + address;
	unsigned int read_byte_count = 0;

	for (int currQWord = 0; currQWord < bytes/4; currQWord++)
	{
		void* cpuPtr = 0;
		// By default we display 0 if we cannot read the inquired memory
		char qword[4] = {0, 0, 0, 0}; 
		if (vm::check_addr(address + read_byte_count, 0, 1)) [[likely]]
		{
			cpuPtr = vm::get_super_ptr(address + read_byte_count);
			const be_t<u32> rmem = read_from_ptr<be_t<u32>>(static_cast<const u8*>(cpuPtr));
			qword[0] = rmem >> 24;
			qword[1] = rmem >> 16;
			qword[2] = rmem >> 8;
			qword[3] = rmem >> 0;
		}
		std::memcpy(buffer+read_byte_count, qword, 4);
		read_byte_count += 4;

	}
	return read_byte_count;
}

void Achievements::RACallbackWriteMemory(unsigned int address, unsigned char value)
{
	// We pinky promise we are not touching the executable space
	address = MAIN_MEMORY_START_ADDRESS + address;
	cpu_thread::suspend_all(nullptr, {}, [&]
	{
		if (!vm::check_addr(address))
		{
			return;
		}
		*vm::get_super_ptr(address) = value;
	});
}


uint32_t Achievements::GetExposedEEMemorySize()
{
	return MAIN_MEMORY_SIZE;
}

std::vector<std::tuple<int, std::string, bool>> Achievements::GetMenuItems()
{
	std::array<RA_MenuItem, 64> items;
	const int num_items = RA_GetPopupMenuItems(items.data());

	std::vector<std::tuple<int, std::string, bool>> ret;
	ret.reserve(static_cast<u32>(num_items));

	for (int i = 0; i < num_items; i++)
	{
		const RA_MenuItem& it = items[i];
		if (!it.sLabel)
		{
			// separator
			ret.emplace_back(0, std::string(), false);
		}
		else
		{
			// option, maybe checkable
			ret.emplace_back(static_cast<int>(it.nID), wchar_to_utf8(it.sLabel), it.bChecked);
		}
	}

	return ret;
}

void Achievements::ActivateMenuItem(int item)
{
	RA_InvokeDialog(item);
}

bool Achievements::RAIntegrationInitialized()
{
return s_raintegration_initialized;
}