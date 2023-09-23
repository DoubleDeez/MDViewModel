#pragma once

#include "MDVMInspectorDragAndDropFunctionBase.h"


class FMDVMInspectorDragAndDropGetter : public FMDVMInspectorDragAndDropFunctionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropGetter, FMDVMInspectorDragAndDropFunctionBase)

protected:
	virtual TSubclassOf<UMDVMNode_CallFunctionBase> GetNodeClass() const override;
};
