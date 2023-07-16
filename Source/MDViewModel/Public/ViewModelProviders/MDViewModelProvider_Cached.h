#pragma once

#include "Engine/MemberReference.h"
#include "FieldNotification/FieldId.h"
#include "InstancedStruct.h"
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
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_Global);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_LocalPlayer);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_World);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerController);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_OwningHUD);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_OwningPawn);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerState);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_GameState);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_ViewTarget);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_ViewTargetPlayerState);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_Relative);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty);

// Wrapped on DelegateHandle to track which object the handle was for
struct MDVIEWMODEL_API FMDWrappedDelegateHandle
{
	FDelegateHandle Handle;
	TWeakObjectPtr<UObject> DelegateOwner;
};

// Each assignment on a widget needs an individual binding, we track that use this struct as a key
struct MDVIEWMODEL_API FMDVMCachedProviderBindingKey
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
	// View model lifetime will be tied to a FieldNotify property or function on the widget, using the property as its context object (must be an Actor or other supported type)
	RelativeProperty,
	// To support custom lifetimes, ViewModelLifetimeTag was added in place of this enum
	Custom UMETA(Hidden),
};

USTRUCT(DisplayName = "Cached Provider Settings")
struct MDVIEWMODEL_API FMDViewModelProvider_Cached_Settings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Provider", meta = (InlineEditConditionToggle))
	bool bOverrideCachedViewModelKey = false;

	// By default, the cached viewmodel is keyed by the view model assignment name.
	// It can be desirable to use a different view model assignment name than the cached name so this property allows that.
	UPROPERTY(EditAnywhere, Category = "Provider", meta = (EditCondition = "bOverrideCachedViewModelKey"))
	FName CachedViewModelKeyOverride = MDViewModelUtils::DefaultViewModelName;

	// What is the desired lifetime of the cached view model? This also determines the View Model's Outer object
	UPROPERTY(EditAnywhere, Category = "Provider", meta = (Categories = "MDVM.Provider.Cached.Lifetimes"))
	FGameplayTag ViewModelLifetimeTag;

	UE_DEPRECATED(All, "The lifetime enum is deprecated, use ViewModelLifetimeTag instead.")
	UPROPERTY()
	EMDViewModelProvider_CacheLifetime ViewModelLifetime = EMDViewModelProvider_CacheLifetime::Global;

	// For Relative lifetime (MDVM.Provider.Cached.Lifetimes.Relative), this view model's lifetime and context object will be tied to the view model assignment selected here
	UPROPERTY(EditAnywhere, Category = "Provider|Relative", meta = (EditCondition = "bIsRelative", EditConditionHides))
	FMDViewModelAssignmentReference RelativeViewModel;

#if WITH_EDITORONLY_DATA
	// For Relative Property lifetime (MDVM.Provider.Cached.Lifetimes.RelativeProperty), this is the FieldNotify property or function on the widget that will be used at the context for the view model (must be an Actor or other supported type)
	UPROPERTY(EditAnywhere, Category = "Provider|Relative Property", meta = (GetOptions = "GetRelativePropertyNames", EditCondition = "bIsRelativeProperty", EditConditionHides))
	FName RelativePropertyName;

	UPROPERTY(Transient)
	bool bIsRelative = false;
	UPROPERTY(Transient)
	bool bIsRelativeProperty = false;
#endif

	UPROPERTY()
	FMemberReference RelativePropertyReference;

	const FGameplayTag& GetLifetimeTag() const;
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
	static T* FindOrCreateCachedViewModel(UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings = {});
	static UMDViewModelBase* FindOrCreateCachedViewModel(UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings = {});

	// Try to find an existing view model for the given context object
	template<typename T>
	static T* FindCachedViewModel(const UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings = {});
	static UMDViewModelBase* FindCachedViewModel(const UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings = {});

	virtual void Deinitialize() override;

	virtual FGameplayTag GetProviderTag() const override { return TAG_MDVMProvider_Cached; }

	virtual UMDViewModelBase* SetViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) override;

#if WITH_EDITOR
	virtual FText GetDisplayName() const override { return INVTEXT("Cached Viewmodel"); }
	virtual FText GetDescription() const override { return INVTEXT("The view model will be grabbed from (or added to) the selected cache, keyed by the view model name and class."); }

	virtual UScriptStruct* GetProviderSettingsStruct() const override { return FMDViewModelProvider_Cached_Settings::StaticStruct(); }
	virtual bool ValidateProviderSettings(const FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint, TArray<FText>& OutIssues) const override;
	virtual void OnProviderSettingsInitializedInEditor(FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint) const override;
	virtual void OnProviderSettingsPropertyChanged(FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint) const override;
#endif

protected:
	virtual IMDViewModelCacheInterface* ResolveAndBindViewModelCache(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data, const FMDViewModelProvider_Cached_Settings& Settings);
	
	UMDViewModelBase* FindOrCreateCachedViewModel_Internal(UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings);
	UMDViewModelBase* FindCachedViewModel_Internal(const UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings) const;

	void BindOnWidgetDestroy(UUserWidget& Widget);
	void OnWidgetDestroy(TWeakObjectPtr<UUserWidget> WidgetPtr);

	void RefreshViewModel(TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnGameStateChanged(AGameStateBase* GameState, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);
	void OnViewTargetChanged(APlayerController* PC, AActor* OldViewTarget, AActor* NewViewTarget, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);
	void OnRelativeViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnFieldValueChanged(UObject* Widget, UE::FieldNotification::FFieldId FieldId, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnViewModelCacheShutdown(const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& ViewModelCache, TWeakInterfacePtr<IMDViewModelCacheInterface> BoundCache);

	IMDViewModelCacheInterface* ResolveGlobalCache(const UGameInstance* GameInstance) const;
	IMDViewModelCacheInterface* ResolveLocalPlayerCache(const ULocalPlayer* LocalPlayer) const;
	IMDViewModelCacheInterface* ResolveWorldCache(const UWorld* World) const;
	IMDViewModelCacheInterface* ResolveActorCache(AActor* Actor) const;
	const IMDViewModelCacheInterface* ResolveActorCache(const AActor* Actor) const;
	IMDViewModelCacheInterface* ResolveObjectCache(UObject* Object) const;
	const IMDViewModelCacheInterface* ResolveObjectCache(const UObject* Object) const;

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

	IMDViewModelCacheInterface* ResolveRelativePropertyCacheAndBindDelegates(const FMemberReference& Reference, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	void BindViewTargetDelegates(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	TMap<FMDVMCachedProviderBindingKey, FMDWrappedDelegateHandle> WidgetDelegateHandles;
	TMap<FMDVMCachedProviderBindingKey, FDelegateHandle> ViewTargetDelegateHandles;
	TMap<FMDViewModelAssignment, TMap<TWeakObjectPtr<UUserWidget>, TWeakInterfacePtr<IMDViewModelCacheInterface>>> BoundAssignments;
};

template <typename T>
T* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "T must derive from UMDViewModelBase");
	return Cast<T>(FindOrCreateCachedViewModel(CacheContextObject, ViewModelName, ViewModelClass, ViewModelSettings));
}

template <typename T>
T* UMDViewModelProvider_Cached::FindCachedViewModel(const UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "T must derive from UMDViewModelBase");
	return Cast<T>(FindCachedViewModel(CacheContextObject, ViewModelName, ViewModelClass, ViewModelSettings));
}
