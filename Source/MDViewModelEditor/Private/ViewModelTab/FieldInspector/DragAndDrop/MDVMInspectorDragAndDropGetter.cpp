#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropGetter.h"

#include "Nodes/MDVMNode_CallGetter.h"


TSubclassOf<UMDVMNode_CallFunctionBase> FMDVMInspectorDragAndDropGetter::GetNodeClass() const
{
	return UMDVMNode_CallGetter::StaticClass();
}
