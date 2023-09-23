#include "Nodes/MDVMNode_CallHelper.h"

#include "Nodes/MDViewModelNodeSpawner.h"

FText UMDVMNode_CallHelper::GetFunctionContextFormat() const
{
	return INVTEXT("Helper on {ViewModelClass} ({ViewModelName})");
}

bool UMDVMNode_CallHelper::IsFunctionValidForNode(const UFunction& Func) const
{
	return Super::IsFunctionValidForNode(Func) && Func.HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure) && !Func.HasAnyFunctionFlags(FUNC_Static);
}

UBlueprintNodeSpawner* UMDVMNode_CallHelper::CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const UFunction* Function, const UBlueprint* Blueprint) const
{
	return UMDViewModelNodeSpawner::Create(UMDVMNode_CallHelper::StaticClass(), INVTEXT("View Model Helpers"), AssignmentReference, Function, Blueprint);
}
