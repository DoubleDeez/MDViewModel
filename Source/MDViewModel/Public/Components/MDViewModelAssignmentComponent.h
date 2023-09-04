#pragma once

#include "Components/ActorComponent.h"
#include "Util/MDViewModelAssignmentData.h"
#include "MDViewModelAssignmentComponent.generated.h"

// Auto-generated component to track view models on an actor
UCLASS(Hidden, meta=(BlueprintSpawnableComponent))
class MDVIEWMODEL_API UMDViewModelAssignmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments);

protected:
	virtual void BeginPlay() override;

	// TODO - Details customization to hide all properties and add a button to open view model tab

	UPROPERTY()
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;

};
