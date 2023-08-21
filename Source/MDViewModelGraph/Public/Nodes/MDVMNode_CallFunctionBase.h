#pragma once

#include "K2Node_CallFunction.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDVMNode_CallFunctionBase.generated.h"

class UWidgetBlueprint;

// Base class for calling functions on assigned view models
UCLASS(Abstract)
class MDVIEWMODELGRAPH_API UMDVMNode_CallFunctionBase : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;
	
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;
	virtual bool IsActionFilteredOut(const FBlueprintActionFilter& Filter) override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	virtual bool IncludeParentNodeContextMenu() const override { return true; }

	virtual void ReconstructNode() override;
	virtual void AllocateDefaultPins() override;

	virtual bool IsNodePure() const override { return bIsSetPure; }
	
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;

	virtual FText GetFunctionContextString() const override;
	virtual FText GetFunctionContextFormat() const;
	
	virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const override;

	void InitializeViewModelFunctionParams(const FMDViewModelAssignmentReference& VMAssignment, const UFunction* Function, const UWidgetBlueprint* WidgetBP = nullptr);

protected:
	virtual bool IsFunctionValidForNode(const UFunction& Func) const { return false; }
	virtual bool CanTogglePurity() const;

	virtual UBlueprintNodeSpawner* CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const UFunction* Function, const UWidgetBlueprint* WidgetBP) const { return nullptr;}
	
private:
	void BindAssignmentNameChanged();
	void OnAssignmentNameChanged(TSubclassOf<UMDViewModelBase> VMClass, const FName& OldName, const FName& NewName);
	void UnbindAssignmentNameChanged();
	
	void TogglePurity();
	
	UPROPERTY()
	FMDViewModelAssignmentReference Assignment;

	UPROPERTY()
	TWeakObjectPtr<const UWidgetBlueprint> ExpectedWidgetBP;

	UPROPERTY()
	bool bIsSetPure = false;
};