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
#include "WidgetBlueprint.h"
#endif

UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Cached, "MDVM.Provider.Cached");

bool FMDVMCachedProviderBindingKey::operator==(const FMDVMCachedProviderBindingKey& Other) const
{
	return Other.Assignment == Assignment && Other.WidgetPtr == WidgetPtr;
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
		// TODO - Most of these cases need to know when the player controller has changed

		IMDViewModelCacheInterface* ViewModelCache = nullptr;
		if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::Global)
		{
			ViewModelCache = ResolveGlobalCache(Widget.GetGameInstance());
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::LocalPlayer)
		{
			ViewModelCache = ResolveLocalPlayerCache(Widget.GetOwningLocalPlayer());
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::World)
		{
			ViewModelCache = ResolveWorldCache(Widget.GetWorld());
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::OwningPlayerController)
		{
			ViewModelCache = ResolveActorCache(Widget.GetOwningPlayer());
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::OwningHUD)
		{
			ViewModelCache = ResolveHUDCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::OwningPawn)
		{
			ViewModelCache = ResolvePawnCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::OwningPlayerState)
		{
			ViewModelCache = ResolvePlayerStateCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::GameState)
		{
			ViewModelCache = ResolveGameStateCacheAndBindDelegates(Widget.GetWorld(), Widget, Assignment, Data);
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::ViewTarget)
		{
			ViewModelCache = ResolveViewTargetCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::ViewTargetPlayerState)
		{
			ViewModelCache = ResolveViewTargetPlayerStateCacheAndBindDelegates(Widget.GetOwningPlayer(), Widget, Assignment, Data);
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::Relative)
		{
			ViewModelCache = ResolveRelativeViewModelCacheAndBindDelegates(Settings->RelativeViewModel, Widget, Assignment, Data);
		}

		if (ViewModelCache != nullptr)
		{
			UMDViewModelBase* ViewModelInstance = ViewModelCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
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
	const FMDViewModelProvider_Cached_Settings* SettingsPtr = Settings.GetMutablePtr<FMDViewModelProvider_Cached_Settings>();
	if (SettingsPtr == nullptr)
	{
		OutIssues.Add(INVTEXT("Provider Settings are not valid"));
		return false;
	}

	if (SettingsPtr->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::Relative)
	{
		if (SettingsPtr->RelativeViewModel.ViewModelClass.IsNull())
		{
			OutIssues.Add(INVTEXT("A valid Relative View Model is required to use the Relative view model life time option."));
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
			SettingsPtr->RelativeViewModel.OnGetWidgetClass.BindWeakLambda(WidgetBlueprint, [WidgetBlueprint]()
			{
				return IsValid(WidgetBlueprint) ? Cast<UClass>(WidgetBlueprint->GeneratedClass) : nullptr;
			});
		}
	}
}
#endif

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
	CleanUpBindingKeyMap(RelativeViewModelDelegateHandles);
	
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
	if (IsValid(Extension) && !RelativeViewModelDelegateHandles.Contains(BindingKey))
	{
		auto Delegate = FMDVMOnViewModelSet::FDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnRelativeViewModelChanged, MakeWeakObjectPtr(&Widget), Assignment, Data); 
		FDelegateHandle Handle = Extension->ListenForChanges(MoveTemp(Delegate), Reference.ViewModelClass.Get(), Reference.ViewModelName);
		RelativeViewModelDelegateHandles.Add(MoveTemp(BindingKey), MoveTemp(Handle));
	}
	
	const UMDViewModelBase* RelativeViewModel = Reference.ResolveViewModelAssignment(&Widget);
	if (IsValid(RelativeViewModel))
	{
		return ResolveObjectCacheAndBindDelegates(RelativeViewModel->GetContextObject(), Widget, Assignment, Data);
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveObjectCacheAndBindDelegates(UObject* Object, UUserWidget& Widget,
	const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
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
	else if (APlayerController* PlayerController = Cast<APlayerController>(Object))
	{
		return ResolveActorCache(PlayerController);
	}
	else if (const AHUD* HUD = Cast<AHUD>(Object))
	{
		return ResolveHUDCacheAndBindDelegates(HUD->PlayerOwner, Widget, Assignment, Data);
	}
	else if (APawn* Pawn = Cast<APawn>(Object))
	{
		if (APlayerController* PawnPC = Pawn->GetController<APlayerController>())
		{
			return ResolvePawnCacheAndBindDelegates(PawnPC, Widget, Assignment, Data);
		}
		else if (APlayerState* PawnPS = Pawn->GetPlayerState())
		{
			return ResolvePawnCacheAndBindDelegates(PawnPS, Widget, Assignment, Data);
		}
		else
		{
			return ResolveActorCache(Pawn);
		}
	}
	else if (APlayerState* PlayerState = Cast<APlayerState>(Object))
	{
		if (APlayerController* PlayerStatePC = PlayerState->GetPlayerController())
		{
			return ResolvePlayerStateCacheAndBindDelegates(PlayerStatePC, Widget, Assignment, Data);
		}
		else if (APawn* PlayerStatePawn = PlayerState->GetPawn())
		{
			return ResolvePlayerStateCacheAndBindDelegates(PlayerStatePawn, Widget, Assignment, Data);
		}
		else
		{
			return ResolveActorCache(PlayerState);
		}
	}
	else if (const AGameStateBase* GameState = Cast<AGameStateBase>(Object))
	{
		return ResolveGameStateCacheAndBindDelegates(GameState->GetWorld(), Widget, Assignment, Data);
	}
	else if (AActor* Actor = Cast<AActor>(Object))
	{
		return ResolveActorCache(Actor);
	}

	return nullptr;
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
