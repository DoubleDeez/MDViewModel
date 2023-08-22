#include "Util/MDViewModelAssignment.h"

#include "Serialization/CompactBinaryWriter.h"
#include "ViewModel/MDViewModelBase.h"


bool FMDViewModelAssignment::IsValid() const
{
	return ::IsValid(ViewModelClass) && ProviderTag.IsValid() && ViewModelName.IsValid();
}

bool FMDViewModelAssignment::operator==(const FMDViewModelAssignment& Other) const
{
	return ViewModelClass == Other.ViewModelClass && ProviderTag == Other.ProviderTag && ViewModelName == Other.ViewModelName;
}

FCbWriter& operator<<(FCbWriter& Writer, const FMDViewModelAssignment& Assignment)
{
	Writer.BeginObject();
	Writer << "Class" << GetNameSafe(Assignment.ViewModelClass);
	Writer << "Provider" << Assignment.ProviderTag.GetTagName();
	Writer << "Name" << Assignment.ViewModelName;
	Writer.EndObject();
	return Writer;
}
