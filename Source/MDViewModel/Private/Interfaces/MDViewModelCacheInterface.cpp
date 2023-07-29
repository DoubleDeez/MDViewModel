#include "Interfaces/MDViewModelCacheInterface.h"

#include "Util/MDViewModelInstanceKey.h"
#include "ViewModel/MDViewModelBase.h"

UMDViewModelBase* IMDViewModelCacheInterface::GetOrCreateViewModel(const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	if (bIsShutdown)
	{
		return nullptr;
	}

	const FMDViewModelInstanceKey Key = { CachedViewModelKey, ViewModelClass };
	if (!ensure(Key.IsValid()))
	{
		return nullptr;
	}

	TObjectPtr<UMDViewModelBase>& ViewModel = GetViewModelCache().FindOrAdd(Key);
	if (!IsValid(ViewModel))
	{
		ViewModel = NewObject<UMDViewModelBase>(GetTransientPackage(), Key.ViewModelClass);
		ViewModel->InitializeViewModelWithContext(ViewModelSettings, GetViewModelOwner());
	}

	return ViewModel;
}

UMDViewModelBase* IMDViewModelCacheInterface::GetViewModel(const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass) const
{
	if (bIsShutdown)
	{
		return nullptr;
	}

	const FMDViewModelInstanceKey Key = { CachedViewModelKey, ViewModelClass };
	if (!ensure(Key.IsValid()))
	{
		return nullptr;
	}

	return GetViewModelCache().FindRef(Key);
}

void IMDViewModelCacheInterface::BroadcastShutdown()
{
	bIsShutdown = true;

	// Empty out the cache before we shutdown
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& Cache = GetViewModelCache();
	const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> ShutdownViewModels = MoveTemp(Cache);
	Cache.Reset();

	OnViewModelCacheShuttingDown.Broadcast(ShutdownViewModels);
}

const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& IMDViewModelCacheInterface::GetViewModelCache() const
{
	return const_cast<IMDViewModelCacheInterface*>(this)->GetViewModelCache();
}
