#include "Nodes/MDVMNode_CallCommand.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "KismetCompiler.h"
#include "Util/MDViewModelGraphStatics.h"
#include "Nodes/MDVMNode_GetViewModel.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"

UMDViewModelCommandNodeSpawner* UMDViewModelCommandNodeSpawner::Create(const FMDViewModelAssignmentReference& Assignment, const UFunction* Function, const UWidgetBlueprint* WidgetBP)
{
	UMDViewModelCommandNodeSpawner* Spawner = NewObject<UMDViewModelCommandNodeSpawner>();
	Spawner->NodeClass = UMDVMNode_CallCommand::StaticClass();
	Spawner->Assignment = Assignment;
	Spawner->FunctionPtr = Function;
	Spawner->WidgetBPPtr = WidgetBP;

	const FText VMClassDisplayName = Assignment.ViewModelClass != nullptr ? Assignment.ViewModelClass->GetDisplayNameText() : FText::GetEmpty(); 

	FBlueprintActionUiSpec& MenuSignature = Spawner->DefaultMenuSignature;
	MenuSignature.MenuName = UK2Node_CallFunction::GetUserFacingFunctionName(Function);
	MenuSignature.Category = FText::Format(INVTEXT("View Model Commands|{0} ({1})"), VMClassDisplayName, FText::FromName(Assignment.ViewModelName));
	MenuSignature.Tooltip = FText::FromString(UK2Node_CallFunction::GetDefaultTooltipForFunction(Function));
	MenuSignature.Keywords = UK2Node_CallFunction::GetKeywordsForFunction(Function);

	if (MenuSignature.Keywords.IsEmpty())
	{
		MenuSignature.Keywords = FText::Format(INVTEXT("{0} {1}"), VMClassDisplayName, MenuSignature.MenuName);
	}

	return Spawner;
}

FBlueprintNodeSignature UMDViewModelCommandNodeSpawner::GetSpawnerSignature() const
{
	FBlueprintNodeSignature SpawnerSignature(NodeClass);
	SpawnerSignature.AddNamedValue(TEXT("ViewModelName"), Assignment.ViewModelName.ToString());
	if (FunctionPtr.IsValid())
	{
		SpawnerSignature.AddSubObject(FunctionPtr.Get());
	}

	return SpawnerSignature;
}

UEdGraphNode* UMDViewModelCommandNodeSpawner::Invoke(UEdGraph* ParentGraph, const FBindingSet& Bindings, const FVector2D Location) const
{
	auto InitNode = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FMDViewModelAssignmentReference VMAssignment, const UFunction* Func, const UWidgetBlueprint* WidgetBP)
	{
		if (UMDVMNode_CallCommand* CommandNode = Cast<UMDVMNode_CallCommand>(NewNode))
		{
			CommandNode->InitializeViewModelCommandParams(VMAssignment, Func, WidgetBP);
		}
	};

	FCustomizeNodeDelegate InitNodeDelegate = FCustomizeNodeDelegate::CreateStatic(InitNode, Assignment, FunctionPtr.Get(), WidgetBPPtr.Get());
	return SpawnNode<UEdGraphNode>(NodeClass, ParentGraph, Bindings, Location, MoveTemp(InitNodeDelegate));
}

void UMDVMNode_CallCommand::GetMenuActions(FBlueprintActionDatabaseRegistrar& InActionRegistrar) const
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
				
				if (Func->HasAllFunctionFlags(FUNC_BlueprintCallable) && !Func->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Static))
				{
					if (InActionRegistrar.IsOpenForRegistration(WidgetBP))
					{
						FMDViewModelAssignmentReference AssignmentReference;
						AssignmentReference.ViewModelClass = It.Key().ViewModelClass;
						AssignmentReference.ViewModelName = It.Key().ViewModelName;
						
						UMDViewModelCommandNodeSpawner* NodeSpawner = UMDViewModelCommandNodeSpawner::Create(AssignmentReference, Func, WidgetBP);
						InActionRegistrar.AddBlueprintAction(WidgetBP, NodeSpawner);
					}
				}
			}
		}
	}
}

bool UMDVMNode_CallCommand::IsActionFilteredOut(const FBlueprintActionFilter& Filter)
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

void UMDVMNode_CallCommand::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin* InvalidVMPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT("InvalidVM"));
	InvalidVMPin->PinFriendlyName = INVTEXT("Invalid View Model");
	InvalidVMPin->PinToolTip = TEXT("Use this pin to handle calling the command when the view model is null.");

	// Always hide the self pin, it's going to come from our auto-generated GetViewModel node in ExpandNode
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	if (UEdGraphPin* SelfPin = Schema->FindSelfPin(*this, EGPD_Input))
	{
		SelfPin->bHidden = true;	
	}
}

void UMDVMNode_CallCommand::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
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

FText UMDVMNode_CallCommand::GetFunctionContextString() const
{
	const FText CallFunctionClassName = (Assignment.ViewModelClass.Get() != nullptr) ? Assignment.ViewModelClass.Get()->GetDisplayNameText() : FText::GetEmpty();
	FFormatNamedArguments Args;
	Args.Add(TEXT("ViewModelClass"), CallFunctionClassName);
	Args.Add(TEXT("ViewModelName"), FText::FromName(Assignment.ViewModelName));
	return FText::Format(INVTEXT("Command on {ViewModelClass} ({ViewModelName})"), Args);
}

FNodeHandlingFunctor* UMDVMNode_CallCommand::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FNodeHandlingFunctor(CompilerContext);
}

void UMDVMNode_CallCommand::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
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
		GetViewModelNode->AllocateDefaultPins();
		GetViewModelNode->SetDefaultAssignment(Assignment);

		// CallFunction
		UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallFuncNode->SetFromFunction(Function);
		CallFuncNode->AllocateDefaultPins();

		// Connect GetViewModel return value to CallFunction self pin
		UEdGraphPin* GetViewModelReturnValuePin = GetViewModelNode->GetReturnValuePin();
		UEdGraphPin* CallFuncSelfPin = Schema->FindSelfPin(*CallFuncNode, EGPD_Input);
		GetViewModelReturnValuePin->MakeLinkTo(CallFuncSelfPin);

		// Connect Branch True pin to CallFunction exec pin
		UEdGraphPin* GetViewModelTruePin = GetViewModelNode->GetTruePin();
		UEdGraphPin* CallFuncExecInPin = Schema->FindExecutionPin(*CallFuncNode, EGPD_Input);
		GetViewModelTruePin->MakeLinkTo(CallFuncExecInPin);

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
	}
}

void UMDVMNode_CallCommand::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

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
		static constexpr TCHAR ErrorFormat[] = TEXT("Command [@@] is from (%s) with name (%s) but is not assigned to this widget. Assign a view model with the name and type or delete this command node.");
		const FText ClassName = Assignment.ViewModelClass != nullptr ? Assignment.ViewModelClass->GetDisplayNameText() : INVTEXT("[INVALID]");
		MessageLog.Error(*FString::Printf(ErrorFormat, *ClassName.ToString(), *Assignment.ViewModelName.ToString()), this);
	}
}

void UMDVMNode_CallCommand::InitializeViewModelCommandParams(const FMDViewModelAssignmentReference& VMAssignment, const UFunction* Function, const UWidgetBlueprint* WidgetBP)
{
	SetFromFunction(Function);
	Assignment = VMAssignment;
	ExpectedWidgetBP = WidgetBP;
}
