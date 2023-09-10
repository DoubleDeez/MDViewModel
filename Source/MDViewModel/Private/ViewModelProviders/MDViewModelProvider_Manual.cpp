#include "ViewModelProviders/MDViewModelProvider_Manual.h"


UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Manual, "MDVM.Provider.Manual");

UMDViewModelBase* UMDViewModelProvider_Manual::SetViewModel(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	// Intentional no-op, external systems are expected to do the assignment
	return nullptr;
}
