#pragma once

#include "Interfaces/MDViewModelCacheInterface.h"
#include "Subsystems/WorldSubsystem.h"
#include "Util/MDViewModelInstanceKey.h"
#include "MDWorldViewModelCache.generated.h"

class UMDViewModelBase;

/**
 *
 */
UCLASS()
class MDVIEWMODEL_API UMDWorldViewModelCache : public UWorldSubsystem, public IMDViewModelCacheInterface
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	virtual UObject* GetViewModelOwner() const override;
	virtual TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& GetViewModelCache() override { return CachedViewModels; }

private:
	UPROPERTY(VisibleAnywhere, Transient, Category = "View Model")
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> CachedViewModels;
};
