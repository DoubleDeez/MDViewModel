#include "Components/MDViewModelAssignmentComponent.h"

void UMDViewModelAssignmentComponent::SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments)
{
	Assignments = InAssignments;
}

const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& UMDViewModelAssignmentComponent::GetAssignments() const
{
	return Assignments;
}

void UMDViewModelAssignmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// TODO - Init view models (or on activate?)
}
