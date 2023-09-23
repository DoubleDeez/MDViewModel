#pragma once

#include "MDVMInspectorDragAndDropActionBase.h"


class UMDVMNode_CallFunctionBase;

class MDVIEWMODELEDITOR_API FMDVMInspectorDragAndDropFunctionBase : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropFunctionBase, FMDVMInspectorDragAndDropActionBase)

	virtual FText GetActionTitle() const override;

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;

	template<typename T>
	friend TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateMDVMDragAndDrop(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment);

protected:
	virtual TSubclassOf<UMDVMNode_CallFunctionBase> GetNodeClass() const = 0;

	TWeakObjectPtr<const UFunction> FunctionPtr;
};

template <typename T>
TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateMDVMDragAndDrop(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment)
{
	static_assert(TIsDerivedFrom<T, FMDVMInspectorDragAndDropFunctionBase>::Value, "T must derive from FMDVMInspectorDragAndDropFunctionBase");

	TSharedRef<T> Action = MakeShared<T>();
	Action->FunctionPtr = InFunctionPtr;
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}
