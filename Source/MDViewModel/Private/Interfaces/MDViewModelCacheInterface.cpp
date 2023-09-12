#include "Interfaces/MDViewModelCacheInterface.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
#include "Logging/StructuredLog.h"
#endif
#include "Util/MDViewModelInstanceKey.h"
#include "Util/MDViewModelLog.h"
#include "ViewModel/MDViewModelBase.h"

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
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		const FName NameBase = *FString::Printf(TEXT("%s_%s_%s"), *GetDebugViewModelNameBase().ToString(), *Key.ViewModelClass->GetName(), *Key.ViewModelName.ToString());
		const FName VMObjectName = MakeUniqueObjectName(GetTransientPackage(), Key.ViewModelClass, NameBase);
#else
		const FName VMObjectName = NAME_None;
#endif
		
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
		UE_LOGFMT(LogMDViewModel, Verbose, "Creating Cached View Model with Key [{Key}] on Cache [{CacheName}]",
			("Key", Key),
			("CacheName", GetCacheDebugName()));
#else
		UE_LOG(LogMDViewModel, Verbose, TEXT("Creating Cached View Model with Key [%s (%s)] on Cache [%s]"),
			*GetNameSafe(Key.ViewModelClass),
			*Key.ViewModelName.ToString(),
			*GetCacheDebugName());
#endif
		ViewModel = NewObject<UMDViewModelBase>(GetTransientPackage(), Key.ViewModelClass, VMObjectName);
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
	else
	{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
		UE_LOGFMT(LogMDViewModel, Verbose, "Retrieving Cached View Model [{ViewModel}] with Key [{Key}] from Cache [{CacheName}]",
			("ViewModel", ViewModel->GetName()),
			("Key", Key),
			("CacheName", GetCacheDebugName()));
#else
		UE_LOG(LogMDViewModel, Verbose, TEXT("Retrieving Cached View Model [%s] with Key [%s (%s)] from Cache [%s]"),
			*ViewModel->GetName(),
			*GetNameSafe(Key.ViewModelClass),
			*Key.ViewModelName.ToString(),
			*GetCacheDebugName());
#endif
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
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
	UE_LOGFMT(LogMDViewModel, Verbose, "Shutting down View Model Cache [{CacheName}]", ("CacheName", GetCacheDebugName()));
#else
	UE_LOG(LogMDViewModel, Verbose, TEXT("Shutting down View Model Cache [%s]"), *GetCacheDebugName());
#endif
	
	bIsShutdown = true;

	// Empty out the cache before we shutdown
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& Cache = GetViewModelCache();
	const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> ShutdownViewModels = MoveTemp(Cache);
	Cache.Reset();

	OnShuttingDown.Broadcast();

	for (auto It = ShutdownViewModels.CreateConstIterator(); It; ++It)
	{
		UMDViewModelBase* ViewModel = It.Value();
		if (IsValid(ViewModel))
		{
			ViewModel->ShutdownViewModelFromProvider();
		}
	}
}

const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& IMDViewModelCacheInterface::GetViewModelCache() const
{
	return const_cast<IMDViewModelCacheInterface*>(this)->GetViewModelCache();
}

FName IMDViewModelCacheInterface::CreateDebugViewModelNameBase() const
{
	return *FString::Printf(TEXT("CachedVM_%s"), *GetCacheDebugName());
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
const FName& IMDViewModelCacheInterface::GetDebugViewModelNameBase()
{
	if (DebugViewModelNameBase == NAME_None)
	{
		DebugViewModelNameBase = CreateDebugViewModelNameBase();
	}

	return DebugViewModelNameBase;
}
#endif
