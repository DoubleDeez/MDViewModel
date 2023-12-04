#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropChanged.h"

#include "EdGraphSchema_K2_Actions.h"
#include "Nodes/MDVMNode_ViewModelChanged.h"


TSharedRef<FMDVMInspectorDragAndDropChanged> FMDVMInspectorDragAndDropChanged::Create(const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropChanged> Action = MakeShared<FMDVMInspectorDragAndDropChanged>();
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

FText FMDVMInspectorDragAndDropChanged::GetActionTitle() const
{
#if WITH_EDITORONLY_DATA
	if (VMAssignment.IsAssignmentValid())
	{
		return VMAssignment.GetDisplayText();
	}
#endif

	return FMDVMInspectorDragAndDropActionBase::GetActionTitle();
}

UEdGraphNode* FMDVMInspectorDragAndDropChanged::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	return FEdGraphSchemaAction_K2NewNode::CreateNode(
		&Graph,
		TArrayView<UEdGraphPin*>(),
		GraphPosition,
		[](UEdGraph* InParentGraph)->UK2Node*
		{
			return NewObject<UK2Node>(InParentGraph, UMDVMNode_ViewModelChanged::StaticClass());
		},
		[this](UK2Node* NewNode)
		{
			if (UMDVMNode_ViewModelChanged* ChangedNode = Cast<UMDVMNode_ViewModelChanged>(NewNode))
			{
				ChangedNode->InitializeViewModelChangedParams(VMAssignment);
			}
		},
		EK2NewNodeFlags::SelectNewNode
	);
}
