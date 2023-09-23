#include "Util/MDViewModelConfig.h"

#include "GameplayTagsManager.h"
#include "ViewModelProviders/MDViewModelProvider_Cached.h"

UMDViewModelConfig::UMDViewModelConfig()
	: DefaultViewModelLifetime(TAG_MDVMProvider_Cached_Lifetimes_Global)
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("MD View Model");
}

void UMDViewModelConfig::PostInitProperties()
{
	Super::PostInitProperties();

	UGameplayTagsManager::Get().CallOrRegister_OnDoneAddingNativeTagsDelegate(
		FSimpleMulticastDelegate::FDelegate::CreateWeakLambda(this, [this]() { ReloadConfig(); }));
}
