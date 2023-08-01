#pragma once

#include "Interfaces/MDViewModelCacheInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MDObjectViewModelCache.generated.h"

class FMDObjectViewModelCache : public IMDViewModelCacheInterface
{
public:
	void AddReferencedObjects(FReferenceCollector& Collector);
	
	void HandleObjectDestroyed();
	
	TWeakObjectPtr<UObject> Object;
	
protected:
	virtual UObject* GetViewModelOwner() const override { return Object.Get(); }

	virtual TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& GetViewModelCache() override { return CachedViewModels; }

private:
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> CachedViewModels;
};

/**
 * Caches view models relative to any UObject
 */
UCLASS()
class MDVIEWMODEL_API UMDObjectViewModelCacheSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UMDObjectViewModelCacheSystem* Get(const UObject* WorldContextObject);
	
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static IMDViewModelCacheInterface* ResolveCacheForObject(UObject* Object, const UObject* WorldContextObject);
	static IMDViewModelCacheInterface* ResolveCacheForObject(const UObject* Object, const UObject* WorldContextObject);
	
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

protected:
	virtual void CheckCacheForDestroyedObjects();

private:
	TMap<TWeakObjectPtr<const UObject>, FMDObjectViewModelCache> ObjectCacheMap;
};
