#include "Interfaces/MDViewModelCacheInterface.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
#include "Logging/StructuredLog.h"
#endif
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
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

	TObjectPtr<UMDViewModelBase>& ViewModelRef = GetViewModelCache().FindOrAdd(Key);
	UMDViewModelBase* ViewModelPtr = ViewModelRef;
	if (!IsValid(ViewModelPtr))
	{
		UObject* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
		UObject* VMOuter = IsValid(GameInstance) ? GameInstance : GetTransientPackage();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		const FName NameBase = *FString::Printf(TEXT("%s_%s_%s"), *GetDebugViewModelNameBase().ToString(), *Key.ViewModelClass->GetName(), *Key.ViewModelName.ToString());
		const FName VMObjectName = MakeUniqueObjectName(VMOuter, Key.ViewModelClass, NameBase);
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
		ViewModelRef = NewObject<UMDViewModelBase>(VMOuter, Key.ViewModelClass, VMObjectName);
		ViewModelPtr = ViewModelRef;
#if DO_ENSURE
		ViewModelPtr->OnViewModelShutDown.AddWeakLambda(Cast<UObject>(this), [this, WeakViewModel = MakeWeakObjectPtr(ViewModelPtr)]()
		{
			ensureAlwaysMsgf(bIsShutdown, TEXT("Shutting down cached view model while the cache is still alive. This view model does not need ShutdownViewModelFromProvider called on it. View Model [%s] | Cache [%s]")
				, *GetNameSafe(WeakViewModel.Get())
				, *GetCacheDebugName());
		});
#endif
		ViewModelPtr->InitializeViewModelWithContext(ViewModelSettings, GetViewModelOwner(), WorldContextObject);
	}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	else if (ViewModelPtr->GetViewModelSettings() != ViewModelSettings)
	{
		// TODO - The view model editor should be able to facilitate "Make my settings the same as that other view model assignment" to make this less likely to happen
		FString CachedSettingsString;
		ViewModelPtr->GetViewModelSettings().ExportTextItem(CachedSettingsString, {}, nullptr, PPF_None, nullptr);
		FString RequestedSettingsString;
		ViewModelSettings.ExportTextItem(RequestedSettingsString, {}, nullptr, PPF_None, nullptr);
		UE_LOG(LogMDViewModel, Error, TEXT("The cached view model [%s] has different view model settings than currently being requested which can lead to unexpected bugs:\r\nCached: %s\r\nRequested: %s"), *ViewModelPtr->GetName(), *CachedSettingsString, *RequestedSettingsString);
	}
#endif
	else
	{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
		UE_LOGFMT(LogMDViewModel, Verbose, "Retrieving Cached View Model [{ViewModel}] with Key [{Key}] from Cache [{CacheName}]",
			("ViewModel", ViewModelPtr->GetName()),
			("Key", Key),
			("CacheName", GetCacheDebugName()));
#else
		UE_LOG(LogMDViewModel, Verbose, TEXT("Retrieving Cached View Model [%s] with Key [%s (%s)] from Cache [%s]"),
			*ViewModelPtr->GetName(),
			*GetNameSafe(Key.ViewModelClass),
			*Key.ViewModelName.ToString(),
			*GetCacheDebugName());
#endif
	}

	return ViewModelPtr;
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
