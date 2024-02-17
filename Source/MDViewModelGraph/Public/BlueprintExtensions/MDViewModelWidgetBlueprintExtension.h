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
	virtual void PostLoad() override;

	virtual TArray<FMDViewModelEditorAssignment>& GetAssignments() override { return Assignments; }

protected:
	UPROPERTY()
	TArray<FMDViewModelEditorAssignment> Assignments;
};
