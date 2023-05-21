#include "Util/MDViewModelUtils.h"

#include "Engine/Engine.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"

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

	UMDViewModelProviderBase* FindViewModelProvider(const FGameplayTag& ProviderTag)
	{
		if (GEngine != nullptr)
		{
			UMDViewModelProviderBase* const* ProviderPtr = GEngine->GetEngineSubsystemArray<UMDViewModelProviderBase>().FindByPredicate([&ProviderTag](const UMDViewModelProviderBase* Provider)
			{
				return IsValid(Provider) && Provider->GetProviderTag() == ProviderTag;
			});

			if (ProviderPtr != nullptr)
			{
				return *ProviderPtr;
			}
		}

		return nullptr;
	}
}
