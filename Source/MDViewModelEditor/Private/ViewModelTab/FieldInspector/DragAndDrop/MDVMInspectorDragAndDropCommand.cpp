#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropCommand.h"

#include "EdGraphSchema_K2_Actions.h"
#include "Nodes/MDVMNode_CallCommand.h"


TSharedRef<FMDVMInspectorDragAndDropCommand> FMDVMInspectorDragAndDropCommand::Create(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropCommand> Action = MakeShared<FMDVMInspectorDragAndDropCommand>();
	Action->FunctionPtr = InFunctionPtr;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

UEdGraphNode* FMDVMInspectorDragAndDropCommand::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	if (!FunctionPtr.IsValid())
	{
		return nullptr;
	}
	
	return FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_CallCommand>(
		&Graph,
		GraphPosition,
		EK2NewNodeFlags::SelectNewNode,
		[&](UMDVMNode_CallCommand* NewInstance)
		{
			NewInstance->InitializeViewModelCommandParams(VMAssignment, FunctionPtr.Get());
		}
	);
}

FText FMDVMInspectorDragAndDropCommand::GetActionTitle() const
{
#if WITH_EDITORONLY_DATA
	if (const UFunction* Func = FunctionPtr.Get())
	{
		return Func->GetDisplayNameText();
	}
#endif
	
	return FMDVMInspectorDragAndDropActionBase::GetActionTitle();
}
