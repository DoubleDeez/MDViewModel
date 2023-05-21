#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"

#include "MDViewModelModule.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"


void UMDViewModelAssignmentEditorObject::PopulateFromAssignment(const FMDViewModelEditorAssignment& Assignment, UWidgetBlueprint* WidgetBlueprint)
{
	ViewModelClass = Assignment.Assignment.ViewModelClass;
	ViewModelProvider = Assignment.Assignment.ProviderTag;
	ViewModelInstanceName = Assignment.Assignment.ViewModelName;

	bOverrideName = (ViewModelInstanceName != MDViewModelUtils::DefaultViewModelName);

	ProviderSettings = Assignment.Data.ProviderSettings;
	
	if (UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(ViewModelProvider))
	{
		if (ProviderSettings.GetScriptStruct() != Provider->GetProviderSettingsStruct())
		{
			ProviderSettings.InitializeAs(Provider->GetProviderSettingsStruct());
		}
		
		Provider->OnProviderSettingsInitializedInEditor(ProviderSettings, WidgetBlueprint);
	}
	else
	{
		ProviderSettings.Reset();
	}

	const UMDViewModelBase* ViewModelCDO = ViewModelClass->GetDefaultObject<UMDViewModelBase>();
	if (Assignment.Data.ViewModelSettings.IsValid() && Assignment.Data.ViewModelSettings.GetScriptStruct() == ViewModelCDO->GetViewModelSettingsStruct())
	{
		ViewModelSettings = Assignment.Data.ViewModelSettings;
	}
	else
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
