#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropEvent.h"

#include "EdGraphSchema_K2_Actions.h"
#include "Nodes/MDVMNode_ViewModelEvent.h"


TSharedRef<FMDVMInspectorDragAndDropEvent> FMDVMInspectorDragAndDropEvent::Create(TWeakFieldPtr<const FMulticastDelegateProperty> InEventPropertyPtr, const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropEvent> Action = MakeShared<FMDVMInspectorDragAndDropEvent>();
	Action->EventPropertyPtr = InEventPropertyPtr;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

FText FMDVMInspectorDragAndDropEvent::GetActionTitle() const
{
#if WITH_EDITORONLY_DATA
	if (const FMulticastDelegateProperty* EventProperty = EventPropertyPtr.Get())
	{
		return EventProperty->GetDisplayNameText();
	}
#endif

	return FMDVMInspectorDragAndDropActionBase::GetActionTitle();
}

UEdGraphNode* FMDVMInspectorDragAndDropEvent::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	if (!EventPropertyPtr.IsValid())
	{
		return nullptr;
	}

	return FEdGraphSchemaAction_K2NewNode::CreateNode(
		&Graph,
		TArrayView<UEdGraphPin*>(),
		GraphPosition,
		[](UEdGraph* InParentGraph)->UK2Node*
		{
			return NewObject<UK2Node>(InParentGraph, UMDVMNode_ViewModelEvent::StaticClass());
		},
		[this](UK2Node* NewNode)
		{
			if (UMDVMNode_ViewModelEvent* EventNode = Cast<UMDVMNode_ViewModelEvent>(NewNode))
			{
				EventNode->InitializeViewModelEventParams(VMAssignment, EventPropertyPtr.Get()->GetFName());
			}
		},
		EK2NewNodeFlags::SelectNewNode
	);
}
