#pragma once

#include "MDVMNode_CallFunctionBase.h"
#include "MDVMNode_CallGetter.generated.h"

/**
 * 
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_CallGetter : public UMDVMNode_CallFunctionBase
{
	GENERATED_BODY()

public:
	virtual FText GetFunctionContextFormat() const override;

protected:
	virtual bool IsFunctionValidForNode(const UFunction& Func) const override;

	virtual UBlueprintNodeSpawner* CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const UFunction* Function, const UWidgetBlueprint* WidgetBP) const override;
};
