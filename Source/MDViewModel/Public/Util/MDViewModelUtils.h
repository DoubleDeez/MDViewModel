#pragma once

#include "CoreMinimal.h"

namespace MDViewModelUtils
{
	// Special-case view model name that internally maps to the viewmodel class name, useful since most cases will only
	// need a single viewmodel instance so naming won't matter
	MDVIEWMODEL_API extern const FName DefaultViewModelName;
	
	MDVIEWMODEL_API FName ResolveViewModelName(const UClass* ViewModelClass, const FName& ViewModelName = DefaultViewModelName);
}
