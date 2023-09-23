#pragma once

#include "K2Node_CallFunction.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDVMNode_SetViewModel.generated.h"

/**
 * Custom node for UMDViewModelFunctionLibrary::BP_SetViewModel to support pre-populating the view model assignment
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_SetViewModel : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	UMDVMNode_SetViewModel();

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;

	virtual void AllocateDefaultPins() override;

	void SetDefaultAssignment(const FMDViewModelAssignmentReference& Assignment);

private:
	UPROPERTY()
	FMDViewModelAssignmentReference PendingAssignment;
};
