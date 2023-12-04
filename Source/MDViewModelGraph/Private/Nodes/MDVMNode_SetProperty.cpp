#include "Nodes/MDVMNode_SetProperty.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintExtensions/MDViewModelAssignableInterface.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "Nodes/MDViewModelNodeSpawner.h"
#include "Nodes/MDVMNode_GetViewModel.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModel/MDViewModelBlueprintBase.h"

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
#include "INotifyFieldValueChanged.h"
#else
#include "FieldNotification/IFieldValueChanged.h"
#endif

namespace MDVMNSP_Private
{
	bool IsPropertyNativeFieldNotify(const FProperty* Property)
	{
		if (Property && Property->IsNative())
		{
			const UClass* VarSourceClass = Property->GetOwnerClass();
			if (VarSourceClass && VarSourceClass->ImplementsInterface(UNotifyFieldValueChanged::StaticClass()) && VarSourceClass->GetDefaultObject())
			{
				const TScriptInterface<INotifyFieldValueChanged> DefaultObject = VarSourceClass->GetDefaultObject();
				return DefaultObject->GetFieldNotificationDescriptor().GetField(VarSourceClass, Property->GetFName()).IsValid();
			}
		}

		return false;
	}
}

void UMDVMNode_SetProperty::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* InvalidVMPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT("InvalidVM"));
	InvalidVMPin->PinFriendlyName = INVTEXT("Invalid View Model");
	InvalidVMPin->PinToolTip = TEXT("Use this pin to handle trying to get the property when the view model is null.");

	if (GetVarName() != NAME_None)
	{
		CreatePinForVariable(EGPD_Input);
	}

	UK2Node_Variable::AllocateDefaultPins();

	BindAssignmentChanges();
}

void UMDVMNode_SetProperty::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	UK2Node_Variable::ReallocatePinsDuringReconstruction(OldPins);
}

void UMDVMNode_SetProperty::BeginDestroy()
{
	UnbindAssignmentChanges();

	Super::BeginDestroy();
}

void UMDVMNode_SetProperty::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
{
	const UBlueprint* Blueprint = Cast<UBlueprint>(InActionRegistrar.GetActionKeyFilter());
	if (Blueprint == nullptr || !InActionRegistrar.IsOpenForRegistration(Blueprint) || (Blueprint->GeneratedClass == nullptr && Blueprint->SkeletonGeneratedClass == nullptr))
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
				if (Prop->GetOwnerUObject() == UMDViewModelBase::StaticClass() || Prop->GetOwnerUObject() == UMDViewModelBlueprintBase::StaticClass() || Prop->GetOwnerUObject() == UObject::StaticClass())
				{
					continue;
				}

				if (IsPropertyValidForNode(Prop))
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

bool UMDVMNode_SetProperty::IsActionFilteredOut(const FBlueprintActionFilter& Filter)
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
			if (Pair.Key.ViewModelClass == Assignment.ViewModelClass.Get() && Pair.Key.ViewModelName == Assignment.ViewModelName)
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

FText UMDVMNode_SetProperty::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleCache.IsOutOfDate(this))
	{
		const FProperty* Property = GetPropertyForVariable();
		const FText DisplayName = Property != nullptr ? Property->GetDisplayNameText() : INVTEXT("NONE");
		TitleCache.SetCachedText(
			FText::Format(
				INVTEXT("Set {0}{1}\nProperty on {2} ({3})"),
				DisplayName,
				(MDVMNSP_Private::IsPropertyNativeFieldNotify(GetPropertyForVariable()) ? INVTEXT(" w/ Broadcast") : FText::GetEmpty()),
				Assignment.ViewModelClass.Get()->GetDisplayNameText(),
				FText::FromName(Assignment.ViewModelName)
			), this);
	}

	return TitleCache.GetCachedText();
}

void UMDVMNode_SetProperty::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	UK2Node_Variable::ExpandNode(CompilerContext, SourceGraph);

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	UEdGraphPin* ExecInPin = Schema->FindExecutionPin(*this, EGPD_Input);
	UEdGraphPin* ThenPin = Schema->FindExecutionPin(*this, EGPD_Output);

	// GetViewModel
	UMDVMNode_GetViewModel* GetViewModelNode = CompilerContext.SpawnIntermediateNode<UMDVMNode_GetViewModel>(this, SourceGraph);
	GetViewModelNode->SetIsPureGet(false);
	GetViewModelNode->AllocateDefaultPins();
	GetViewModelNode->SetDefaultAssignment(Assignment);

	UEdGraphPin* ExecGetViewModelPin = Schema->FindExecutionPin(*GetViewModelNode, EGPD_Input);
	UEdGraphPin* GetViewModelReturnValuePin = GetViewModelNode->GetReturnValuePin();
	UEdGraphPin* GetViewModelThenValuePin = GetViewModelNode->GetTruePin();
	UEdGraphPin* GetViewModelFailedPin = GetViewModelNode->GetFalsePin();
	UEdGraphPin* InvalidVMPin = FindPinChecked(TEXT("InvalidVM"));

	CompilerContext.MovePinLinksToIntermediate(*ExecInPin, *ExecGetViewModelPin);
	CompilerContext.MovePinLinksToIntermediate(*InvalidVMPin, *GetViewModelFailedPin);

	// VariableSet
	UK2Node_VariableSet* VariableSetNode = CompilerContext.SpawnIntermediateNode<UK2Node_VariableSet>(this, SourceGraph);
	VariableSetNode->VariableReference = VariableReference;
	VariableSetNode->AllocateDefaultPins();

	FProperty* VariableProperty = GetPropertyForVariable();

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	// VariableSet doesn't broadcast Native Field Notify properties, so we'll do it here manually
	// IsFieldNotifyProperty() and HasFieldNotificationBroadcast() only check BP properties
	// See: https://github.com/EpicGames/UnrealEngine/pull/11166
	if (MDVMNSP_Private::IsPropertyNativeFieldNotify(VariableProperty) && !VariableSetNode->IsFieldNotifyProperty() && !VariableSetNode->HasFieldNotificationBroadcast())
	{
		TTuple<UEdGraphPin*, UEdGraphPin*> ExecThenPins = FKismetCompilerUtilities::GenerateFieldNotificationSetNode(CompilerContext, SourceGraph, this, GetViewModelReturnValuePin, VariableProperty, VariableReference, VariableSetNode->HasLocalRepNotify(), VariableSetNode->ShouldFlushDormancyOnSet(), VariableSetNode->IsNetProperty());

		GetViewModelThenValuePin->MakeLinkTo(ExecThenPins.Get<0>());
		CompilerContext.MovePinLinksToIntermediate(*ThenPin, *ExecThenPins.Get<1>());
	}
	else
#endif
	{
		// Connect GetViewModel return value to VariableSet node self pin
		UEdGraphPin* VariableSetNodeSelfPin = Schema->FindSelfPin(*VariableSetNode, EGPD_Input);
		GetViewModelReturnValuePin->MakeLinkTo(VariableSetNodeSelfPin);

		GetViewModelThenValuePin->MakeLinkTo(Schema->FindExecutionPin(*VariableSetNode, EGPD_Input));
		CompilerContext.MovePinLinksToIntermediate(*ThenPin, *Schema->FindExecutionPin(*VariableSetNode, EGPD_Output));
	}

	BreakAllNodeLinks();
}

void UMDVMNode_SetProperty::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(GetBlueprint(), Assignments);

	bool bWidgetHasAssignment = false;
	for (const auto& Pair : Assignments)
	{
		if (Pair.Key.ViewModelClass == Assignment.ViewModelClass.Get() && Pair.Key.ViewModelName == Assignment.ViewModelName)
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

void UMDVMNode_SetProperty::InitializeViewModelPropertyParams(const FMDViewModelAssignmentReference& VMAssignment, const FProperty* Property, const UBlueprint* Blueprint)
{
	SetFromProperty(Property, false, VMAssignment.ViewModelClass.Get());
	Assignment = VMAssignment;
	ExpectedBlueprintPtr = Blueprint;
}

UBlueprintNodeSpawner* UMDVMNode_SetProperty::CreateNodeSpawner(const FMDViewModelAssignmentReference& AssignmentReference, const FProperty* Property, const UBlueprint* Blueprint) const
{
	return UMDViewModelNodeSpawner::Create(UMDVMNode_SetProperty::StaticClass(), INVTEXT("View Model Properties"), AssignmentReference, Property, Blueprint, INVTEXT("Set {0}"));
}

void UMDVMNode_SetProperty::BindAssignmentChanges()
{
	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	if (IMDViewModelAssignableInterface* Assignments = FMDViewModelGraphStatics::GetAssignableInterface(Blueprint))
	{
		if (!Assignments->OnAssignmentChanged.IsBoundToObject(this))
		{
			Assignments->OnAssignmentChanged.AddUObject(this, &UMDVMNode_SetProperty::OnAssignmentChanged);
		}
	}
}

void UMDVMNode_SetProperty::OnAssignmentChanged(const FMDViewModelAssignmentReference& Old, const FMDViewModelAssignmentReference& New)
{
	if (Assignment == Old)
	{
		Modify();
		Assignment = New;
		UClass* NewClass = New.ViewModelClass.LoadSynchronous();
		const FProperty* NewProp = IsValid(NewClass) ? NewClass->FindPropertyByName(VariableReference.GetMemberName()) : nullptr;
		SetFromProperty(NewProp, false, NewClass);
		TitleCache.MarkDirty();
	}
}

void UMDVMNode_SetProperty::UnbindAssignmentChanges()
{
	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	if (IMDViewModelAssignableInterface* Assignments = FMDViewModelGraphStatics::GetAssignableInterface(Blueprint))
	{
		Assignments->OnAssignmentChanged.RemoveAll(this);
	}
}

bool UMDVMNode_SetProperty::IsPropertyValidForNode(const FProperty* Property) const
{
	return Property != nullptr && Property->HasAnyPropertyFlags(CPF_BlueprintVisible) && !Property->HasAnyPropertyFlags(CPF_BlueprintAssignable | CPF_BlueprintReadOnly);
}
