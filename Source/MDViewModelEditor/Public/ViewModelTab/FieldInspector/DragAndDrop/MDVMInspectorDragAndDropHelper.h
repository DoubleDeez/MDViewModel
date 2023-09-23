#pragma once

#include "MDVMInspectorDragAndDropFunctionBase.h"


class FMDVMInspectorDragAndDropHelper : public FMDVMInspectorDragAndDropFunctionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMInspectorDragAndDropHelper, FMDVMInspectorDragAndDropFunctionBase)

protected:
	virtual TSubclassOf<UMDVMNode_CallFunctionBase> GetNodeClass() const override;
};
