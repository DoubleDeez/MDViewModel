#include "Nodes/MDViewModelNodeSpawner.h"

#include "K2Node_CallFunction.h"
#include "Nodes/MDVMNode_CallFunctionBase.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"

UMDViewModelNodeSpawner* UMDViewModelNodeSpawner::Create(TSubclassOf<UEdGraphNode> NodeClass, const FText& Category, const FMDViewModelAssignmentReference& Assignment, const UFunction* Function, const UWidgetBlueprint* WidgetBP)
{
	UMDViewModelNodeSpawner* Spawner = NewObject<UMDViewModelNodeSpawner>();
	Spawner->NodeClass = NodeClass;
	Spawner->Assignment = Assignment;
	Spawner->FunctionPtr = Function;
	Spawner->WidgetBPPtr = WidgetBP;

	const FText VMClassDisplayName = Assignment.ViewModelClass != nullptr ? Assignment.ViewModelClass->GetDisplayNameText() : FText::GetEmpty(); 

	FBlueprintActionUiSpec& MenuSignature = Spawner->DefaultMenuSignature;
	MenuSignature.MenuName = UK2Node_CallFunction::GetUserFacingFunctionName(Function);
	MenuSignature.Category = FText::Format(INVTEXT("{0}|{1} ({2})"), Category, VMClassDisplayName, FText::FromName(Assignment.ViewModelName));
	MenuSignature.Tooltip = FText::FromString(UK2Node_CallFunction::GetDefaultTooltipForFunction(Function));
	MenuSignature.Keywords = UK2Node_CallFunction::GetKeywordsForFunction(Function);

	if (MenuSignature.Keywords.IsEmpty())
	{
		MenuSignature.Keywords = FText::Format(INVTEXT("{0} {1}"), VMClassDisplayName, MenuSignature.MenuName);
	}

	return Spawner;
}

FBlueprintNodeSignature UMDViewModelNodeSpawner::GetSpawnerSignature() const
{
	FBlueprintNodeSignature SpawnerSignature(NodeClass);
	SpawnerSignature.AddSubObject(Assignment.ViewModelClass.Get());
	SpawnerSignature.AddNamedValue(TEXT("ViewModelName"), Assignment.ViewModelName.ToString());
	if (FunctionPtr.IsValid())
	{
		SpawnerSignature.AddSubObject(FunctionPtr.Get());
	}

	return SpawnerSignature;
}

UEdGraphNode* UMDViewModelNodeSpawner::Invoke(UEdGraph* ParentGraph, const FBindingSet& Bindings, const FVector2D Location) const
{
	auto InitNode = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FMDViewModelAssignmentReference VMAssignment, const UFunction* Func, const UWidgetBlueprint* WidgetBP)
	{
		if (UMDVMNode_CallFunctionBase* CommandNode = Cast<UMDVMNode_CallFunctionBase>(NewNode))
		{
			CommandNode->InitializeViewModelFunctionParams(VMAssignment, Func, WidgetBP);
		}
	};

	FCustomizeNodeDelegate InitNodeDelegate = FCustomizeNodeDelegate::CreateStatic(InitNode, Assignment, FunctionPtr.Get(), WidgetBPPtr.Get());
	return SpawnNode<UEdGraphNode>(NodeClass, ParentGraph, Bindings, Location, MoveTemp(InitNodeDelegate));
}
