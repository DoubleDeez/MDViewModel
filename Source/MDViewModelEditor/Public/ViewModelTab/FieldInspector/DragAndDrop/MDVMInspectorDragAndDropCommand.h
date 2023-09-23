#pragma once

#include "MDVMInspectorDragAndDropFunctionBase.h"


class FMDVMInspectorDragAndDropCommand : public FMDVMInspectorDragAndDropFunctionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropCommand, FMDVMInspectorDragAndDropFunctionBase)

protected:
	virtual TSubclassOf<UMDVMNode_CallFunctionBase> GetNodeClass() const override;
};
