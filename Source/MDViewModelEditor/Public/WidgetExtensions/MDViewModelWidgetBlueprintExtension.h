#pragma once

#include "CoreMinimal.h"
#include "WidgetBlueprintExtension.h"
#include "Util/MDViewModelAssignmentData.h"
#include "MDViewModelWidgetBlueprintExtension.generated.h"

class UMDViewModelBase;
struct FMDViewModelEditorAssignment;

/**
 * Editor-only class that holds design-time assigned view models
 */
UCLASS()
class MDVIEWMODELEDITOR_API UMDViewModelWidgetBlueprintExtension : public UWidgetBlueprintExtension
{
	GENERATED_BODY()

public:
	const TArray<FMDViewModelEditorAssignment>& GetAssignments() const { return Assignments; }

	void AddAssignment(FMDViewModelEditorAssignment&& Assignment);
	void UpdateAssignment(const FMDViewModelEditorAssignment& Assignment, FMDViewModelEditorAssignment&& UpdatedAssignment);
	void RemoveAssignment(const FMDViewModelEditorAssignment& Assignment);

	bool DoesContainViewModelAssignment(TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None) const;
	bool HasAssignments() const { return !Assignments.IsEmpty(); }

	FSimpleMulticastDelegate OnAssignmentsChanged;

protected:
	virtual void HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext) override;
	virtual void HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class) override;
	virtual bool HandleValidateGeneratedClass(UWidgetBlueprintGeneratedClass* Class) override;
	virtual void HandleEndCompilation() override;

	UPROPERTY()
	TArray<FMDViewModelEditorAssignment> Assignments;

private:
	FWidgetBlueprintCompilerContext* CompilerContext = nullptr;
};
