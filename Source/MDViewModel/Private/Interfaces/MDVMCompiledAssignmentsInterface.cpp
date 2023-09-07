#include "Interfaces/MDVMCompiledAssignmentsInterface.h"

#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "ViewModel/MDViewModelBase.h"

bool IMDVMCompiledAssignmentsInterface::HasAssignments() const
{
	return !GetAssignments().IsEmpty();
}

void IMDVMCompiledAssignmentsInterface::SearchAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	OutViewModelAssignments = GetAssignments();

	if (ProviderTag.IsValid() || ViewModelName != NAME_None || ViewModelClass != nullptr)
	{
		// Remove assignments that don't match the filter
		for (auto It = OutViewModelAssignments.CreateIterator(); It; ++It)
		{
			if (ProviderTag.IsValid() && !ProviderTag.MatchesTagExact(It.Key().ProviderTag))
			{
				It.RemoveCurrent();
				continue;
			}

			if (ViewModelName != NAME_None && ViewModelName != It.Key().ViewModelName)
			{
				It.RemoveCurrent();
				continue;
			}

			if (ViewModelClass != nullptr && ViewModelClass != It.Key().ViewModelClass)
			{
				It.RemoveCurrent();
				continue;
			}
		}
	}
}
