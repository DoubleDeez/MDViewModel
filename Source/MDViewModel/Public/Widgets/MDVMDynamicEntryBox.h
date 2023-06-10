#pragma once

#include "Components/DynamicEntryBox.h"
#include "Util/MDViewModelUtils.h"
#include "MDVMDynamicEntryBox.generated.h"

class UMDViewModelBase;

/**
 * A dynamic entry box that can be populated by a list of view models
 */
UCLASS()
class MDVIEWMODEL_API UMDVMDynamicEntryBox : public UDynamicEntryBox
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable , Category = "DynamicEntryBox")
	void PopulateItems(const TArray<UMDViewModelBase*>& ViewModels);

protected:
	UPROPERTY(EditAnywhere, Category = "View Model")
	FName ViewModelName = MDViewModelUtils::DefaultViewModelName;

	UPROPERTY(Transient)
	TArray<TSubclassOf<UMDViewModelBase>> AssignedViewModelClasses;
};
