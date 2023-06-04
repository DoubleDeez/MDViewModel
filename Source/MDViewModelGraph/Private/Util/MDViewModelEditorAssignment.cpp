#include "Util/MDViewModelEditorAssignment.h"

bool FMDViewModelEditorAssignment::operator==(const FMDViewModelEditorAssignment& Other) const
{
	return Other.Assignment == Assignment;
}
