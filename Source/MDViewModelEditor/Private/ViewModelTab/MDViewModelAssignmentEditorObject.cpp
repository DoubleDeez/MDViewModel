#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"

#include "ViewModel/MDViewModelBase.h"


void UMDViewModelAssignmentEditorObject::PopulateFromAssignment(const FMDViewModelEditorAssignment& Assignment)
{
	ViewModelClass = Assignment.Assignment.ViewModelClass;
	ViewModelProvider = Assignment.Assignment.ProviderTag;
	ViewModelInstanceName = Assignment.Assignment.ViewModelName;

	bOverrideName = (ViewModelInstanceName != MDViewModelUtils::DefaultViewModelName);

	// TODO - Handle cases where ProviderSettings/ViewModelSettings no longer match what the Provider/ViewModel ask for

	ProviderSettings = Assignment.Data.ProviderSettings;

	if (Assignment.Data.ViewModelSettings.IsValid())
	{
		ViewModelSettings = Assignment.Data.ViewModelSettings;
	}
	else if (const UMDViewModelBase* ViewModelCDO = ViewModelClass->GetDefaultObject<UMDViewModelBase>())
	{
		ViewModelSettings.InitializeAs(ViewModelCDO->GetViewModelSettingsStruct());
	}
}

FMDViewModelEditorAssignment UMDViewModelAssignmentEditorObject::CreateAssignment() const
{
	FMDViewModelEditorAssignment Assignment;

	Assignment.Assignment.ViewModelClass = ViewModelClass;
	Assignment.Assignment.ProviderTag = ViewModelProvider;
	Assignment.Assignment.ViewModelName = bOverrideName ? ViewModelInstanceName : MDViewModelUtils::DefaultViewModelName;

	Assignment.Data.ProviderSettings = ProviderSettings;
	Assignment.Data.ViewModelSettings = ViewModelSettings;

	return Assignment;
}
