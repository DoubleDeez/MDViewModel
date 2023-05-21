#pragma once

#include "CoreMinimal.h"
#include "MDViewModelProviderBase.h"
#include "NativeGameplayTags.h"
#include "UObject/WeakInterfacePtr.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDViewModelProvider_Cached.generated.h"

struct FMDViewModelInstanceKey;
struct MDViewModelAssignmentData;
class IMDViewModelCacheInterface;
class AActor;
class APlayerController;
class APlayerState;
class APawn;
class AGameStateBase;
class UGameInstance;
class ULocalPlayer;
class UWorld;

MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached);

// Wrapped on DelegateHandle to track which object the handle was for
struct FMDWrappedDelegateHandle
{
	FDelegateHandle Handle;
	TWeakObjectPtr<UObject> DelegateOwner;
};

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
	ViewTarget,
	// View model lifetime will be tied to the player's view target's player state
	ViewTargetPlayerState,
	// View model lifetime will be tied to another view model on the widget and share its context object
	Relative,
	// TODO - Allow external systems to bind to the cached provider to allow custom lifetimes,
	// will need to expose an FName or Tag to indicate which custom lifetime should be used
	Custom UMETA(Hidden),
};

USTRUCT(DisplayName = "Cached Provider Settings")
struct FMDViewModelProvider_Cached_Settings
{
	GENERATED_BODY()

public:
	// What is the desired lifetime of the cached view model? This also determines the View Model's Outer object
	UPROPERTY(EditAnywhere, Category = "Provider")
	EMDViewModelProvider_CacheLifetime ViewModelLifetime = EMDViewModelProvider_CacheLifetime::Global;

	// For Relative lifetime, this view model's lifetime and context object will be tied to the view model assignment selected here 
	UPROPERTY(EditAnywhere, Category = "Provider", meta = (EditCondition = "ViewModelLifetime == EMDViewModelProvider_CacheLifetime::Relative", EditConditionHides))
	FMDViewModelAssignmentReference RelativeViewModel;
};

/**
 * This provider will create an instance of the selected view model and cache it for the selected lifetime.
 * When setting a view model on a widget, if a cached instance exists with the specified View Model Name,
 * it will be used instead of creating a new instance. Useful for reusing view models in multiple widgets.
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelProvider_Cached : public UMDViewModelProviderBase
{
	GENERATED_BODY()
	
public:
	// Use this to make use of a view model cache for manually assigned view models
	template<typename T>
	static T* FindOrCreateCachedViewModel(UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings);
	static UMDViewModelBase* FindOrCreateCachedViewModel(UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings);
		
	virtual void Deinitialize() override;

	virtual FGameplayTag GetProviderTag() const override { return TAG_MDVMProvider_Cached; }

	virtual UMDViewModelBase* SetViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) override;

#if WITH_EDITOR
	virtual FText GetDisplayName() const override { return INVTEXT("Cached Viewmodel"); }
	virtual FText GetDescription() const override { return INVTEXT("The view model will be grabbed from (or added to) the selected cache, keyed by the view model name and class."); }

	virtual UScriptStruct* GetProviderSettingsStruct() const override { return FMDViewModelProvider_Cached_Settings::StaticStruct(); }
	virtual bool ValidateProviderSettings(const FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint, TArray<FText>& OutIssues) const override;
	virtual void OnProviderSettingsInitializedInEditor(FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint) const override;
#endif

protected:
	UMDViewModelBase* FindOrCreateCachedViewModel_Internal(UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings);

	void BindOnWidgetDestroy(UUserWidget& Widget);
	void OnWidgetDestroy(TWeakObjectPtr<UUserWidget> WidgetPtr);
	
	void RefreshViewModel(TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnGameStateChanged(AGameStateBase* GameState, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);
	void OnViewTargetChanged(APlayerController* PC, AActor* OldViewTarget, AActor* NewViewTarget, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);
	void OnRelativeViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnViewModelCacheShutdown(const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& ViewModelCache, TWeakInterfacePtr<IMDViewModelCacheInterface> BoundCache);

	IMDViewModelCacheInterface* ResolveGlobalCache(const UGameInstance* GameInstance) const;
	IMDViewModelCacheInterface* ResolveLocalPlayerCache(const ULocalPlayer* LocalPlayer) const;
	IMDViewModelCacheInterface* ResolveWorldCache(const UWorld* World) const;
	IMDViewModelCacheInterface* ResolveActorCache(AActor* Actor) const;
	IMDViewModelCacheInterface* ResolveObjectCache(UObject* Object) const;

	IMDViewModelCacheInterface* ResolveHUDCacheAndBindDelegates(APlayerController* PlayerController, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	// Only local pawns can be bound to via the player controller, remote pawns must be bound through the player state
	IMDViewModelCacheInterface* ResolvePawnCacheAndBindDelegates(APlayerController* PlayerController, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolvePawnCacheAndBindDelegates(APlayerState* PlayerState, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	
	IMDViewModelCacheInterface* ResolvePlayerStateCacheAndBindDelegates(APlayerController* PlayerController, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolvePlayerStateCacheAndBindDelegates(APawn* Pawn, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	IMDViewModelCacheInterface* ResolveGameStateCacheAndBindDelegates(UWorld* World, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	
	IMDViewModelCacheInterface* ResolveViewTargetCacheAndBindDelegates(const APlayerController* PlayerController, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolveViewTargetPlayerStateCacheAndBindDelegates(const APlayerController* PlayerController, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	
	IMDViewModelCacheInterface* ResolveRelativeViewModelCacheAndBindDelegates(const FMDViewModelAssignmentReference& Reference, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	
	IMDViewModelCacheInterface* ResolveObjectCacheAndBindDelegates(UObject* Object, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	void BindViewTargetDelegates(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	
	TMap<FMDVMCachedProviderBindingKey, FMDWrappedDelegateHandle> WidgetDelegateHandles;
	TMap<FMDVMCachedProviderBindingKey, FDelegateHandle> ViewTargetDelegateHandles;
	TMap<FMDVMCachedProviderBindingKey, FDelegateHandle> RelativeViewModelDelegateHandles;
	TMap<FMDViewModelAssignment, TMap<TWeakObjectPtr<UUserWidget>, TWeakInterfacePtr<IMDViewModelCacheInterface>>> BoundAssignments;
};

template <typename T>
T* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "T must derive from UMDViewModelBase");
	return Cast<T>(FindOrCreateCachedViewModel(CacheContextObject, ViewModelName, ViewModelClass, ViewModelSettings));
}
