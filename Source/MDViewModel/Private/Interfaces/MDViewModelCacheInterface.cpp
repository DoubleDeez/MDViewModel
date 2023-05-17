#include "Interfaces/MDViewModelCacheInterface.h"

#include "Util/MDViewModelInstanceKey.h"
#include "ViewModel/MDViewModelBase.h"

UMDViewModelBase* IMDViewModelCacheInterface::GetOrCreateViewModel(const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
	check(Key.IsValid());

	TObjectPtr<UMDViewModelBase>& ViewModel = GetViewModelCache().FindOrAdd(Key);
	if (ViewModel == nullptr)
	{
		ViewModel = NewObject<UMDViewModelBase>(GetViewModelOwner(), Key.ViewModelClass);
		ViewModel->InitializeViewModel(ViewModelSettings);
	}

	return ViewModel;
}

void IMDViewModelCacheInterface::BroadcastShutdown()
{
	OnViewModelCacheShuttingDown.Broadcast(GetViewModelCache());
}
