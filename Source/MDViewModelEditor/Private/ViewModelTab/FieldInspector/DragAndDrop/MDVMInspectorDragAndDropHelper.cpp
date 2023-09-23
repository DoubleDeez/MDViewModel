#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropHelper.h"

#include "Nodes/MDVMNode_CallHelper.h"


TSubclassOf<UMDVMNode_CallFunctionBase> FMDVMInspectorDragAndDropHelper::GetNodeClass() const
{
	return UMDVMNode_CallHelper::StaticClass();
}
