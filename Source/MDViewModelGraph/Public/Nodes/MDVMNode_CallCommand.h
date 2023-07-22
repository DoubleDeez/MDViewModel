#pragma once

#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "Util/MDViewModelAssignmentReference.h"

#include "MDVMNode_CallCommand.generated.h"

class UWidgetBlueprint;

UCLASS(Transient)
class UMDViewModelCommandNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_BODY()

public:
	static UMDViewModelCommandNodeSpawner* Create(const FMDViewModelAssignmentReference& Assignment, const UFunction* Function, const UWidgetBlueprint* WidgetBP);
	
	virtual FBlueprintNodeSignature GetSpawnerSignature() const override;
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph, const FBindingSet& Bindings, const FVector2D Location) const override;

protected:
	UPROPERTY()
	FMDViewModelAssignmentReference Assignment;

	UPROPERTY()
	TWeakObjectPtr<const UFunction> FunctionPtr;

	UPROPERTY()
	TWeakObjectPtr<const UWidgetBlueprint> WidgetBPPtr;
};

/**
 * Node to shortcut getting a view model instance and calling a function on it
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_CallCommand : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;
	virtual bool IsActionFilteredOut(const FBlueprintActionFilter& Filter) override;

	virtual void AllocateDefaultPins() override;
	
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;

	virtual FText GetFunctionContextString() const override;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;

	void InitializeViewModelCommandParams(const FMDViewModelAssignmentReference& VMAssignment, const UFunction* Function, const UWidgetBlueprint* WidgetBP = nullptr);

private:
	UPROPERTY()
	FMDViewModelAssignmentReference Assignment;

	UPROPERTY()
	TWeakObjectPtr<const UWidgetBlueprint> ExpectedWidgetBP;
};
