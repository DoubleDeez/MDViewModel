#include "Util/MDViewModelUtils.h"

#include "UObject/Class.h"
#include "UObject/Object.h"

namespace MDViewModelUtils
{
	const FName DefaultViewModelName = TEXT("Default");

	FName ResolveViewModelName(const UClass* ViewModelClass, const FName& ViewModelName)
	{
		if (ViewModelName != DefaultViewModelName && ViewModelName != NAME_None)
		{
			return ViewModelName;
		}

		if (IsValid(ViewModelClass))
		{
			return ViewModelClass->GetFName();
		}

		return NAME_None;
	}
}
