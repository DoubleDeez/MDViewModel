#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropFunctionBase.h"

#include "EdGraphSchema_K2_Actions.h"
#include "Nodes/MDVMNode_CallFunctionBase.h"

FText FMDVMInspectorDragAndDropFunctionBase::GetActionTitle() const
{
#if WITH_EDITORONLY_DATA
	if (const UFunction* Func = FunctionPtr.Get())
	{
		return Func->GetDisplayNameText();
	}
#endif

	return FMDVMInspectorDragAndDropActionBase::GetActionTitle();
}

UEdGraphNode* FMDVMInspectorDragAndDropFunctionBase::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	if (!FunctionPtr.IsValid())
	{
		return nullptr;
	}

	return FEdGraphSchemaAction_K2NewNode::CreateNode(
		&Graph,
		TArrayView<UEdGraphPin*>(),
		GraphPosition,
		[NodeClass = GetNodeClass()](UEdGraph* InParentGraph)->UK2Node*
		{
			return NewObject<UK2Node>(InParentGraph, NodeClass);
		},
		[this](UK2Node* NewNode)
		{
			if (UMDVMNode_CallFunctionBase* FuncNode = Cast<UMDVMNode_CallFunctionBase>(NewNode))
			{
				FuncNode->InitializeViewModelFunctionParams(VMAssignment, FunctionPtr.Get());
			}
		},
		EK2NewNodeFlags::SelectNewNode
	);
}
