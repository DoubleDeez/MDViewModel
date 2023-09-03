#pragma once

#include "MDVMNode_CallFunctionBase.h"
#include "Util/MDViewModelAssignmentReference.h"

#include "MDVMNode_CallCommand.generated.h"

class UBlueprint;

/**
 * Node to shortcut getting a view model instance and calling a function on it
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDVMNode_CallCommand : public UMDVMNode_CallFunctionBase
{
	GENERATED_BODY()

public:
	virtual FText GetFunctionContextFormat() const override;

protected:
	virtual bool IsFunctionValidForNode(const UFunction& Func) const override;

	virtual UBlueprintNodeSpawner* CreateNodeSpawner(const FMDViewModelAssignmentReference& Assignment, const UFunction* Function, const UBlueprint* Blueprint) const override;
};
