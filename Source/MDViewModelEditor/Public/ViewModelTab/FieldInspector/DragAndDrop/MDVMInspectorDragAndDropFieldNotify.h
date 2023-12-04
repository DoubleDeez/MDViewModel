#pragma once

#include "MDVMInspectorDragAndDropActionBase.h"


class UMDVMNode_ViewModelFieldNotify;

class FMDVMInspectorDragAndDropFieldNotify : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropFieldNotify, FMDVMInspectorDragAndDropActionBase)

	static TSharedRef<FMDVMInspectorDragAndDropFieldNotify> Create(FFieldVariant InField, const FMDViewModelAssignmentReference& InVMAssignment);

	virtual FText GetActionTitle() const override;

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;

private:
	FFieldVariant Field;
};
