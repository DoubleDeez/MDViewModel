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
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;
	virtual bool IsActionFilteredOut(const FBlueprintActionFilter& Filter) override;

	virtual void AllocateDefaultPins() override;
	
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;

	virtual FText GetFunctionContextString() const override;
	virtual FText GetFunctionContextFormat() const;
	
	virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const override;

	void InitializeViewModelFunctionParams(const FMDViewModelAssignmentReference& VMAssignment, const UFunction* Function, const UWidgetBlueprint* WidgetBP = nullptr);

protected:
	virtual bool IsFunctionValidForNode(const UFunction& Func) const { return false; }

	virtual UBlueprintNodeSpawner* CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const UFunction* Function, const UWidgetBlueprint* WidgetBP) const { return nullptr;}
	
private:
	UPROPERTY()
	FMDViewModelAssignmentReference Assignment;

	UPROPERTY()
	TWeakObjectPtr<const UWidgetBlueprint> ExpectedWidgetBP;
};
