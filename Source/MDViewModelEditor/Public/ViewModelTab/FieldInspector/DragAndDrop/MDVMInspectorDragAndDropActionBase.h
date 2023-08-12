#pragma once

#include "GraphEditorDragDropAction.h"
#include "Util/MDViewModelAssignmentReference.h"


class FMDVMInspectorDragAndDropActionBase : public FGraphEditorDragDropAction
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropActionBase, FGraphEditorDragDropAction)
	
	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) override;
	virtual FReply DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition) override;
	virtual FReply DroppedOnPanel( const TSharedRef< SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) override;
	
	virtual bool IsSupportedBySchema(const UEdGraphSchema* Schema) const override;
	virtual void HoverTargetChanged() override;

	virtual FText GetNodeTitle() const;
	virtual FText GetActionTitle() const { return INVTEXT("INVALID"); }

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) { return nullptr; }
	
protected:
	FMDViewModelAssignmentReference VMAssignment;
};
