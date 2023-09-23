#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropCommand.h"

#include "Nodes/MDVMNode_CallCommand.h"


TSubclassOf<UMDVMNode_CallFunctionBase> FMDVMInspectorDragAndDropCommand::GetNodeClass() const
{
	return UMDVMNode_CallCommand::StaticClass();
}
