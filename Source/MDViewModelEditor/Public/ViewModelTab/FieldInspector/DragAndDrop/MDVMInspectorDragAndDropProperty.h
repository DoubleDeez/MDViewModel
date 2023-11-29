#pragma once
#include "MDVMInspectorDragAndDropActionBase.h"
#include "UObject/WeakFieldPtr.h"

struct FMDVMCachedViewModelPropertyNodeParams
{
	FMDViewModelAssignmentReference VMAssignment;
	TWeakFieldPtr<const FProperty> PropertyPtr;
	FVector2D GraphPosition = FVector2D(0);
	TWeakObjectPtr<UEdGraph> Graph;
	TWeakObjectPtr<UEdGraphNode> Node;
	FEdGraphPinReference Pin;
};

class FMDVMInspectorDragAndDropProperty : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropProperty, FMDVMInspectorDragAndDropActionBase)

	static TSharedRef<FMDVMInspectorDragAndDropProperty> Create(TWeakFieldPtr<const FProperty> InPropertyPtr, const FMDViewModelAssignmentReference& InVMAssignment);

	static void CreateGetter(FMDVMCachedViewModelPropertyNodeParams Params);
	static void CreateSetter(FMDVMCachedViewModelPropertyNodeParams Params);
	static void FinalizeNode(UEdGraphNode* NewNode, const FMDVMCachedViewModelPropertyNodeParams& Params);

	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) override;

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;

	virtual FText GetActionTitle() const override;

	void SetAltDrag(bool InIsAltDrag) {	bAltDrag = InIsAltDrag; }
	void SetCtrlDrag(bool InIsCtrlDrag) { bControlDrag = InIsCtrlDrag; }
	void SetCanSet(bool InCanSet);

protected:
	TWeakFieldPtr<const FProperty> PropertyPtr;

private:
	TOptional<bool> bIsGetter;
	bool bCanSet = true;
	bool bControlDrag = false;
	bool bAltDrag = false;
};
