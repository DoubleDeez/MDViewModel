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
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerState.h"
#include "Subsystems/MDGlobalViewModelCache.h"
#include "Subsystems/MDLocalPlayerViewModelCache.h"
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
	"View model lifetime will be tied to a FieldNotify property or function on the widget, using the property/return value as its context object (must be an Actor or other supported type)");

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

bool FMDVMCachedProviderBindingKey::operator==(const FMDVMCachedProviderBindingKey& Other) const
{
	return Other.Assignment == Assignment && Other.WidgetPtr == WidgetPtr;
}

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

UMDViewModelBase* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(UObject* CacheContextObject, const FName& ViewModelName,
                                                                           TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	UMDViewModelProvider_Cached* Provider = IsValid(GEngine) ? GEngine->GetEngineSubsystem<UMDViewModelProvider_Cached>() : nullptr;
	if (IsValid(Provider))
	{
		return Provider->FindOrCreateCachedViewModel_Internal(CacheContextObject, ViewModelName, ViewModelClass, ViewModelSettings);
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
	if (ensure(Settings))
	{
		if (IMDViewModelCacheInterface* ViewModelCache = ResolveAndBindViewModelCache(Widget, Assignment, Data, *Settings))
		{
			const FName& ViewModelKey = Settings->bOverrideCachedViewModelKey ? Settings->CachedViewModelKeyOverride : Assignment.ViewModelName;
			UMDViewModelBase* ViewModelInstance = ViewModelCache->GetOrCreateViewModel(ViewModelKey, Assignment.ViewModelClass, Data.ViewModelSettings);
			if (IsValid(ViewModelInstance))
			{
				UMDViewModelFunctionLibrary::SetViewModel(&Widget, ViewModelInstance, Assignment.ViewModelClass, Assignment.ViewModelName);
			}
			else
			{
				UMDViewModelFunctionLibrary::ClearViewModel(&Widget, Assignment.ViewModelClass, Assignment.ViewModelName);
			}

			if (!ViewModelCache->OnViewModelCacheShuttingDown.IsBoundToObject(this))
			{
				ViewModelCache->OnViewModelCacheShuttingDown.AddUObject(this, &UMDViewModelProvider_Cached::OnViewModelCacheShutdown, TWeakInterfacePtr<IMDViewModelCacheInterface>(ViewModelCache));
			}

			BoundAssignments.FindOrAdd(Assignment).Add(&Widget, TWeakInterfacePtr<IMDViewModelCacheInterface>(ViewModelCache));

			return ViewModelInstance;
		}
		else
		{
			UMDViewModelFunctionLibrary::ClearViewModel(&Widget, Assignment.ViewModelClass, Assignment.ViewModelName);
		}
	}

	return nullptr;
}

#if WITH_EDITOR
bool UMDViewModelProvider_Cached::ValidateProviderSettings(const FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint, TArray<FText>& OutIssues) const
{
	const FMDViewModelProvider_Cached_Settings* SettingsPtr = Settings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (SettingsPtr == nullptr)
	{
		OutIssues.Add(INVTEXT("Provider Settings are not valid"));
		return false;
	}

	const FGameplayTag& Lifetime = SettingsPtr->GetLifetimeTag();
	if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Relative)
	{
		if (SettingsPtr->RelativeViewModel.ViewModelClass.IsNull())
		{
			OutIssues.Add(INVTEXT("A valid Relative View Model is required to use the Relative view model life time option."));
			return false;
		}
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty)
	{
		if (SettingsPtr->RelativePropertyName == NAME_None)
		{
			OutIssues.Add(INVTEXT("A valid Relative Property is required to use the Relative Property life time option."));
			return false;
		}
	}

	return Super::ValidateProviderSettings(Settings, WidgetBlueprint, OutIssues);
}

void UMDViewModelProvider_Cached::OnProviderSettingsInitializedInEditor(FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint) const
{
	if (IsValid(WidgetBlueprint))
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

			const UFunction* Function = SettingsPtr->RelativePropertyReference.ResolveMember<UFunction>(WidgetBlueprint->SkeletonGeneratedClass);
			const FObjectPropertyBase* Property = SettingsPtr->RelativePropertyReference.ResolveMember<FObjectPropertyBase>(WidgetBlueprint->SkeletonGeneratedClass);
			SettingsPtr->RelativePropertyName = (Function != nullptr) ? Function->GetFName() : ((Property != nullptr) ? Property->GetFName() : NAME_None);

			const FGameplayTag& LifetimeTag = SettingsPtr->GetLifetimeTag();
			SettingsPtr->bIsRelative = LifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_Relative;
			SettingsPtr->bIsRelativeProperty = LifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty;
		}
	}
}

void UMDViewModelProvider_Cached::OnProviderSettingsPropertyChanged(FInstancedStruct& Settings, UWidgetBlueprint* WidgetBlueprint) const
{
	if (IsValid(WidgetBlueprint) && IsValid(WidgetBlueprint->SkeletonGeneratedClass))
	{
		FMDViewModelProvider_Cached_Settings* SettingsPtr = Settings.GetMutablePtr<FMDViewModelProvider_Cached_Settings>();
		if (ensure(SettingsPtr))
		{
#if WITH_EDITORONLY_DATA
			const FGameplayTag& LifetimeTag = SettingsPtr->GetLifetimeTag();
			if (LifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty)
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
			else
			{
				SettingsPtr->RelativePropertyReference = {};
				SettingsPtr->RelativePropertyName = NAME_None;
			}

			SettingsPtr->bIsRelative = LifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_Relative;
			SettingsPtr->bIsRelativeProperty = LifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty;
#endif
		}
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

	return nullptr;
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel_Internal(UObject* CacheContextObject, const FName& ViewModelName,
	TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	if (IMDViewModelCacheInterface* ViewModelCache = ResolveObjectCache(CacheContextObject))
	{
		if (!ViewModelCache->OnViewModelCacheShuttingDown.IsBoundToObject(this))
		{
			ViewModelCache->OnViewModelCacheShuttingDown.AddUObject(this, &UMDViewModelProvider_Cached::OnViewModelCacheShutdown, TWeakInterfacePtr<IMDViewModelCacheInterface>(ViewModelCache));
		}

		return ViewModelCache->GetOrCreateViewModel(ViewModelName, ViewModelClass, ViewModelSettings);
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
	auto CleanUpBindingKeyMap = [&WidgetPtr](auto& Map)
	{
		for (auto It = Map.CreateIterator(); It; ++It)
		{
			if (It.Key().WidgetPtr == WidgetPtr)
			{
				It.RemoveCurrent();
			}
		}
	};

	CleanUpBindingKeyMap(WidgetDelegateHandles);
	CleanUpBindingKeyMap(ViewTargetDelegateHandles);

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

void UMDViewModelProvider_Cached::OnViewModelCacheShutdown(const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& ViewModelCache, TWeakInterfacePtr<IMDViewModelCacheInterface> BoundCache)
{
	for (const TTuple<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& ViewModelInstance : ViewModelCache)
	{
		// Find all widgets with this viewmodel assignment and clear it
		const FMDViewModelAssignment Assignment = {
			ViewModelInstance.Key.ViewModelClass,
			TAG_MDVMProvider_Cached,
			ViewModelInstance.Key.ViewModelName
		};

		if (const TMap<TWeakObjectPtr<UUserWidget>, TWeakInterfacePtr<IMDViewModelCacheInterface>>* BoundWidgets = BoundAssignments.Find(Assignment))
		{
			for (const TTuple<TWeakObjectPtr<UUserWidget>, TWeakInterfacePtr<IMDViewModelCacheInterface>>& BoundWidget : *BoundWidgets)
			{
				if (BoundWidget.Value == BoundCache)
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

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveObjectCache(UObject* Object) const
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

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveHUDCacheAndBindDelegates(APlayerController* PlayerController,
	UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPCUpdatePollingComponent* Poller = IsValid(PlayerController)
		? UMDVMPCUpdatePollingComponent::FindOrAddPollingComponent(PlayerController)
		: nullptr;

	FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
	if (WidgetDelegateHandles.Contains(BindingKey) && WidgetDelegateHandles[BindingKey].DelegateOwner != Poller)
	{
		if (auto* OldOwner = Cast<UMDVMPCUpdatePollingComponent>(WidgetDelegateHandles[BindingKey].DelegateOwner.Get()))
		{
			OldOwner->UnbindOnHUDChanged(WidgetDelegateHandles[BindingKey].Handle);
		}

		WidgetDelegateHandles.Remove(BindingKey);
	}

	if (IsValid(PlayerController))
	{
		if (IsValid(Poller) && !WidgetDelegateHandles.Contains(BindingKey))
		{
			FSimpleDelegate Delegate = FSimpleDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
			FMDWrappedDelegateHandle Handle;
			Handle.DelegateOwner = Poller;
			Handle.Handle = Poller->BindOnHUDChanged(MoveTemp(Delegate));
			WidgetDelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
		}

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

	FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
	if (WidgetDelegateHandles.Contains(BindingKey) && WidgetDelegateHandles[BindingKey].DelegateOwner != Intermediate)
	{
		if (auto* OldOwner = Cast<UMDVMPCDynamicDelegateIntermediate>(WidgetDelegateHandles[BindingKey].DelegateOwner.Get()))
		{
			OldOwner->OnPawnChanged.Remove(WidgetDelegateHandles[BindingKey].Handle);
		}

		WidgetDelegateHandles.Remove(BindingKey);
	}

	if (IsValid(PlayerController))
	{
		if (IsValid(Intermediate) && !WidgetDelegateHandles.Contains(BindingKey))
		{
			FMDWrappedDelegateHandle Handle;
			Handle.DelegateOwner = Intermediate;
			Handle.Handle = Intermediate->OnPawnChanged.AddUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
			WidgetDelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
		}

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

	FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
	if (WidgetDelegateHandles.Contains(BindingKey) && WidgetDelegateHandles[BindingKey].DelegateOwner != Intermediate)
	{
		if (auto* OldOwner = Cast<UMDVMPSDynamicDelegateIntermediate>(WidgetDelegateHandles[BindingKey].DelegateOwner.Get()))
		{
			OldOwner->OnPawnChanged.Remove(WidgetDelegateHandles[BindingKey].Handle);
		}

		WidgetDelegateHandles.Remove(BindingKey);
	}

	if (IsValid(PlayerState))
	{
		if (IsValid(Intermediate) && !WidgetDelegateHandles.Contains(BindingKey))
		{
			FMDWrappedDelegateHandle Handle;
			Handle.DelegateOwner = Intermediate;
			Handle.Handle = Intermediate->OnPawnChanged.AddUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
			WidgetDelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
		}

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

	FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
	if (WidgetDelegateHandles.Contains(BindingKey) && WidgetDelegateHandles[BindingKey].DelegateOwner != Poller)
	{
		if (auto* OldOwner = Cast<UMDVMPCUpdatePollingComponent>(WidgetDelegateHandles[BindingKey].DelegateOwner.Get()))
		{
			OldOwner->UnbindOnPlayerStateChanged(WidgetDelegateHandles[BindingKey].Handle);
		}

		WidgetDelegateHandles.Remove(BindingKey);
	}

	if (IsValid(PlayerController))
	{
		if (IsValid(Poller) && !WidgetDelegateHandles.Contains(BindingKey))
		{
			FSimpleDelegate Delegate = FSimpleDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
			FMDWrappedDelegateHandle Handle;
			Handle.DelegateOwner = Poller;
			Handle.Handle = Poller->BindOnPlayerStateChanged(MoveTemp(Delegate));
			WidgetDelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
		}

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

	FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
	if (WidgetDelegateHandles.Contains(BindingKey) && WidgetDelegateHandles[BindingKey].DelegateOwner != Poller)
	{
		if (auto* OldOwner = Cast<UMDVMPawnUpdatePollingComponent>(WidgetDelegateHandles[BindingKey].DelegateOwner.Get()))
		{
			OldOwner->UnbindOnPlayerStateChanged(WidgetDelegateHandles[BindingKey].Handle);
		}

		WidgetDelegateHandles.Remove(BindingKey);
	}

	if (IsValid(Pawn))
	{
		if (IsValid(Poller) && !WidgetDelegateHandles.Contains(BindingKey))
		{
			FSimpleDelegate Delegate = FSimpleDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
			FMDWrappedDelegateHandle Handle;
			Handle.DelegateOwner = Poller;
			Handle.Handle = Poller->BindOnPlayerStateChanged(MoveTemp(Delegate));
			WidgetDelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
		}

		return ResolveActorCache(Pawn->GetPlayerState());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveGameStateCacheAndBindDelegates(UWorld* World,
	UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
	if (WidgetDelegateHandles.Contains(BindingKey) && WidgetDelegateHandles[BindingKey].DelegateOwner != World)
	{
		if (auto* OldOwner = Cast<UWorld>(WidgetDelegateHandles[BindingKey].DelegateOwner.Get()))
		{
			OldOwner->GameStateSetEvent.Remove(WidgetDelegateHandles[BindingKey].Handle);
		}

		WidgetDelegateHandles.Remove(BindingKey);
	}

	if (IsValid(World))
	{
		if (!WidgetDelegateHandles.Contains(BindingKey))
		{
			FMDWrappedDelegateHandle Handle;
			Handle.DelegateOwner = World;
			Handle.Handle = World->GameStateSetEvent.AddUObject(this, &UMDViewModelProvider_Cached::OnGameStateChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
			WidgetDelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
		}

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
	FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(&Widget);
	if (WidgetDelegateHandles.Contains(BindingKey) && WidgetDelegateHandles[BindingKey].DelegateOwner != Extension)
	{
		if (auto* OldOwner = Cast<UMDViewModelWidgetExtension>(WidgetDelegateHandles[BindingKey].DelegateOwner.Get()))
		{
			OldOwner->StopListeningForChanges(WidgetDelegateHandles[BindingKey].Handle, Reference.ViewModelClass.Get(), Reference.ViewModelName);
		}

		WidgetDelegateHandles.Remove(BindingKey);
	}
	
	if (IsValid(Extension) && !WidgetDelegateHandles.Contains(BindingKey))
	{
		FMDWrappedDelegateHandle Handle;
		Handle.DelegateOwner = Extension;
		auto Delegate = FMDVMOnViewModelSet::FDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnRelativeViewModelChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
		Handle.Handle = Extension->ListenForChanges(MoveTemp(Delegate), Reference.ViewModelClass.Get(), Reference.ViewModelName);
		WidgetDelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
	}

	const UMDViewModelBase* RelativeViewModel = Reference.ResolveViewModelAssignment(&Widget);
	if (IsValid(RelativeViewModel))
	{
		return ResolveObjectCache(RelativeViewModel->GetContextObject());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveRelativePropertyCacheAndBindDelegates(const FMemberReference& Reference, UUserWidget& Widget,
	const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (Settings == nullptr)
	{
		return nullptr;
	}

	UFunction* Function = Settings->RelativePropertyReference.ResolveMember<UFunction>(Widget.GetClass());
	const FObjectPropertyBase* Property = Settings->RelativePropertyReference.ResolveMember<FObjectPropertyBase>(Widget.GetClass());
	const FName FieldName = (Function != nullptr) ? Function->GetFName() : ((Property != nullptr) ? Property->GetFName() : NAME_None);
	if (FieldName == NAME_None)
	{
		return nullptr;
	}

	const UE::FieldNotification::FFieldId FieldId = Widget.GetFieldNotificationDescriptor().GetField(Widget.GetClass(), FieldName);

	FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
	if (!WidgetDelegateHandles.Contains(BindingKey))
	{
		FMDWrappedDelegateHandle Handle;
		Handle.DelegateOwner = &Widget;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnFieldValueChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
		Handle.Handle = Widget.AddFieldValueChangedDelegate(FieldId, Delegate);
		WidgetDelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
	}

	UObject* ContextObject = nullptr;
	if (Property != nullptr)
	{
		ContextObject = Property->GetObjectPropertyValue_InContainer(&Widget);
	}
	else if (Function != nullptr)
	{
		const FObjectPropertyBase* ReturnProp = CastField<FObjectPropertyBase>(Function->GetReturnProperty());
		checkf(ReturnProp != nullptr && Function->NumParms == 1 && Function->ParmsSize == sizeof(ContextObject),
			TEXT("Function [%s] on [%s] is no longer a valid RelativeProperty function, update the view model assignment to fix this"), *Function->GetName(), *Widget.GetName());
		Widget.ProcessEvent(Function, &ContextObject);
	}

	return ResolveObjectCache(ContextObject);
}

void UMDViewModelProvider_Cached::BindViewTargetDelegates(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
	if (!ViewTargetDelegateHandles.Contains(BindingKey))
	{
		FDelegateHandle Handle = FGameDelegates::Get().GetViewTargetChangedDelegate().AddUObject(this, &UMDViewModelProvider_Cached::OnViewTargetChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
		ViewTargetDelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
	}
}
