#include "Subsystems/MDObjectViewModelCache.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Launch/Resources/Version.h"
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
		It.Value().HandleObjectDestroyed();
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

	FMDObjectViewModelCache& Cache = CacheSystem->ObjectCacheMap.FindOrAdd(Object);
	if (Cache.Object != Object)
	{
		Cache.Object = Object;
	}

	return &Cache;
}

IMDViewModelCacheInterface* UMDObjectViewModelCacheSystem::ResolveCacheForObject(const UObject* Object, const UObject* WorldContextObject)
{
	UMDObjectViewModelCacheSystem* CacheSystem = UMDObjectViewModelCacheSystem::Get(WorldContextObject);
	if (!IsValid(CacheSystem))
	{
		return nullptr;
	}

	FCbWriter Writer;

	Writer << NAME_None;

	return CacheSystem->ObjectCacheMap.Find(Object);
}

void UMDObjectViewModelCacheSystem::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	if (UMDObjectViewModelCacheSystem* CacheSystem = Cast<UMDObjectViewModelCacheSystem>(InThis))
	{
		for (auto It = CacheSystem->ObjectCacheMap.CreateIterator(); It; ++It)
		{
			It.Value().AddReferencedObjects(Collector);
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
			It.Value().HandleObjectDestroyed();
			It.RemoveCurrent();
		}
	}
}
