#pragma once
#include "MDVMInspectorDragAndDropActionBase.h"
#include "UObject/WeakFieldPtr.h"

class FMDVMInspectorDragAndDropProperty : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropProperty, FMDVMInspectorDragAndDropActionBase)
	
	static TSharedRef<FMDVMInspectorDragAndDropProperty> Create(TWeakFieldPtr<const FProperty> InPropertyPtr, const FMDViewModelAssignmentReference& InVMAssignment);

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;
	
	virtual FText GetActionTitle() const override;
	
protected:
	TWeakFieldPtr<const FProperty> PropertyPtr;
};
