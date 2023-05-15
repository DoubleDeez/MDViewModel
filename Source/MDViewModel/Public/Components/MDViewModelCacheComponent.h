#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MDViewModelCacheComponent.generated.h"

class UMDViewModelBase;

UCLASS()
class MDVIEWMODEL_API UMDViewModelCacheComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static UMDViewModelCacheComponent* FindOrAddCache(AActor* Owner);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UMDViewModelBase* GetOrCreateViewModel(const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings);

private:
	UPROPERTY(VisibleAnywhere, Transient, Category = "View Model")
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> CachedViewModels;
};
