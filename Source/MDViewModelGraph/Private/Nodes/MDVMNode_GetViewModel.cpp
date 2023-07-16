#include "Nodes/MDVMNode_GetViewModel.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

UMDVMNode_GetViewModel::UMDVMNode_GetViewModel()
{
	FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UMDViewModelFunctionLibrary, BP_GetViewModel), UMDViewModelFunctionLibrary::StaticClass());
}

void UMDVMNode_GetViewModel::PostReconstructNode()
{
	Super::PostReconstructNode();

	UpdateReturnPin();
}

void UMDVMNode_GetViewModel::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
	
	UpdateReturnPin();
}

void UMDVMNode_GetViewModel::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
	const UClass* ActionKey = GetClass();
	if (InActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		InActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UMDVMNode_GetViewModel::UpdateReturnPin() const
{
	// Change the return pin's class to the assigned view model class
	if (UEdGraphPin* ReturnPin = GetReturnValuePin())
	{
		if (const UEdGraphPin* AssignmentPin = FindPin(TEXT("Assignment")))
		{
			const FString DefaultString = AssignmentPin->GetDefaultAsString();
			if (!DefaultString.IsEmpty())
			{
				FMDViewModelAssignmentReference Assignment;
				UScriptStruct* PinLiteralStructType = FMDViewModelAssignmentReference::StaticStruct();
				PinLiteralStructType->ImportText(*DefaultString, &Assignment, nullptr, PPF_SerializedAsImportText, GError, PinLiteralStructType->GetName());

				ReturnPin->PinType.PinSubCategoryObject = Assignment.ViewModelClass.LoadSynchronous();
			}
		}
	}
}
