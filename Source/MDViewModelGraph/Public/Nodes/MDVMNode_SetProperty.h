#pragma once

#include "K2Node_VariableSet.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDVMNode_SetProperty.generated.h"

/**
 * Custom node for setting a variable on a view model with built-in view model validation
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_SetProperty : public UK2Node_Variable
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void BeginDestroy() override;

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const override;
	virtual bool IsActionFilteredOut(const FBlueprintActionFilter& Filter) override;

	virtual bool DrawNodeAsVariable() const override { return false; }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const override;

	void InitializeViewModelPropertyParams(const FMDViewModelAssignmentReference& VMAssignment, const FProperty* Property, const UBlueprint* Blueprint = nullptr);

protected:
	UBlueprintNodeSpawner* CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const FProperty* Property, const UBlueprint* Blueprint) const;

private:
	void BindAssignmentChanges();
	void OnAssignmentChanged(const FMDViewModelAssignmentReference& Old, const FMDViewModelAssignmentReference& New);
	void UnbindAssignmentChanges();

	bool IsPropertyValidForNode(const FProperty* Property) const;

	UPROPERTY()
	FMDViewModelAssignmentReference Assignment;

	UPROPERTY()
	TWeakObjectPtr<const UBlueprint> ExpectedBlueprintPtr;

	FNodeTextCache TitleCache;
};
