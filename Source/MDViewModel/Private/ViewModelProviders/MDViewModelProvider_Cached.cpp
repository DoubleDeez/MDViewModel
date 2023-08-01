#include "ViewModelProviders/MDViewModelProvider_Cached.h"

#include "Blueprint/UserWidget.h"
#include "Components/MDViewModelCacheComponent.h"
#include "Components/MDVMPCUpdatePollingComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameDelegates.h"
#include "Components/MDVMPawnUpdatePollingComponent.h"
#include "Components/MDVMPCDynamicDelegateIntermediate.h"
#include "Components/MDVMPSDynamicDelegateIntermediate.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagAssetInterface.h"
#include "Subsystems/MDGlobalViewModelCache.h"
#include "Subsystems/MDLocalPlayerViewModelCache.h"
#include "Subsystems/MDObjectViewModelCache.h"
#include "Subsystems/MDWorldViewModelCache.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"

#if WITH_EDITOR
#include "Kismet2/BlueprintEditorUtils.h"
#include "WidgetBlueprint.h"
#endif

UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Cached, "MDVM.Provider.Cached");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_Global, "MDVM.Provider.Cached.Lifetimes.Global",
	"View model lifetime will be tied to the game instance");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_LocalPlayer, "MDVM.Provider.Cached.Lifetimes.LocalPlayer",
	"View model lifetime will be tied to the widget's owning local player");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_World, "MDVM.Provider.Cached.Lifetimes.World",
	"View model lifetime will be tied to the world");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerController, "MDVM.Provider.Cached.Lifetimes.OwningPlayerController",
	"View model lifetime will be tied to the widget's owning player controller");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_OwningHUD, "MDVM.Provider.Cached.Lifetimes.OwningHUD",
	"View model lifetime will be tied to the widget's owning player's HUD");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_OwningPawn, "MDVM.Provider.Cached.Lifetimes.OwningPawn",
	"View model lifetime will be tied to the widget's owning pawn");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerState, "MDVM.Provider.Cached.Lifetimes.OwningPlayerState",
	"View model lifetime will be tied to the widget's owning player state");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_GameState, "MDVM.Provider.Cached.Lifetimes.GameState",
	"View model lifetime will be tied to the game state");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_ViewTarget, "MDVM.Provider.Cached.Lifetimes.ViewTarget",
	"View model lifetime will be tied to the player's view target");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_ViewTargetPlayerState, "MDVM.Provider.Cached.Lifetimes.ViewTargetPlayerState",
	"View model lifetime will be tied to the player's view target's player state");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_Relative, "MDVM.Provider.Cached.Lifetimes.Relative",
	"View model lifetime will be tied to another view model on the widget and share its context object");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty, "MDVM.Provider.Cached.Lifetimes.RelativeProperty",
	"View model lifetime will be tied to a FieldNotify property or function on the widget, using the property/return value as its context object (must be UObject-based)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty, "MDVM.Provider.Cached.Lifetimes.RelativeViewModelProperty",
	"View model lifetime will be tied to a FieldNotify property or function on the specified relative view model, using the property/return value as its context object (must be UObject-based).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_WorldActor, "MDVM.Provider.Cached.Lifetimes.WorldActor",
	"View model lifetime will be tied to the first actor found in the world that passes the specified filter.");

namespace MDVMCachedProvider_Private
{
	const FGameplayTag& RemapLifetime(EMDViewModelProvider_CacheLifetime Lifetime)
	{
		static const TMap<EMDViewModelProvider_CacheLifetime, FGameplayTag> LifetimeMap =
		{
			{ EMDViewModelProvider_CacheLifetime::Global, TAG_MDVMProvider_Cached_Lifetimes_Global },
			{ EMDViewModelProvider_CacheLifetime::LocalPlayer, TAG_MDVMProvider_Cached_Lifetimes_LocalPlayer },
			{ EMDViewModelProvider_CacheLifetime::World, TAG_MDVMProvider_Cached_Lifetimes_World },
			{ EMDViewModelProvider_CacheLifetime::OwningPlayerController, TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerController },
			{ EMDViewModelProvider_CacheLifetime::OwningHUD, TAG_MDVMProvider_Cached_Lifetimes_OwningHUD },
			{ EMDViewModelProvider_CacheLifetime::OwningPawn, TAG_MDVMProvider_Cached_Lifetimes_OwningPawn },
			{ EMDViewModelProvider_CacheLifetime::OwningPlayerState, TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerState },
			{ EMDViewModelProvider_CacheLifetime::GameState, TAG_MDVMProvider_Cached_Lifetimes_GameState },
			{ EMDViewModelProvider_CacheLifetime::ViewTarget, TAG_MDVMProvider_Cached_Lifetimes_ViewTarget },
			{ EMDViewModelProvider_CacheLifetime::ViewTargetPlayerState, TAG_MDVMProvider_Cached_Lifetimes_ViewTargetPlayerState },
			{ EMDViewModelProvider_CacheLifetime::Relative, TAG_MDVMProvider_Cached_Lifetimes_Relative },
			{ EMDViewModelProvider_CacheLifetime::RelativeProperty, TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty }
		};

		const FGameplayTag* LifetimeTag = LifetimeMap.Find(Lifetime);
		return LifetimeTag != nullptr ? *LifetimeTag : FGameplayTag::EmptyTag;
	}
}

enum EMDVMDelegateIndex
{
	MDVMDI_Default = 0,
	MDVMDI_ViewTarget = 1
};

const FGameplayTag& FMDViewModelProvider_Cached_Settings::GetLifetimeTag() const
{
	if (ViewModelLifetimeTag.IsValid())
	{
		return ViewModelLifetimeTag;
	}
	
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return MDVMCachedProvider_Private::RemapLifetime(ViewModelLifetime);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, const FName& CachedViewModelKey,
                                                                           TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	return FindOrCreateCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings);
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& CachedViewModelKey, const FInstancedStruct& ViewModelSettings)
{
	UMDViewModelProvider_Cached* Provider = IsValid(GEngine) ? GEngine->GetEngineSubsystem<UMDViewModelProvider_Cached>() : nullptr;
	if (IsValid(Provider))
	{
		return Provider->FindOrCreateCachedViewModel_Internal(WorldContextObject, CacheContextObject, CachedViewModelKey, ViewModelClass, ViewModelSettings);
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& CachedViewModelKey)
{
	const UMDViewModelProvider_Cached* Provider = IsValid(GEngine) ? GEngine->GetEngineSubsystem<UMDViewModelProvider_Cached>() : nullptr;
	if (IsValid(Provider))
	{
		return Provider->FindCachedViewModel_Internal(WorldContextObject, CacheContextObject, CachedViewModelKey, ViewModelClass);
	}

	return nullptr;
}

void UMDViewModelProvider_Cached::Deinitialize()
{
	FGameDelegates::Get().GetViewTargetChangedDelegate().RemoveAll(this);

	Super::Deinitialize();
}

UMDViewModelBase* UMDViewModelProvider_Cached::SetViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UMDViewModelProvider_Cached::SetViewModel);

	BindOnWidgetDestroy(Widget);

	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(Settings != nullptr))
	{
		IMDViewModelCacheInterface* ViewModelCache = ResolveAndBindViewModelCache(Widget, Assignment, Data, *Settings);
		return SetViewModelFromCache(&Widget, ViewModelCache, Widget, Assignment, Data);
	}

	return nullptr;
}

#if WITH_EDITOR
bool UMDViewModelProvider_Cached::ValidateProviderSettings(const FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint, const FMDViewModelAssignment& Assignment, TArray<FText>& OutIssues) const
{
	const FMDViewModelProvider_Cached_Settings* SettingsPtr = Settings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (SettingsPtr == nullptr)
	{
		OutIssues.Add(INVTEXT("Provider Settings are not valid"));
		return false;
	}

	const FGameplayTag& Lifetime = SettingsPtr->GetLifetimeTag();
	if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Relative || Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty)
	{
		if (SettingsPtr->RelativeViewModel.ViewModelClass.IsNull())
		{
			OutIssues.Add(INVTEXT("A valid Relative View Model is required to use the Relative view model life time option."));
			return false;
		}
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty || Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty)
	{
		if (SettingsPtr->RelativePropertyName == NAME_None)
		{
			OutIssues.Add(INVTEXT("A valid Relative Property is required to use the Relative [View Model] Property life time option."));
			return false;
		}
	}

	return Super::ValidateProviderSettings(Settings, WidgetBlueprint, Assignment, OutIssues);
}

void UMDViewModelProvider_Cached::OnProviderSettingsInitializedInEditor(FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint, const FMDViewModelAssignment& Assignment) const
{
	FMDViewModelProvider_Cached_Settings* SettingsPtr = Settings.GetMutablePtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(SettingsPtr))
	{
		// Fixup old lifetime enum to tag
		if (!SettingsPtr->ViewModelLifetimeTag.IsValid())
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			SettingsPtr->ViewModelLifetimeTag = MDVMCachedProvider_Private::RemapLifetime(SettingsPtr->ViewModelLifetime);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
			FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBlueprint);
		}
		
		SettingsPtr->RelativeViewModel.OnGetWidgetClass.BindWeakLambda(WidgetBlueprint, [WidgetBlueprint]()
		{
			return IsValid(WidgetBlueprint) ? Cast<UClass>(WidgetBlueprint->GeneratedClass) : nullptr;
		});

		if (SettingsPtr->ViewModelLifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty && IsValid(WidgetBlueprint))
		{
			const UFunction* Function = SettingsPtr->RelativePropertyReference.ResolveMember<UFunction>(WidgetBlueprint->SkeletonGeneratedClass);
			const FObjectPropertyBase* Property = SettingsPtr->RelativePropertyReference.ResolveMember<FObjectPropertyBase>(WidgetBlueprint->SkeletonGeneratedClass);
			SettingsPtr->RelativePropertyName = (Function != nullptr) ? Function->GetFName() : ((Property != nullptr) ? Property->GetFName() : NAME_None);
		}
		else if (SettingsPtr->ViewModelLifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty && IsValid(SettingsPtr->RelativeViewModel.ViewModelClass.LoadSynchronous()))
		{
			const UFunction* Function = SettingsPtr->RelativePropertyReference.ResolveMember<UFunction>(SettingsPtr->RelativeViewModel.ViewModelClass.LoadSynchronous());
			const FObjectPropertyBase* Property = SettingsPtr->RelativePropertyReference.ResolveMember<FObjectPropertyBase>(SettingsPtr->RelativeViewModel.ViewModelClass.LoadSynchronous());
			SettingsPtr->RelativePropertyName = (Function != nullptr) ? Function->GetFName() : ((Property != nullptr) ? Property->GetFName() : NAME_None);
		}
	}
}

void UMDViewModelProvider_Cached::OnAssignmentUpdated(FInstancedStruct& ProviderSettings, UWidgetBlueprint* WidgetBlueprint, const FMDViewModelAssignment& Assignment) const
{
	FMDViewModelProvider_Cached_Settings* SettingsPtr = ProviderSettings.GetMutablePtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(SettingsPtr))
	{
#if WITH_EDITORONLY_DATA
		const FGameplayTag& LifetimeTag = SettingsPtr->GetLifetimeTag();
		if (LifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty && IsValid(WidgetBlueprint) && IsValid(WidgetBlueprint->SkeletonGeneratedClass))
		{
			if (const FProperty* Property = WidgetBlueprint->SkeletonGeneratedClass->FindPropertyByName(SettingsPtr->RelativePropertyName))
			{
				SettingsPtr->RelativePropertyReference.SetFromField<FProperty>(Property, WidgetBlueprint->SkeletonGeneratedClass);
			}
			else if (const UFunction* Function = WidgetBlueprint->SkeletonGeneratedClass->FindFunctionByName(SettingsPtr->RelativePropertyName))
			{
				SettingsPtr->RelativePropertyReference.SetFromField<UFunction>(Function, WidgetBlueprint->SkeletonGeneratedClass);
			}
			else
			{
				SettingsPtr->RelativePropertyReference = {};
			}
		}
		else if (LifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty && IsValid(SettingsPtr->RelativeViewModel.ViewModelClass.LoadSynchronous()))
		{
			const TSubclassOf<UMDViewModelBase> RelativeViewModelClass = SettingsPtr->RelativeViewModel.ViewModelClass.LoadSynchronous();
			if (const FProperty* Property = RelativeViewModelClass->FindPropertyByName(SettingsPtr->RelativePropertyName))
			{
				SettingsPtr->RelativePropertyReference.SetFromField<FProperty>(Property, RelativeViewModelClass);
			}
			else if (const UFunction* Function = RelativeViewModelClass->FindFunctionByName(SettingsPtr->RelativePropertyName))
			{
				SettingsPtr->RelativePropertyReference.SetFromField<UFunction>(Function, RelativeViewModelClass);
			}
			else
			{
				SettingsPtr->RelativePropertyReference = {};
			}
		}
		else
		{
			SettingsPtr->RelativePropertyReference = {};
			SettingsPtr->RelativePropertyName = NAME_None;
		}
#endif
	}
}
#endif

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveAndBindViewModelCache(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data,
	const FMDViewModelProvider_Cached_Settings& Settings)
{
	// TODO - Most of these cases need to know when the player controller has changed
	
	const FGameplayTag& Lifetime = Settings.GetLifetimeTag();
	if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Global)
	{
		return ResolveGlobalCache(Widget.GetGameInstance());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_LocalPlayer)
	{
		return ResolveLocalPlayerCache(Widget.GetOwningLocalPlayer());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_World)
	{
		return ResolveWorldCache(Widget.GetWorld());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerController)
	{
		return ResolveActorCache(Widget.GetOwningPlayer());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningHUD)
	{
		return ResolveHUDCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningPawn)
	{
		return ResolvePawnCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerState)
	{
		return ResolvePlayerStateCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_GameState)
	{
		return ResolveGameStateCacheAndBindDelegates(Widget.GetWorld(), Widget, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_ViewTarget)
	{
		return ResolveViewTargetCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_ViewTargetPlayerState)
	{
		return ResolveViewTargetPlayerStateCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Relative)
	{
		return ResolveRelativeViewModelCacheAndBindDelegates(Settings.RelativeViewModel, Widget, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty)
	{
		return ResolveRelativePropertyCacheAndBindDelegates(Settings.RelativePropertyReference, Widget, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty)
	{
		return ResolveRelativeViewModelPropertyCacheAndBindDelegates(Settings.RelativeViewModel, Settings.RelativePropertyReference, Widget, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_WorldActor)
	{
		return ResolveWorldActorCacheAndBindDelegates(Settings.WorldActorFilter, Widget, Assignment, Data);
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel_Internal(const UObject* WorldContextObject, UObject* CacheContextObject, const FName& ViewModelName,
	TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	if (IMDViewModelCacheInterface* ViewModelCache = ResolveObjectCache(CacheContextObject, WorldContextObject))
	{
		if (!ViewModelCache->OnViewModelCacheShuttingDown.IsBoundToObject(this))
		{
			ViewModelCache->OnViewModelCacheShuttingDown.AddUObject(this, &UMDViewModelProvider_Cached::OnViewModelCacheShutdown, ViewModelCache->GetCacheHandle());
		}

		return ViewModelCache->GetOrCreateViewModel(WorldContextObject, ViewModelName, ViewModelClass, ViewModelSettings);
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindCachedViewModel_Internal(const UObject* WorldContextObject, const UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass) const
{
	if (const IMDViewModelCacheInterface* ViewModelCache = ResolveObjectCache(CacheContextObject, WorldContextObject))
	{
		return ViewModelCache->GetViewModel(ViewModelName, ViewModelClass);
	}

	return nullptr;
}

void UMDViewModelProvider_Cached::BindOnWidgetDestroy(UUserWidget& Widget)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(&Widget);
	if (IsValid(Extension) && !Extension->OnBeginDestroy.IsBoundToObject(this))
	{
		Extension->OnBeginDestroy.AddUObject(this, &UMDViewModelProvider_Cached::OnWidgetDestroy, MakeWeakObjectPtr(&Widget));
	}
}

void UMDViewModelProvider_Cached::OnWidgetDestroy(TWeakObjectPtr<UUserWidget> WidgetPtr)
{
	for (auto It = WidgetDelegateHandles.CreateIterator(); It; ++It)
	{
		if (It.Key().WidgetPtr == WidgetPtr)
		{
			It.RemoveCurrent();
		}
	}

	// Clean up BoundAssignments of all WidgetPtr instances and empty inner maps
	{
		for (auto It = BoundAssignments.CreateIterator(); It; ++It)
		{
			for (auto InnerIt = It.Value().CreateIterator(); InnerIt; ++InnerIt)
			{
				if (InnerIt.Key() == WidgetPtr)
				{
					InnerIt.RemoveCurrent();
				}
			}

			if (It.Value().IsEmpty())
			{
				It.RemoveCurrent();
			}
		}
	}
}

void UMDViewModelProvider_Cached::RefreshViewModel(TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	UUserWidget* Widget = WidgetPtr.Get();
	if (IsValid(Widget))
	{
		SetViewModel(*Widget, Assignment, Data);
	}
}

UMDViewModelBase* UMDViewModelProvider_Cached::SetViewModelFromCache(const UObject* WorldContextObject, IMDViewModelCacheInterface* CacheInterface, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(Settings != nullptr) && CacheInterface != nullptr)
	{
		const FName& ViewModelKey = Settings->bOverrideCachedViewModelKey ? Settings->CachedViewModelKeyOverride : Assignment.ViewModelName;
		UMDViewModelBase* ViewModelInstance = CacheInterface->GetOrCreateViewModel(WorldContextObject, ViewModelKey, Assignment.ViewModelClass, Data.ViewModelSettings);
		if (IsValid(ViewModelInstance))
		{
			UMDViewModelFunctionLibrary::SetViewModel(&Widget, ViewModelInstance, Assignment.ViewModelClass, Assignment.ViewModelName);
		}
		else
		{
			UMDViewModelFunctionLibrary::ClearViewModel(&Widget, Assignment.ViewModelClass, Assignment.ViewModelName);
		}

		if (!CacheInterface->OnViewModelCacheShuttingDown.IsBoundToObject(this))
		{
			CacheInterface->OnViewModelCacheShuttingDown.AddUObject(this, &UMDViewModelProvider_Cached::OnViewModelCacheShutdown, CacheInterface->GetCacheHandle());
		}

		BoundAssignments.FindOrAdd(Assignment).Add(&Widget, CacheInterface->GetCacheHandle());

		return ViewModelInstance;
	}
	else
	{
		UMDViewModelFunctionLibrary::ClearViewModel(&Widget, Assignment.ViewModelClass, Assignment.ViewModelName);
	}

	return nullptr;
}

void UMDViewModelProvider_Cached::OnGameStateChanged(AGameStateBase* GameState, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment,
                                                     FMDViewModelAssignmentData Data)
{
	RefreshViewModel(WidgetPtr, Assignment, Data);
}

void UMDViewModelProvider_Cached::OnViewTargetChanged(APlayerController* PC, AActor* OldViewTarget, AActor* NewViewTarget, TWeakObjectPtr<UUserWidget> WidgetPtr,
	FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	UUserWidget* Widget = WidgetPtr.Get();
	// View Target binding is only on owning player so we can check that directly here
	if (IsValid(Widget) && Widget->GetOwningPlayer() == PC)
	{
		RefreshViewModel(WidgetPtr, Assignment, Data);
	}
}

void UMDViewModelProvider_Cached::OnRelativeViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel,
	TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	RefreshViewModel(WidgetPtr, Assignment, Data);
}

void UMDViewModelProvider_Cached::OnFieldValueChanged(UObject* Widget, UE::FieldNotification::FFieldId FieldId, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment,
	FMDViewModelAssignmentData Data)
{
	RefreshViewModel(WidgetPtr, Assignment, Data);
}

void UMDViewModelProvider_Cached::OnActorSpawned(AActor* Actor, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	// Don't go through refresh view model since we have the Actor we need already so we can avoid the expensive TActorIterator
	
	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	UUserWidget* Widget = WidgetPtr.Get();
	if (IsValid(Actor) && IsValid(Widget))
	{
		if (DoesActorPassFilter(Actor, Settings->WorldActorFilter))
		{
			IMDViewModelCacheInterface* CacheInterface = ResolveWorldActorCacheAndBindDelegates(Actor, *Widget, Assignment, Data);
			SetViewModelFromCache(Actor, CacheInterface, *Widget, Assignment, Data);
		}
	}
}

void UMDViewModelProvider_Cached::OnActorRemoved(AActor* Actor, TWeakObjectPtr<AActor> BoundActor, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	if (BoundActor.Get() == Actor)
	{
		RefreshViewModel(WidgetPtr, Assignment, Data);
	}
}

void UMDViewModelProvider_Cached::OnViewModelCacheShutdown(const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& ViewModelCache, int32 BoundCacheHandle)
{
	for (const TTuple<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& ViewModelInstance : ViewModelCache)
	{
		// Find all widgets with this viewmodel assignment and clear it
		const FMDViewModelAssignment Assignment = {
			ViewModelInstance.Key.ViewModelClass,
			TAG_MDVMProvider_Cached,
			ViewModelInstance.Key.ViewModelName
		};

		if (const TMap<TWeakObjectPtr<UUserWidget>, int32>* BoundWidgets = BoundAssignments.Find(Assignment))
		{
			for (const TTuple<TWeakObjectPtr<UUserWidget>, int32>& BoundWidget : *BoundWidgets)
			{
				if (BoundWidget.Value == BoundCacheHandle)
				{
					UMDViewModelFunctionLibrary::ClearViewModel(BoundWidget.Key.Get(), Assignment.ViewModelClass, Assignment.ViewModelName);
				}
			}
		}

		UMDViewModelBase* ViewModel = ViewModelInstance.Value;
		if (IsValid(ViewModel))
		{
			ViewModel->ShutdownViewModelFromProvider();
		}
	}
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveGlobalCache(const UGameInstance* GameInstance) const
{
	return UGameInstance::GetSubsystem<UMDGlobalViewModelCache>(GameInstance);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveLocalPlayerCache(const ULocalPlayer* LocalPlayer) const
{
	return ULocalPlayer::GetSubsystem<UMDLocalPlayerViewModelCache>(LocalPlayer);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveWorldCache(const UWorld* World) const
{
	return UWorld::GetSubsystem<UMDWorldViewModelCache>(World);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveActorCache(AActor* Actor) const
{
	return UMDViewModelCacheComponent::FindOrAddCache(Actor);
}

const IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveActorCache(const AActor* Actor) const
{
	if (IsValid(Actor))
	{
		return Actor->FindComponentByClass<UMDViewModelCacheComponent>();
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveObjectCache(UObject* Object, const UObject* WorldContextObject) const
{
	if (const UGameInstance* GameInstance = Cast<UGameInstance>(Object))
	{
		return ResolveGlobalCache(GameInstance);
	}
	else if (const UWorld* World = Cast<UWorld>(Object))
	{
		return ResolveWorldCache(World);
	}
	else if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Object))
	{
		return ResolveLocalPlayerCache(LocalPlayer);
	}
	else if (AActor* Actor = Cast<AActor>(Object))
	{
		return ResolveActorCache(Actor);
	}
	else if (IsValid(Object))
	{
		return UMDObjectViewModelCacheSystem::ResolveCacheForObject(Object, WorldContextObject);
	}

	return nullptr;
}

const IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveObjectCache(const UObject* Object, const UObject* WorldContextObject) const
{
	if (const UGameInstance* GameInstance = Cast<UGameInstance>(Object))
	{
		return ResolveGlobalCache(GameInstance);
	}
	else if (const UWorld* World = Cast<UWorld>(Object))
	{
		return ResolveWorldCache(World);
	}
	else if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Object))
	{
		return ResolveLocalPlayerCache(LocalPlayer);
	}
	else if (const AActor* Actor = Cast<AActor>(Object))
	{
		return ResolveActorCache(Actor);
	}
	else if (IsValid(Object))
	{
		return UMDObjectViewModelCacheSystem::ResolveCacheForObject(Object, WorldContextObject);
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveHUDCacheAndBindDelegates(APlayerController* PlayerController,
                                                                                         UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPCUpdatePollingComponent* Poller = IsValid(PlayerController)
		? UMDVMPCUpdatePollingComponent::FindOrAddPollingComponent(PlayerController)
		: nullptr;

	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	UnbindDelegateIfNewOwner<UMDVMPCUpdatePollingComponent>(BindingKey, Poller, MDVMDI_Default, [](auto& OldPoller, FDelegateHandle& Handle)
	{
		OldPoller.UnbindOnHUDChanged(Handle);
	});

	if (IsValid(PlayerController))
	{
		BindDelegateIfUnbound<UMDVMPCUpdatePollingComponent>(MoveTemp(BindingKey), Poller, MDVMDI_Default, [&](auto& Owner)
		{
			FSimpleDelegate Delegate = FSimpleDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
			return Owner.BindOnHUDChanged(MoveTemp(Delegate));
		});

		return ResolveActorCache(PlayerController->GetHUD());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolvePawnCacheAndBindDelegates(APlayerController* PlayerController,
	UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPCDynamicDelegateIntermediate* Intermediate = IsValid(PlayerController)
		? UMDVMPCDynamicDelegateIntermediate::FindOrAddListener(PlayerController)
		: nullptr;

	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	UnbindDelegateIfNewOwner<UMDVMPCDynamicDelegateIntermediate>(BindingKey, Intermediate, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.OnPawnChanged.Remove(Handle);
	});

	if (IsValid(PlayerController))
	{
		BindDelegateIfUnbound<UMDVMPCDynamicDelegateIntermediate>(MoveTemp(BindingKey), Intermediate, MDVMDI_Default, [&](auto& Owner)
		{
			return Owner.OnPawnChanged.AddUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
		});

		return ResolveActorCache(PlayerController->GetPawn());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolvePawnCacheAndBindDelegates(APlayerState* PlayerState,
	UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPSDynamicDelegateIntermediate* Intermediate = IsValid(PlayerState)
		? UMDVMPSDynamicDelegateIntermediate::FindOrAddListener(PlayerState)
		: nullptr;

	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	UnbindDelegateIfNewOwner<UMDVMPSDynamicDelegateIntermediate>(BindingKey, Intermediate, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.OnPawnChanged.Remove(Handle);
	});

	if (IsValid(PlayerState))
	{
		BindDelegateIfUnbound<UMDVMPSDynamicDelegateIntermediate>(MoveTemp(BindingKey), Intermediate, MDVMDI_Default, [&](auto& Owner)
		{
			return Owner.OnPawnChanged.AddUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
		});

		return ResolveActorCache(PlayerState->GetPawn());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolvePlayerStateCacheAndBindDelegates(APlayerController* PlayerController,
	UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPCUpdatePollingComponent* Poller = IsValid(PlayerController)
		? UMDVMPCUpdatePollingComponent::FindOrAddPollingComponent(PlayerController)
		: nullptr;

	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	UnbindDelegateIfNewOwner<UMDVMPCUpdatePollingComponent>(BindingKey, Poller, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.UnbindOnPlayerStateChanged(Handle);
	});

	if (IsValid(PlayerController))
	{
		BindDelegateIfUnbound<UMDVMPCUpdatePollingComponent>(MoveTemp(BindingKey), Poller, MDVMDI_Default, [&](auto& Owner)
		{
			FSimpleDelegate Delegate = FSimpleDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
			return Owner.BindOnPlayerStateChanged(MoveTemp(Delegate));
		});

		return ResolveActorCache(PlayerController->PlayerState);
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolvePlayerStateCacheAndBindDelegates(APawn* Pawn, UUserWidget& Widget,
	const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPawnUpdatePollingComponent* Poller = IsValid(Pawn)
		? UMDVMPawnUpdatePollingComponent::FindOrAddPollingComponent(Pawn)
		: nullptr;

	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	UnbindDelegateIfNewOwner<UMDVMPawnUpdatePollingComponent>(BindingKey, Poller, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.UnbindOnPlayerStateChanged(Handle);
	});

	if (IsValid(Pawn))
	{
		BindDelegateIfUnbound<UMDVMPawnUpdatePollingComponent>(MoveTemp(BindingKey), Poller, MDVMDI_Default, [&](auto& Owner)
		{
			FSimpleDelegate Delegate = FSimpleDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
			return Owner.BindOnPlayerStateChanged(MoveTemp(Delegate));
		});

		return ResolveActorCache(Pawn->GetPlayerState());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveGameStateCacheAndBindDelegates(UWorld* World,
	UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	UnbindDelegateIfNewOwner<UWorld>(BindingKey, World, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.GameStateSetEvent.Remove(Handle);
	});

	if (IsValid(World))
	{
		BindDelegateIfUnbound<UWorld>(MoveTemp(BindingKey), World, MDVMDI_Default, [&](auto& Owner)
		{
			return Owner.GameStateSetEvent.AddUObject(this, &UMDViewModelProvider_Cached::OnGameStateChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
		});

		return ResolveActorCache(World->GetGameState());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveViewTargetCacheAndBindDelegates(const APlayerController* PlayerController,
	UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	BindViewTargetDelegates(Widget, Assignment, Data);

	if (IsValid(PlayerController))
	{
		// We don't need to detect CameraManager changes since FGameDelegates::Get().GetViewTargetChangedDelegate() is global
		const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
		if (IsValid(CameraManager))
		{
			return ResolveActorCache(CameraManager->GetViewTarget());
		}
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveViewTargetPlayerStateCacheAndBindDelegates(const APlayerController* PlayerController,
	UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	BindViewTargetDelegates(Widget, Assignment, Data);

	if (IsValid(PlayerController))
	{
		// We don't need to detect CameraManager changes since FGameDelegates::Get().GetViewTargetChangedDelegate() is global
		const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
		if (IsValid(CameraManager))
		{
			return ResolvePlayerStateCacheAndBindDelegates(Cast<APawn>(CameraManager->GetViewTarget()), Widget, Assignment, Data);
		}
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveRelativeViewModelCacheAndBindDelegates(const FMDViewModelAssignmentReference& Reference,
	UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	const UMDViewModelBase* RelativeViewModel = ResolveViewModelAndBindDelegates(Reference, MDVMDI_Default, Widget, Assignment, Data);
	if (!IsValid(RelativeViewModel))
	{
		return nullptr;
	}

	const UObject* WorldContextObject = RelativeViewModel->GetWorld();
	if (!IsValid(WorldContextObject))
	{
		WorldContextObject = Widget.GetWorld();
	}
	return ResolveObjectCache(RelativeViewModel->GetContextObject(), WorldContextObject);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveRelativePropertyCacheAndBindDelegates(const FMemberReference& Reference, UUserWidget& Widget,
	const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	return ResolveFieldCacheAndBindDelegates(&Widget, Reference, MDVMDI_Default, Widget, Assignment, Data);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveRelativeViewModelPropertyCacheAndBindDelegates(const FMDViewModelAssignmentReference& VMReference,
	const FMemberReference& PropertyReference, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	constexpr int32 ViewModelDelegateIndex = MDVMDI_Default;	
	UMDViewModelBase* RelativeViewModel = ResolveViewModelAndBindDelegates(VMReference, MDVMDI_Default, Widget, Assignment, Data);
	
	if (!IsValid(RelativeViewModel))
	{
		return nullptr;
	}

	constexpr int32 PropertyDelegateIndex = ViewModelDelegateIndex + 1;
	return ResolveFieldCacheAndBindDelegates( RelativeViewModel, PropertyReference, PropertyDelegateIndex, Widget, Assignment, Data);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveFieldCacheAndBindDelegates(UObject* Owner, const FMemberReference& Reference,
	int32 DelegateIndex, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	INotifyFieldValueChanged* FieldNotifyOwner = Cast<INotifyFieldValueChanged>(Owner);
	if (!IsValid(Owner) || FieldNotifyOwner == nullptr)
	{
		return nullptr;
	}

	UFunction* Function = Reference.ResolveMember<UFunction>(Owner->GetClass());
	const FObjectPropertyBase* Property = Reference.ResolveMember<FObjectPropertyBase>(Owner->GetClass());
	const FName FieldName = (Function != nullptr) ? Function->GetFName() : ((Property != nullptr) ? Property->GetFName() : NAME_None);
	if (FieldName == NAME_None)
	{
		return nullptr;
	}

	const UE::FieldNotification::FFieldId FieldId = FieldNotifyOwner->GetFieldNotificationDescriptor().GetField(Owner->GetClass(), FieldName);
	
	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	UnbindDelegateIfNewOwner<INotifyFieldValueChanged>(BindingKey, Owner, DelegateIndex, [&FieldId](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.RemoveFieldValueChangedDelegate(FieldId, Handle);
	});

	BindDelegateIfUnbound<INotifyFieldValueChanged>(MoveTemp(BindingKey), FieldNotifyOwner, DelegateIndex, [&](auto& NewOwner)
	{
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnFieldValueChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
		return NewOwner.AddFieldValueChangedDelegate(FieldId, Delegate);
	});
		
	UObject* PropertyValue = nullptr;
	if (Property != nullptr)
	{
		PropertyValue = Property->GetObjectPropertyValue_InContainer(Owner);
	}
	else if (Function != nullptr)
	{
		const FObjectPropertyBase* ReturnProp = CastField<FObjectPropertyBase>(Function->GetReturnProperty());
		checkf(ReturnProp != nullptr && Function->NumParms == 1 && Function->ParmsSize == sizeof(PropertyValue),
			TEXT("Function [%s] on [%s] is no longer a valid RelativeProperty function, update the view model assignment to fix this"), *Function->GetName(), *Owner->GetName());
		Owner->ProcessEvent(Function, &PropertyValue);
	}

	const UObject* WorldContextObject = Owner->GetWorld();
	if (!IsValid(WorldContextObject))
	{
		WorldContextObject = Widget.GetWorld();
	}
	return ResolveObjectCache(PropertyValue, WorldContextObject);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveWorldActorCacheAndBindDelegates(const FMDVMWorldActorFilter& Filter, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	AActor* FoundActor = nullptr;
	if (const UWorld* World = Widget.GetWorld())
	{
		const TSubclassOf<AActor> ActorClass = Filter.ActorClass.IsNull() ? AActor::StaticClass() : Filter.ActorClass.Get();
		if (ActorClass != nullptr)
		{
			for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
			{
				AActor* Candidate = *It;
				if (DoesActorPassFilter(Candidate, Filter))
				{
					FoundActor = Candidate;
					break;
				}
			}
		}
	}

	return ResolveWorldActorCacheAndBindDelegates(FoundActor, Widget, Assignment, Data);	
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveWorldActorCacheAndBindDelegates(AActor* Actor, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UWorld* World = Widget.GetWorld();

	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	UnbindDelegateIfNewOwner<UWorld>(BindingKey, World, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.RemoveOnActorSpawnedHandler(Handle);
		OldOwner.RemoveOnActorRemovedFromWorldHandler(Handle);
	});
	
	if (IsValid(Actor))
	{
		// Unbind if we were bound since we don't need to listen for it anymore
		UnbindDelegate<UWorld>(BindingKey, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
		{
			OldOwner.RemoveOnActorSpawnedHandler(Handle);
		});

		// Bind to when the actor is removed from the world so we can find a new actor that meets our criteria
		BindDelegateIfUnbound<UWorld>(MoveTemp(BindingKey), World, MDVMDI_Default, [&](auto& Owner)
		{
			auto Delegate = FOnActorRemovedFromWorld::FDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnActorRemoved, MakeWeakObjectPtr(Actor), MakeWeakObjectPtr(&Widget), Assignment, Data);
			return Owner.AddOnActorRemovedFromWorldHandler(MoveTemp(Delegate));
		});
		
		return ResolveActorCache(Actor);
	}
	else
	{
		UnbindDelegate<UWorld>(BindingKey, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
		{
			OldOwner.RemoveOnActorRemovedFromWorldHandler(Handle);
		});
		
		BindDelegateIfUnbound<UWorld>(MoveTemp(BindingKey), World, MDVMDI_Default, [&](auto& Owner)
		{
			auto Delegate = FOnActorSpawned::FDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnActorSpawned, MakeWeakObjectPtr(&Widget), Assignment, Data);
			return Owner.AddOnActorSpawnedHandler(MoveTemp(Delegate));
		});

		return nullptr;
	}
}

UMDViewModelBase* UMDViewModelProvider_Cached::ResolveViewModelAndBindDelegates(const FMDViewModelAssignmentReference& Reference, int32 DelegateIndex, UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(&Widget);
	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	UnbindDelegateIfNewOwner<UMDViewModelWidgetExtension>(BindingKey, Extension, DelegateIndex, [Reference](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.StopListeningForChanges(Handle, Reference.ViewModelClass.Get(), Reference.ViewModelName);
	});

	BindDelegateIfUnbound<UMDViewModelWidgetExtension>(MoveTemp(BindingKey), Extension, DelegateIndex, [&](auto& Owner)
	{
		auto Delegate = FMDVMOnViewModelSet::FDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnRelativeViewModelChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
		return Owner.ListenForChanges(MoveTemp(Delegate), Reference.ViewModelClass.Get(), Reference.ViewModelName);
	});

	return Reference.ResolveViewModelAssignment(&Widget);
}

void UMDViewModelProvider_Cached::BindViewTargetDelegates(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	FMDVMAssignmentWidgetKey BindingKey = { Assignment, &Widget };
	BindDelegateIfUnbound<UUserWidget>(MoveTemp(BindingKey), &Widget, MDVMDI_ViewTarget, [&](auto& Owner)
	{
		return FGameDelegates::Get().GetViewTargetChangedDelegate().AddUObject(this, &UMDViewModelProvider_Cached::OnViewTargetChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
	});
}

bool UMDViewModelProvider_Cached::DoesActorPassFilter(AActor* Candidate, const FMDVMWorldActorFilter& Filter) const
{
	if (!IsValid(Candidate))
	{
		return false;
	}

	// This actor is on its way out
	const UMDViewModelCacheComponent* ExistingCache = Candidate->FindComponentByClass<UMDViewModelCacheComponent>();
	if (ExistingCache != nullptr && ExistingCache->IsShutdown())
	{
		return false;
	}

	if (!Filter.AllowedNetRoles.Contains(Candidate->GetLocalRole()))
	{
		return false;
	}

	const TSubclassOf<AActor> ActorClass = Filter.ActorClass.Get();
	if (ActorClass != nullptr && !Candidate->IsA(ActorClass))
	{
		return false;
	}

	const TSubclassOf<UInterface> RequiredInterface = Filter.RequiredInterface.Get();
	if (RequiredInterface != nullptr && !Candidate->GetClass()->ImplementsInterface(RequiredInterface))
	{
		return false;
	}

	const TSubclassOf<UActorComponent> RequiredComponentClass = Filter.RequiredComponentClass.Get();
	if (RequiredComponentClass != nullptr && Candidate->FindComponentByClass(RequiredComponentClass) == nullptr)
	{
		return false;
	}

	const TSubclassOf<UInterface> RequiredComponentInterface = Filter.RequiredComponentInterface.Get();
	if (RequiredComponentInterface != nullptr && Candidate->FindComponentByInterface(RequiredComponentInterface) == nullptr)
	{
		return false;
	}

	if (!Filter.TagQuery.IsEmpty())
	{
		const IGameplayTagAssetInterface* CandidateGameplayTagAsset = Cast<IGameplayTagAssetInterface>(Candidate);
		if (CandidateGameplayTagAsset == nullptr)
		{
			return false;
		}

		FGameplayTagContainer CandidateTags;
		CandidateGameplayTagAsset->GetOwnedGameplayTags(CandidateTags);

		if (!Filter.TagQuery.Matches(CandidateTags))
		{
			return false;
		}
	}

	return true;
}
