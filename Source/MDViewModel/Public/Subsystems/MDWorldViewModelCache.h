#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Util/MDViewModelInstanceKey.h"
#include "MDWorldViewModelCache.generated.h"

class UMDViewModelBase;

/**
 *
 */
UCLASS()
class MDVIEWMODEL_API UMDWorldViewModelCache : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UMDViewModelBase* GetOrCreateViewModel(const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass);

private:
	UPROPERTY(VisibleAnywhere, Transient, Category = "View Model")
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> CachedViewModels;
};
