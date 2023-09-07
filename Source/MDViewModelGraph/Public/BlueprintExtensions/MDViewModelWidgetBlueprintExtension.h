#pragma once

#include "MDViewModelAssignableInterface.h"
#include "WidgetBlueprintExtension.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "MDViewModelWidgetBlueprintExtension.generated.h"

class UMDViewModelBase;
struct FMDViewModelEditorAssignment;

/**
 * Editor-only class that holds design-time assigned view models for widgets
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDViewModelWidgetBlueprintExtension : public UWidgetBlueprintExtension, public IMDViewModelAssignableInterface
{
	GENERATED_BODY()

public:
	virtual const TArray<FMDViewModelEditorAssignment>& GetAssignments() const override { return Assignments; }
	virtual TArray<FMDViewModelEditorAssignment>& GetAssignments() override { return Assignments; }

protected:
	virtual void HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext) override;
	virtual void HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class) override;
	virtual void HandleEndCompilation() override;

	UPROPERTY()
	TArray<FMDViewModelEditorAssignment> Assignments;

private:
	FWidgetBlueprintCompilerContext* CompilerContext = nullptr;
};
