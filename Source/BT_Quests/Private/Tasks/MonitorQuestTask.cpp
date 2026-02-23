// Copyright (C) Varian Daemon 2025. All Rights Reserved.


#include "Tasks/MonitorQuestTask.h"

UMonitorQuestTask::UMonitorQuestTask(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	Tooltip = "Task that monitors the progress of a quest";
	MenuDisplayName = "Monitor Quest";
#endif

	AcceptQuestOnActivate = false;
}
