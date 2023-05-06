#pragma once

#include "CoreMinimal.h"
#include "MDViewModelProvider_AllBase.h"
#include "NativeGameplayTags.h"

MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Manual);

/**
 * Special case provider that does not actually provide view models.
 * Used to indicate that a view model assignment exists but the actual view model will
 * be assigned manually using something like UMDViewModelFunctionLibrary::AssignViewModel by an external system.
 */
class MDVIEWMODEL_API FMDViewModelProvider_Manual : public FMDViewModelProvider_AllBase
{
public:
	virtual UMDViewModelBase* AssignViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) override;

#if WITH_EDITOR
	virtual FText GetDisplayName() const override { return INVTEXT("Manual Assignment"); }
	virtual FText GetDescription() const override { return INVTEXT("The view model will have to be manually assigned to the widget at runtime."); }
#endif
};
