#include "ViewModelProviders/MDViewModelProvider_Cached.h"

#include "GameDelegates.h"
#include "Blueprint/UserWidget.h"
#include "Components/MDViewModelCacheComponent.h"
#include "Components/MDViewModelPawnPossessionListenerComponent.h"
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

UMDViewModelBase* FMDViewModelProvider_Cached::AssignViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(Settings))
	{
		UMDViewModelBase* ViewModelInstance = nullptr;
		if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::Global)
		{
			UMDGlobalViewModelCache* GlobalCache = UGameInstance::GetSubsystem<UMDGlobalViewModelCache>(Widget.GetGameInstance());
			if (ensure(GlobalCache))
			{
				ViewModelInstance = GlobalCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::LocalPlayer)
		{
			UMDLocalPlayerViewModelCache* LocalPlayerCache = ULocalPlayer::GetSubsystem<UMDLocalPlayerViewModelCache>(Widget.GetOwningLocalPlayer());
			if (ensure(LocalPlayerCache))
			{
				ViewModelInstance = LocalPlayerCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::World)
		{
			UMDWorldViewModelCache* WorldCache = UWorld::GetSubsystem<UMDWorldViewModelCache>(Widget.GetWorld());
			if (ensure(WorldCache))
			{
				ViewModelInstance = WorldCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
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
					ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
				}
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::OwningHUD)
		{
			const APlayerController* PlayerController = Widget.GetOwningPlayer();
			if (IsValid(PlayerController))
			{
				AHUD* HUD = PlayerController->GetHUD();
				if (IsValid(HUD))
				{
					UMDViewModelCacheComponent* Cache = UMDViewModelCacheComponent::FindOrAddCache(HUD);
					if (ensure(IsValid(Cache)))
					{
						ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
					}
				}

				// TODO - listen for HUD changes (needs polling)
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
					ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
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
						FDelegateHandle Handle = ListenerComponent->OnPawnChanged.AddSP(this, &FMDViewModelProvider_Cached::OnPawnChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
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
					ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
				}
			}

			// TODO - Listen for playerstate changed (needs polling)
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
					ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
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
				APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
				if (IsValid(CameraManager))
				{
					AActor* ViewTarget = CameraManager->GetViewTarget();
					if (IsValid(ViewTarget))
					{
						UMDViewModelCacheComponent* Cache = UMDViewModelCacheComponent::FindOrAddCache(ViewTarget);
						if (ensure(IsValid(Cache)))
						{
							ViewModelInstance = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
						}
					}
				}

				// TODO - Listen for camera manager changed (needs polling)
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
	}

	return nullptr;
}

void FMDViewModelProvider_Cached::OnGameStateChanged(AGameStateBase* GameState, TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment,
	FMDViewModelAssignmentData Data)
{
	UUserWidget* Widget = WidgetPtr.Get();
	if (IsValid(Widget))
	{
		AssignViewModel(*Widget, Assignment, Data);
	}
}

void FMDViewModelProvider_Cached::OnPawnChanged(TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	UUserWidget* Widget = WidgetPtr.Get();
	if (IsValid(Widget))
	{
		AssignViewModel(*Widget, Assignment, Data);
	}
}

void FMDViewModelProvider_Cached::OnViewTargetChanged(APlayerController* PC, AActor* OldViewTarget, AActor* NewViewTarget, TWeakObjectPtr<UUserWidget> WidgetPtr,
	FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	UUserWidget* Widget = WidgetPtr.Get();
	if (IsValid(Widget) && Widget->GetOwningPlayer() == PC)
	{
		AssignViewModel(*Widget, Assignment, Data);
	}
}
