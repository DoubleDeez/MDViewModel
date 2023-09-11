#pragma once

#include "Subsystems/EngineSubsystem.h"
#include "MDViewModelGraphSubsystem.generated.h"

UCLASS()
class MDVIEWMODELGRAPH_API UMDViewModelGraphSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	static void BroadcastBlueprintViewModelAssignmentsChanged(UBlueprint* Blueprint);
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBlueprintViewModelAssignmentsChanged, UBlueprint*)
	FOnBlueprintViewModelAssignmentsChanged OnAssignmentsChanged;
};
