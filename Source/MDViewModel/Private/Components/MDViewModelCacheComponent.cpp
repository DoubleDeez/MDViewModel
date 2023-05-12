#include "Components/MDViewModelCacheComponent.h"

#include "Util/MDViewModelInstanceKey.h"
#include "Util/MDViewModelUtils.h"
#include "ViewModel/MDViewModelBase.h"


UMDViewModelCacheComponent* UMDViewModelCacheComponent::FindOrAddCache(AActor* Owner)
{
	if (IsValid(Owner))
	{
		UMDViewModelCacheComponent* Result = Owner->FindComponentByClass<UMDViewModelCacheComponent>();
		if (IsValid(Result))
		{
			return Result;
		}

		constexpr bool bManualAttachment = false;
		constexpr bool bDeferredFinish = false;
		return Cast<UMDViewModelCacheComponent>(Owner->AddComponentByClass(UMDViewModelCacheComponent::StaticClass(), bManualAttachment, FTransform::Identity, bDeferredFinish));
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelCacheComponent::GetOrCreateViewModel(const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass)
{
	const FMDViewModelInstanceKey Key = { MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName), ViewModelClass };
	check(Key.IsValid());

	TObjectPtr<UMDViewModelBase>& ViewModel = CachedViewModels.FindOrAdd(Key);
	if (ViewModel == nullptr)
	{
		ViewModel = NewObject<UMDViewModelBase>(GetOwner(), Key.ViewModelClass);
		ViewModel->InitializeViewModel();
	}

	return ViewModel;
}
