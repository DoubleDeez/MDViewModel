#include "Nodes/MDVMNode_SetViewModel.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Util/MDViewModelFunctionLibrary.h"

UMDVMNode_SetViewModel::UMDVMNode_SetViewModel()
{
	FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UMDViewModelFunctionLibrary, BP_SetViewModel), UMDViewModelFunctionLibrary::StaticClass());
}

void UMDVMNode_SetViewModel::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
	const UClass* ActionKey = GetClass();
	if (InActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		InActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UMDVMNode_SetViewModel::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	if (PendingAssignment.IsAssignmentValid())
	{
		SetDefaultAssignment(PendingAssignment);
		PendingAssignment = {};
	}
}

void UMDVMNode_SetViewModel::SetDefaultAssignment(const FMDViewModelAssignmentReference& Assignment)
{
	if (UEdGraphPin* AssignmentPin = FindPin(TEXT("Assignment")))
	{
		FString AssignmentValue;
		FMDViewModelAssignmentReference::StaticStruct()->ExportText(AssignmentValue, &Assignment, &Assignment, nullptr, PPF_SerializedAsImportText, nullptr);

		if (AssignmentValue != AssignmentPin->GetDefaultAsString())
		{
			AssignmentPin->GetSchema()->TrySetDefaultValue(*AssignmentPin, AssignmentValue);
		}
	}
	else
	{
		PendingAssignment = Assignment;
	}
}
