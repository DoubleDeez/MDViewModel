#include "Nodes/MDVMNode_SetViewModel.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

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

void UMDVMNode_SetViewModel::AutowireNewNode(UEdGraphPin* FromPin)
{
	// If FromPin is a view model output, auto connect it to our View Model pin, otherwise the Super call will connect it to the Object pin
	UEdGraphPin* ViewModelPin = FindPin(TEXT("ViewModel"));
	if (FromPin != nullptr && FromPin->Direction == EGPD_Output && FromPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		const UClass* PinTypeClass = Cast<UClass>(FromPin->PinType.PinSubCategoryObject.Get());
		if (IsValid(PinTypeClass) && PinTypeClass->IsChildOf<UMDViewModelBase>())
		{
			const UEdGraphSchema_K2* K2Schema = CastChecked<UEdGraphSchema_K2>(GetSchema());
			if (K2Schema->TryCreateConnection(FromPin, ViewModelPin))
			{
				if (FromPin->GetOwningNode() != nullptr)
				{
					FromPin->GetOwningNode()->NodeConnectionListChanged();
					return;
				}
			}
		}
	}

	Super::AutowireNewNode(FromPin);
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
