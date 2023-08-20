#pragma once

#include "MDVMInspectorDragAndDropActionBase.h"


class FMDVMInspectorDragAndDropGetter : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropGetter, FMDVMInspectorDragAndDropActionBase)
	
	static TSharedRef<FMDVMInspectorDragAndDropGetter> Create(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment);

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;
	
	virtual FText GetActionTitle() const override;
	
protected:
	TWeakObjectPtr<const UFunction> FunctionPtr;
};
