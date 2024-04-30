#include "ViewModelProviders/MDViewModelProvider_Manual.h"

#include "Interfaces/MDViewModelRuntimeInterface.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Manual, "MDVM.Provider.Manual");

UMDViewModelBase* UMDViewModelProvider_Manual::SetViewModel(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	// Get the existing view model if there is one, since it would have been manually set
	return Object.GetViewModel(FMDViewModelAssignmentReference(Assignment));
}
