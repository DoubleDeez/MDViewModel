#include "Util/MDViewModelInstanceKey.h"

#include "Serialization/CompactBinaryWriter.h"
#include "ViewModel/MDViewModelBase.h"

bool FMDViewModelInstanceKey::IsValid() const
{
	return ViewModelName != NAME_None && ViewModelClass != nullptr;
}

bool FMDViewModelInstanceKey::operator==(const FMDViewModelInstanceKey& Other) const
{
	return Other.ViewModelName == ViewModelName && Other.ViewModelClass == ViewModelClass;
}

FCbWriter& operator<<(FCbWriter& Writer, const FMDViewModelInstanceKey& Key)
{
	Writer.BeginObject();
	Writer << "Name" << Key.ViewModelName;
	Writer << "Class" << GetNameSafe(Key.ViewModelClass.Get());
	Writer.EndObject();
	return Writer;
}
