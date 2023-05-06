#pragma once

#include "CoreMinimal.h"
#include "MDViewModelModule.h"
#include "MDViewModelAssignmentData.h"
#include "Util/MDViewModelUtils.h"
#include "ViewModelProviders/MDViewModelProvider_Manual.h"
#include "ViewModelProviders/MDViewModelProvider_Unique.h"

class UUserWidget;

// Usage Example
// MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_WITH_PROVIDER(UMyWidget, UMyViewModel, MDViewModelUtils::DefaultViewModelName, TAG_MDVMProvider_Unique);

// Assign a named viewmodel using a specific provider and settings to a widget class
#define MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_WITH_PROVIDER_AND_DATA(WIDGET_CLASS, VM_CLASS, VM_NAME, PROVIDER_TAG, DATA) TSharedPtr<TMDViewModelNativeAssignment<WIDGET_CLASS, VM_CLASS>> MDVMNativeAssignment_##WIDGET_CLASS = MakeShared<TMDViewModelNativeAssignment<WIDGET_CLASS, VM_CLASS>>(PROVIDER_TAG, VM_NAME, DATA);

// Assign a default-named viewmodel using a specific provider and settings
#define MDVM_ASSIGN_NATIVE_VIEWMODEL_WITH_PROVIDER_AND_DATA(WIDGET_CLASS, VM_CLASS, PROVIDER_TAG, DATA) MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_WITH_PROVIDER_AND_DATA(WIDGET_CLASS, VM_CLASS, MDViewModelUtils::DefaultViewModelName, PROVIDER_TAG, DATA)

// Assign a named viewmodel using a specific provider to a widget class
#define MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_WITH_PROVIDER(WIDGET_CLASS, VM_CLASS, VM_NAME, PROVIDER_TAG) MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_WITH_PROVIDER_AND_DATA(WIDGET_CLASS, VM_CLASS, VM_NAME, PROVIDER_TAG, FMDViewModelAssignmentData())

// Assign a default-named viewmodel using a specific provider
#define MDVM_ASSIGN_NATIVE_VIEWMODEL_WITH_PROVIDER(WIDGET_CLASS, VM_CLASS, PROVIDER_TAG) MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_WITH_PROVIDER(WIDGET_CLASS, VM_CLASS, MDViewModelUtils::DefaultViewModelName, PROVIDER_TAG)

// Assign a named viewmodel that will be manually set later in code or blueprint
#define MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_MANUAL(WIDGET_CLASS, VM_CLASS, VM_NAME) MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_WITH_PROVIDER(WIDGET_CLASS, VM_CLASS, VM_NAME, TAG_MDVMProvider_Manual)

// Assign a named viewmodel that will be automatically created for the widget at initialization, unique per widget instance
#define MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_UNIQUE(WIDGET_CLASS, VM_CLASS, VM_NAME) MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_WITH_PROVIDER(WIDGET_CLASS, VM_CLASS, VM_NAME, TAG_MDVMProvider_Unique)

// Assign a named viewmodel that will be manually set later in code or blueprint
#define MDVM_ASSIGN_NATIVE_VIEWMODEL_MANUAL(WIDGET_CLASS, VM_CLASS) MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_MANUAL(WIDGET_CLASS, VM_CLASS, MDViewModelUtils::DefaultViewModelName)

// Assign a default-named viewmodel that will be automatically created for the widget at initialization, unique per widget instance
#define MDVM_ASSIGN_NATIVE_VIEWMODEL_UNIQUE(WIDGET_CLASS, VM_CLASS) MDVM_ASSIGN_NATIVE_NAMED_VIEWMODEL_UNIQUE(WIDGET_CLASS, VM_CLASS, MDViewModelUtils::DefaultViewModelName)

/**
 * Not to be used directly, use the above macros instead.
 * This class takes a bunch of steps to ensure that we have valid data when registration happens
 */
template<typename TWidget, typename TViewModel>
class TMDViewModelNativeAssignment final : public FNoncopyable
{
public:
	TMDViewModelNativeAssignment(const FNativeGameplayTag& ProviderTag, const FName& VMName, const FMDViewModelAssignmentData& Data);
	~TMDViewModelNativeAssignment();

private:
	void OnNativeAssignmentsRequested();

	TSubclassOf<UUserWidget> WidgetClass;
	const FNativeGameplayTag& ProviderTag;
	FName VMName;
	FMDViewModelAssignmentData Data;
};

template <typename TWidget, typename TViewModel>
TMDViewModelNativeAssignment<TWidget, TViewModel>::TMDViewModelNativeAssignment(const FNativeGameplayTag& ProviderTag, const FName& VMName, const FMDViewModelAssignmentData& Data)
	: ProviderTag(ProviderTag)
	, VMName(VMName)
	, Data(Data)
{
	FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
	check(!ViewModelModule.bHasRequestedNativeAssignments);
	ViewModelModule.OnNativeAssignmentsRequested.AddRaw(this, &TMDViewModelNativeAssignment<TWidget, TViewModel>::OnNativeAssignmentsRequested);
}

template <typename TWidget, typename TViewModel>
TMDViewModelNativeAssignment<TWidget, TViewModel>::~TMDViewModelNativeAssignment()
{
	if (FMDViewModelModule* ViewModelModule = FModuleManager::GetModulePtr<FMDViewModelModule>(TEXT("MDViewModel")))
	{
		ViewModelModule->UnregisterNativeAssignment(WidgetClass);
		ViewModelModule->OnNativeAssignmentsRequested.RemoveAll(this);
	}
}

template <typename TWidget, typename TViewModel>
void TMDViewModelNativeAssignment<TWidget, TViewModel>::OnNativeAssignmentsRequested()
{
	FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));

	FMDViewModelAssignment Assignment;
	Assignment.ProviderTag = ProviderTag;
	Assignment.ViewModelName = VMName;
	Assignment.ViewModelClass = TViewModel::StaticClass();

	ViewModelModule.RegisterNativeAssignment(TWidget::StaticClass(), MoveTemp(Assignment), MoveTemp(Data));
}
