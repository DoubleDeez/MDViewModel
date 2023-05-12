#include "ViewModelProviders/MDViewModelProvider_Cached.h"

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

UMDViewModelBase* FMDViewModelProvider_Cached::AssignViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(Settings))
	{
		UMDViewModelBase* ViewModelBase = nullptr;
		if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::Global)
		{
			UMDGlobalViewModelCache* GlobalCache = UGameInstance::GetSubsystem<UMDGlobalViewModelCache>(Widget.GetGameInstance());
			if (ensure(GlobalCache))
			{
				ViewModelBase = GlobalCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::LocalPlayer)
		{
			UMDLocalPlayerViewModelCache* LocalPlayerCache = ULocalPlayer::GetSubsystem<UMDLocalPlayerViewModelCache>(Widget.GetOwningLocalPlayer());
			if (ensure(LocalPlayerCache))
			{
				ViewModelBase = LocalPlayerCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::World)
		{
			UMDWorldViewModelCache* WorldCache = UWorld::GetSubsystem<UMDWorldViewModelCache>(Widget.GetWorld());
			if (ensure(WorldCache))
			{
				ViewModelBase = WorldCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
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
					ViewModelBase = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
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
						ViewModelBase = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
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
					ViewModelBase = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
				}
			}

			// We need to know when the pawn changes to update the view model
			APlayerController* PlayerController = Widget.GetOwningPlayer();
			if (IsValid(PlayerController))
			{
				UMDViewModelPawnPossessionListenerComponent* ListenerComponent = UMDViewModelPawnPossessionListenerComponent::FindOrAddListener(PlayerController);
				if (ensure(IsValid(ListenerComponent)))
				{
					ListenerComponent->OnPawnChanged.AddSP(this, &FMDViewModelProvider_Cached::OnPawnChanged, MakeWeakObjectPtr(&Widget), Assignment, Data);
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
					ViewModelBase = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
				}
			}

			// TODO - Listen for playerstate changes
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::GameState)
		{
			const UWorld* World = Widget.GetWorld();
			if (IsValid(World))
			{
				AGameStateBase* GameState = World->GetGameState();
				UMDViewModelCacheComponent* Cache = UMDViewModelCacheComponent::FindOrAddCache(GameState);
				if (ensure(IsValid(Cache)))
				{
					ViewModelBase = Cache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
				}
			}
		}

		if (ViewModelBase != nullptr)
		{
			UMDViewModelFunctionLibrary::SetViewModel(&Widget, ViewModelBase, Assignment.ViewModelName);
		}
	}

	return nullptr;
}

void FMDViewModelProvider_Cached::OnPawnChanged(TWeakObjectPtr<UUserWidget> WidgetPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	UUserWidget* Widget = WidgetPtr.Get();
	if (IsValid(Widget))
	{
		AssignViewModel(*Widget, Assignment, Data);
	}
}
