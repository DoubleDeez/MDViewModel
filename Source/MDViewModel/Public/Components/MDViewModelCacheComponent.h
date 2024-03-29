﻿#pragma once

#include "Components/ActorComponent.h"
#include "Interfaces/MDViewModelCacheInterface.h"
#include "MDViewModelCacheComponent.generated.h"

class UMDViewModelBase;

UCLASS()
class MDVIEWMODEL_API UMDViewModelCacheComponent : public UActorComponent, public IMDViewModelCacheInterface
{
	GENERATED_BODY()

public:
	static UMDViewModelCacheComponent* FindOrAddCache(AActor* Owner);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual UObject* GetViewModelOwner() const override;
	virtual TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& GetViewModelCache() override { return CachedViewModels; }
	virtual FString GetCacheDebugName() const override;

private:
	UPROPERTY(VisibleAnywhere, Transient, Category = "View Model")
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> CachedViewModels;
};
