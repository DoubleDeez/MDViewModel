#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropHelper.h"

#include "Nodes/MDVMNode_CallHelper.h"


TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDVMInspectorDragAndDropHelper::Create(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropHelper> Action = MakeShared<FMDVMInspectorDragAndDropHelper>();
	Action->FunctionPtr = InFunctionPtr;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

TSubclassOf<UMDVMNode_CallFunctionBase> FMDVMInspectorDragAndDropHelper::GetNodeClass() const
{
	return UMDVMNode_CallHelper::StaticClass();
}
