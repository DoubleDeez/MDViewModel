#include "Util/MDViewModelNativeProvider.h"

#include "MDViewModelModule.h"


void TMDViewModelNativeProvider_Private::RegisterProvider(const FGameplayTag& ProviderTag, const TSharedRef<FMDViewModelProviderBase>& Provider)
{
	FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
	ViewModelModule.RegisterViewModelProvider(ProviderTag, Provider);
}

void TMDViewModelNativeProvider_Private::UnregisterProvider(const FGameplayTag& ProviderTag)
{
	if (FMDViewModelModule* ViewModelModule = FModuleManager::GetModulePtr<FMDViewModelModule>(TEXT("MDViewModel")))
	{
		ViewModelModule->UnregisterViewModelProvider(ProviderTag);
	}
}
