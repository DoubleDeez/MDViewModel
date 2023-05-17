#include "ViewModelProviders/MDViewModelProvider_Cached.h"

#include "GameDelegates.h"
#include "Blueprint/UserWidget.h"
#include "Components/MDViewModelCacheComponent.h"
#include "Components/MDViewModelPawnPossessionListenerComponent.h"
#include "Components/MDVMPlayerControllerUpdatePollingComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerState.h"
#include "Subsystems/MDGlobalViewModelCache.h"
#include "Subsystems/MDLocalPlayerViewModelCache.h"
#include "Subsystems/MDWorldViewModelCache.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "Util/MDViewModelNativeProvider.h"
#include "ViewModel/MDViewModelBase.h"


UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Cached, "MDVM.Provider.Cached");

MDVM_REGISTER_PROVIDER(FMDViewModelProvider_Cached, TAG_MDVMProvider_Cached);

bool FMDVMCachedProviderBindingKey::operator==(const FMDVMCachedProviderBindingKey& Other) const
{
	return Other.Assignment == Assignment && Other.WidgetPtr == WidgetPtr;
}

FMDViewModelProvider_Cached::~FMDViewModelProvider_Cached()
{
	FGameDelegates::Get().GetViewTargetChangedDelegate().RemoveAll(this);
}

UMDViewModelBase* FMDViewModelProvider_Cached::SetViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(Settings))
	{
		// TODO - Most of these cases need to know when the player controller has changed

		UMDViewModelBase* ViewModelInstance = nullptr;
		IMDViewModelCacheInterface* ViewModelCache = nullptr;
		if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::Global)
		{
			UMDGlobalViewModelCache* GlobalCache = UGameInstance::GetSubsystem<UMDGlobalViewModelCache>(Widget.GetGameInstance());
			if (ensure(GlobalCache))
			{
				ViewModelCache = GlobalCache;
				ViewModelInstance = GlobalCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::LocalPlayer)
		{
			UMDLocalPlayerViewModelCache* LocalPlayerCache = ULocalPlayer::GetSubsystem<UMDLocalPlayerViewModelCache>(Widget.GetOwningLocalPlayer());
			if (ensure(LocalPlayerCache))
			{
				ViewModelCache = LocalPlayerCache;
				ViewModelInstance = LocalPlayerCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::World)
		{
			UMDWorldViewModelCache* WorldCache = UWorld::GetSubsystem<UMDWorldViewModelCache>(Widget.GetWorld());
			if (ensure(WorldCache))
			{
				ViewModelCache = WorldCache;
				ViewModelInstance = WorldCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::OwningPlayerController)
		{
			APlayerController* PlayerController = Widget.GetOwningPlayer();
			if (IsValid(PlayerController))
			{
				UMDViewModelCacheComponent* Cache = UMDViewModelCacheComponent::FindOrAddCache(PlayerController);
				if (ensure(IsValid(Cache)))
				{
					ViewModelCache = Cache;
					ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
				}
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::OwningHUD)
		{
			APlayerController* PlayerController = Widget.GetOwningPlayer();
			if (IsValid(PlayerController))
			{
				AHUD* HUD = PlayerController->GetHUD();
				if (IsValid(HUD))
				{
					UMDViewModelCacheComponent* Cache = UMDViewModelCacheComponent::FindOrAddCache(HUD);
					if (ensure(IsValid(Cache)))
					{
						ViewModelCache = Cache;
						ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
					}
				}

				FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
				if (!DelegateHandles.Contains(BindingKey))
				{
					UMDVMPlayerControllerUpdatePollingComponent* Poller = UMDVMPlayerControllerUpdatePollingComponent::FindOrAddPollingComponent(PlayerController);
					if (IsValid(Poller))
					{
						FSimpleDelegate Delegate = FSimpleDelegate::CreateSP(this, &FMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
						FDelegateHandle Handle = Poller->BindOnHUDChanged(MoveTemp(Delegate));
						DelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
					}
				}
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::OwningPawn)
		{
			APawn* Pawn = Widget.GetOwningPlayerPawn();
			if (IsValid(Pawn))
			{
				UMDViewModelCacheComponent* Cache = UMDViewModelCacheComponent::FindOrAddCache(Pawn);
				if (ensure(IsValid(Cache)))
				{
					ViewModelCache = Cache;
					ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
				}
			}

			FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
			if (!DelegateHandles.Contains(BindingKey))
			{
				// We need to know when the pawn changes to update the view model
				APlayerController* PlayerController = Widget.GetOwningPlayer();
				if (IsValid(PlayerController))
				{
					// PlayerController->OnPossessedPawnChanged is a dynamic delegate which doesn't support SharedPtr bindings so we add an intermediate component to bridge that gap
					UMDViewModelPawnPossessionListenerComponent* ListenerComponent = UMDViewModelPawnPossessionListenerComponent::FindOrAddListener(PlayerController);
					if (ensure(IsValid(ListenerComponent)))
					{
						FDelegateHandle Handle = ListenerComponent->OnPawnChanged.AddSP(this, &FMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
						DelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
					}
				}
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::OwningPlayerState)
		{
			APlayerState* PlayerState = Widget.GetOwningPlayerState();
			if (IsValid(PlayerState))
			{
				UMDViewModelCacheComponent* Cache = UMDViewModelCacheComponent::FindOrAddCache(PlayerState);
				if (ensure(IsValid(Cache)))
				{
					ViewModelCache = Cache;
					ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
				}
			}

			FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
			if (!DelegateHandles.Contains(BindingKey))
			{
				APlayerController* PlayerController = Widget.GetOwningPlayer();
				if (IsValid(PlayerController))
				{
					UMDVMPlayerControllerUpdatePollingComponent* Poller = UMDVMPlayerControllerUpdatePollingComponent::FindOrAddPollingComponent(PlayerController);
					if (IsValid(Poller))
					{
						FSimpleDelegate Delegate = FSimpleDelegate::CreateSP(this, &FMDViewModelProvider_Cached::RefreshViewModel, MakeWeakObjectPtr(&Widget), Assignment, Data);
						FDelegateHandle Handle = Poller->BindOnPlayerStateChanged(MoveTemp(Delegate));
						DelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
					}
				}
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::GameState)
		{
			UWorld* World = Widget.GetWorld();
			if (IsValid(World))
			{
				AGameStateBase* GameState = World->GetGameState();
				UMDViewModelCacheComponent* Cache = UMDViewModelCacheComponent::FindOrAddCache(GameState);
				if (ensure(IsValid(Cache)))
				{
					ViewModelCache = Cache;
					ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
				}

				FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
				if (!DelegateHandles.Contains(BindingKey))
				{
					FDelegateHandle Handle = World->GameStateSetEvent.AddSP(this, &FMDViewModelProvider_Cached::OnGameStateChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
					DelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
				}
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::ViewTarget)
		{
			const APlayerController* PlayerController = Widget.GetOwningPlayer();
			if (IsValid(PlayerController))
			{
				const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
				if (IsValid(CameraManager))
				{
					AActor* ViewTarget = CameraManager->GetViewTarget();
					if (IsValid(ViewTarget))
					{
						UMDViewModelCacheComponent* Cache = UMDViewModelCacheComponent::FindOrAddCache(ViewTarget);
						if (ensure(IsValid(Cache)))
						{
							ViewModelCache = Cache;
							ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass, Data.ViewModelSettings);
						}
					}
				}
			}

			FMDVMCachedProviderBindingKey BindingKey = { Assignment, &Widget };
			if (!DelegateHandles.Contains(BindingKey))
			{
				FDelegateHandle Handle = FGameDelegates::Get().GetViewTargetChangedDelegate().AddSP(this, &FMDViewModelProvider_Cached::OnViewTargetChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
				DelegateHandles.Emplace(MoveTemp(BindingKey), MoveTemp(Handle));
			}
		}

		if (ViewModelInstance != nullptr)
		{
			UMDViewModelFunctionLibrary::SetViewModel(&Widget, ViewModelInstance, Assignment.ViewModelClass, Assignment.ViewModelName);
		}
		else
		{
			UMDViewModelFunctionLibrary::ClearViewModel(&Widget, Assignment.ViewModelClass, Assignment.ViewModelName);
		}

		if (ViewModelCache != nullptr && !ViewModelCache->OnViewModelCacheShuttingDown.IsBoundToObject(this))
		{
			ViewModelCache->OnViewModelCacheShuttingDown.AddSP(this, &FMDViewModelProvider_Cached::OnViewModelCacheShutdown, TWeakInterfacePtr<IMDViewModelCacheInterface>(ViewModelCache));
		}

		BoundAssignments.FindOrAdd(Assignment).Add(&Widget, TWeakInterfacePtr<IMDViewModelCacheInterface>(ViewModelCache));
	}

	return nullptr;
}

void FMDViewModelProvider_Cached::RefreshViewModel(TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	UUserWidget* Widget = WidgetPtr.Get();
	if (IsValid(Widget))
	{
		SetViewModel(*Widget, Assignment, Data);
	}
}

void FMDViewModelProvider_Cached::OnGameStateChanged(AGameStateBase* GameState, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment,
	FMDViewModelAssignmentData Data)
{
	UUserWidget* Widget = WidgetPtr.Get();
	if (IsValid(Widget))
	{
		SetViewModel(*Widget, Assignment, Data);
	}
}

void FMDViewModelProvider_Cached::OnViewTargetChanged(APlayerController* PC, AActor* OldViewTarget, AActor* NewViewTarget, TWeakObjectPtr<UUserWidget> WidgetPtr,
	FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	UUserWidget* Widget = WidgetPtr.Get();
	if (IsValid(Widget) && Widget->GetOwningPlayer() == PC)
	{
		SetViewModel(*Widget, Assignment, Data);
	}
}

void FMDViewModelProvider_Cached::OnViewModelCacheShutdown(const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& ViewModelCache, TWeakInterfacePtr<IMDViewModelCacheInterface> BoundCache)
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
			ViewModel->ShutdownViewModel();
		}
	}
}
