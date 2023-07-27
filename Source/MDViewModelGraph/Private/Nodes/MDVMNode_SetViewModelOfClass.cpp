#include "Nodes/MDVMNode_SetViewModelOfClass.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

UMDVMNode_SetViewModelOfClass::UMDVMNode_SetViewModelOfClass()
{
	FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UMDViewModelFunctionLibrary, BP_SetViewModelOfClass), UMDViewModelFunctionLibrary::StaticClass());
}

void UMDVMNode_SetViewModelOfClass::PostReconstructNode()
{
	Super::PostReconstructNode();

	UpdateAssignmentBasedData();
}

void UMDVMNode_SetViewModelOfClass::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Pin != nullptr && Pin->GetFName() == TEXT("Assignment"))
	{
		UpdateAssignmentBasedData();
	}
}

void UMDVMNode_SetViewModelOfClass::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UpdateAssignmentBasedData();
}

void UMDVMNode_SetViewModelOfClass::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	UpdateAssignmentBasedData();
}

void UMDVMNode_SetViewModelOfClass::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
	const UClass* ActionKey = GetClass();
	if (InActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		InActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UMDVMNode_SetViewModelOfClass::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	if (!bAreViewModelSettingsValid)
	{
		static constexpr TCHAR ErrorFormat[] = TEXT("Node [@@] requires valid data of type %s for View Model Settings.");
		MessageLog.Error(*FString::Printf(ErrorFormat, *ViewModelSettingsDisplayName.ToString()), this);
	}
}

void UMDVMNode_SetViewModelOfClass::UpdateAssignmentBasedData()
{
	FMDViewModelAssignmentReference Assignment;
	GetAssignmentFromPinDefaults(Assignment);

	const TSubclassOf<UMDViewModelBase> ViewModelClass = Assignment.ViewModelClass.LoadSynchronous();
	
	// Change the return pin's class to the assigned view model class
	if (IsValid(ViewModelClass))
	{
		if (UEdGraphPin* ReturnPin = GetReturnValuePin())
		{
			ReturnPin->PinType.PinSubCategoryObject = ViewModelClass;
		}
		
#if WITH_EDITOR
		bool bAreViewModelSettingsRequired = false;
		
		// Check if view model settings are required by the view model
		if (const UMDViewModelBase* VMCDO = ViewModelClass.GetDefaultObject())
		{
			bAreViewModelSettingsRequired = VMCDO->GetViewModelSettingsStruct() != nullptr;
			ViewModelSettingsDisplayName = bAreViewModelSettingsRequired ? VMCDO->GetViewModelSettingsStruct()->GetDisplayNameText() : FText::GetEmpty();
		}
		else
		{
			ViewModelSettingsDisplayName = FText::GetEmpty();
		}

		UEdGraphPin* VMSettingsPin = FindPinChecked(TEXT("ViewModelSettings"));
		VMSettingsPin->bHidden = !IsValid(ViewModelClass) || !bAreViewModelSettingsRequired;
		if (bAreViewModelSettingsRequired)
		{
			bAreViewModelSettingsValid = VMSettingsPin->HasAnyConnections();
		}
		else
		{
			bAreViewModelSettingsValid = true;
			VMSettingsPin->BreakAllPinLinks(true);
		}		
#endif

		ErrorMsg.Reset();
		ErrorType = EMessageSeverity::Info + 1;
		bHasCompilerMessage = false;
		
		FCompilerResultsLog Log;
		ValidateNodeDuringCompilation(Log);
	}
}

void UMDVMNode_SetViewModelOfClass::GetAssignmentFromPinDefaults(FMDViewModelAssignmentReference& OutAssignment) const
{
	const UEdGraphPin* AssignmentPin = FindPinChecked(TEXT("Assignment"));
	const FString DefaultString = AssignmentPin->GetDefaultAsString();
	if (!DefaultString.IsEmpty())
	{
		UScriptStruct* PinLiteralStructType = FMDViewModelAssignmentReference::StaticStruct();
		PinLiteralStructType->ImportText(*DefaultString, &OutAssignment, nullptr, PPF_SerializedAsImportText, GError, PinLiteralStructType->GetName());
	}
}
