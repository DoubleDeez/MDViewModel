#include "Util/MDVMAssignmentWidgetKey.h"


bool FMDVMAssignmentWidgetKey::operator==(const FMDVMAssignmentWidgetKey& Other) const
{
	return Other.Assignment == Assignment && Other.WidgetPtr == WidgetPtr;
}
