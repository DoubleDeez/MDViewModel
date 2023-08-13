#pragma once

#include "BlueprintNodeSpawner.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDViewModelNodeSpawner.generated.h"

class UWidgetBlueprint;

UCLASS()
class MDVIEWMODELGRAPH_API UMDViewModelNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_BODY()

public:
	static UMDViewModelNodeSpawner* Create(TSubclassOf<UEdGraphNode> NodeClass, const FText& Category, const FMDViewModelAssignmentReference& Assignment, const UFunction* Function, const UWidgetBlueprint* WidgetBP);
	
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
