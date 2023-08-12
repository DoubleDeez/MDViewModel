#pragma once

#include "MDVMInspectorDragAndDropActionBase.h"


class FMDVMInspectorDragAndDropCommand : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropCommand, FMDVMInspectorDragAndDropActionBase)
	
	static TSharedRef<FMDVMInspectorDragAndDropCommand> Create(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment);

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;
	
	virtual FText GetActionTitle() const override;
	
protected:
	TWeakObjectPtr<const UFunction> FunctionPtr;
};
