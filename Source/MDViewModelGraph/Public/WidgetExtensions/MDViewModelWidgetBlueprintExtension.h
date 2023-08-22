#pragma once

#include "WidgetBlueprintExtension.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "MDViewModelWidgetBlueprintExtension.generated.h"

class UMDViewModelBase;
struct FMDViewModelEditorAssignment;

/**
 * Editor-only class that holds design-time assigned view models
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDViewModelWidgetBlueprintExtension : public UWidgetBlueprintExtension
{
	GENERATED_BODY()

public:
	const TArray<FMDViewModelEditorAssignment>& GetAssignments() const { return Assignments; }
	void GetBPAndParentClassAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const;

	void AddAssignment(FMDViewModelEditorAssignment&& Assignment);
	void UpdateAssignment(const FMDViewModelEditorAssignment& Assignment, FMDViewModelEditorAssignment&& UpdatedAssignment);
	void RemoveAssignment(const FMDViewModelEditorAssignment& Assignment);

	bool DoesContainViewModelAssignment(TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None) const;
	bool HasAssignments() const { return !Assignments.IsEmpty(); }

	FSimpleMulticastDelegate OnAssignmentsChanged;

	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnViewModelAssignmentNameChanged, TSubclassOf<UMDViewModelBase>, const FName& /*OldName*/, const FName& /*NewName*/);
	FOnViewModelAssignmentNameChanged OnAssignmentNameChanged;

	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnViewModelAssignmentClassChanged, const FName&, TSubclassOf<UMDViewModelBase> /*OldClass*/, TSubclassOf<UMDViewModelBase> /*NewClass*/);
	FOnViewModelAssignmentClassChanged OnAssignmentClassChanged;

protected:
	virtual void HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext) override;
	virtual void HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class) override;
	virtual void HandleEndCompilation() override;

	UPROPERTY()
	TArray<FMDViewModelEditorAssignment> Assignments;

private:
	FWidgetBlueprintCompilerContext* CompilerContext = nullptr;
};
