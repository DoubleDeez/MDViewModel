#include "Nodes/MDVMNode_GetProperty.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Nodes/MDViewModelNodeSpawner.h"
#include "Nodes/MDVMNode_GetViewModel.h"
#include "ScopedTransaction.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprintExtension.h"
#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"

void UMDVMNode_GetProperty::AllocateDefaultPins()
{
	if (!IsNodePure())
	{
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	
		UEdGraphPin* InvalidVMPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT("InvalidVM"));
		InvalidVMPin->PinFriendlyName = INVTEXT("Invalid View Model");
		InvalidVMPin->PinToolTip = TEXT("Use this pin to handle trying to get the property when the view model is null.");
	}

	CreatePinForVariable(EGPD_Output);

	if (IsNodePure())
	{
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, TEXT("IsViewModelValid"));
	}
	
	UK2Node_Variable::AllocateDefaultPins();

	BindAssignmentChanges();
}

void UMDVMNode_GetProperty::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	UK2Node_Variable::ReallocatePinsDuringReconstruction(OldPins);
}

void UMDVMNode_GetProperty::BeginDestroy()
{
	UnbindAssignmentChanges();
	
	Super::BeginDestroy();
}

void UMDVMNode_GetProperty::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
	const UBlueprint* Blueprint = Cast<UBlueprint>(InActionRegistrar.GetActionKeyFilter());
	if (Blueprint == nullptr || (Blueprint->GeneratedClass == nullptr && Blueprint->SkeletonGeneratedClass == nullptr))
	{
		return;
	}

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(Blueprint, Assignments);

	for (auto It = Assignments.CreateConstIterator(); It; ++It)
	{
		for (TFieldIterator<const FProperty> PropIt(It.Key().ViewModelClass); PropIt; ++PropIt)
		{
			if (const FProperty* Prop = *PropIt)
			{
				if (Prop->GetOwnerUObject() == UMDViewModelBase::StaticClass() || Prop->GetOwnerUObject() == UObject::StaticClass())
				{
					continue;
				}
				
				if (IsPropertyValidForNode(Prop))
				{
					if (InActionRegistrar.IsOpenForRegistration(Blueprint))
					{
						FMDViewModelAssignmentReference AssignmentReference;
						AssignmentReference.ViewModelClass = It.Key().ViewModelClass;
						AssignmentReference.ViewModelName = It.Key().ViewModelName;

						if (UBlueprintNodeSpawner* NodeSpawner = CreateNodeSpawner(AssignmentReference, Prop, Blueprint))
						{
							InActionRegistrar.AddBlueprintAction(Blueprint, NodeSpawner);
						}
					}
				}
			}
		}
	}
}

bool UMDVMNode_GetProperty::IsActionFilteredOut(const FBlueprintActionFilter& Filter)
{
	for (const UBlueprint* Blueprint : Filter.Context.Blueprints)
	{
		if (Blueprint == nullptr || Blueprint->GeneratedClass == nullptr || ExpectedBlueprintPtr != Blueprint)
		{
			return true;
		}
		
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
		FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(Blueprint, Assignments);
		
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

FText UMDVMNode_GetProperty::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleCache.IsOutOfDate(this))
	{
		const FProperty* Property = GetPropertyForVariable();
		const FText DisplayName = Property != nullptr ? Property->GetDisplayNameText() : INVTEXT("NONE");
		TitleCache.SetCachedText(FText::Format(INVTEXT("{0}\nProperty on {1} ({2})"), DisplayName, Assignment.ViewModelClass.Get()->GetDisplayNameText(), FText::FromName(Assignment.ViewModelName)), this);
	}

	return TitleCache.GetCachedText();
}

void UMDVMNode_GetProperty::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	UK2Node_Variable::GetNodeContextMenuActions(Menu, Context);

	FText MenuEntryTitle;
	FText MenuEntryTooltip;

	auto CanExecutePurityToggle = [](bool const bInCanTogglePurity)->bool
	{
		return bInCanTogglePurity;
	};

	if (IsNodePure())
	{
		MenuEntryTitle   = INVTEXT("Convert to Validated Get");
		MenuEntryTooltip = INVTEXT("Adds in branching execution pins so that you can separately handle when the View Model is valid/invalid.");
	}
	else
	{
		MenuEntryTitle   = INVTEXT("Convert to pure Get");
		MenuEntryTooltip = INVTEXT("Removes the execution pins to make the node more versatile.");
	}

	FToolMenuSection& Section = Menu->AddSection("MDVMNodeGetProperty", INVTEXT("Get View Model Property"));
	Section.AddMenuEntry(
		"TogglePurity",
		MenuEntryTitle,
		MenuEntryTooltip,
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateUObject(const_cast<UMDVMNode_GetProperty*>(this), &UMDVMNode_GetProperty::ToggleValidation),
		FCanExecuteAction::CreateStatic(CanExecutePurityToggle, !Context->bIsDebugging),
		FIsActionChecked()
	));
}

void UMDVMNode_GetProperty::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	UK2Node_Variable::ExpandNode(CompilerContext, SourceGraph);

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	// GetViewModel
	UMDVMNode_GetViewModel* GetViewModelNode = CompilerContext.SpawnIntermediateNode<UMDVMNode_GetViewModel>(this, SourceGraph);
	GetViewModelNode->SetIsPureGet(IsNodePure());
	GetViewModelNode->AllocateDefaultPins();
	GetViewModelNode->SetDefaultAssignment(Assignment);

	// VariableGet
	UK2Node_VariableGet* VariableGetNode = CompilerContext.SpawnIntermediateNode<UK2Node_VariableGet>(this, SourceGraph);
	VariableGetNode->VariableReference = VariableReference;
	VariableGetNode->AllocateDefaultPins();
	
	// Connect GetViewModel return value to VariableGetNode self pin
	UEdGraphPin* GetViewModelReturnValuePin = GetViewModelNode->GetReturnValuePin();
	UEdGraphPin* VariableGetNodeSelfPin = Schema->FindSelfPin(*VariableGetNode, EGPD_Input);
	GetViewModelReturnValuePin->MakeLinkTo(VariableGetNodeSelfPin);
	
	const UEdGraphPin* ExecInPin = Schema->FindExecutionPin(*this, EGPD_Input);
	const UEdGraphPin* ThenPin = Schema->FindExecutionPin(*this, EGPD_Output);
	for(int32 SrcPinIdx = 0; SrcPinIdx < Pins.Num(); SrcPinIdx++)
	{
		UEdGraphPin* SrcPin = Pins[SrcPinIdx];
		if (SrcPin == nullptr)
		{
			continue;
		}

		UEdGraphPin* DestPin = [&]()
		{
			if (SrcPin == ExecInPin)
			{
				return Schema->FindExecutionPin(*GetViewModelNode, EGPD_Input);
			}

			if (SrcPin == ThenPin)
			{
				return GetViewModelNode->GetTruePin();
			}

			if (SrcPin->GetFName() == TEXT("IsViewModelValid"))
			{
				return GetViewModelNode->GetIsValidPin();
			}

			if (SrcPin->GetFName() == TEXT("InvalidVM"))
			{
				return GetViewModelNode->GetFalsePin();
			}
			
			return VariableGetNode->FindPin(SrcPin->PinName);
		}();
		
		if (DestPin != nullptr)
		{
			CompilerContext.MovePinLinksToIntermediate(*SrcPin, *DestPin);
		}
	}
	
	BreakAllNodeLinks();
}

void UMDVMNode_GetProperty::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(GetBlueprint(), Assignments);

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

void UMDVMNode_GetProperty::InitializeViewModelPropertyParams(const FMDViewModelAssignmentReference& VMAssignment, const FProperty* Property, const UBlueprint* Blueprint)
{
	SetFromProperty(Property, false, VMAssignment.ViewModelClass.Get());
	Assignment = VMAssignment;
	ExpectedBlueprintPtr = Blueprint;
}

UBlueprintNodeSpawner* UMDVMNode_GetProperty::CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const FProperty* Property, const UBlueprint* Blueprint) const
{
	return UMDViewModelNodeSpawner::Create(UMDVMNode_GetProperty::StaticClass(), INVTEXT("View Model Properties"), AssignmentReference, Property, Blueprint);
}

void UMDVMNode_GetProperty::BindAssignmentChanges()
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			if (!VMExtension->OnAssignmentChanged.IsBoundToObject(this))
			{
				VMExtension->OnAssignmentChanged.AddUObject(this, &UMDVMNode_GetProperty::OnAssignmentChanged);
			}
		}
	}
	else if (IsValid(Blueprint))
	{
		// TODO - Actor View Models
	}
}

void UMDVMNode_GetProperty::OnAssignmentChanged(const FName& OldName, const FName& NewName, TSubclassOf<UMDViewModelBase> OldClass, TSubclassOf<UMDViewModelBase> NewClass)
{
	if (Assignment.ViewModelClass.Get() == OldClass && Assignment.ViewModelName == OldName)
	{
		Modify();
		Assignment.ViewModelName = NewName;
		Assignment.ViewModelClass = NewClass;
		const FProperty* NewProp = NewClass->FindPropertyByName(VariableReference.GetMemberName());
		SetFromProperty(NewProp, false, NewClass);
		TitleCache.MarkDirty();
	}
}

void UMDVMNode_GetProperty::UnbindAssignmentChanges()
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			VMExtension->OnAssignmentChanged.RemoveAll(this);
		}
	}
	else if (IsValid(Blueprint))
	{
		// TODO - Actor View Models
	}
}

bool UMDVMNode_GetProperty::IsPropertyValidForNode(const FProperty* Property) const
{
	return Property != nullptr && Property->HasAnyPropertyFlags(CPF_BlueprintVisible);
}

void UMDVMNode_GetProperty::ToggleValidation()
{
	FText TransactionTitle;
	if(!IsNodePure())
	{
		TransactionTitle = INVTEXT("Convert to Validated Get");
	}
	else
	{
		TransactionTitle = INVTEXT("Convert to Impure Get");
	}
	
	const FScopedTransaction Transaction(TransactionTitle);
	Modify();

	SetPurity(!IsNodePure());
}
