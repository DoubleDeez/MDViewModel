#pragma once

#include "MDVMNode_DynamicBindingBase.h"
#include "MDVMNode_ViewModelChanged.generated.h"

/**
 * Custom node for binding to view models changing
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_ViewModelChanged : public UMDVMNode_DynamicBindingBase
{
	GENERATED_BODY()

public:
	// Name of the function that we're binding to
	UPROPERTY()
	FName FunctionNameToBind = NAME_None;

	//~ Begin UObject Interface
	virtual bool Modify(bool bAlwaysMarkDirty = true) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface
	virtual void ReconstructNode() override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override { return false; }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void AllocateDefaultPins() override;
	//~ End UEdGraphNode Interface

	//~ Begin K2Node Interface
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual UClass* GetDynamicBindingClass() const override;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual bool IsFunctionEntryCompatible(const class UK2Node_FunctionEntry* EntryNode) const override;
	//~ End K2Node Interface

	void InitializeViewModelChangedParams(const FMDViewModelAssignmentReference& InAssignment);

protected:
	virtual void OnAssignmentChanged() override;
};
