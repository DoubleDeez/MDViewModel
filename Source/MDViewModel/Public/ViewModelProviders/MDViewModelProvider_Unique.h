#pragma once

#include "MDViewModelProviderBase.h"
#include "NativeGameplayTags.h"
#include "MDViewModelProvider_Unique.generated.h"

MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Unique);

/**
 * This provider will create a unique instance of the selected viewmodel for each assigned widget.
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelProvider_Unique : public UMDViewModelProviderBase
{
	GENERATED_BODY()

public:
	virtual UMDViewModelBase* SetViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) override;

	virtual FGameplayTag GetProviderTag() const override { return TAG_MDVMProvider_Unique; }

#if WITH_EDITOR
	virtual FText GetDisplayName() const override { return INVTEXT("Unique Instance"); }
	virtual FText GetDescription() const override { return INVTEXT("A unique view model instance will be created for the assigned widget."); }
#endif
};
