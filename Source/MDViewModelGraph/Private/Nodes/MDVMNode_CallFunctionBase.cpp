#include "Nodes/MDVMNode_CallFunctionBase.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "Nodes/MDViewModelNodeSpawner.h"
#include "Nodes/MDVMNode_GetViewModel.h"
#include "ScopedTransaction.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"
#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"

void UMDVMNode_CallFunctionBase::BeginDestroy()
{
	UnbindAssignmentNameChanged();
	
	Super::BeginDestroy();
}

void UMDVMNode_CallFunctionBase::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
	const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(InActionRegistrar.GetActionKeyFilter());
	if (WidgetBP == nullptr || (WidgetBP->GeneratedClass == nullptr && WidgetBP->SkeletonGeneratedClass == nullptr))
	{
		return;
	}

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(WidgetBP, Assignments);

	for (auto It = Assignments.CreateConstIterator(); It; ++It)
	{
		for (TFieldIterator<UFunction> FuncIt(It.Key().ViewModelClass); FuncIt; ++FuncIt)
		{
			if (const UFunction* Func = *FuncIt)
			{
				if (Func->GetOuterUClass() == UMDViewModelBase::StaticClass())
				{
					continue;
				}
				
				if (IsFunctionValidForNode(*Func))
				{
					if (InActionRegistrar.IsOpenForRegistration(WidgetBP))
					{
						FMDViewModelAssignmentReference AssignmentReference;
						AssignmentReference.ViewModelClass = It.Key().ViewModelClass;
						AssignmentReference.ViewModelName = It.Key().ViewModelName;

						if (UBlueprintNodeSpawner* NodeSpawner = CreateNodeSpawner(AssignmentReference, Func, WidgetBP))
						{
							InActionRegistrar.AddBlueprintAction(WidgetBP, NodeSpawner);
						}
					}
				}
			}
		}
	}
}

bool UMDVMNode_CallFunctionBase::IsActionFilteredOut(const FBlueprintActionFilter& Filter)
{
	for (const UBlueprint* Blueprint : Filter.Context.Blueprints)
	{
		const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint);
		if (WidgetBP == nullptr || WidgetBP->GeneratedClass == nullptr || ExpectedWidgetBP != Blueprint)
		{
			return true;
		}
		
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
		FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(WidgetBP, Assignments);
		
		bool bWidgetHasAssignment = false;
		for (const auto& Pair : Assignments)
		{
			if (Pair.Key.ViewModelClass == Assignment.ViewModelClass && Pair.Key.ViewModelName == Assignment.ViewModelName)
			{
				bWidgetHasAssignment = true;
				break;
			}
		}

		if (!bWidgetHasAssignment)
		{
			return true;
		}
	}
	
	return Super::IsActionFilteredOut(Filter);
}

void UMDVMNode_CallFunctionBase::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	if (CanTogglePurity())
	{
		FToolMenuSection& Section = Menu->AddSection("MDVMCallFunction", INVTEXT("View Model"));
		Section.AddMenuEntry(
			"TogglePurity",
			bIsSetPure ? INVTEXT("Convert to Validated Node") : INVTEXT("Convert to Pure Node"),
			bIsSetPure ? INVTEXT("Exposes exec pins to handle if the view model is null") : INVTEXT("Convert to Pure Node without validation"),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateUObject(const_cast<UMDVMNode_CallFunctionBase*>(this), &UMDVMNode_CallFunctionBase::TogglePurity),
			FCanExecuteAction::CreateLambda([bCanTogglePurity = !Context->bIsDebugging](){ return bCanTogglePurity; }),
			FIsActionChecked()
			)
		);
	}
}

void UMDVMNode_CallFunctionBase::ReconstructNode()
{
	Super::ReconstructNode();

	BindAssignmentNameChanged();
}

void UMDVMNode_CallFunctionBase::AllocateDefaultPins()
{
	EFunctionFlags OriginalFlags = FUNC_None;
	UFunction* Func = GetTargetFunction();
	if (Func != nullptr)
	{
		OriginalFlags = Func->FunctionFlags;
		bIsSetPure ? (Func->FunctionFlags |= FUNC_BlueprintPure) : (Func->FunctionFlags &= ~FUNC_BlueprintPure);
	}
	
	Super::AllocateDefaultPins();

	if (!IsNodePure())
	{
		UEdGraphPin* InvalidVMPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT("InvalidVM"));
		InvalidVMPin->PinFriendlyName = INVTEXT("Invalid View Model");
		InvalidVMPin->PinToolTip = TEXT("Use this pin to handle calling the command when the view model is null.");
	}

	// Always hide the self pin, it's going to come from our auto-generated GetViewModel node in ExpandNode
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	if (UEdGraphPin* SelfPin = Schema->FindSelfPin(*this, EGPD_Input))
	{
		SelfPin->bHidden = true;	
	}
	
	if (Func != nullptr)
	{
		Func->FunctionFlags = OriginalFlags;
	}
}

void UMDVMNode_CallFunctionBase::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	if (Pin.GetName() == TEXT("InvalidVM"))
	{
		// Don't get CallFunction override our tooltip
		HoverTextOut = Pin.PinToolTip;
	}
	else
	{
		Super::GetPinHoverText(Pin, HoverTextOut);
	}
}

FText UMDVMNode_CallFunctionBase::GetFunctionContextString() const
{
	const UClass* VMClass = Assignment.ViewModelClass.Get();
	const FText CallFunctionClassName = (VMClass != nullptr) ? VMClass->GetDisplayNameText() : FText::GetEmpty();
	FFormatNamedArguments Args;
	Args.Add(TEXT("ViewModelClass"), CallFunctionClassName);
	Args.Add(TEXT("ViewModelName"), FText::FromName(Assignment.ViewModelName));
	return FText::Format(GetFunctionContextFormat(), Args);
}

FText UMDVMNode_CallFunctionBase::GetFunctionContextFormat() const
{
	return INVTEXT("Function on {ViewModelClass} ({ViewModelName})");
}

FNodeHandlingFunctor* UMDVMNode_CallFunctionBase::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FNodeHandlingFunctor(CompilerContext);
}

void UMDVMNode_CallFunctionBase::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	// Skip UK2Node_CallFunction, we're generating a CallFunction node below instead
	UK2Node::ExpandNode(CompilerContext, SourceGraph);

	// Generate a CallFunction node to call the actual command function and a GetViewModel node to get the view model we're calling the command on
	UFunction* Function = GetTargetFunction();
	if (Function != nullptr)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// GetViewModel
		UMDVMNode_GetViewModel* GetViewModelNode = CompilerContext.SpawnIntermediateNode<UMDVMNode_GetViewModel>(this, SourceGraph);
		GetViewModelNode->SetIsPureGet(IsNodePure());
		GetViewModelNode->AllocateDefaultPins();
		GetViewModelNode->SetDefaultAssignment(Assignment);
		
		const EFunctionFlags OriginalFlags = Function->FunctionFlags;
		bIsSetPure ? (Function->FunctionFlags |= FUNC_BlueprintPure) : (Function->FunctionFlags &= ~FUNC_BlueprintPure);
		
		// CallFunction
		UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallFuncNode->SetFromFunction(Function);
		CallFuncNode->AllocateDefaultPins();

		// Connect GetViewModel return value to CallFunction self pin
		UEdGraphPin* GetViewModelReturnValuePin = GetViewModelNode->GetReturnValuePin();
		UEdGraphPin* CallFuncSelfPin = Schema->FindSelfPin(*CallFuncNode, EGPD_Input);
		GetViewModelReturnValuePin->MakeLinkTo(CallFuncSelfPin);

		// Perform Exec connections
		if (!IsNodePure())
		{
			// Connect GetViewModel True pin to function Exec input
			UEdGraphPin* GetViewModelTruePin = GetViewModelNode->GetTruePin();
			UEdGraphPin* CallFuncExecInPin = Schema->FindExecutionPin(*CallFuncNode, EGPD_Input);
			GetViewModelTruePin->MakeLinkTo(CallFuncExecInPin);
		}

		// Copy over all other connections
		const UEdGraphPin* SelfPin = Schema->FindSelfPin(*this, EGPD_Input);
		const UEdGraphPin* ExecInPin = Schema->FindExecutionPin(*this, EGPD_Input);
		for(int32 SrcPinIdx = 0; SrcPinIdx < Pins.Num(); SrcPinIdx++)
		{
			UEdGraphPin* SrcPin = Pins[SrcPinIdx];
			if (SrcPin == nullptr || SrcPin == SelfPin)
			{
				continue;
			}

			UEdGraphPin* DestPin = [&]()
			{
				if (SrcPin == ExecInPin)
				{
					return Schema->FindExecutionPin(*GetViewModelNode, EGPD_Input);
				}

				if (SrcPin->GetFName() == TEXT("InvalidVM"))
				{
					return GetViewModelNode->GetFalsePin();
				}
				
				return CallFuncNode->FindPin(SrcPin->PinName);
			}();
			
			if (DestPin != nullptr)
			{
				CompilerContext.MovePinLinksToIntermediate(*SrcPin, *DestPin);
			}
		}
		
		BreakAllNodeLinks();
	
		if (Function != nullptr)
		{
			Function->FunctionFlags = OriginalFlags;
		}
	}
}

void UMDVMNode_CallFunctionBase::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	EFunctionFlags OriginalFlags = FUNC_None;
	UFunction* Func = GetTargetFunction();
	if (Func != nullptr)
	{
		if (!IsFunctionValidForNode(*Func))
		{
			static constexpr TCHAR ErrorFormat[] = TEXT("Node [@@] has incompatible function (%s) and must be deleted.");
			MessageLog.Error(*FString::Printf(ErrorFormat, *Func->GetName()), this);
		}
		
		OriginalFlags = Func->FunctionFlags;
		bIsSetPure ? (Func->FunctionFlags |= FUNC_BlueprintPure) : (Func->FunctionFlags &= ~FUNC_BlueprintPure);
	}
	
	Super::ValidateNodeDuringCompilation(MessageLog);
	
	if (Func != nullptr)
	{
		Func->FunctionFlags = OriginalFlags;
	}

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(Cast<UWidgetBlueprint>(GetBlueprint()), Assignments);

	bool bWidgetHasAssignment = false;
	for (const auto& Pair : Assignments)
	{
		if (Pair.Key.ViewModelClass == Assignment.ViewModelClass && Pair.Key.ViewModelName == Assignment.ViewModelName)
		{
			bWidgetHasAssignment = true;
			break;
		}
	}

	if (!bWidgetHasAssignment)
	{
		static constexpr TCHAR ErrorFormat[] = TEXT("Node [@@] is from (%s) with name (%s) but is not assigned to this widget. Assign a view model with the name and type or delete this node.");
		const FText ClassName = Assignment.ViewModelClass != nullptr ? Assignment.ViewModelClass->GetDisplayNameText() : INVTEXT("[INVALID]");
		MessageLog.Error(*FString::Printf(ErrorFormat, *ClassName.ToString(), *Assignment.ViewModelName.ToString()), this);
	}
}

void UMDVMNode_CallFunctionBase::InitializeViewModelFunctionParams(const FMDViewModelAssignmentReference& VMAssignment, const UFunction* Function, const UWidgetBlueprint* WidgetBP)
{
	SetFromFunction(Function);
	Assignment = VMAssignment;
	ExpectedWidgetBP = WidgetBP;
	bIsSetPure = bIsPureFunc;
}

bool UMDVMNode_CallFunctionBase::CanTogglePurity() const
{
	if (const UFunction* Func = GetTargetFunction())
	{
		return Func->HasAllFunctionFlags(FUNC_BlueprintPure);
	}

	return false;
}

void UMDVMNode_CallFunctionBase::BindAssignmentNameChanged()
{
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(FBlueprintEditorUtils::FindBlueprintForNode(this)))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			if (!VMExtension->OnAssignmentNameChanged.IsBoundToObject(this))
			{
				VMExtension->OnAssignmentNameChanged.AddUObject(this, &UMDVMNode_CallFunctionBase::OnAssignmentNameChanged);
			}
		}
	}
}

void UMDVMNode_CallFunctionBase::OnAssignmentNameChanged(TSubclassOf<UMDViewModelBase> VMClass, const FName& OldName, const FName& NewName)
{
	if (Assignment.ViewModelClass.Get() == VMClass && Assignment.ViewModelName == OldName)
	{
		Modify();
		Assignment.ViewModelName = NewName;
	}
}

void UMDVMNode_CallFunctionBase::UnbindAssignmentNameChanged()
{
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(FBlueprintEditorUtils::FindBlueprintForNode(this)))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			VMExtension->OnAssignmentNameChanged.RemoveAll(this);
		}
	}
}

void UMDVMNode_CallFunctionBase::TogglePurity()
{
	const FScopedTransaction Transaction(bIsSetPure ? INVTEXT("Convert to Validated Node") : INVTEXT("Convert to Pure Node"));
	Modify();

	
	bIsSetPure = !bIsSetPure;

	if (Pins.Num() > 0)
	{
		ReconstructNode();
	}
}
