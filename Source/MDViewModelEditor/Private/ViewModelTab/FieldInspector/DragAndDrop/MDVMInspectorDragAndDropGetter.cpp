#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropGetter.h"

#include "EdGraphSchema_K2_Actions.h"
#include "Nodes/MDVMNode_CallGetter.h"


TSharedRef<FMDVMInspectorDragAndDropGetter> FMDVMInspectorDragAndDropGetter::Create(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropGetter> Action = MakeShared<FMDVMInspectorDragAndDropGetter>();
	Action->FunctionPtr = InFunctionPtr;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

UEdGraphNode* FMDVMInspectorDragAndDropGetter::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	if (!FunctionPtr.IsValid())
	{
		return nullptr;
	}
	
	return FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_CallGetter>(
		&Graph,
		GraphPosition,
		EK2NewNodeFlags::SelectNewNode,
		[&](UMDVMNode_CallGetter* NewInstance)
		{
			NewInstance->InitializeViewModelFunctionParams(VMAssignment, FunctionPtr.Get());
		}
	);
}

FText FMDVMInspectorDragAndDropGetter::GetActionTitle() const
{
#if WITH_EDITORONLY_DATA
	if (const UFunction* Func = FunctionPtr.Get())
	{
		return Func->GetDisplayNameText();
	}
#endif
	
	return FMDVMInspectorDragAndDropActionBase::GetActionTitle();
}
