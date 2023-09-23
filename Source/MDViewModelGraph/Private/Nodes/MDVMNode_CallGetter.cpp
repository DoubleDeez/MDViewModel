#include "Nodes/MDVMNode_CallGetter.h"

#include "Nodes/MDViewModelNodeSpawner.h"

FText UMDVMNode_CallGetter::GetFunctionContextFormat() const
{
	return INVTEXT("Getter on {ViewModelClass} ({ViewModelName})");
}

bool UMDVMNode_CallGetter::IsFunctionValidForNode(const UFunction& Func) const
{
	if (!Func.HasAllFunctionFlags(FUNC_BlueprintCallable) || Func.HasAnyFunctionFlags(FUNC_Static))
	{
		return false;
	}

	return Super::IsFunctionValidForNode(Func) && Func.NumParms == 1 && Func.GetReturnProperty() != nullptr;
}

UBlueprintNodeSpawner* UMDVMNode_CallGetter::CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const UFunction* Function, const UBlueprint* Blueprint) const
{
	return UMDViewModelNodeSpawner::Create(UMDVMNode_CallGetter::StaticClass(), INVTEXT("View Model Getters"), AssignmentReference, Function, Blueprint);
}
