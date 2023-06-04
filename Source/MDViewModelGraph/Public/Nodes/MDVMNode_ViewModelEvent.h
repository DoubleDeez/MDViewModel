#pragma once

#include "K2Node_Event.h"
#include "MDVMNode_ViewModelEvent.generated.h"

class UMDViewModelBase;

/**
 * Custom node for binding to delegates on View Models
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_ViewModelEvent : public UK2Node_Event
{
	GENERATED_BODY()

public:
	// Class of the view model we're binding to
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	// Name of the view model we're binding to
	UPROPERTY()
	FName ViewModelName = NAME_None;

	// Name of the delegate on the viewmodel we're going to bind to
	UPROPERTY()
	FName DelegatePropertyName = NAME_None;

	// Name of the function that we're binding to the delegate
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
	virtual bool HasDeprecatedReference() const override;
	virtual FEdGraphNodeDeprecationResponse GetDeprecationResponse(EEdGraphNodeDeprecationType DeprecationType) const override;
	//~ End UEdGraphNode Interface

	//~ Begin K2Node Interface
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual UClass* GetDynamicBindingClass() const override;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const override;
	virtual void HandleVariableRenamed(UBlueprint* InBlueprint, UClass* InVariableClass, UEdGraph* InGraph, const FName& InOldVarName, const FName& InNewVarName) override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	//~ End K2Node Interface

	virtual bool IsUsedByAuthorityOnlyDelegate() const override;

	// Return the delegate property that this event is bound to
	FMulticastDelegateProperty* GetTargetDelegateProperty() const;

	// Gets the proper display name for the property
	FText GetTargetDelegateDisplayName() const;

	void InitializeViewModelEventParams(TSubclassOf<UMDViewModelBase> InViewModelClass, const FName& InViewModelName, const FName& InDelegatePropertyName);

private:
	FNodeTextCache CachedNodeTitle;

};
