#pragma once

#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "MDViewModelEditorAssignment.generated.h"

/**
 *
 */
USTRUCT()
struct MDVIEWMODELGRAPH_API FMDViewModelEditorAssignment
{
	GENERATED_BODY()

public:
	// The actual assignment
	UPROPERTY()
	FMDViewModelAssignment Assignment;

	// Additional data the user set in the editor
	UPROPERTY()
	FMDViewModelAssignmentData Data;

	// The class that this assignment is from, if this assignment is from a parent
	UPROPERTY(Transient)
	TObjectPtr<UClass> SuperAssignmentOwner;

	bool operator==(const FMDViewModelEditorAssignment& Other) const;
};
