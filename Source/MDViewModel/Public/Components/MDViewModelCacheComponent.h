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
	
	UMDViewModelBase* GetOrCreateViewModel(const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass);

private:
	UPROPERTY(VisibleAnywhere, Transient, Category = "View Model")
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> CachedViewModels;
};
