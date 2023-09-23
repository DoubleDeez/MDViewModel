#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "MDViewModelConfig.generated.h"

/**
 * Project-wide settings for MDViewModel
 */
UCLASS(DefaultConfig, Config = ViewModel)
class MDVIEWMODEL_API UMDViewModelConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMDViewModelConfig();

	virtual void PostInitProperties() override;

	// The default lifetime to set when adding a new view model assignment
	UPROPERTY(EditDefaultsOnly, Config, Category = "Providers|Cached", meta = (Categories = "MDVM.Provider.Cached.Lifetimes"))
	FGameplayTag DefaultViewModelLifetime;
};
