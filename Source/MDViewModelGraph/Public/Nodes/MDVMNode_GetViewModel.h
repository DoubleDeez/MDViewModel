#pragma once

#include "K2Node_CallFunction.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDVMNode_GetViewModel.generated.h"

/**
 * Custom node for UMDViewModelFunctionLibrary::BP_GetViewModel to auto-cast the return value to the assigned view model class and optionally provide validation
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_GetViewModel : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	UMDVMNode_GetViewModel();

	virtual void AllocateDefaultPins() override;
	virtual void ReconstructNode() override;
	virtual void PostReconstructNode() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	virtual bool IncludeParentNodeContextMenu() const override { return true; }

	virtual bool IsNodePure() const override { return bIsPureGet; }
	
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const override;

	virtual FText GetPinDisplayName(const UEdGraphPin* Pin) const override;

	void SetDefaultAssignment(const FMDViewModelAssignmentReference& Assignment);

private:
	void UpdateReturnPin() const;
	void TogglePurity();
	
	UPROPERTY()
	FMDViewModelAssignmentReference PendingAssignment;

	UPROPERTY()
	bool bIsPureGet = false;

	mutable bool bDoesNeedOutputRemapping = false;
};
