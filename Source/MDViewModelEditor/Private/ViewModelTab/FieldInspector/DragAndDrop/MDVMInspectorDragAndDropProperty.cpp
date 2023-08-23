#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropProperty.h"

#include "EdGraphSchema_K2_Actions.h"
#include "K2Node_VariableGet.h"
#include "ViewModel/MDViewModelBase.h"


TSharedRef<FMDVMInspectorDragAndDropProperty> FMDVMInspectorDragAndDropProperty::Create(TWeakFieldPtr<const FProperty> InPropertyPtr, const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMInspectorDragAndDropProperty> Action = MakeShared<FMDVMInspectorDragAndDropProperty>();
	Action->PropertyPtr = InPropertyPtr;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

UEdGraphNode* FMDVMInspectorDragAndDropProperty::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	if (!PropertyPtr.IsValid())
	{
		return nullptr;
	}

	// TODO - MDVMNode_PropertyGet
	return FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_VariableGet>(
		&Graph,
		GraphPosition,
		EK2NewNodeFlags::SelectNewNode,
		[&](UK2Node_VariableGet* NewInstance)
		{
			NewInstance->SetFromProperty(PropertyPtr.Get(), false, VMAssignment.ViewModelClass.Get());
			//NewInstance->InitializeViewModelFunctionParams(VMAssignment, PropertyPtr.Get());
		}
	);
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
