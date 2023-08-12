#pragma once

#include "K2Node_CallFunction.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDVMNode_GetViewModel.generated.h"

/**
 * Custom node for UMDViewModelFunctionLibrary::BP_GetViewModel to auto-cast the return value to the assigned view model class
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_GetViewModel : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	UMDVMNode_GetViewModel();

	virtual void AllocateDefaultPins() override;
	virtual void PostReconstructNode() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;

	void SetDefaultAssignment(const FMDViewModelAssignmentReference& Assignment);

private:
	void UpdateReturnPin() const;

	FMDViewModelAssignmentReference PendingAssignment;
};
