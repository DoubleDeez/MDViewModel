#pragma once

#include "K2Node_CallFunction.h"
#include "MDVMNode_SetViewModelOfClass.generated.h"

struct FMDViewModelAssignmentReference;

/**
 * Custom node for UMDViewModelFunctionLibrary::BP_SetViewModelOfClass to auto-cast the return value and validate the settings
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_SetViewModelOfClass : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	UMDVMNode_SetViewModelOfClass();

	virtual void PostReconstructNode() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void AllocateDefaultPins() override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;

	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const override;
	
private:
	void UpdateAssignmentBasedData();

	void GetAssignmentFromPinDefaults(FMDViewModelAssignmentReference& OutAssignment) const;

	UPROPERTY()
	bool bAreViewModelSettingsValid = true;

	UPROPERTY()
	FText ViewModelSettingsDisplayName;
};
