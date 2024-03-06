#pragma once

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
#include "FieldNotificationId.h"
#else
#include "FieldNotification/FieldId.h"
#endif

#include "Engine/MemberReference.h"
#include "InstancedStruct.h"
#include "Interfaces/MDViewModelCacheInterface.h"
#include "Interfaces/MDViewModelRuntimeInterface.h"
#include "MDViewModelProviderBase.h"
#include "NativeGameplayTags.h"
#include "UObject/WeakInterfacePtr.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDVMAssignmentObjectKey.h"
#include "MDViewModelProvider_Cached.generated.h"

struct MDViewModelAssignment;
struct MDViewModelAssignmentData;
class AActor;
class APlayerController;
class APlayerState;
class APawn;
class AGameStateBase;
class UActorComponent;
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
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_WorldActor);
MDVIEWMODEL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MDVMProvider_Cached_Lifetimes_Self);

USTRUCT()
struct MDVIEWMODEL_API FMDVMWorldActorFilter
{
	GENERATED_BODY()

public:
	// Only actors of this class will be considered
	UPROPERTY(EditAnywhere, Category = "World Actor", meta = (AllowAbstract))
	TSoftClassPtr<AActor> ActorClass;

	// Only actors that implement this interface will be considered
	UPROPERTY(EditAnywhere, Category = "World Actor", meta = (AllowAbstract))
	TSoftClassPtr<UInterface> RequiredInterface;

	// Only actors that have a component of this class will be considered
	// NOTE: Only actors that have the component when initializing the view model will be considered, adding the component to an actor afterwards will not update the view model.
	UPROPERTY(EditAnywhere, Category = "World Actor", meta = (AllowAbstract))
	TSoftClassPtr<UActorComponent> RequiredComponentClass;

	// Only actors that have a component that implement this interface will be considered
	// NOTE: Only actors that have the component when initializing the view model will be considered, adding the component to an actor afterwards will not update the view model.
	UPROPERTY(EditAnywhere, Category = "World Actor", meta = (AllowAbstract))
	TSoftClassPtr<UInterface> RequiredComponentInterface;

	// Only actors that match this query will be considered (the actor must implement IGameplayTagAssetInterface to provider its gameplay tags)
	UPROPERTY(EditAnywhere, Category = "World Actor")
	FGameplayTagQuery TagQuery;

	// Only actors with the specified local net roles will be considered
	UPROPERTY(EditAnywhere, Category = "World Actor")
	TSet<TEnumAsByte<ENetRole>> AllowedNetRoles = { ROLE_None, ROLE_SimulatedProxy, ROLE_AutonomousProxy, ROLE_Authority };
};

// Wrapped on DelegateHandle to track which object the handle was for
struct MDVIEWMODEL_API FMDWrappedDelegateHandle
{
	FDelegateHandle Handle;
	TWeakObjectPtr<UObject> DelegateOwner;

	bool IsBound() const
	{
		// Only a handle is required to be considered Bound
		return Handle.IsValid();
	}
};

UENUM(meta = (Deprecated))
enum class EMDViewModelProvider_CacheLifetime
{
	Global,
	LocalPlayer,
	World,
	OwningPlayerController,
	OwningHUD,
	OwningPawn,
	OwningPlayerState,
	GameState,
	ViewTarget,
	ViewTargetPlayerState,
	Relative,
	RelativeProperty,
	Custom UMETA(Hidden),
	Invalid UMETA(Hidden),
};

USTRUCT(DisplayName = "Cached Provider Settings")
struct MDVIEWMODEL_API FMDViewModelProvider_Cached_Settings
{
	GENERATED_BODY()

public:
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FMDViewModelProvider_Cached_Settings() = default;
	FMDViewModelProvider_Cached_Settings(const FMDViewModelProvider_Cached_Settings& Other) = default;
	FMDViewModelProvider_Cached_Settings& operator=(const FMDViewModelProvider_Cached_Settings& Other) = default;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

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
	EMDViewModelProvider_CacheLifetime ViewModelLifetime = EMDViewModelProvider_CacheLifetime::Invalid;

	// For Relative lifetime, this view model's lifetime and context object will be tied to the view model assignment selected here
	UPROPERTY(EditAnywhere, Category = "Provider|Relative", meta = (MDVMShowNonManual="true", EditConditionLifetime = "MDVM.Provider.Cached.Lifetimes.Relative,MDVM.Provider.Cached.Lifetimes.RelativeViewModelProperty"))
	FMDViewModelAssignmentReference RelativeViewModel;

	// For World Actor lifetime, the first actor in the world that passes this filter will be the view model's cache and context object
	UPROPERTY(EditAnywhere, Category = "Provider|World Actor", meta = (EditConditionLifetime = "MDVM.Provider.Cached.Lifetimes.WorldActor"))
	FMDVMWorldActorFilter WorldActorFilter;

#if WITH_EDITORONLY_DATA
	// For Relative [View Model] Property lifetime, this is the FieldNotify property or function on the widget that will be used at the context for the view model (must be a UObject-derived type)
	UPROPERTY(EditAnywhere, Category = "Provider|Relative Property", meta = (GetOptions = "GetRelativePropertyNames", EditConditionLifetime = "MDVM.Provider.Cached.Lifetimes.RelativeProperty,MDVM.Provider.Cached.Lifetimes.RelativeViewModelProperty"))
	FName RelativePropertyName;
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
	static T* FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), const FInstancedStruct& ViewModelSettings = {});
	static UMDViewModelBase* FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings = {});
	template<typename T>
	static T* FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), const FName& CachedViewModelKey = MDViewModelUtils::DefaultViewModelName, const FInstancedStruct& ViewModelSettings = {});
	static UMDViewModelBase* FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& CachedViewModelKey = MDViewModelUtils::DefaultViewModelName, const FInstancedStruct& ViewModelSettings = {});
	
	template<typename T>
	static T* FindOrCreateCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), const FInstancedStruct& ViewModelSettings = {});
	static UMDViewModelBase* FindOrCreateCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings = {});
	template<typename T>
	static T* FindOrCreateCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), const FName& CachedViewModelKey = MDViewModelUtils::DefaultViewModelName, const FInstancedStruct& ViewModelSettings = {});
	static UMDViewModelBase* FindOrCreateCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& CachedViewModelKey = MDViewModelUtils::DefaultViewModelName, const FInstancedStruct& ViewModelSettings = {});

	// Try to find an existing view model for the given context object
	template<typename T>
	static T* FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), const FName& CachedViewModelKey = MDViewModelUtils::DefaultViewModelName);
	static UMDViewModelBase* FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& CachedViewModelKey = MDViewModelUtils::DefaultViewModelName);

	virtual void Deinitialize() override;

	virtual FGameplayTag GetProviderTag() const override { return TAG_MDVMProvider_Cached; }

	virtual UMDViewModelBase* SetViewModel(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) override;

#if WITH_EDITOR
	virtual FText GetDisplayName() const override { return INVTEXT("Cached Viewmodel"); }
	virtual FText GetDescription(const FInstancedStruct& ProviderSettings) const override;

	virtual UScriptStruct* GetProviderSettingsStruct() const override { return FMDViewModelProvider_Cached_Settings::StaticStruct(); }
	virtual bool ValidateProviderSettings(const FInstancedStruct& Settings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment, TArray<FText>& OutIssues) const override;
	virtual void OnProviderSettingsInitializedInEditor(FInstancedStruct& Settings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment) const override;
	virtual void OnAssignmentUpdated(FInstancedStruct& ProviderSettings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment) const override;
	virtual void GetExpectedContextObjectTypes(const FInstancedStruct& ProviderSettings, const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutContextObjectClasses) const override;
	virtual void GetExpectedContextObjectType(const FMDViewModelProvider_Cached_Settings& ProviderSettings, const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutContextObjectClasses) const;
#endif

protected:
	virtual IMDViewModelCacheInterface* ResolveAndBindViewModelCache(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data, const FMDViewModelProvider_Cached_Settings& Settings);

	UMDViewModelBase* FindOrCreateCachedViewModel_Internal(const UObject* WorldContextObject, UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings);
	UMDViewModelBase* FindOrCreateCachedViewModel_Internal(const UObject* WorldContextObject, const UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings);
	UMDViewModelBase* FindCachedViewModel_Internal(const UObject* WorldContextObject, const UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass) const;

	void BindOnObjectDestroy(IMDViewModelRuntimeInterface& Object);
	void OnObjectDestroy(TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr);

	void RefreshViewModel(TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);
	UMDViewModelBase* SetViewModelFromCache(const UObject* WorldContextObject, IMDViewModelCacheInterface* CacheInterface, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	void OnGameStateChanged(AGameStateBase* GameState, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);
	void OnViewTargetChanged(APlayerController* PC, AActor* OldViewTarget, AActor* NewViewTarget, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);
	void OnRelativeViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnFieldValueChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnActorSpawned(AActor* Actor, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);
	void OnActorRemoved(AActor* Actor, TWeakObjectPtr<AActor> BoundActor, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnLevelAddedToWorld(ULevel* Level, UWorld* World, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);
	void OnLevelRemovedFromWorld(ULevel* Level, UWorld* World, TWeakObjectPtr<AActor> BoundActor, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data);

	void OnViewModelCacheShuttingDown(TWeakInterfacePtr<IMDViewModelCacheInterface> CachePtr);

	IMDViewModelCacheInterface* RedirectCache(const UObject* WorldContextObject, IMDViewModelCacheInterface* Cache, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings) const;

	IMDViewModelCacheInterface* ResolveGlobalCache(const UGameInstance* GameInstance) const;
	IMDViewModelCacheInterface* ResolveLocalPlayerCache(const ULocalPlayer* LocalPlayer) const;
	IMDViewModelCacheInterface* ResolveWorldCache(const UWorld* World) const;
	IMDViewModelCacheInterface* ResolveActorCache(AActor* Actor) const;
	const IMDViewModelCacheInterface* ResolveActorCache(const AActor* Actor) const;
	IMDViewModelCacheInterface* ResolveObjectCache(UObject* Object, const UObject* WorldContextObject) const;
	const IMDViewModelCacheInterface* ResolveObjectCache(const UObject* Object, const UObject* WorldContextObject) const;

	IMDViewModelCacheInterface* ResolveHUDCacheAndBindDelegates(APlayerController* PlayerController, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	// Only local pawns can be bound to via the player controller, remote pawns must be bound through the player state
	IMDViewModelCacheInterface* ResolvePawnCacheAndBindDelegates(APlayerController* PlayerController, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolvePawnCacheAndBindDelegates(APlayerState* PlayerState, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	IMDViewModelCacheInterface* ResolvePlayerStateCacheAndBindDelegates(APlayerController* PlayerController, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolvePlayerStateCacheAndBindDelegates(APawn* Pawn, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	IMDViewModelCacheInterface* ResolveGameStateCacheAndBindDelegates(UWorld* World, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	IMDViewModelCacheInterface* ResolveViewTargetCacheAndBindDelegates(const APlayerController* PlayerController, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolveViewTargetPlayerStateCacheAndBindDelegates(const APlayerController* PlayerController, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	IMDViewModelCacheInterface* ResolveRelativeViewModelCacheAndBindDelegates(const FMDViewModelAssignmentReference& Reference, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolveRelativePropertyCacheAndBindDelegates(const FMemberReference& Reference, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolveRelativeViewModelPropertyCacheAndBindDelegates(const FMDViewModelAssignmentReference& VMReference, const FMemberReference& PropertyReference, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolveFieldCacheAndBindDelegates(UObject* Owner, const FMemberReference& Reference, int32 DelegateIndex, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	IMDViewModelCacheInterface* ResolveWorldActorCacheAndBindDelegates(const FMDVMWorldActorFilter& Filter, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	IMDViewModelCacheInterface* ResolveWorldActorCacheAndBindDelegates(AActor* Actor, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	// Bind RefreshViewModel to the specified view model changing and get the view model
	UMDViewModelBase* ResolveViewModelAndBindDelegates(const FMDViewModelAssignmentReference& Reference, int32 DelegateIndex, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	void BindViewTargetDelegates(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	void BindLevelAddedToWorldDelegates(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);
	void BindLevelRemovedFromWorldDelegates(TWeakObjectPtr<AActor> BoundActor, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data);

	bool DoesActorPassFilter(AActor* Candidate, const FMDVMWorldActorFilter& Filter) const;

	// Checks WidgetDelegateHandles to see if the handle at the specified index is bound, if not it adds a new entry and calls BindFunc to populate it
	template<typename T, typename TBindingKey, typename = typename TEnableIf<std::is_same_v<typename TDecay<TBindingKey>::Type, FMDVMAssignmentObjectKey>>::Type>
	void BindDelegateIfUnbound(TBindingKey&& BindingKey, T* Owner, int32 DelegateIndex, TFunctionRef<FDelegateHandle(T&)> BindFunc);

	// Checks WidgetDelegateHandles to see if the specified delegate exists, if so it calls UnbindFunc and resets the stored delegate data
	template<typename T>
	void UnbindDelegate(const FMDVMAssignmentObjectKey& BindingKey, int32 DelegateIndex, TFunctionRef<void(T&, FDelegateHandle&)> UnbindFunc);

	// Checks WidgetDelegateHandles to see if the delegate owner is different, if so it calls UnbindFunc and resets the stored delegate data
	template<typename T>
	void UnbindDelegateIfNewOwner(const FMDVMAssignmentObjectKey& BindingKey, const T* Owner, int32 DelegateIndex, TFunctionRef<void(T&, FDelegateHandle&)> UnbindFunc);

	// Certain lifetimes may require binding multiple delegates, they can index into the array to store multiple handles
	TMap<FMDVMAssignmentObjectKey, TArray<FMDWrappedDelegateHandle, TInlineAllocator<4>>> ObjectDelegateHandles;

	// TODO - Make TInterfaceKey or TObjectKey accept IInterface
	// Maps View Model caches to bound object assignments
	TMap<TWeakInterfacePtr<IMDViewModelCacheInterface>, TMultiMap<TWeakInterfacePtr<IMDViewModelRuntimeInterface>, FMDViewModelAssignmentReference>> BoundCaches;
	// Maps bound object assignments to the view model cache they're bound to
	TMap<TWeakInterfacePtr<IMDViewModelRuntimeInterface>, TMap<FMDViewModelAssignmentReference, TWeakInterfacePtr<IMDViewModelCacheInterface>>> BoundAssignments;

	template<typename T>
	bool IsValidObject(const T* Object) const;
};

template <typename T>
T* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	return FindOrCreateCachedViewModel<T>(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings);
}

template <typename T>
T* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& CachedViewModelKey, const FInstancedStruct& ViewModelSettings)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "T must derive from UMDViewModelBase");
	return Cast<T>(FindOrCreateCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings));
}

template <typename T>
T* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	return FindOrCreateCachedViewModel<T>(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings);
}

template <typename T>
T* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& CachedViewModelKey, const FInstancedStruct& ViewModelSettings)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "T must derive from UMDViewModelBase");
	return Cast<T>(FindOrCreateCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings));
}

template <typename T>
T* UMDViewModelProvider_Cached::FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& CachedViewModelKey)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "T must derive from UMDViewModelBase");
	return Cast<T>(FindCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey));
}

template <typename T, typename TBindingKey, typename>
void UMDViewModelProvider_Cached::BindDelegateIfUnbound(TBindingKey&& BindingKey, T* Owner, int32 DelegateIndex, TFunctionRef<FDelegateHandle(T&)> BindFunc)
{
	if (!IsValidObject(Owner))
	{
		return;
	}

	// Find or Add a Delegate Wrapper at the specified DelegateIndex
	auto& WrapperArray = ObjectDelegateHandles.FindOrAdd(Forward<FMDVMAssignmentObjectKey>(BindingKey));
	if (WrapperArray.IsValidIndex(DelegateIndex))
	{
		if (WrapperArray[DelegateIndex].IsBound())
		{
			// Already bound, we're good
			return;
		}
	}
	else
	{
		WrapperArray.AddDefaulted(DelegateIndex - WrapperArray.Num() + 1);
	}

	FMDWrappedDelegateHandle& Wrapper = WrapperArray[DelegateIndex];
	Wrapper.DelegateOwner = Cast<UObject>(Owner);
	Wrapper.Handle = BindFunc(*Owner);
}

template <typename T>
void UMDViewModelProvider_Cached::UnbindDelegate(const FMDVMAssignmentObjectKey& BindingKey, int32 DelegateIndex, TFunctionRef<void(T&, FDelegateHandle&)> UnbindFunc)
{
	auto* WrapperArrayPtr = ObjectDelegateHandles.Find(BindingKey);
	if (WrapperArrayPtr == nullptr || !WrapperArrayPtr->IsValidIndex(DelegateIndex))
	{
		return;
	}

	FMDWrappedDelegateHandle& Wrapper = (*WrapperArrayPtr)[DelegateIndex];
	T* OldOwner = Cast<T>(Wrapper.DelegateOwner.Get());
	if (!IsValidObject(OldOwner))
	{
		Wrapper = {}; // Clear anyway, otherwise we'll still be seen as "bound"
		return;
	}

	UnbindFunc(*OldOwner, Wrapper.Handle);
	Wrapper = {};
}

template <typename T>
void UMDViewModelProvider_Cached::UnbindDelegateIfNewOwner(const FMDVMAssignmentObjectKey& BindingKey, const T* Owner, int32 DelegateIndex, TFunctionRef<void(T&, FDelegateHandle&)> UnbindFunc)
{
	auto* WrapperArrayPtr = ObjectDelegateHandles.Find(BindingKey);
	if (WrapperArrayPtr == nullptr || !WrapperArrayPtr->IsValidIndex(DelegateIndex))
	{
		return;
	}

	FMDWrappedDelegateHandle& Wrapper = (*WrapperArrayPtr)[DelegateIndex];
	UObject* OldOwnerObject = Wrapper.DelegateOwner.Get();
	if constexpr (TIsIInterface<T>::Value)
	{
		if (OldOwnerObject == Cast<UObject>(Owner))
		{
			return;
		}
	}
	else if (OldOwnerObject == Owner)
	{
		return;
	}

	T* OldOwner = Cast<T>(OldOwnerObject);
	if (!IsValidObject(OldOwner))
	{
		Wrapper = {}; // Clear anyway, otherwise we'll still be seen as "bound"
		return;
	}

	UnbindFunc(*OldOwner, Wrapper.Handle);
	Wrapper = {};
}

template <typename T>
bool UMDViewModelProvider_Cached::IsValidObject(const T* Object) const
{
	using DecayedT = typename TDecay<T>::Type;
	if constexpr (TIsDerivedFrom<DecayedT, UObject>::Value)
	{
		return IsValid(Object);
	}

	if constexpr (TIsIInterface<DecayedT>::Value)
	{
		return IsValid(Cast<UObject>(Object));
	}

	return Object != nullptr;
}
