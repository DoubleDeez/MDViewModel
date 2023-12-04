#pragma once

#include "MDVMInspectorDragAndDropActionBase.h"
#include "UObject/WeakFieldPtr.h"


class UMDVMNode_ViewModelEvent;

class FMDVMInspectorDragAndDropEvent : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropEvent, FMDVMInspectorDragAndDropActionBase)

	static TSharedRef<FMDVMInspectorDragAndDropEvent> Create(TWeakFieldPtr<const FMulticastDelegateProperty> InEventPropertyPtr, const FMDViewModelAssignmentReference& InVMAssignment);

	virtual FText GetActionTitle() const override;

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;

private:
	TWeakFieldPtr<const FMulticastDelegateProperty> EventPropertyPtr;
};
