#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Util/MDViewModelInstanceKey.h"
#include "MDLocalPlayerViewModelCache.generated.h"

class UMDViewModelBase;

/**
 *
 */
UCLASS()
class MDVIEWMODEL_API UMDLocalPlayerViewModelCache : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UMDViewModelBase* GetOrCreateViewModel(const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass);

private:
	UPROPERTY(VisibleAnywhere, Transient, Category = "View Model")
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> CachedViewModels;

};
