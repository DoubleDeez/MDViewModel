#include "Util/MDViewModelAssignment.h"
#include "ViewModel/MDViewModelBase.h"


bool FMDViewModelAssignment::IsValid() const
{
	return ::IsValid(ViewModelClass) && ProviderTag.IsValid() && ViewModelName.IsValid();
}

bool FMDViewModelAssignment::operator==(const FMDViewModelAssignment& Other) const
{
	return ViewModelClass == Other.ViewModelClass && ProviderTag == Other.ProviderTag && ViewModelName == Other.ViewModelName;
}
