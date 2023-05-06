#include "Util/MDViewModelInstanceKey.h"

#include "ViewModel/MDViewModelBase.h"

bool FMDViewModelInstanceKey::IsValid() const
{
	return ViewModelName != NAME_None && ViewModelClass != nullptr;
}

bool FMDViewModelInstanceKey::operator==(const FMDViewModelInstanceKey& Other) const
{
	return Other.ViewModelName == ViewModelName && Other.ViewModelClass == ViewModelClass;
}
