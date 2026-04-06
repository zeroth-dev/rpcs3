#pragma once
#include "rcheevos.h"
#include "RA_Interface.h"
#include <qaction.h>

namespace Achievements
{

	void InitializeRAIntegration(void* main_window_handle);

	int RACallbackIsActive();
	void RACallbackCauseUnpause();
	void RACallbackCausePause();
	void RACallbackRebuildMenu();
	void RACallbackEstimateTitle(char* buf);
	void RACallbackResetEmulator();
	void RACallbackLoadROM(const char* unused);
	unsigned char RACallbackReadMemory(unsigned int address);
	unsigned int RACallbackReadBlock(unsigned int address, unsigned char* buffer, unsigned int bytes);
	void RACallbackWriteMemory(unsigned int address, unsigned char value);
	uint32_t GetExposedEEMemorySize();
	std::vector<std::tuple<int, std::string, bool>> GetMenuItems();
	void ActivateMenuItem(int item);
	bool RAIntegrationInitialized();

}