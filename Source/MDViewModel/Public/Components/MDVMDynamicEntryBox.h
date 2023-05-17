#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicEntryBox.h"
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
};
