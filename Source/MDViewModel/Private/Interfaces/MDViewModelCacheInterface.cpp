﻿#include "Interfaces/MDViewModelCacheInterface.h"

#include "Util/MDViewModelInstanceKey.h"
#include "Util/MDViewModelLog.h"
#include "ViewModel/MDViewModelBase.h"

uint64 IMDViewModelCacheInterface::LastHandle = 0;

UMDViewModelBase* IMDViewModelCacheInterface::GetOrCreateViewModel(const UObject* WorldContextObject, const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	if (bIsShutdown)
	{
		return nullptr;
	}

	const FMDViewModelInstanceKey Key = { CachedViewModelKey, ViewModelClass };
	if (!Key.IsValid())
	{
		return nullptr;
	}

	TObjectPtr<UMDViewModelBase>& ViewModel = GetViewModelCache().FindOrAdd(Key);
	if (!IsValid(ViewModel))
	{
		ViewModel = NewObject<UMDViewModelBase>(GetTransientPackage(), Key.ViewModelClass);
		ViewModel->InitializeViewModelWithContext(ViewModelSettings, GetViewModelOwner(), WorldContextObject);
	}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	else if (ViewModel->GetViewModelSettings() != ViewModelSettings)
	{
		// TODO - The view model editor should be able to facilitate "Make my settings the same as that other view model assignment" to make this less likely to happen
		FString CachedSettingsString;
		ViewModel->GetViewModelSettings().ExportTextItem(CachedSettingsString, {}, nullptr, PPF_None, nullptr);
		FString RequestedSettingsString;
		ViewModelSettings.ExportTextItem(RequestedSettingsString, {}, nullptr, PPF_None, nullptr);
		UE_LOG(LogMDViewModel, Warning, TEXT("The cached view model [%s] has different view model settings than currently being requested:\r\nCached: %s\r\nRequested: %s"), *ViewModel->GetName(), *CachedSettingsString, *RequestedSettingsString);
	}
#endif

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
