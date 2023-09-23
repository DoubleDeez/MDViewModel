#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropGetter.h"

#include "Nodes/MDVMNode_CallGetter.h"


TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDVMInspectorDragAndDropGetter::Create(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropGetter> Action = MakeShared<FMDVMInspectorDragAndDropGetter>();
	Action->FunctionPtr = InFunctionPtr;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

TSubclassOf<UMDVMNode_CallFunctionBase> FMDVMInspectorDragAndDropGetter::GetNodeClass() const
{
	return UMDVMNode_CallGetter::StaticClass();
}
