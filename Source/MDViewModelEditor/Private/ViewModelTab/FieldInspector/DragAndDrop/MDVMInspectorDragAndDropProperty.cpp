#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropProperty.h"

#include "EdGraphSchema_K2_Actions.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Nodes/MDVMNode_GetProperty.h"
#include "Nodes/MDVMNode_SetProperty.h"
#include "ViewModel/MDViewModelBase.h"


TSharedRef<FMDVMInspectorDragAndDropProperty> FMDVMInspectorDragAndDropProperty::Create(TWeakFieldPtr<const FProperty> InPropertyPtr, const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropProperty> Action = MakeShared<FMDVMInspectorDragAndDropProperty>();
	Action->PropertyPtr = InPropertyPtr;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	Action->SetCanSet(InPropertyPtr.IsValid() && !InPropertyPtr->HasAllPropertyFlags(CPF_BlueprintReadOnly));
	return Action;
}

void FMDVMInspectorDragAndDropProperty::CreateGetter(FMDVMCachedViewModelPropertyNodeParams Params)
{
	if (Params.Graph.IsValid() && Params.PropertyPtr.IsValid())
	{
		UMDVMNode_GetProperty* NewNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_GetProperty>(
			Params.Graph.Get(),
			Params.GraphPosition,
			EK2NewNodeFlags::SelectNewNode,
			[&](UMDVMNode_GetProperty* NewInstance)
			{
				NewInstance->InitializeViewModelPropertyParams(Params.VMAssignment, Params.PropertyPtr.Get());
			}
		);

		FinalizeNode(NewNode, Params);
	}
}

void FMDVMInspectorDragAndDropProperty::CreateSetter(FMDVMCachedViewModelPropertyNodeParams Params)
{
	if (Params.Graph.IsValid())
	{
		UMDVMNode_SetProperty* NewNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_SetProperty>(
			Params.Graph.Get(),
			Params.GraphPosition,
			EK2NewNodeFlags::SelectNewNode,
			[&](UMDVMNode_SetProperty* NewInstance)
			{
				NewInstance->InitializeViewModelPropertyParams(Params.VMAssignment, Params.PropertyPtr.Get());
			}
		);

		FinalizeNode(NewNode, Params);
	}
}

void FMDVMInspectorDragAndDropProperty::FinalizeNode(UEdGraphNode* NewNode, const FMDVMCachedViewModelPropertyNodeParams& Params)
{
	if (NewNode)
	{
		if (UEdGraphPin* FromPin = Params.Pin.Get())
		{
			NewNode->AutowireNewNode(FromPin);
		}
		else if (const UEdGraphNode* FromNode = Params.Node.Get())
		{
			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
			if (UEdGraphPin* FromNodeIn = Schema->FindExecutionPin(*FromNode, EGPD_Input))
			{
				NewNode->AutowireNewNode(FromNodeIn);
			}
			else if (UEdGraphPin* FromNodeOut = Schema->FindExecutionPin(*FromNode, EGPD_Output))
			{
				NewNode->AutowireNewNode(FromNodeOut);
			}
		}
	}
}

FReply FMDVMInspectorDragAndDropProperty::DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	const bool bModifiedKeysActive = ModifierKeys.IsControlDown() || ModifierKeys.IsAltDown();
	const bool bAutoCreateGetter = !bCanSet || (bModifiedKeysActive ? ModifierKeys.IsControlDown() : bControlDrag);
	const bool bAutoCreateSetter = bModifiedKeysActive ? ModifierKeys.IsAltDown() : bAltDrag;

	if (bAutoCreateGetter || bAutoCreateSetter)
	{
		bIsGetter = bAutoCreateGetter;
	}
	else if (const UEdGraphPin* FromPin = GetHoveredPin())
	{
		if (FromPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object || FromPin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject)
		{
			const UClass* PinClass = Cast<UClass>(FromPin->PinType.PinSubCategoryObject.Get());
			if (PinClass != nullptr && PinClass->IsChildOf<UMDViewModelBase>())
			{
				bIsGetter = FromPin->Direction == EGPD_Input;
			}
		}
	}

	if (bIsGetter.IsSet())
	{
		return FMDVMInspectorDragAndDropActionBase::DroppedOnPin(ScreenPosition, GraphPosition);
	}

	FMDVMCachedViewModelPropertyNodeParams Params = { VMAssignment, PropertyPtr, GraphPosition, GetHoveredGraph(), GetHoveredNode(), GetHoveredPin() };

	FMenuBuilder MenuBuilder(true, NULL);
	const FText Title = GetActionTitle();

	MenuBuilder.BeginSection("ViewModelDropped", Title );

	MenuBuilder.AddMenuEntry(
		FText::Format( INVTEXT("Get {0}"), Title ),
		FText::Format( INVTEXT("Create Getter for '{0}'\n(Ctrl-drag to automatically create a getter)"), Title ),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FMDVMInspectorDragAndDropProperty::CreateGetter, Params))
	);

	MenuBuilder.AddMenuEntry(
		FText::Format( INVTEXT("Set {0}"), Title ),
		FText::Format( INVTEXT("Create Setter for '{0}'\n(Alt-drag to automatically create a setter)"), Title ),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FMDVMInspectorDragAndDropProperty::CreateSetter, Params))
	);

	MenuBuilder.EndSection();

	TSharedPtr<SWidget> MenuParent = HoveredPanelWidget.Pin();
	if (!MenuParent.IsValid())
	{
		MenuParent = FSlateApplication::Get().GetActiveTopLevelWindow();
	}

	FSlateApplication::Get().PushMenu(
		MenuParent.ToSharedRef(),
		FWidgetPath(),
		MenuBuilder.MakeWidget(),
		ScreenPosition,
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu)
	);

	return FReply::Handled();
}

UEdGraphNode* FMDVMInspectorDragAndDropProperty::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	if (!bIsGetter.IsSet() || !PropertyPtr.IsValid())
	{
		return nullptr;
	}

	if (bIsGetter.GetValue())
	{
		bIsGetter.Reset();
		return FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_GetProperty>(
			&Graph,
			GraphPosition,
			EK2NewNodeFlags::SelectNewNode,
			[&](UMDVMNode_GetProperty* NewInstance)
			{
				NewInstance->InitializeViewModelPropertyParams(VMAssignment, PropertyPtr.Get());
			}
		);
	}
	else
	{
		bIsGetter.Reset();
		return FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_SetProperty>(
			&Graph,
			GraphPosition,
			EK2NewNodeFlags::SelectNewNode,
			[&](UMDVMNode_SetProperty* NewInstance)
			{
				NewInstance->InitializeViewModelPropertyParams(VMAssignment, PropertyPtr.Get());
			}
		);
	}
}

FText FMDVMInspectorDragAndDropProperty::GetActionTitle() const
{
#if WITH_EDITORONLY_DATA
	if (const FProperty* Prop = PropertyPtr.Get())
	{
		return Prop->GetDisplayNameText();
	}
#endif

	return FMDVMInspectorDragAndDropActionBase::GetActionTitle();
}

void FMDVMInspectorDragAndDropProperty::SetCanSet(bool InCanSet)
{
	bCanSet = InCanSet;
	if (!bCanSet)
	{
		bIsGetter = true;
	}
}
