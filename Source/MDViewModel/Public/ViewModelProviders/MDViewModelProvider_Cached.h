#pragma once

#include "CoreMinimal.h"
#include "MDViewModelProvider_AllBase.h"
#include "NativeGameplayTags.h"
#include "MDViewModelProvider_Cached.generated.h"

MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached);

UENUM()
enum class EMDViewModelProvider_CacheLifetime
{
	// Viewmodel lifetime will be tied to the game instance
	Global,
	// Viewmodel lifetime will be tied to the widget's owning local player
	LocalPlayer,
	// Viewmodel lifetime will be tied to the world
	World
};

USTRUCT(DisplayName = "Cached Provider Settings")
struct FMDViewModelProvider_Cached_Settings
{
	GENERATED_BODY()

public:
	// What is the desired lifetime of the cached viewmodel?
	UPROPERTY(EditAnywhere, Category = "Provider")
	EMDViewModelProvider_CacheLifetime ViewModelLifetime = EMDViewModelProvider_CacheLifetime::Global;
};

/**
 * This provider will create an instance of the selected viewmodel and cache it for the selected lifetime.
 * When setting a viewmodel on a widget, if a cached instance exists with the specified Viewmodel Name,
 * it will be used instead of creating a new instance. Useful for reusing viewmodels in multiple widgets.
 */
class MDVIEWMODEL_API FMDViewModelProvider_Cached : public FMDViewModelProvider_AllBase
{
public:
	virtual UMDViewModelBase* AssignViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) override;

#if WITH_EDITOR
	virtual FText GetDisplayName() const override { return INVTEXT("Cached Viewmodel"); }
	virtual FText GetDescription() const override { return INVTEXT("The view model will be grabbed from (or added to) the selected cache, keyed by the view model name and class."); }

	virtual UScriptStruct* GetProviderSettingsStruct() const override { return FMDViewModelProvider_Cached_Settings::StaticStruct(); }
#endif
};
