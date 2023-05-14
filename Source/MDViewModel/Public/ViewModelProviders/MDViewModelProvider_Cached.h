#pragma once

#include "CoreMinimal.h"
#include "MDViewModelProvider_AllBase.h"
#include "NativeGameplayTags.h"
#include "Util/MDViewModelAssignment.h"
#include "MDViewModelProvider_Cached.generated.h"

class APlayerController;
class AGameStateBase;

MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached);

// Each assignment on a widget needs an individual binding, we track that use this struct as a key
struct FMDVMCachedProviderBindingKey
{
	FMDViewModelAssignment Assignment;

	TWeakObjectPtr<UUserWidget> WidgetPtr;

	bool operator==(const FMDVMCachedProviderBindingKey& Other) const;

	bool operator!=(const FMDVMCachedProviderBindingKey& Other) const
	{
		return !(*this == Other);
	}
};

inline uint32 GetTypeHash(const FMDVMCachedProviderBindingKey& Key)
{
	return HashCombine(GetTypeHash(Key.Assignment), GetTypeHash(Key.WidgetPtr));
}

UENUM()
enum class EMDViewModelProvider_CacheLifetime
{
	// View model lifetime will be tied to the game instance
	Global,
	// View model lifetime will be tied to the widget's owning local player
	LocalPlayer,
	// View model lifetime will be tied to the world
	World,
	// View model lifetime will be tied to the widget's owning player controller
	OwningPlayerController,
	// View model lifetime will be tied to the widget's owning player's HUD
	OwningHUD,
	// View model lifetime will be tied to the widget's owning pawn
	OwningPawn,
	// View model lifetime will be tied to the widget's owning player state
	OwningPlayerState,
	// View model lifetime will be tied to the game state
	GameState,
	// View model lifetime will be tied to the player's view target
	ViewTarget
};

USTRUCT(DisplayName = "Cached Provider Settings")
struct FMDViewModelProvider_Cached_Settings
{
	GENERATED_BODY()

public:
	// What is the desired lifetime of the cached viewmodel? This also determines the View Model's Outer object
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
	virtual ~FMDViewModelProvider_Cached() override;

	virtual UMDViewModelBase* SetViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) override;

#if WITH_EDITOR
	virtual FText GetDisplayName() const override { return INVTEXT("Cached Viewmodel"); }
	virtual FText GetDescription() const override { return INVTEXT("The view model will be grabbed from (or added to) the selected cache, keyed by the view model name and class."); }

	virtual UScriptStruct* GetProviderSettingsStruct() const override { return FMDViewModelProvider_Cached_Settings::StaticStruct(); }
#endif

protected:
	void OnPawnChanged(TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnGameStateChanged(AGameStateBase* GameState, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnViewTargetChanged(APlayerController* PC, AActor* OldViewTarget, AActor* NewViewTarget, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	// TODO - Need to clean this map up when the corresponding objects are destroyed
	TMap<FMDVMCachedProviderBindingKey, FDelegateHandle> DelegateHandles;
};
