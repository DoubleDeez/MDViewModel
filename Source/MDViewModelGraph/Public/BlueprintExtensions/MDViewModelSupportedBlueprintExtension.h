#pragma once

#include "Blueprint/BlueprintExtension.h"
#include "MDViewModelAssignableInterface.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "MDViewModelSupportedBlueprintExtension.generated.h"

/**
 * Editor-only class that holds design-time assigned view models for non-Widget Blueprints
 * Assignment compilation happens in UMDViewModelBlueprintCompilerExtension::HandleActorBlueprintPreCompile/HandleGeneralBlueprintPreCompile
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDViewModelSupportedBlueprintExtension : public UBlueprintExtension, public IMDViewModelAssignableInterface
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

	virtual TArray<FMDViewModelEditorAssignment>& GetAssignments() override { return Assignments; }

protected:
	UPROPERTY()
	TArray<FMDViewModelEditorAssignment> Assignments;

};
