#pragma once

#include "Interfaces/MDViewModelCacheInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Util/MDViewModelInstanceKey.h"
#include "MDGlobalViewModelCache.generated.h"

class UMDViewModelBase;

/**
 * Caches view models relative to the game instance
 */
UCLASS()
class MDVIEWMODEL_API UMDGlobalViewModelCache : public UGameInstanceSubsystem, public IMDViewModelCacheInterface
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Deinitialize() override;

protected:
	virtual UObject* GetViewModelOwner() const override;
	virtual TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& GetViewModelCache() override { return CachedViewModels; }
	virtual FString GetCacheDebugName() const override { return TEXT("Global Cache"); }

private:
	UPROPERTY(VisibleAnywhere, Transient, Category = "View Model")
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> CachedViewModels;
};
