#pragma once

#include "MDVMInspectorDragAndDropActionBase.h"
#include "UObject/WeakFieldPtr.h"

class UMDVMNode_ViewModelChanged;

class FMDVMInspectorDragAndDropChanged : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropChanged, FMDVMInspectorDragAndDropActionBase)

	static TSharedRef<FMDVMInspectorDragAndDropChanged> Create(const FMDViewModelAssignmentReference& InVMAssignment);

	virtual FText GetActionTitle() const override;

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;
};
