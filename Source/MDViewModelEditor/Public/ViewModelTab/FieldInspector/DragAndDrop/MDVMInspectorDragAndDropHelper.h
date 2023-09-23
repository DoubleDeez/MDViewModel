#pragma once

#include "MDVMInspectorDragAndDropFunctionBase.h"


class FMDVMInspectorDragAndDropHelper : public FMDVMInspectorDragAndDropFunctionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropHelper, FMDVMInspectorDragAndDropFunctionBase)

	static TSharedRef<FMDVMInspectorDragAndDropActionBase> Create(TWeakObjectPtr<const UFunction> InFunctionPtr, const FMDViewModelAssignmentReference& InVMAssignment);

protected:
	virtual TSubclassOf<UMDVMNode_CallFunctionBase> GetNodeClass() const override;
};
