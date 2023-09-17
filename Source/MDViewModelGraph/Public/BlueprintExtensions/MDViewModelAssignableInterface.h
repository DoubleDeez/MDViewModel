#pragma once

#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "UObject/Interface.h"
#include "MDViewModelAssignableInterface.generated.h"

struct FMDViewModelAssignmentReference;
class UMDViewModelBase;
struct FMDViewModelAssignmentData;
struct FMDViewModelAssignment;
struct FMDViewModelEditorAssignment;

UINTERFACE()
class UMDViewModelAssignableInterface : public UInterface
{
	GENERATED_BODY()
};

// Interface for an object that can be assigned view models
class MDVIEWMODELGRAPH_API IMDViewModelAssignableInterface
{
	GENERATED_BODY()

public:
	virtual TArray<FMDViewModelEditorAssignment>& GetAssignments() = 0;

	virtual UBlueprint* GetBlueprint() const;

	void ModifyObject();

	const TArray<FMDViewModelEditorAssignment>& GetAssignments() const;
	void GetAllAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const;

	void AddAssignment(FMDViewModelEditorAssignment&& Assignment);
	void UpdateAssignment(const FMDViewModelEditorAssignment& Assignment, FMDViewModelEditorAssignment&& UpdatedAssignment);
	void RemoveAssignment(const FMDViewModelEditorAssignment& Assignment);

	bool DoesContainViewModelAssignment(TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None) const;
	bool HasAssignments() const;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnViewModelAssignmentChanged, const FMDViewModelAssignmentReference& /*OldAssignment*/, const FMDViewModelAssignmentReference& /*NewAssignment*/);
	FOnViewModelAssignmentChanged OnAssignmentChanged;

protected:
	void GetParentAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const;
	void SearchParentAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None) const;
};
