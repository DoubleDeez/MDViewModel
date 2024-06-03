#pragma once

#include "GameplayTagContainer.h"
#include "Subsystems/EngineSubsystem.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "MDViewModelProviderBase.generated.h"

struct FInstancedStruct;
struct FMDViewModelAssignment;
struct FMDViewModelAssignmentData;
class IMDViewModelRuntimeInterface;
class UGameInstance;
class UMDViewModelBase;

struct FMDViewModelSupportedClass
{
	TSubclassOf<UMDViewModelBase> Class;

	bool bAllowChildClasses = true;
};

/**
 * Abstract base class for creating custom view model providers
 *
 * A View Model Provider sets view models on a passed-in object
 */
UCLASS(Abstract)
class MDVIEWMODEL_API UMDViewModelProviderBase : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// Override this to return false if your provider doesn't perform the actual assignment since your provider will not be able to pass along view model settings
	virtual bool DoesSupportViewModelSettings() const { return true; }

	// Override this to indicate that the user can manually set view model instances for assignments using this provider (using Set View Model, Create And Set View Model, MDVMDynamicEntryBox, etc)
	virtual bool DoesAllowManualSetting() const { return false; }

	// Called a view model instance is manually set for an assignment using this provider
	virtual void OnAssignmentManuallySet() {}

	virtual FGameplayTag GetProviderTag() const PURE_VIRTUAL(UMDViewModelProviderBase::GetProviderTag, return FGameplayTag::EmptyTag;)

	// Set the view model on the object using the specified assignment and data
	virtual UMDViewModelBase* SetViewModel(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)  PURE_VIRTUAL(UMDViewModelProviderBase::SetViewModel, return nullptr;)

#if WITH_EDITOR
	virtual void GetSupportedViewModelClasses(TArray<FMDViewModelSupportedClass>& OutViewModelClasses);
	virtual bool DoesCreateViewModels() const { return true; }
	virtual FText GetDisplayName() const  PURE_VIRTUAL(UMDViewModelProviderBase::GetDisplayName, return FText::GetEmpty();)
	virtual FText GetDescription(const FInstancedStruct& ProviderSettings) const  PURE_VIRTUAL(UMDViewModelProviderBase::GetDescription, return FText::GetEmpty();)

	virtual UScriptStruct* GetProviderSettingsStruct() const { return nullptr; }
	virtual void OnProviderSettingsInitializedInEditor(FInstancedStruct& Settings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment) const {};
	virtual void OnAssignmentUpdated(FInstancedStruct& ProviderSettings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment) const {};
	virtual bool ValidateProviderSettings(const FInstancedStruct& Settings, UBlueprint* Blueprint, const FMDViewModelAssignment& Assignment, TArray<FText>& OutIssues) const { return true; }
	virtual void GetExpectedContextObjectTypes(const FInstancedStruct& ProviderSettings, const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutContextObjectClasses) const {}
#endif

};
