#include "ViewModelProviders/MDViewModelProvider_Cached.h"

#include "Components/MDViewModelCacheComponent.h"
#include "Components/MDVMPCUpdatePollingComponent.h"
#include "Components/MDVMPawnUpdatePollingComponent.h"
#include "Components/MDVMPCDynamicDelegateIntermediate.h"
#include "Components/MDVMPSDynamicDelegateIntermediate.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameDelegates.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagAssetInterface.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
#include "Logging/StructuredLog.h"
#endif
#include "Subsystems/MDGlobalViewModelCache.h"
#include "Subsystems/MDLocalPlayerViewModelCache.h"
#include "Subsystems/MDObjectViewModelCache.h"
#include "Subsystems/MDWorldViewModelCache.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelConfig.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "Util/MDViewModelLog.h"
#include "ViewModel/MDViewModelBase.h"

#if WITH_EDITOR
#include "Kismet2/BlueprintEditorUtils.h"
#endif

UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Cached, "MDVM.Provider.Cached");

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1
#define UE_DEFINE_GAMEPLAY_TAG_COMMENT(TagName, Tag, Comment) FNativeGameplayTag TagName(UE_PLUGIN_NAME, UE_MODULE_NAME, Tag, TEXT(Comment), ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD); static_assert(UE::GameplayTags::Private::HasFileExtension(__FILE__, ".cpp"), "UE_DEFINE_GAMEPLAY_TAG can only be used in .cpp files, if you're trying to share tags across modules, use UE_DECLARE_GAMEPLAY_TAG_EXTERN in the public header, and UE_DEFINE_GAMEPLAY_TAG in the private .cpp");
#endif

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_Global, "MDVM.Provider.Cached.Lifetimes.Global",
	"View model lifetime will be tied to the game instance");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_LocalPlayer, "MDVM.Provider.Cached.Lifetimes.LocalPlayer",
	"View model lifetime will be tied to the object's owning local player. Object must be related to a player controller to function.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_World, "MDVM.Provider.Cached.Lifetimes.World",
	"View model lifetime will be tied to the world");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerController, "MDVM.Provider.Cached.Lifetimes.OwningPlayerController",
	"View model lifetime will be tied to the object's owning player controller. Object must be related to a player controller to function.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_OwningHUD, "MDVM.Provider.Cached.Lifetimes.OwningHUD",
	"View model lifetime will be tied to the object's owning player's HUD. Object must be related to a player controller to function.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_OwningPawn, "MDVM.Provider.Cached.Lifetimes.OwningPawn",
	"View model lifetime will be tied to the object's owning player's pawn. Object must be related to a player controller to function.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerState, "MDVM.Provider.Cached.Lifetimes.OwningPlayerState",
	"View model lifetime will be tied to the object's owning player state. Object must be related to a player controller to function.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_GameState, "MDVM.Provider.Cached.Lifetimes.GameState",
	"View model lifetime will be tied to the game state");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_ViewTarget, "MDVM.Provider.Cached.Lifetimes.ViewTarget",
	"View model lifetime will be tied to the player's view target. Object must be related to a player controller to function.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_ViewTargetPlayerState, "MDVM.Provider.Cached.Lifetimes.ViewTargetPlayerState",
	"View model lifetime will be tied to the player's view target's player state. Object must be related to a player controller to function.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_Relative, "MDVM.Provider.Cached.Lifetimes.Relative",
	"View model lifetime will be tied to another view model on the object and share its context object");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty, "MDVM.Provider.Cached.Lifetimes.RelativeProperty",
	"View model lifetime will be tied to a FieldNotify property or function on the object, using the property/return value as its context object (must be UObject-based)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty, "MDVM.Provider.Cached.Lifetimes.RelativeViewModelProperty",
	"View model lifetime will be tied to a FieldNotify property or function on the specified relative view model, using the property/return value as its context object (must be UObject-based).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_WorldActor, "MDVM.Provider.Cached.Lifetimes.WorldActor",
	"View model lifetime will be tied to the first actor found in the world that passes the specified filter.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_MDVMProvider_Cached_Lifetimes_Self, "MDVM.Provider.Cached.Lifetimes.Self",
	"View model lifetime will be tied to this object.");

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1
#undef UE_DEFINE_GAMEPLAY_TAG_COMMENT
#endif

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
	MDVMDI_ViewTarget = 1,
	MDVMDI_LevelWorldChanged = 2,
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

UMDViewModelBase* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	return FindOrCreateCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings);
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& CachedViewModelKey, const FInstancedStruct& ViewModelSettings)
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
	FWorldDelegates::LevelAddedToWorld.RemoveAll(this);
	FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);

	Super::Deinitialize();
}

UMDViewModelBase* UMDViewModelProvider_Cached::SetViewModel(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UMDViewModelProvider_Cached::SetViewModel);

	BindOnObjectDestroy(Object);

	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(Settings != nullptr))
	{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
		UE_LOGFMT(LogMDViewModel, Verbose, "Setting Cached View Model with Assignment [{Assignment}] with Lifetime [{Lifetime}] for Object [{ObjectName}]",
			("ObjectName", GetPathNameSafe(Object.GetOwningObject())),
			("Lifetime", Settings->GetLifetimeTag().GetTagName()),
			("Assignment", Assignment));
#else
		UE_LOG(LogMDViewModel, Verbose, TEXT("Setting Cached View Model with Assignment [%s (%s)] with Lifetime [%s] for Object [%s]"),
			*GetNameSafe(Assignment.ViewModelClass),
			*Assignment.ViewModelName.ToString(),
			*Settings->GetLifetimeTag().GetTagName().ToString(),
			*GetPathNameSafe(Object.GetOwningObject())
		);
#endif
		IMDViewModelCacheInterface* ViewModelCache = ResolveAndBindViewModelCache(Object, Assignment, Data, *Settings);
		ViewModelCache = RedirectCache(Object.ResolveWorld(), ViewModelCache, Assignment.ViewModelClass, Data.ViewModelSettings);
		return SetViewModelFromCache(Object.ResolveWorld(), ViewModelCache, Object, Assignment, Data);
	}

	return nullptr;
}

#if WITH_EDITOR
FText UMDViewModelProvider_Cached::GetDescription(const FInstancedStruct& ProviderSettings) const
{
	const FMDViewModelProvider_Cached_Settings* SettingsPtr = ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (SettingsPtr == nullptr)
	{
		// Settings are null for static references to this provider (eg. in the provider selector)
		return INVTEXT("The view model will be grabbed from (or added to) the selected cache, keyed by the view model name and class.");
	}

	static const FText DescriptionFormat = INVTEXT("The view model will be grabbed from (or added to) the selected cache, keyed by the view model name and class.\n\nSelected Lifetime: {0}\n{1}");

	const FGameplayTag& LifetimeTag = SettingsPtr->GetLifetimeTag();
	FString TagComment;
	FName TagSource;
	bool bTagIsExplicit;
	bool bTagIsRestricted;
	bool bTagAllowsNonRestrictedChildren;

	UGameplayTagsManager::Get().GetTagEditorData(LifetimeTag.GetTagName(), TagComment, TagSource, bTagIsExplicit, bTagIsRestricted, bTagAllowsNonRestrictedChildren);
	return FText::Format(DescriptionFormat, FText::FromName(LifetimeTag.GetTagName()), FText::FromString(TagComment));
}

bool UMDViewModelProvider_Cached::ValidateProviderSettings(const FInstancedStruct& Settings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment, TArray<FText>& OutIssues) const
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

	return Super::ValidateProviderSettings(Settings, Blueprint, Assignment, OutIssues);
}

void UMDViewModelProvider_Cached::OnProviderSettingsInitializedInEditor(FInstancedStruct& Settings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment) const
{
	FMDViewModelProvider_Cached_Settings* SettingsPtr = Settings.GetMutablePtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(SettingsPtr))
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		// Fixup old lifetime enum to tag
		if (!SettingsPtr->ViewModelLifetimeTag.IsValid() && SettingsPtr->ViewModelLifetime != EMDViewModelProvider_CacheLifetime::Invalid)
		{
			SettingsPtr->ViewModelLifetimeTag = MDVMCachedProvider_Private::RemapLifetime(SettingsPtr->ViewModelLifetime);
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		}
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		if (!SettingsPtr->ViewModelLifetimeTag.IsValid())
		{
			SettingsPtr->ViewModelLifetimeTag = GetDefault<UMDViewModelConfig>()->DefaultViewModelLifetime;
		}

		SettingsPtr->RelativeViewModel.OnGetBoundObjectClass.BindWeakLambda(Blueprint, [Blueprint]()
		{
			return IsValid(Blueprint) ? Cast<UClass>(Blueprint->GeneratedClass) : nullptr;
		});

		if (SettingsPtr->ViewModelLifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty && IsValid(Blueprint))
		{
			const UFunction* Function = SettingsPtr->RelativePropertyReference.ResolveMember<UFunction>(Blueprint->SkeletonGeneratedClass);
			const FObjectPropertyBase* Property = SettingsPtr->RelativePropertyReference.ResolveMember<FObjectPropertyBase>(Blueprint->SkeletonGeneratedClass);
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

void UMDViewModelProvider_Cached::OnAssignmentUpdated(FInstancedStruct& ProviderSettings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment) const
{
	FMDViewModelProvider_Cached_Settings* SettingsPtr = ProviderSettings.GetMutablePtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(SettingsPtr))
	{
#if WITH_EDITORONLY_DATA
		const FGameplayTag& LifetimeTag = SettingsPtr->GetLifetimeTag();
		if (LifetimeTag == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty && IsValid(Blueprint) && IsValid(Blueprint->SkeletonGeneratedClass))
		{
			if (const FProperty* Property = Blueprint->SkeletonGeneratedClass->FindPropertyByName(SettingsPtr->RelativePropertyName))
			{
				SettingsPtr->RelativePropertyReference.SetFromField<FProperty>(Property, Blueprint->SkeletonGeneratedClass);
			}
			else if (const UFunction* Function = Blueprint->SkeletonGeneratedClass->FindFunctionByName(SettingsPtr->RelativePropertyName))
			{
				SettingsPtr->RelativePropertyReference.SetFromField<UFunction>(Function, Blueprint->SkeletonGeneratedClass);
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

void UMDViewModelProvider_Cached::GetExpectedContextObjectTypes(const FInstancedStruct& ProviderSettings, const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutContextObjectClasses) const
{
	const FMDViewModelProvider_Cached_Settings* SettingsPtr = ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(SettingsPtr))
	{
		GetExpectedContextObjectType(*SettingsPtr, ViewModelSettings, Blueprint, OutContextObjectClasses);
	}
}

void UMDViewModelProvider_Cached::GetExpectedContextObjectType(const FMDViewModelProvider_Cached_Settings& ProviderSettings, const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutContextObjectClasses) const
{
	const FGameplayTag& Lifetime = ProviderSettings.GetLifetimeTag();
	UClass* AssignedObjectClass = Blueprint->SkeletonGeneratedClass != nullptr ? Blueprint->SkeletonGeneratedClass : Blueprint->GeneratedClass;
	if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Global)
	{
		OutContextObjectClasses.Add(UGameInstance::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_LocalPlayer)
	{
		OutContextObjectClasses.Add(ULocalPlayer::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_World)
	{
		OutContextObjectClasses.Add(UWorld::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerController)
	{
		OutContextObjectClasses.Add(APlayerController::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningHUD)
	{
		OutContextObjectClasses.Add(AHUD::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningPawn)
	{
		OutContextObjectClasses.Add(APawn::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerState)
	{
		OutContextObjectClasses.Add(APlayerState::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_GameState)
	{
		OutContextObjectClasses.Add(AGameStateBase::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_ViewTarget)
	{
		OutContextObjectClasses.Add(AActor::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_ViewTargetPlayerState)
	{
		OutContextObjectClasses.Add(APlayerState::StaticClass());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Relative)
	{
		const TSubclassOf<UMDViewModelBase> ViewModelClass = ProviderSettings.RelativeViewModel.ViewModelClass.LoadSynchronous();
		if (const UMDViewModelBase* ViewModelCDO = ViewModelClass.GetDefaultObject())
		{
			ViewModelCDO->GetStoredContextObjectTypes(ViewModelSettings, Blueprint, OutContextObjectClasses);
		}
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty)
	{
		const FMemberReference& Reference = ProviderSettings.RelativePropertyReference;
		const UFunction* Function = Reference.ResolveMember<UFunction>(AssignedObjectClass);
		const FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(IsValid(Function) ? MDViewModelUtils::GetFunctionReturnProperty(Function) : Reference.ResolveMember<FProperty>(AssignedObjectClass));

		if (Property != nullptr)
		{
			OutContextObjectClasses.Add(Property->PropertyClass);
		}
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty)
	{
		UClass* RelativeVMClass = ProviderSettings.RelativeViewModel.ViewModelClass.LoadSynchronous();
		const FMemberReference& Reference = ProviderSettings.RelativePropertyReference;
		const UFunction* Function = Reference.ResolveMember<UFunction>(RelativeVMClass);
		const FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(IsValid(Function) ? MDViewModelUtils::GetFunctionReturnProperty(Function) : Reference.ResolveMember<FProperty>(RelativeVMClass));

		if (Property != nullptr)
		{
			OutContextObjectClasses.Add(Property->PropertyClass);
		}
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_WorldActor)
	{
		OutContextObjectClasses.Add(ProviderSettings.WorldActorFilter.ActorClass.IsNull() ? AActor::StaticClass() : ProviderSettings.WorldActorFilter.ActorClass.LoadSynchronous());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Self)
	{
		OutContextObjectClasses.Add(AssignedObjectClass);
	}
}
#endif

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveAndBindViewModelCache(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data,
	const FMDViewModelProvider_Cached_Settings& Settings)
{
	const FGameplayTag& Lifetime = Settings.GetLifetimeTag();
	if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Global)
	{
		return ResolveGlobalCache(Object.ResolveGameInstance());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_LocalPlayer)
	{
		return ResolveLocalPlayerCache(Object.ResolveOwningLocalPlayer());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_World)
	{
		return ResolveWorldCache(Object.ResolveWorld());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerController)
	{
		return ResolveActorCache(Object.ResolveOwningPlayer());
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningHUD)
	{
		return ResolveHUDCacheAndBindDelegates(Object.ResolveOwningPlayer(), Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningPawn)
	{
		return ResolvePawnCacheAndBindDelegates(Object.ResolveOwningPlayer(), Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_OwningPlayerState)
	{
		return ResolvePlayerStateCacheAndBindDelegates(Object.ResolveOwningPlayer(), Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_GameState)
	{
		return ResolveGameStateCacheAndBindDelegates(Object.ResolveWorld(), Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_ViewTarget)
	{
		return ResolveViewTargetCacheAndBindDelegates(Object.ResolveOwningPlayer(), Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_ViewTargetPlayerState)
	{
		return ResolveViewTargetPlayerStateCacheAndBindDelegates(Object.ResolveOwningPlayer(), Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Relative)
	{
		return ResolveRelativeViewModelCacheAndBindDelegates(Settings.RelativeViewModel, Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeProperty)
	{
		return ResolveRelativePropertyCacheAndBindDelegates(Settings.RelativePropertyReference, Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_RelativeViewModelProperty)
	{
		return ResolveRelativeViewModelPropertyCacheAndBindDelegates(Settings.RelativeViewModel, Settings.RelativePropertyReference, Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_WorldActor)
	{
		return ResolveWorldActorCacheAndBindDelegates(Settings.WorldActorFilter, Object, Assignment, Data);
	}
	else if (Lifetime == TAG_MDVMProvider_Cached_Lifetimes_Self)
	{
		return ResolveObjectCache(Object.GetOwningObject(), Object.ResolveWorld());
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel_Internal(const UObject* WorldContextObject, UObject* CacheContextObject, const FName& ViewModelName,
	TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	if (ensureAlwaysMsgf(IsValid(CacheContextObject), TEXT("Attempting to Find or Create a Cached View Model on an invalid CacheContextObject")))
	{
		if (IMDViewModelCacheInterface* ViewModelCache = ResolveObjectCache(CacheContextObject, WorldContextObject))
		{
			ViewModelCache = RedirectCache(WorldContextObject, ViewModelCache, ViewModelClass, ViewModelSettings);
			if (ensureAlwaysMsgf(ViewModelCache != nullptr, TEXT("Failed to redirect View Model Cache for context [%s] with View Model Class [%s]"), *GetNameSafe(CacheContextObject), *GetNameSafe(ViewModelClass)))
			{
				return ViewModelCache->GetOrCreateViewModel(WorldContextObject, ViewModelName, ViewModelClass, ViewModelSettings);
			}
		}
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindOrCreateCachedViewModel_Internal(const UObject* WorldContextObject, const UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings)
{
	if (ensureAlwaysMsgf(IsValid(CacheContextObject), TEXT("Attempting to Find or Create a Cached View Model on an invalid CacheContextObject")))
	{
		if (IMDViewModelCacheInterface* ViewModelCache = ResolveObjectCache(const_cast<UObject*>(CacheContextObject), WorldContextObject))
		{
			ViewModelCache = RedirectCache(WorldContextObject, ViewModelCache, ViewModelClass, ViewModelSettings);
			if (ensureAlwaysMsgf(ViewModelCache != nullptr, TEXT("Failed to redirect View Model Cache for context [%s] with View Model Class [%s]"), *GetNameSafe(CacheContextObject), *GetNameSafe(ViewModelClass)))
			{
				return ViewModelCache->GetOrCreateViewModel(WorldContextObject, ViewModelName, ViewModelClass, ViewModelSettings);
			}
		}
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelProvider_Cached::FindCachedViewModel_Internal(const UObject* WorldContextObject, const UObject* CacheContextObject, const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass) const
{
	if (IsValid(CacheContextObject))
	{
		if (const IMDViewModelCacheInterface* ViewModelCache = ResolveObjectCache(CacheContextObject, WorldContextObject))
		{
			ViewModelCache = RedirectCache(WorldContextObject, const_cast<IMDViewModelCacheInterface*>(ViewModelCache), ViewModelClass, {});
			if (ensureAlwaysMsgf(ViewModelCache != nullptr, TEXT("Failed to redirect View Model Cache for context [%s] with View Model Class [%s]"), *GetNameSafe(CacheContextObject), *GetNameSafe(ViewModelClass)))
			{
				return ViewModelCache->GetViewModel(ViewModelName, ViewModelClass);
			}
		}
	}

	return nullptr;
}

void UMDViewModelProvider_Cached::BindOnObjectDestroy(IMDViewModelRuntimeInterface& Object)
{
	if (!Object.OnBeginDestroy.IsBoundToObject(this))
	{
		Object.OnBeginDestroy.AddUObject(this, &UMDViewModelProvider_Cached::OnObjectDestroy, Object.MakeWeak());
	}
}

void UMDViewModelProvider_Cached::OnObjectDestroy(TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr)
{
	for (auto It = ObjectDelegateHandles.CreateIterator(); It; ++It)
	{
		if (!It.Key().ObjectPtr.IsValid() || It.Key().ObjectPtr == ObjectPtr)
		{
			It.RemoveCurrent();
		}
	}

	// Clean up BoundCaches of all ObjectPtr instances and empty inner maps
	{
		if (TMap<FMDViewModelAssignmentReference, TWeakInterfacePtr<IMDViewModelCacheInterface>>* AssignmentToCacheMap = BoundAssignments.Find(ObjectPtr))
		{
			for (const auto& Pair : *AssignmentToCacheMap)
			{
				if (auto* CacheBoundAssignments = BoundCaches.Find(Pair.Value))
				{
					CacheBoundAssignments->Remove(ObjectPtr);

					if (CacheBoundAssignments->IsEmpty())
					{
						BoundCaches.Remove(Pair.Value);
					}
				}
			}

			BoundAssignments.Remove(ObjectPtr);
		}
	}
}

void UMDViewModelProvider_Cached::RefreshViewModel(TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	if (IMDViewModelRuntimeInterface* Object = ObjectPtr.Get())
	{
		SetViewModel(*Object, Assignment, Data);
	}
}

UMDViewModelBase* UMDViewModelProvider_Cached::SetViewModelFromCache(const UObject* WorldContextObject, IMDViewModelCacheInterface* CacheInterface, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	if (ensure(Settings != nullptr) && CacheInterface != nullptr)
	{
		const FName& ViewModelKey = Settings->bOverrideCachedViewModelKey ? Settings->CachedViewModelKeyOverride : Assignment.ViewModelName;
		UMDViewModelBase* ViewModelInstance = CacheInterface->GetOrCreateViewModel(WorldContextObject, ViewModelKey, Assignment.ViewModelClass, Data.ViewModelSettings);
		if (IsValid(ViewModelInstance))
		{
			Object.SetViewModel(ViewModelInstance, FMDViewModelAssignmentReference(Assignment));
		}
		else
		{
			Object.ClearViewModel(FMDViewModelAssignmentReference(Assignment));
		}

		if (!CacheInterface->OnShuttingDown.IsBoundToObject(this))
		{
			TWeakInterfacePtr<IMDViewModelCacheInterface> WeakCachePtr = CacheInterface;
			CacheInterface->OnShuttingDown.AddUObject(this, &UMDViewModelProvider_Cached::OnViewModelCacheShuttingDown, WeakCachePtr);
		}

		auto& AssignmentToCacheMap = BoundAssignments.FindOrAdd(&Object);
		const FMDViewModelAssignmentReference AssignmentRef = FMDViewModelAssignmentReference(Assignment);
		const TWeakInterfacePtr<IMDViewModelCacheInterface> PreviousCachePtr = AssignmentToCacheMap.FindRef(AssignmentRef);
		if (PreviousCachePtr != CacheInterface)
		{
			// If CacheInterface is different than the last time this assignment was set, we need to clear the previous BoundCaches mapping so it doesn't unintentionally clear the new mapping
			if (PreviousCachePtr.IsValid())
			{
				if (auto* PreviousCacheMap = BoundCaches.Find(PreviousCachePtr))
				{
					PreviousCacheMap->Remove(&Object, AssignmentRef);
				}
			}

			AssignmentToCacheMap.Add(AssignmentRef, CacheInterface);
			BoundCaches.FindOrAdd(CacheInterface).Add(&Object, AssignmentRef);
		}

		return ViewModelInstance;
	}
	else
	{
		Object.ClearViewModel(FMDViewModelAssignmentReference(Assignment));
	}

	return nullptr;
}

void UMDViewModelProvider_Cached::OnGameStateChanged(AGameStateBase* GameState, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment,
                                                     FMDViewModelAssignmentData Data)
{
	RefreshViewModel(ObjectPtr, Assignment, Data);
}

void UMDViewModelProvider_Cached::OnViewTargetChanged(APlayerController* PC, AActor* OldViewTarget, AActor* NewViewTarget, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	const IMDViewModelRuntimeInterface* Object = ObjectPtr.Get();
	// View Target binding is only on owning player so we can check that directly here
	if (Object != nullptr && Object->ResolveOwningPlayer() == PC)
	{
		RefreshViewModel(ObjectPtr, Assignment, Data);
	}
}

void UMDViewModelProvider_Cached::OnRelativeViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel,
	TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	RefreshViewModel(ObjectPtr, Assignment, Data);
}

void UMDViewModelProvider_Cached::OnFieldValueChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment,
	FMDViewModelAssignmentData Data)
{
	RefreshViewModel(ObjectPtr, Assignment, Data);
}

void UMDViewModelProvider_Cached::OnActorSpawned(AActor* Actor, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	// Don't go through refresh view model since we have the Actor we need already so we can avoid the expensive TActorIterator

	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	IMDViewModelRuntimeInterface* Object = ObjectPtr.Get();
	if (IsValid(Actor) && Object != nullptr)
	{
		if (DoesActorPassFilter(Actor, Settings->WorldActorFilter))
		{
			IMDViewModelCacheInterface* CacheInterface = ResolveWorldActorCacheAndBindDelegates(Actor, *Object, Assignment, Data);
			SetViewModelFromCache(Actor, CacheInterface, *Object, Assignment, Data);
		}
	}
}

void UMDViewModelProvider_Cached::OnActorRemoved(AActor* Actor, TWeakObjectPtr<AActor> BoundActor, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	if (BoundActor.Get() == Actor)
	{
		RefreshViewModel(ObjectPtr, Assignment, Data);
	}
}

void UMDViewModelProvider_Cached::OnLevelAddedToWorld(ULevel* Level, UWorld* World, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	// Don't go through refresh view model since we have the Level with the list of actors it's adding so we can avoid the expensive TActorIterator

	const FMDViewModelProvider_Cached_Settings* Settings = Data.ProviderSettings.GetPtr<FMDViewModelProvider_Cached_Settings>();
	IMDViewModelRuntimeInterface* Object = ObjectPtr.Get();
	if (Object != nullptr && Settings != nullptr && IsValid(World) && IsValid(Level) && Object->ResolveWorld() == World)
	{
		for (AActor* Actor : Level->Actors)
		{
			if (IsValid(Actor) && DoesActorPassFilter(Actor, Settings->WorldActorFilter))
			{
				IMDViewModelCacheInterface* CacheInterface = ResolveWorldActorCacheAndBindDelegates(Actor, *Object, Assignment, Data);
				SetViewModelFromCache(Actor, CacheInterface, *Object, Assignment, Data);

				break;
			}
		}
	}
}

void UMDViewModelProvider_Cached::OnLevelRemovedFromWorld(ULevel* Level, UWorld* World, TWeakObjectPtr<AActor> BoundActor, TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr, FMDViewModelAssignment Assignment, FMDViewModelAssignmentData Data)
{
	const IMDViewModelRuntimeInterface* Object = ObjectPtr.Get();
	if (Object != nullptr && Object->ResolveWorld() == World && IsValid(Level) && Level->Actors.Contains(BoundActor.Get()))
	{
		RefreshViewModel(ObjectPtr, Assignment, Data);
	}
}

void UMDViewModelProvider_Cached::OnViewModelCacheShuttingDown(TWeakInterfacePtr<IMDViewModelCacheInterface> CachePtr)
{
	const TMultiMap<TWeakInterfacePtr<IMDViewModelRuntimeInterface>, FMDViewModelAssignmentReference>* CacheAssignmentsPtr = BoundCaches.Find(CachePtr);
	if (CacheAssignmentsPtr != nullptr)
	{
		for (auto It = CacheAssignmentsPtr->CreateConstIterator(); It; ++It)
		{
			const TWeakInterfacePtr<IMDViewModelRuntimeInterface>& ObjectPtr = It.Key();
			const FMDViewModelAssignmentReference& Assignment = It.Value();
			if (IMDViewModelRuntimeInterface* Object = ObjectPtr.Get())
			{
				TMap<FMDViewModelAssignmentReference, TWeakInterfacePtr<IMDViewModelCacheInterface>>& AssignmentToCacheMap = BoundAssignments.FindChecked(ObjectPtr);
				TWeakInterfacePtr<IMDViewModelCacheInterface> RemovedCachePtr = AssignmentToCacheMap.FindAndRemoveChecked(Assignment);
				Object->ClearViewModel(FMDViewModelAssignmentReference(Assignment));

				if (RemovedCachePtr != CachePtr)
				{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
					UE_LOGFMT(LogMDViewModel, Error, "BoundCaches had [{ObjectName}]'s assignment [{Assignment}] bound to [{BoundCacheName}] but BoundAssignments had [{BoundAssignmentName}]",
						("ObjectName", GetNameSafe(Cast<UObject>(Object))),
						("Assignment", Assignment),
						("BoundCacheName", GetNameSafe(Cast<UObject>(CachePtr.Get()))),
						("BoundAssignmentName", GetNameSafe(Cast<UObject>(RemovedCachePtr.Get())))
					);
#else
					UE_LOG(LogMDViewModel, Error, TEXT("BoundCaches had [%s]'s assignment [%s (%s)] bound to [%s] but BoundAssignments had [%s]"),
						*GetNameSafe(Cast<UObject>(Object)),
						*GetNameSafe(Assignment.ViewModelClass.Get()),
						*Assignment.ViewModelName.ToString(),
						*GetNameSafe(Cast<UObject>(CachePtr.Get())),
						*GetNameSafe(Cast<UObject>(RemovedCachePtr.Get()))
					);
#endif
				}
			}
		}

		BoundCaches.Remove(CachePtr);
	}
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::RedirectCache(const UObject* WorldContextObject, IMDViewModelCacheInterface* Cache, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings) const
{
	if (!IsValid(ViewModelClass) || Cache == nullptr)
	{
		return Cache;
	}

	UObject* CacheContextObject = Cache->GetViewModelOwner();
	UObject* RedirectedCacheContextObject = ViewModelClass.GetDefaultObject()->CDORedirectCachedContextObject(WorldContextObject, CacheContextObject, ViewModelSettings);
	if (!IsValid(RedirectedCacheContextObject) || RedirectedCacheContextObject == CacheContextObject)
	{
		return Cache;
	}

	return ResolveObjectCache(RedirectedCacheContextObject, WorldContextObject);
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
	// Use the object VM cache system for template objects since we can't modify them
	if (IsValid(Object) && Object->IsTemplate())
	{
		return UMDObjectViewModelCacheSystem::ResolveCacheForObject(Object, WorldContextObject);
	}

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
		// Actor templates can't have caches added to them, so fallback to an Object-style cache if the actor doesn't have a cache
		if (Actor->IsTemplate())
		{
			return UMDObjectViewModelCacheSystem::ResolveCacheForObject(Object, WorldContextObject);
		}
		else
		{
			return ResolveActorCache(Actor);
		}
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
                                                                                         IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPCUpdatePollingComponent* Poller = IsValid(PlayerController)
		? UMDVMPCUpdatePollingComponent::FindOrAddPollingComponent(PlayerController)
		: nullptr;

	FMDVMAssignmentObjectKey BindingKey = { Assignment, Object.GetOwningObject() };
	UnbindDelegateIfNewOwner<UMDVMPCUpdatePollingComponent>(BindingKey, Poller, MDVMDI_Default, [](auto& OldPoller, FDelegateHandle& Handle)
	{
		OldPoller.UnbindOnHUDChanged(Handle);
	});

	if (IsValid(PlayerController))
	{
		BindDelegateIfUnbound<UMDVMPCUpdatePollingComponent>(MoveTemp(BindingKey), Poller, MDVMDI_Default, [&](auto& Owner)
		{
			FSimpleDelegate Delegate = FSimpleDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, Object.MakeWeak(), Assignment, Data);
			return Owner.BindOnHUDChanged(MoveTemp(Delegate));
		});

		return ResolveActorCache(PlayerController->GetHUD());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolvePawnCacheAndBindDelegates(APlayerController* PlayerController,
	IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPCDynamicDelegateIntermediate* Intermediate = IsValid(PlayerController)
		? UMDVMPCDynamicDelegateIntermediate::FindOrAddListener(PlayerController)
		: nullptr;

	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegateIfNewOwner<UMDVMPCDynamicDelegateIntermediate>(BindingKey, Intermediate, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.OnPawnChanged.Remove(Handle);
	});

	if (IsValid(PlayerController))
	{
		BindDelegateIfUnbound<UMDVMPCDynamicDelegateIntermediate>(MoveTemp(BindingKey), Intermediate, MDVMDI_Default, [&](auto& Owner)
		{
			return Owner.OnPawnChanged.AddUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, Object.MakeWeak(), Assignment, Data);
		});

		return ResolveActorCache(PlayerController->GetPawn());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolvePawnCacheAndBindDelegates(APlayerState* PlayerState,
	IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPSDynamicDelegateIntermediate* Intermediate = IsValid(PlayerState)
		? UMDVMPSDynamicDelegateIntermediate::FindOrAddListener(PlayerState)
		: nullptr;

	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegateIfNewOwner<UMDVMPSDynamicDelegateIntermediate>(BindingKey, Intermediate, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.OnPawnChanged.Remove(Handle);
	});

	if (IsValid(PlayerState))
	{
		BindDelegateIfUnbound<UMDVMPSDynamicDelegateIntermediate>(MoveTemp(BindingKey), Intermediate, MDVMDI_Default, [&](auto& Owner)
		{
			return Owner.OnPawnChanged.AddUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, Object.MakeWeak(), Assignment, Data);
		});

		return ResolveActorCache(PlayerState->GetPawn());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolvePlayerStateCacheAndBindDelegates(APlayerController* PlayerController,
	IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPCUpdatePollingComponent* Poller = IsValid(PlayerController)
		? UMDVMPCUpdatePollingComponent::FindOrAddPollingComponent(PlayerController)
		: nullptr;

	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegateIfNewOwner<UMDVMPCUpdatePollingComponent>(BindingKey, Poller, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.UnbindOnPlayerStateChanged(Handle);
	});

	if (IsValid(PlayerController))
	{
		BindDelegateIfUnbound<UMDVMPCUpdatePollingComponent>(MoveTemp(BindingKey), Poller, MDVMDI_Default, [&](auto& Owner)
		{
			FSimpleDelegate Delegate = FSimpleDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, Object.MakeWeak(), Assignment, Data);
			return Owner.BindOnPlayerStateChanged(MoveTemp(Delegate));
		});

		return ResolveActorCache(PlayerController->PlayerState);
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolvePlayerStateCacheAndBindDelegates(APawn* Pawn, IMDViewModelRuntimeInterface& Object,
	const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UMDVMPawnUpdatePollingComponent* Poller = IsValid(Pawn)
		? UMDVMPawnUpdatePollingComponent::FindOrAddPollingComponent(Pawn)
		: nullptr;

	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegateIfNewOwner<UMDVMPawnUpdatePollingComponent>(BindingKey, Poller, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.UnbindOnPlayerStateChanged(Handle);
	});

	if (IsValid(Pawn))
	{
		BindDelegateIfUnbound<UMDVMPawnUpdatePollingComponent>(MoveTemp(BindingKey), Poller, MDVMDI_Default, [&](auto& Owner)
		{
			FSimpleDelegate Delegate = FSimpleDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::RefreshViewModel, Object.MakeWeak(), Assignment, Data);
			return Owner.BindOnPlayerStateChanged(MoveTemp(Delegate));
		});

		return ResolveActorCache(Pawn->GetPlayerState());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveGameStateCacheAndBindDelegates(UWorld* World,
	IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegateIfNewOwner<UWorld>(BindingKey, World, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.GameStateSetEvent.Remove(Handle);
	});

	if (IsValid(World))
	{
		BindDelegateIfUnbound<UWorld>(MoveTemp(BindingKey), World, MDVMDI_Default, [&](auto& Owner)
		{
			return Owner.GameStateSetEvent.AddUObject(this, &UMDViewModelProvider_Cached::OnGameStateChanged, Object.MakeWeak(), Assignment, Data);
		});

		return ResolveActorCache(World->GetGameState());
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveViewTargetCacheAndBindDelegates(const APlayerController* PlayerController,
	IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	BindViewTargetDelegates(Object, Assignment, Data);

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
	IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	BindViewTargetDelegates(Object, Assignment, Data);

	if (IsValid(PlayerController))
	{
		// We don't need to detect CameraManager changes since FGameDelegates::Get().GetViewTargetChangedDelegate() is global
		const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
		if (IsValid(CameraManager))
		{
			return ResolvePlayerStateCacheAndBindDelegates(Cast<APawn>(CameraManager->GetViewTarget()), Object, Assignment, Data);
		}
	}

	return nullptr;
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveRelativeViewModelCacheAndBindDelegates(const FMDViewModelAssignmentReference& Reference,
	IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	const UMDViewModelBase* RelativeViewModel = ResolveViewModelAndBindDelegates(Reference, MDVMDI_Default, Object, Assignment, Data);
	if (!IsValid(RelativeViewModel))
	{
		return nullptr;
	}

	const UObject* WorldContextObject = RelativeViewModel->GetWorld();
	if (!IsValid(WorldContextObject))
	{
		WorldContextObject = Object.ResolveWorld();
	}
	return ResolveObjectCache(RelativeViewModel->GetContextObject(), WorldContextObject);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveRelativePropertyCacheAndBindDelegates(const FMemberReference& Reference, IMDViewModelRuntimeInterface& Object,
	const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	return ResolveFieldCacheAndBindDelegates(Object.GetOwningObject(), Reference, MDVMDI_Default, Object, Assignment, Data);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveRelativeViewModelPropertyCacheAndBindDelegates(const FMDViewModelAssignmentReference& VMReference,
	const FMemberReference& PropertyReference, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	constexpr int32 ViewModelDelegateIndex = MDVMDI_Default;
	UMDViewModelBase* RelativeViewModel = ResolveViewModelAndBindDelegates(VMReference, MDVMDI_Default, Object, Assignment, Data);

	if (!IsValid(RelativeViewModel))
	{
		return nullptr;
	}

	constexpr int32 PropertyDelegateIndex = ViewModelDelegateIndex + 1;
	return ResolveFieldCacheAndBindDelegates( RelativeViewModel, PropertyReference, PropertyDelegateIndex, Object, Assignment, Data);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveFieldCacheAndBindDelegates(UObject* Owner, const FMemberReference& Reference,
	int32 DelegateIndex, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
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

	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegateIfNewOwner<INotifyFieldValueChanged>(BindingKey, FieldNotifyOwner, DelegateIndex, [&FieldId](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.RemoveFieldValueChangedDelegate(FieldId, Handle);
	});

	BindDelegateIfUnbound<INotifyFieldValueChanged>(MoveTemp(BindingKey), FieldNotifyOwner, DelegateIndex, [&](auto& NewOwner)
	{
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnFieldValueChanged, Object.MakeWeak(), Assignment, Data);
		return NewOwner.AddFieldValueChangedDelegate(FieldId, Delegate);
	});

	UObject* PropertyValue = nullptr;
	if (Property != nullptr)
	{
		PropertyValue = Property->GetObjectPropertyValue_InContainer(Owner);
	}
	else if (Function != nullptr)
	{
		const FObjectPropertyBase* ReturnProp = CastField<FObjectPropertyBase>(MDViewModelUtils::GetFunctionReturnProperty(Function));
		checkf(ReturnProp != nullptr && Function->NumParms == 1 && Function->ParmsSize == sizeof(PropertyValue),
			TEXT("Function [%s] on [%s] is no longer a valid RelativeProperty function, update the view model assignment to fix this"), *Function->GetName(), *Owner->GetName());
		Owner->ProcessEvent(Function, &PropertyValue);
	}

	const UObject* WorldContextObject = Owner->GetWorld();
	if (!IsValid(WorldContextObject))
	{
		WorldContextObject = Object.ResolveWorld();
	}
	return ResolveObjectCache(PropertyValue, WorldContextObject);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveWorldActorCacheAndBindDelegates(const FMDVMWorldActorFilter& Filter, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	AActor* FoundActor = nullptr;
	if (const UWorld* World = Object.ResolveWorld())
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

	return ResolveWorldActorCacheAndBindDelegates(FoundActor, Object, Assignment, Data);
}

IMDViewModelCacheInterface* UMDViewModelProvider_Cached::ResolveWorldActorCacheAndBindDelegates(AActor* Actor, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	UWorld* World = Object.ResolveWorld();

	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegateIfNewOwner<UWorld>(BindingKey, World, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.RemoveOnActorSpawnedHandler(Handle);
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
		OldOwner.RemoveOnActorRemovedFromWorldHandler(Handle);
#else
		OldOwner.RemoveOnActorDestroyededHandler(Handle);
#endif
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
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			using DelegateType = FOnActorRemovedFromWorld;
#else
			using DelegateType = FOnActorDestroyed;
#endif
			auto Delegate =	DelegateType::FDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnActorRemoved, MakeWeakObjectPtr(Actor), Object.MakeWeak(), Assignment, Data);

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			return Owner.AddOnActorRemovedFromWorldHandler(MoveTemp(Delegate));
#else
			return Owner.AddOnActorDestroyedHandler(MoveTemp(Delegate));
#endif
		});

		BindLevelRemovedFromWorldDelegates(MakeWeakObjectPtr(Actor), Object, Assignment, Data);

		return ResolveActorCache(Actor);
	}
	else
	{
		UnbindDelegate<UWorld>(BindingKey, MDVMDI_Default, [](auto& OldOwner, FDelegateHandle& Handle)
		{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			OldOwner.RemoveOnActorRemovedFromWorldHandler(Handle);
#else
			OldOwner.RemoveOnActorDestroyededHandler(Handle);
#endif
		});

		BindDelegateIfUnbound<UWorld>(MoveTemp(BindingKey), World, MDVMDI_Default, [&](auto& Owner)
		{
			auto Delegate = FOnActorSpawned::FDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnActorSpawned, Object.MakeWeak(), Assignment, Data);
			return Owner.AddOnActorSpawnedHandler(MoveTemp(Delegate));
		});

		BindLevelAddedToWorldDelegates(Object, Assignment, Data);

		return nullptr;
	}
}

UMDViewModelBase* UMDViewModelProvider_Cached::ResolveViewModelAndBindDelegates(const FMDViewModelAssignmentReference& Reference, int32 DelegateIndex, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegateIfNewOwner<IMDViewModelRuntimeInterface>(BindingKey, &Object, DelegateIndex, [Reference](auto& OldOwner, FDelegateHandle& Handle)
	{
		OldOwner.StopListeningForChanges(Handle, Reference);
	});

	BindDelegateIfUnbound<IMDViewModelRuntimeInterface>(MoveTemp(BindingKey), &Object, DelegateIndex, [&](auto& Owner)
	{
		auto Delegate = FMDVMOnViewModelSet::FDelegate::CreateUObject(this, &UMDViewModelProvider_Cached::OnRelativeViewModelChanged, Object.MakeWeak(), Assignment, Data);
		return Owner.ListenForChanges(MoveTemp(Delegate), Reference);
	});

	return Object.GetViewModel(Reference);
}

void UMDViewModelProvider_Cached::BindViewTargetDelegates(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	BindDelegateIfUnbound<IMDViewModelRuntimeInterface>(MoveTemp(BindingKey), &Object, MDVMDI_ViewTarget, [&](auto& Owner)
	{
		return FGameDelegates::Get().GetViewTargetChangedDelegate().AddUObject(this, &UMDViewModelProvider_Cached::OnViewTargetChanged, Object.MakeWeak(), Assignment, Data);
	});
}

void UMDViewModelProvider_Cached::BindLevelAddedToWorldDelegates(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegate<UObject>(BindingKey, MDVMDI_LevelWorldChanged, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		FWorldDelegates::LevelRemovedFromWorld.Remove(Handle);
	});
	
	BindDelegateIfUnbound<IMDViewModelRuntimeInterface>(MoveTemp(BindingKey), &Object, MDVMDI_LevelWorldChanged, [&](auto& Owner)
	{
		return FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UMDViewModelProvider_Cached::OnLevelAddedToWorld, Object.MakeWeak(), Assignment, Data);
	});
}

void UMDViewModelProvider_Cached::BindLevelRemovedFromWorldDelegates(TWeakObjectPtr<AActor> BoundActor, IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	FMDVMAssignmentObjectKey BindingKey = { Assignment, &Object };
	UnbindDelegate<UObject>(BindingKey, MDVMDI_LevelWorldChanged, [](auto& OldOwner, FDelegateHandle& Handle)
	{
		FWorldDelegates::LevelAddedToWorld.Remove(Handle);
	});
	
	BindDelegateIfUnbound<IMDViewModelRuntimeInterface>(MoveTemp(BindingKey), &Object, MDVMDI_LevelWorldChanged, [&](auto& Owner)
	{
		return FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &UMDViewModelProvider_Cached::OnLevelRemovedFromWorld, BoundActor, Object.MakeWeak(), Assignment, Data);
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
