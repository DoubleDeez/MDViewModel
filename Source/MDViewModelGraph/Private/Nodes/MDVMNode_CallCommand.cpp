#include "Nodes/MDVMNode_CallCommand.h"

#include "Nodes/MDViewModelNodeSpawner.h"

FText UMDVMNode_CallCommand::GetFunctionContextFormat() const
{
	return INVTEXT("Command on {ViewModelClass} ({ViewModelName})");
}

bool UMDVMNode_CallCommand::IsFunctionValidForNode(const UFunction& Func) const
{
	return Func.HasAllFunctionFlags(FUNC_BlueprintCallable) && !Func.HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Static);
}

UBlueprintNodeSpawner* UMDVMNode_CallCommand::CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const UFunction* Function, const UBlueprint* Blueprint) const
{
	return UMDViewModelNodeSpawner::Create(UMDVMNode_CallCommand::StaticClass(), INVTEXT("View Model Commands"), AssignmentReference, Function, Blueprint);
}
