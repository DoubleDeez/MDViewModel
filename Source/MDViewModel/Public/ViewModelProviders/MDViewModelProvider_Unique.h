#pragma once

#include "CoreMinimal.h"
#include "MDViewModelProvider_AllBase.h"
#include "NativeGameplayTags.h"

MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Unique);

/**
 * This provider will create a unique instance of the selected viewmodel for each assigned widget.
 */
class MDVIEWMODEL_API FMDViewModelProvider_Unique : public FMDViewModelProvider_AllBase
{
public:
	virtual UMDViewModelBase* SetViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) override;

#if WITH_EDITOR
	virtual FText GetDisplayName() const override { return INVTEXT("Unique Instance"); }
	virtual FText GetDescription() const override { return INVTEXT("A unique view model instance will be created for the assigned widget."); }
#endif
};
