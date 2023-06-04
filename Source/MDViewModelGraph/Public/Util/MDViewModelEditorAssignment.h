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

	// Is this a natively assigned viewmodel
	UPROPERTY()
	bool bIsNative = false;

	// Is this assignment from a super class
	UPROPERTY()
	bool bIsSuper = false;

	bool operator==(const FMDViewModelEditorAssignment& Other) const;
};
