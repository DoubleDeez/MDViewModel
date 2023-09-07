#pragma once

#include "Components/ActorComponent.h"
#include "Interfaces/MDVMCompiledAssignmentsInterface.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "MDViewModelAssignmentComponent.generated.h"

// Auto-generated component to track view models on an actor
UCLASS(Hidden, meta=(BlueprintSpawnableComponent))
class MDVIEWMODEL_API UMDViewModelAssignmentComponent : public UActorComponent, public IMDVMCompiledAssignmentsInterface
{
	GENERATED_BODY()

public:
	void SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments);
	virtual const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& GetAssignments() const override;

protected:
	virtual void BeginPlay() override;

	// TODO - Details customization to hide all properties and add a button to open view model tab

	UPROPERTY(VisibleAnywhere, Category = "MDVM")
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;

};
