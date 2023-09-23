#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropCommand.h"

#include "Nodes/MDVMNode_CallCommand.h"


TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDVMInspectorDragAndDropCommand::Create(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropCommand> Action = MakeShared<FMDVMInspectorDragAndDropCommand>();
	Action->FunctionPtr = InFunctionPtr;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

TSubclassOf<UMDVMNode_CallFunctionBase> FMDVMInspectorDragAndDropCommand::GetNodeClass() const
{
	return UMDVMNode_CallCommand::StaticClass();
}
