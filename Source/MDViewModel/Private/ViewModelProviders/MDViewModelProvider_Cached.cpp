#include "ViewModelProviders/MDViewModelProvider_Cached.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
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
				GlobalCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::LocalPlayer)
		{
			UMDLocalPlayerViewModelCache* LocalPlayerCache = ULocalPlayer::GetSubsystem<UMDLocalPlayerViewModelCache>(Widget.GetOwningLocalPlayer());
			if (ensure(LocalPlayerCache))
			{
				LocalPlayerCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
			}
		}
		else if (Settings->ViewModelLifetime == EMDViewModelProvider_CacheLifetime::World)
		{
			UMDWorldViewModelCache* WorldCache = UWorld::GetSubsystem<UMDWorldViewModelCache>(Widget.GetWorld());
			if (ensure(WorldCache))
			{
				WorldCache->GetOrCreateViewModel(Assignment.ViewModelName, Assignment.ViewModelClass);
			}
		}

		if (ViewModelBase != nullptr)
		{
			UMDViewModelFunctionLibrary::AssignViewModel(&Widget, ViewModelBase, Assignment.ViewModelName);
		}
	}

	return nullptr;
}
