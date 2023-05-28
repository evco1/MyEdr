#pragma once

#include <fltKernel.h>

class CreateProcessNotifyRoutine final
{
public:
	CreateProcessNotifyRoutine() = delete;
	CreateProcessNotifyRoutine(const CreateProcessNotifyRoutine&) = delete;
	CreateProcessNotifyRoutine(CreateProcessNotifyRoutine&&) = delete;
	CreateProcessNotifyRoutine(PCREATE_PROCESS_NOTIFY_ROUTINE_EX processNotifyRoutine);

	~CreateProcessNotifyRoutine();

	CreateProcessNotifyRoutine& operator=(const CreateProcessNotifyRoutine&) = delete;
	CreateProcessNotifyRoutine& operator=(CreateProcessNotifyRoutine&&) = delete;

private:
	PCREATE_PROCESS_NOTIFY_ROUTINE_EX m_processNotifyRoutine;
};
