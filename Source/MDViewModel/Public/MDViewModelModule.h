#pragma once

#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Modules/ModuleManager.h"
#include "Templates/SubclassOf.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"

class UMDViewModelProviderBase;
class UGameInstance;
class UUserWidget;
class UMDViewModelBase;

class MDVIEWMODEL_API FMDViewModelModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void RegisterNativeAssignment(TSubclassOf<UUserWidget> WidgetClass, FMDViewModelAssignment&& Assignment, FMDViewModelAssignmentData&& Data);
	void UnregisterNativeAssignment(TSubclassOf<UUserWidget> WidgetClass);

	void GetNativeAssignments(TSubclassOf<UUserWidget> WidgetClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutAssignments) const;

	void GetViewModelClassesForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, TSet<TSubclassOf<UMDViewModelBase>>& OutViewModelClasses) const;
	void GetViewModelAssignmentsForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const;
	void SearchViewModelAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UUserWidget> WidgetClass, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None) const;

	bool DoesClassOrSuperClassHaveAssignments(TSubclassOf<UUserWidget> WidgetClass) const;

	mutable FSimpleMulticastDelegate OnNativeAssignmentsRequested;
	mutable bool bHasRequestedNativeAssignments = false;

private:
	void RequestNativeAssignments() const;

	TMap<TSubclassOf<UUserWidget>, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>> NativelyAssignedViewModels;
};
