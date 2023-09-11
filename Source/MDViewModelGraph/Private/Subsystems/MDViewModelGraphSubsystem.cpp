#include "Subsystems/MDViewModelGraphSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"

void UMDViewModelGraphSubsystem::BroadcastBlueprintViewModelAssignmentsChanged(UBlueprint* Blueprint)
{
	if (IsValid(Blueprint) && IsValid(GEngine))
	{
		GEngine->GetEngineSubsystem<ThisClass>()->OnAssignmentsChanged.Broadcast(Blueprint);
	}
}
