#pragma once

#include "BlueprintFieldNodeSpawner.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDViewModelNodeSpawner.generated.h"

class UWidgetBlueprint;

UCLASS()
class MDVIEWMODELGRAPH_API UMDViewModelNodeSpawner : public UBlueprintFieldNodeSpawner
{
	GENERATED_BODY()

public:
	static UMDViewModelNodeSpawner* Create(TSubclassOf<UEdGraphNode> NodeClass, const FText& Category, const FMDViewModelAssignmentReference& Assignment, FFieldVariant Field, const UWidgetBlueprint* WidgetBP);
	
	virtual FBlueprintNodeSignature GetSpawnerSignature() const override;
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph, const FBindingSet& Bindings, const FVector2D Location) const override;

protected:
	UPROPERTY()
	FMDViewModelAssignmentReference Assignment;

	UPROPERTY()
	TWeakObjectPtr<const UWidgetBlueprint> WidgetBPPtr;
};
