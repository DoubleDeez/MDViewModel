#pragma once

#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "UObject/Interface.h"
#include "MDVMCompiledAssignmentsInterface.generated.h"

class UMDViewModelBase;
struct FMDViewModelAssignment;
struct FMDViewModelAssignmentData;

UINTERFACE()
class UMDVMCompiledAssignmentsInterface : public UInterface
{
	GENERATED_BODY()
};

// Interface for an object that stores compiled view model assignments
class MDVIEWMODEL_API IMDVMCompiledAssignmentsInterface
{
	GENERATED_BODY()

public:
	virtual const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& GetAssignments() const = 0;
	
	bool HasAssignments() const;
	void SearchAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None) const;

};
