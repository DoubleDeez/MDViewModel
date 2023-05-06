#pragma once

#include "CoreMinimal.h"
#include "WidgetBlueprintExtension.h"
#include "Util/MDViewModelAssignmentData.h"
#include "MDViewModelWidgetBlueprintExtension.generated.h"

class UMDViewModelBase;

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
