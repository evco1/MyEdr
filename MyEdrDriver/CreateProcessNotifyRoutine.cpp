#include <fltKernel.h>

#include "CreateProcessNotifyRoutine.h"
#include "Debug.h"

CreateProcessNotifyRoutine::CreateProcessNotifyRoutine(PCREATE_PROCESS_NOTIFY_ROUTINE_EX processNotifyRoutine) :
	m_processNotifyRoutine{ processNotifyRoutine }
{
	DEBUG_PRINT("%p %u", m_processNotifyRoutine, PsSetCreateProcessNotifyRoutineEx(m_processNotifyRoutine, FALSE));
}

CreateProcessNotifyRoutine::~CreateProcessNotifyRoutine()
{
	DEBUG_PRINT("%p %u", m_processNotifyRoutine, PsSetCreateProcessNotifyRoutineEx(m_processNotifyRoutine, TRUE));
}
