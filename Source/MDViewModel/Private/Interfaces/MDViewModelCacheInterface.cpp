#include "Interfaces/MDViewModelCacheInterface.h"

#include "Util/MDViewModelInstanceKey.h"
#include "ViewModel/MDViewModelBase.h"

UMDViewModelBase* IMDViewModelCacheInterface::GetOrCreateViewModel(const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	if (bIsShutdown)
	{
		return nullptr;
	}
	
	const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
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

void IMDViewModelCacheInterface::BroadcastShutdown()
{
	bIsShutdown = true;
	
	// Empty out the cache before we shutdown
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& Cache = GetViewModelCache();
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> ShutdownViewModels = MoveTemp(Cache);
	Cache.Reset();
	
	OnViewModelCacheShuttingDown.Broadcast(Cache);
}
