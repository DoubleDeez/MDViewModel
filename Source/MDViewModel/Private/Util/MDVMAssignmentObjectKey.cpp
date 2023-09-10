#include "Util/MDVMAssignmentObjectKey.h"

#include "Serialization/CompactBinaryWriter.h"


bool FMDVMAssignmentObjectKey::operator==(const FMDVMAssignmentObjectKey& Other) const
{
	return Other.Assignment == Assignment && Other.ObjectPtr == ObjectPtr;
}

FCbWriter& operator<<(FCbWriter& Writer, const FMDVMAssignmentObjectKey& Key)
{
	Writer.BeginObject();
	Writer << "Assignment" << Key.Assignment;
	Writer << "Object" << GetNameSafe(Key.ObjectPtr.GetObject());
	Writer.EndObject();
	return Writer;
}
