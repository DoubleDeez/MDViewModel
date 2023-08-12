#include "Nodes/MDVMNode_GetViewModel.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "ScopedTransaction.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

UMDVMNode_GetViewModel::UMDVMNode_GetViewModel()
{
	FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UMDViewModelFunctionLibrary, BP_GetViewModel), UMDViewModelFunctionLibrary::StaticClass());
}

void UMDVMNode_GetViewModel::AllocateDefaultPins()
{
	FString ExpandBoolAsExecsValue;
	EFunctionFlags OriginalFlags = FUNC_None;
	UFunction* Func = GetTargetFunction();
	if (Func != nullptr)
	{
		OriginalFlags = Func->FunctionFlags;
		bIsPureGet ? (Func->FunctionFlags |= FUNC_BlueprintPure) : (Func->FunctionFlags &= ~FUNC_BlueprintPure);

		if (bIsPureGet)
		{
			ExpandBoolAsExecsValue = Func->GetMetaData(FBlueprintMetadata::MD_ExpandBoolAsExecs);
			Func->RemoveMetaData(FBlueprintMetadata::MD_ExpandBoolAsExecs);
		}
	}
	
	Super::AllocateDefaultPins();

	if (PendingAssignment.IsAssignmentValid())
	{
		SetDefaultAssignment(PendingAssignment);
		PendingAssignment = {};
	}
	
	if (Func != nullptr)
	{
		Func->FunctionFlags = OriginalFlags;

		if (!Func->HasMetaData(FBlueprintMetadata::MD_ExpandBoolAsExecs))
		{
			Func->SetMetaData(FBlueprintMetadata::MD_ExpandBoolAsExecs, *ExpandBoolAsExecsValue);
		}
	}
}

void UMDVMNode_GetViewModel::ReconstructNode()
{
	Super::ReconstructNode();
	
	// Manually fix up the 'False' pin since the default implementation doesn't allow matching
	// multiple new pins to 1 old pin but we want both 'True' and 'False' to link to whatever 'Then' was previously linked to
	if (bDoesNeedOutputRemapping)
	{
		bDoesNeedOutputRemapping = false;

		UEdGraphPin* TruePin = FindPin(TEXT("True"));
		UEdGraphPin* FalsePin = FindPin(TEXT("False"));
		if (TruePin != nullptr && FalsePin != nullptr)
		{
			for (UEdGraphPin* LinkedPin : TruePin->LinkedTo)
			{
				FalsePin->MakeLinkTo(LinkedPin);
			}
		}
	}
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

void UMDVMNode_GetViewModel::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	FString ExpandBoolAsExecsValue;
	EFunctionFlags OriginalFlags = FUNC_None;
	UFunction* Func = GetTargetFunction();
	if (Func != nullptr)
	{
		OriginalFlags = Func->FunctionFlags;
		bIsPureGet ? (Func->FunctionFlags |= FUNC_BlueprintPure) : (Func->FunctionFlags &= ~FUNC_BlueprintPure);

		if (bIsPureGet)
		{
			ExpandBoolAsExecsValue = Func->GetMetaData(FBlueprintMetadata::MD_ExpandBoolAsExecs);
			Func->RemoveMetaData(FBlueprintMetadata::MD_ExpandBoolAsExecs);
		}
	}
	
	Super::ValidateNodeDuringCompilation(MessageLog);
	
	if (Func != nullptr)
	{
		Func->FunctionFlags = OriginalFlags;

		if (!Func->HasMetaData(FBlueprintMetadata::MD_ExpandBoolAsExecs))
		{
			Func->SetMetaData(FBlueprintMetadata::MD_ExpandBoolAsExecs, *ExpandBoolAsExecsValue);
		}
	}
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

void UMDVMNode_GetViewModel::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	FToolMenuSection& Section = Menu->AddSection("K2NodeVariableGet", INVTEXT("Get View Model"));
	Section.AddMenuEntry(
		"TogglePurity",
		bIsPureGet ? INVTEXT("Convert to Impure Node") : INVTEXT("Convert to Pure Node"),
		bIsPureGet ? INVTEXT("Convert to Impure Node") : INVTEXT("Convert to Pure Node"),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateUObject(const_cast<UMDVMNode_GetViewModel*>(this), &UMDVMNode_GetViewModel::TogglePurity),
		FCanExecuteAction::CreateLambda([bCanTogglePurity = !Context->bIsDebugging](){ return bCanTogglePurity; }),
		FIsActionChecked()
		)
	);
}

UK2Node::ERedirectType UMDVMNode_GetViewModel::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{	
	ERedirectType Result = Super::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);

	if (Result == ERedirectType_None)
	{
		if (OldPin != nullptr && OldPin->GetFName() == UEdGraphSchema_K2::PN_Then && NewPin->GetFName() == TEXT("True"))
		{
			bDoesNeedOutputRemapping = true;
			return ERedirectType_Name;
		}
	}

	return Result;
}

FText UMDVMNode_GetViewModel::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	if (Pin->GetFName() == TEXT("True"))
	{
		return INVTEXT("Valid");
	}
	else if (Pin->GetFName() == TEXT("False"))
	{
		return INVTEXT("Invalid");
	}
	
	return Super::GetPinDisplayName(Pin);
}

void UMDVMNode_GetViewModel::SetDefaultAssignment(const FMDViewModelAssignmentReference& Assignment)
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

void UMDVMNode_GetViewModel::TogglePurity()
{
	const FScopedTransaction Transaction( bIsPureGet ? INVTEXT("Convert to Impure Node") : INVTEXT("Convert to Pure Node") );
	Modify();

	bIsPureGet = !bIsPureGet;

	if (Pins.Num() > 0)
	{
		ReconstructNode();
	}
}
