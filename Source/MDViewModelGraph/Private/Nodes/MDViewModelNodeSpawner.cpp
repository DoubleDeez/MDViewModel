#include "Nodes/MDViewModelNodeSpawner.h"

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Variable.h"
#include "Nodes/MDVMNode_CallFunctionBase.h"
#include "Nodes/MDVMNode_GetProperty.h"
#include "Nodes/MDVMNode_SetProperty.h"
#include "ViewModel/MDViewModelBase.h"

UMDViewModelNodeSpawner* UMDViewModelNodeSpawner::Create(TSubclassOf<UEdGraphNode> NodeClass, const FText& Category, const FMDViewModelAssignmentReference& Assignment, FFieldVariant Field, const UBlueprint* Blueprint, const FText& NameFormat)
{
	UMDViewModelNodeSpawner* Spawner = NewObject<UMDViewModelNodeSpawner>();
	Spawner->SetField(Field);
	Spawner->NodeClass = NodeClass;
	Spawner->Assignment = Assignment;
	Spawner->BlueprintPtr = Blueprint;

	TSubclassOf<UMDViewModelBase> ViewModelClass = Assignment.ViewModelClass.LoadSynchronous();
	const FText VMClassDisplayName = IsValid(ViewModelClass) ? ViewModelClass->GetDisplayNameText() : FText::GetEmpty();

	FBlueprintActionUiSpec& MenuSignature = Spawner->DefaultMenuSignature;
	if (const UFunction* Function = Field.Get<const UFunction>())
	{
		MenuSignature.MenuName = FText::Format(NameFormat, UK2Node_CallFunction::GetUserFacingFunctionName(Function));
		MenuSignature.Category = FText::Format(INVTEXT("{0}|{1} ({2})"), Category, VMClassDisplayName, FText::FromName(Assignment.ViewModelName));
		MenuSignature.Tooltip = FText::FromString(UK2Node_CallFunction::GetDefaultTooltipForFunction(Function));
		MenuSignature.Keywords = UK2Node_CallFunction::GetKeywordsForFunction(Function);
		MenuSignature.Icon = UK2Node_CallFunction::GetPaletteIconForFunction(Function, MenuSignature.IconTint);
	}
	else if (const FProperty* Property = Field.Get<const FProperty>())
	{
		FEdGraphPinType VarType;
		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		K2Schema->ConvertPropertyToPinType(Property, VarType);

		MenuSignature.MenuName = FText::Format(NameFormat, Property->GetDisplayNameText());
		MenuSignature.Category = FText::Format(INVTEXT("{0}|{1} ({2})"), Category, VMClassDisplayName, FText::FromName(Assignment.ViewModelName));
		MenuSignature.Tooltip = Property->GetToolTipText();
		MenuSignature.Icon = UK2Node_Variable::GetVarIconFromPinType(VarType, MenuSignature.IconTint);
	}

	if (MenuSignature.Keywords.IsEmpty())
	{
		MenuSignature.Keywords = FText::Format(INVTEXT("{0} {1}"), VMClassDisplayName, MenuSignature.MenuName);
	}

	return Spawner;
}

FBlueprintNodeSignature UMDViewModelNodeSpawner::GetSpawnerSignature() const
{
	FBlueprintNodeSignature SpawnerSignature = FBlueprintNodeSignature(NodeClass);
	SpawnerSignature.AddSubObject(Assignment.ViewModelClass.LoadSynchronous());
	SpawnerSignature.AddNamedValue(TEXT("ViewModelName"), Assignment.ViewModelName.ToString());

	return SpawnerSignature;
}

UEdGraphNode* UMDViewModelNodeSpawner::Invoke(UEdGraph* ParentGraph, const FBindingSet& Bindings, const FVector2D Location) const
{
	auto InitNode = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FMDViewModelAssignmentReference VMAssignment, FFieldVariant InField, const UBlueprint* Blueprint)
	{
		if (UMDVMNode_CallFunctionBase* CommandNode = Cast<UMDVMNode_CallFunctionBase>(NewNode))
		{
			CommandNode->InitializeViewModelFunctionParams(VMAssignment, InField.Get<const UFunction>(), Blueprint);
		}
		else if (UMDVMNode_GetProperty* GetPropertyNode = Cast<UMDVMNode_GetProperty>(NewNode))
		{
			GetPropertyNode->InitializeViewModelPropertyParams(VMAssignment, InField.Get<const FProperty>(), Blueprint);
		}
		else if (UMDVMNode_SetProperty* SetPropertyNode = Cast<UMDVMNode_SetProperty>(NewNode))
		{
			SetPropertyNode->InitializeViewModelPropertyParams(VMAssignment, InField.Get<const FProperty>(), Blueprint);
		}
	};

	FCustomizeNodeDelegate InitNodeDelegate = FCustomizeNodeDelegate::CreateStatic(InitNode, Assignment, GetField(), BlueprintPtr.Get());
	return SpawnNode<UEdGraphNode>(NodeClass, ParentGraph, Bindings, Location, MoveTemp(InitNodeDelegate));
}
