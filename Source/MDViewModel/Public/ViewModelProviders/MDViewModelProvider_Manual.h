#pragma once

#include "MDViewModelProviderBase.h"
#include "NativeGameplayTags.h"
#include "MDViewModelProvider_Manual.generated.h"

MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Manual);

/**
 * Special case provider that does not actually provide view models.
 * Used to indicate that a view model assignment exists but the actual view model will
 * be set manually using something like UMDViewModelFunctionLibrary::SetViewModel in an external system.
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelProvider_Manual : public UMDViewModelProviderBase
{
	GENERATED_BODY()

public:
	virtual UMDViewModelBase* SetViewModel(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) override;

	virtual bool DoesSupportViewModelSettings() const override { return false; }
	virtual bool DoesAllowManualSetting() const override { return true; }

	virtual FGameplayTag GetProviderTag() const override { return TAG_MDVMProvider_Manual; }

#if WITH_EDITOR
	virtual bool DoesCreateViewModels() const override { return false; }
	virtual FText GetDisplayName() const override { return INVTEXT("Manual Set"); }
	virtual FText GetDescription(const FInstancedStruct& ProviderSettings) const override { return INVTEXT("The view model will have to be manually set on the object at runtime."); }
#endif
};
