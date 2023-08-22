#include "Util/MDVMAssignmentWidgetKey.h"

#include "Blueprint/UserWidget.h"
#include "Serialization/CompactBinaryWriter.h"


bool FMDVMAssignmentWidgetKey::operator==(const FMDVMAssignmentWidgetKey& Other) const
{
	return Other.Assignment == Assignment && Other.WidgetPtr == WidgetPtr;
}

FCbWriter& operator<<(FCbWriter& Writer, const FMDVMAssignmentWidgetKey& Key)
{
	Writer.BeginObject();
	Writer << "Assignment" << Key.Assignment;
	Writer << "Widget" << GetNameSafe(Key.WidgetPtr.Get());
	Writer.EndObject();
	return Writer;
}
