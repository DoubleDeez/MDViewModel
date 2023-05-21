#pragma once

#include "CoreMinimal.h"

struct FGameplayTag;
class UMDViewModelProviderBase;

namespace MDViewModelUtils
{
	// Special-case view model name that internally maps to the viewmodel class name, useful since most cases will only
	// need a single viewmodel instance so naming won't matter
	MDVIEWMODEL_API extern const FName DefaultViewModelName;
	
	MDVIEWMODEL_API FName ResolveViewModelName(const UClass* ViewModelClass, const FName& ViewModelName = DefaultViewModelName);

	MDVIEWMODEL_API UMDViewModelProviderBase* FindViewModelProvider(const FGameplayTag& ProviderTag);
}
