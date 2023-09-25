#pragma once

#include "Interfaces/MDViewModelCacheInterface.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Util/MDViewModelInstanceKey.h"
#include "MDLocalPlayerViewModelCache.generated.h"

class UMDViewModelBase;

/**
 * Caches view models relative to a local player
 */
UCLASS()
class MDVIEWMODEL_API UMDLocalPlayerViewModelCache : public ULocalPlayerSubsystem, public IMDViewModelCacheInterface
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Deinitialize() override;

protected:
	virtual UObject* GetViewModelOwner() const override;
	virtual TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& GetViewModelCache() override { return CachedViewModels; }
	virtual FString GetCacheDebugName() const override;

private:
	UPROPERTY(VisibleAnywhere, Transient, Category = "View Model")
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> CachedViewModels;

};
