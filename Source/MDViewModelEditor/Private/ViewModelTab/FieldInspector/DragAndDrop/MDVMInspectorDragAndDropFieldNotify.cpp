#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropFieldNotify.h"

#include "EdGraphSchema_K2_Actions.h"
#include "Nodes/MDVMNode_ViewModelFieldNotify.h"


TSharedRef<FMDVMInspectorDragAndDropFieldNotify> FMDVMInspectorDragAndDropFieldNotify::Create(FFieldVariant InField, const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropFieldNotify> Action = MakeShared<FMDVMInspectorDragAndDropFieldNotify>();
	Action->Field = InField;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

FText FMDVMInspectorDragAndDropFieldNotify::GetActionTitle() const
{
#if WITH_EDITORONLY_DATA
	if (const FProperty* Property = Field.Get<FProperty>())
	{
		return Property->GetDisplayNameText();
	}
	else if (const UFunction* Function = Field.Get<UFunction>())
	{
		return Function->GetDisplayNameText();
	}
#endif

	return FMDVMInspectorDragAndDropActionBase::GetActionTitle();
}

UEdGraphNode* FMDVMInspectorDragAndDropFieldNotify::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	if (!Field.IsValid())
	{
		return nullptr;
	}

	return FEdGraphSchemaAction_K2NewNode::CreateNode(
		&Graph,
		TArrayView<UEdGraphPin*>(),
		GraphPosition,
		[](UEdGraph* InParentGraph)->UK2Node*
		{
			return NewObject<UK2Node>(InParentGraph, UMDVMNode_ViewModelFieldNotify::StaticClass());
		},
		[this](UK2Node* NewNode)
		{
			if (UMDVMNode_ViewModelFieldNotify* FieldNotifyNode = Cast<UMDVMNode_ViewModelFieldNotify>(NewNode))
			{
				FieldNotifyNode->InitializeViewModelFieldNotifyParams(VMAssignment, Field.GetFName());
			}
		},
		EK2NewNodeFlags::SelectNewNode
	);
}
