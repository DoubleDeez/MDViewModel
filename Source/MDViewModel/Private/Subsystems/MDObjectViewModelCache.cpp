#include "Subsystems/MDObjectViewModelCache.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Serialization/CompactBinaryWriter.h"
#include "ViewModel/MDViewModelBase.h"

void FMDObjectViewModelCache::AddReferencedObjects(FReferenceCollector& Collector)
{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
	Collector.AddStableReferenceMap(CachedViewModels);
#else
	Collector.AddReferencedObjects(CachedViewModels);
#endif
}

void FMDObjectViewModelCache::HandleObjectDestroyed()
{
	BroadcastShutdown();
}

FString FMDObjectViewModelCache::GetCacheDebugName() const
{
	return FString::Printf(TEXT("Object Cache (%s)"), *GetPathNameSafe(Object.Get()));
}

UMDObjectViewModelCacheSystem* UMDObjectViewModelCacheSystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World))
	{
		return nullptr;
	}

	return UGameInstance::GetSubsystem<UMDObjectViewModelCacheSystem>(World->GetGameInstance());
}

bool UMDObjectViewModelCacheSystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is no override implementation defined elsewhere
	return ChildClasses.Num() == 0;
}

void UMDObjectViewModelCacheSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PostGarbageCollectConditionalBeginDestroy.AddUObject(this, &UMDObjectViewModelCacheSystem::CheckCacheForDestroyedObjects);
}

void UMDObjectViewModelCacheSystem::Deinitialize()
{
	FCoreUObjectDelegates::PostGarbageCollectConditionalBeginDestroy.RemoveAll(this);
	
	for (auto It = ObjectCacheMap.CreateIterator(); It; ++It)
	{
		if (It.Value().IsValid())
		{
			It.Value()->HandleObjectDestroyed();
		}
	}
	
	Super::Deinitialize();
}

IMDViewModelCacheInterface* UMDObjectViewModelCacheSystem::ResolveCacheForObject(UObject* Object, const UObject* WorldContextObject)
{
	UMDObjectViewModelCacheSystem* CacheSystem = UMDObjectViewModelCacheSystem::Get(WorldContextObject);
	if (!IsValid(CacheSystem))
	{
		return nullptr;
	}

	TSharedPtr<FMDObjectViewModelCache>& CachePtr = CacheSystem->ObjectCacheMap.FindOrAdd(Object);
	if (!CachePtr.IsValid())
	{
		CachePtr = MakeShared<FMDObjectViewModelCache>();
	}

	if (CachePtr->Object != Object)
	{
		CachePtr->Object = Object;
	}

	return CachePtr.Get();
}

IMDViewModelCacheInterface* UMDObjectViewModelCacheSystem::ResolveCacheForObject(const UObject* Object, const UObject* WorldContextObject)
{
	const UMDObjectViewModelCacheSystem* CacheSystem = UMDObjectViewModelCacheSystem::Get(WorldContextObject);
	if (!IsValid(CacheSystem))
	{
		return nullptr;
	}

	return CacheSystem->ObjectCacheMap.FindRef(Object).Get();
}

void UMDObjectViewModelCacheSystem::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	if (UMDObjectViewModelCacheSystem* CacheSystem = Cast<UMDObjectViewModelCacheSystem>(InThis))
	{
		for (auto It = CacheSystem->ObjectCacheMap.CreateIterator(); It; ++It)
		{
			if (It.Value().IsValid())
			{
				It.Value()->AddReferencedObjects(Collector);
			}
		}
	}
}

void UMDObjectViewModelCacheSystem::CheckCacheForDestroyedObjects()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);

	for (auto It = ObjectCacheMap.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			if (It.Value().IsValid())
			{
				It.Value()->HandleObjectDestroyed();
			}

			It.RemoveCurrent();
		}
	}
}
