#pragma once

#include "K2Node_Event.h"
#include "MDVMNode_ViewModelChanged.generated.h"

class UMDViewModelBase;

/**
 * Custom node for binding to view models changing
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_ViewModelChanged : public UK2Node_Event
{
	GENERATED_BODY()

public:
	// Class of the view model we're binding to
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	// Name of the view model we're binding to
	UPROPERTY()
	FName ViewModelName = NAME_None;

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

	void InitializeViewModelChangedParams(TSubclassOf<UMDViewModelBase> InViewModelClass, const FName& InViewModelName);

private:
	FNodeTextCache CachedNodeTitle;
};
