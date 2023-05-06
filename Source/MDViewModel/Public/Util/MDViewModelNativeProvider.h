#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class FMDViewModelProviderBase;

// Register a viewmodel provider with the specified tag. Variadic arguments are passed to the provider constructor
#define MDVM_REGISTER_PROVIDER(PROVIDER_TYPE, PROVIDER_TAG, ...) TMDViewModelNativeProvider<PROVIDER_TYPE> MDVMNativeProvider_##PROVIDER_TAG(PROVIDER_TAG, __VA_ARGS__);


/**
 * Helper class to statically register viewmodel providers instead of doing it in StartupModule
 * with the ability to pass arguments to the provider's constructor
 */
template<typename T, typename... InArgTypes>
class TMDViewModelNativeProvider final : public FNoncopyable
{
	static_assert(TIsDerivedFrom<T, FMDViewModelProviderBase>::IsDerived, "TMDViewModelNativeProvider may only be used with FMDViewModelProviderBase types");

public:
	TMDViewModelNativeProvider(const FGameplayTag& InProviderTag, InArgTypes&& ...);
	~TMDViewModelNativeProvider();

private:
	const FGameplayTag ProviderTag;

};

namespace TMDViewModelNativeProvider_Private
{
	MDVIEWMODEL_API void RegisterProvider(const FGameplayTag& ProviderTag, const TSharedRef<FMDViewModelProviderBase>& Provider);
	MDVIEWMODEL_API void UnregisterProvider(const FGameplayTag& ProviderTag);
}

template <typename T, typename ... InArgTypes>
TMDViewModelNativeProvider<T, InArgTypes...>::TMDViewModelNativeProvider(const FGameplayTag& InProviderTag, InArgTypes&&... Args)
	: ProviderTag(InProviderTag)
{
	const TSharedRef<FMDViewModelProviderBase> Provider = MakeShared<T>(Forward<InArgTypes>(Args)...);
	TMDViewModelNativeProvider_Private::RegisterProvider(ProviderTag, Provider);
}

template <typename T, typename ... InArgTypes>
TMDViewModelNativeProvider<T, InArgTypes...>::~TMDViewModelNativeProvider()
{
	TMDViewModelNativeProvider_Private::UnregisterProvider(ProviderTag);
}
