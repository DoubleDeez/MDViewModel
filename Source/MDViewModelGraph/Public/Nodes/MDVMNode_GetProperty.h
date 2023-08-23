#pragma once

#include "K2Node_VariableGet.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDVMNode_GetProperty.generated.h"

class UWidgetBlueprint;
/**
 * Custom node for getting a variable from a view model with built-in view model validation
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_GetProperty : public UK2Node_VariableGet
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
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const override;

	void InitializeViewModelPropertyParams(const FMDViewModelAssignmentReference& VMAssignment, const FProperty* Property, const UWidgetBlueprint* WidgetBP = nullptr);

protected:
	UBlueprintNodeSpawner* CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const FProperty* Property, const UWidgetBlueprint* WidgetBP) const;
	
private:
	void BindAssignmentChanges();
	void OnAssignmentNameChanged(TSubclassOf<UMDViewModelBase> VMClass, const FName& OldName, const FName& NewName);
	void OnAssignmentClassChanged(const FName& VMName, TSubclassOf<UMDViewModelBase> OldClass, TSubclassOf<UMDViewModelBase> NewClass);
	void UnbindAssignmentChanges();

	bool IsPropertyValidForNode(const FProperty* Property) const;

	void ToggleValidation();
	
	UPROPERTY()
	FMDViewModelAssignmentReference Assignment;

	UPROPERTY()
	TWeakObjectPtr<const UWidgetBlueprint> ExpectedWidgetBP;

	FNodeTextCache TitleCache;
};
