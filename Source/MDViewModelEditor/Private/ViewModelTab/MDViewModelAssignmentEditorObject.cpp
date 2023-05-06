#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"


void UMDViewModelAssignmentEditorObject::PopulateFromAssignment(const FMDViewModelEditorAssignment& Assignment)
{
	ViewModelClass = Assignment.Assignment.ViewModelClass;
	ViewModelProvider = Assignment.Assignment.ProviderTag;
	ViewModelInstanceName = Assignment.Assignment.ViewModelName;

	bOverrideName = (ViewModelInstanceName != MDViewModelUtils::DefaultViewModelName);

	ProviderSettings = Assignment.Data.ProviderSettings;
}

FMDViewModelEditorAssignment UMDViewModelAssignmentEditorObject::CreateAssignment() const
{
	FMDViewModelEditorAssignment Assignment;

	Assignment.Assignment.ViewModelClass = ViewModelClass;
	Assignment.Assignment.ProviderTag = ViewModelProvider;
	Assignment.Assignment.ViewModelName = bOverrideName ? ViewModelInstanceName : MDViewModelUtils::DefaultViewModelName;

	Assignment.Data.ProviderSettings = ProviderSettings;

	return Assignment;
}
