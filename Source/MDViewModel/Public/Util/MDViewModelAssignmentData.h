#pragma once

#include "InstancedStruct.h"
#include "MDViewModelAssignmentData.generated.h"

/**
 * Data linked to an FMDViewModelAssignment, setup at design/compile time and used at runtime
 */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelAssignmentData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FInstancedStruct ProviderSettings;

	UPROPERTY()
	FInstancedStruct ViewModelSettings;
};
