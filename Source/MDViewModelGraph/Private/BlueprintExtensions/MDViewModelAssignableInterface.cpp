#include "BlueprintExtensions/MDViewModelAssignableInterface.h"

#include "BlueprintActionDatabase.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "ViewModel/MDViewModelBase.h"

UBlueprint* IMDViewModelAssignableInterface::GetBlueprint() const
{
	if (const UObject* ThisObject = Cast<UObject>(this))
	{
		return ThisObject->GetTypedOuter<UBlueprint>();
	}
	
	return nullptr;
}

void IMDViewModelAssignableInterface::ModifyObject()
{
	if (UObject* Object = Cast<UObject>(this))
	{
		Object->Modify();
	}
}

void IMDViewModelAssignableInterface::GetAllAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const
{
	for (const FMDViewModelEditorAssignment& Assignment : GetAssignments())
	{
		OutViewModelAssignments.Add(Assignment.Assignment, Assignment.Data);
	}

	SearchParentAssignments(OutViewModelAssignments);
}

void IMDViewModelAssignableInterface::AddAssignment(FMDViewModelEditorAssignment&& Assignment)
{
	if (!GetAssignments().Contains(Assignment))
	{
		GetAssignments().Emplace(MoveTemp(Assignment));
		OnAssignmentsChanged.Broadcast();

		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
		FBlueprintActionDatabase::Get().RefreshAssetActions(GetBlueprint());
	}
}

void IMDViewModelAssignableInterface::UpdateAssignment(const FMDViewModelEditorAssignment& Assignment, FMDViewModelEditorAssignment&& UpdatedAssignment)
{
	const int32 AssignmentIndex = GetAssignments().IndexOfByKey(Assignment);
	if (AssignmentIndex != INDEX_NONE)
	{
		const FMDViewModelEditorAssignment OldAssignment = GetAssignments()[AssignmentIndex];
		GetAssignments()[AssignmentIndex] = UpdatedAssignment;

		if ((OldAssignment.Assignment.ViewModelName != UpdatedAssignment.Assignment.ViewModelName)
			|| (OldAssignment.Assignment.ViewModelClass != UpdatedAssignment.Assignment.ViewModelClass))
		{
			OnAssignmentChanged.Broadcast(OldAssignment.Assignment.ViewModelName, UpdatedAssignment.Assignment.ViewModelName,
				OldAssignment.Assignment.ViewModelClass, UpdatedAssignment.Assignment.ViewModelClass);
		}
		
		OnAssignmentsChanged.Broadcast();

		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
		FBlueprintActionDatabase::Get().RefreshAssetActions(GetBlueprint());
	}
}

void IMDViewModelAssignableInterface::RemoveAssignment(const FMDViewModelEditorAssignment& Assignment)
{
	if (GetAssignments().Remove(Assignment) > 0)
	{
		OnAssignmentsChanged.Broadcast();
		
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
		FBlueprintActionDatabase::Get().RefreshAssetActions(GetBlueprint());
	}
}

bool IMDViewModelAssignableInterface::DoesContainViewModelAssignment(TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	const bool bHasBPAssignment = GetAssignments().ContainsByPredicate([&](const FMDViewModelEditorAssignment& Assignment)
	{
		if (ViewModelClass != nullptr && Assignment.Assignment.ViewModelClass != ViewModelClass)
		{
			return false;
		}

		if (ProviderTag.IsValid() && ProviderTag != Assignment.Assignment.ProviderTag)
		{
			return false;
		}

		if (ViewModelName != NAME_None && ViewModelName != Assignment.Assignment.ViewModelName)
		{
			return false;
		}

		return true;
	});

	if (bHasBPAssignment)
	{
		return true;
	}
	
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
	SearchParentAssignments(ViewModelAssignments, ViewModelClass, ProviderTag, ViewModelName);

	return !ViewModelAssignments.IsEmpty();
}

bool IMDViewModelAssignableInterface::HasAssignments() const
{
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
	GetAllAssignments(ViewModelAssignments);
	return !ViewModelAssignments.IsEmpty();
}
