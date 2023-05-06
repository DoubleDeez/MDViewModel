#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"

#include "Util/MDViewModelEditorAssignment.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"


void UMDViewModelWidgetBlueprintExtension::AddAssignment(FMDViewModelEditorAssignment&& Assignment)
{
	// TODO - validate that assignment doesn't already exist

	Assignments.Emplace(MoveTemp(Assignment));
	MarkPackageDirty();
	OnAssignmentsChanged.Broadcast();
}

void UMDViewModelWidgetBlueprintExtension::UpdateAssignment(const FMDViewModelEditorAssignment& Assignment, FMDViewModelEditorAssignment&& UpdatedAssignment)
{
	const int32 AssignmentIndex = Assignments.IndexOfByKey(Assignment);
	if (AssignmentIndex != INDEX_NONE)
	{
		FMDViewModelEditorAssignment NewAssignment = Assignments[AssignmentIndex];
		Assignments.RemoveAt(AssignmentIndex);

		// Only copy the properties that are allowed to change
		NewAssignment.Assignment.ViewModelName = UpdatedAssignment.Assignment.ViewModelName;
		NewAssignment.Data.ProviderSettings = UpdatedAssignment.Data.ProviderSettings;

		Assignments.Insert(NewAssignment, AssignmentIndex);

		MarkPackageDirty();
		OnAssignmentsChanged.Broadcast();
	}
}

void UMDViewModelWidgetBlueprintExtension::RemoveAssignment(const FMDViewModelEditorAssignment& Assignment)
{
	if (Assignments.Remove(Assignment) > 0)
	{
		MarkPackageDirty();
		OnAssignmentsChanged.Broadcast();
	}
}

void UMDViewModelWidgetBlueprintExtension::HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext)
{
	Super::HandleBeginCompilation(InCreationContext);

	CompilerContext = &InCreationContext;
}

void UMDViewModelWidgetBlueprintExtension::HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class)
{
	Super::HandleFinishCompilingClass(Class);

	if (CompilerContext != nullptr && !Assignments.IsEmpty())
	{
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> CompiledAssignments;
		CompiledAssignments.Reserve(Assignments.Num());
		for (const FMDViewModelEditorAssignment& Assignment : Assignments)
		{
			CompiledAssignments.Add(Assignment.Assignment, Assignment.Data);
		}

		UMDViewModelWidgetClassExtension* ClassExtension = NewObject<UMDViewModelWidgetClassExtension>(Class);
		ClassExtension->SetAssignments(CompiledAssignments);

		CompilerContext->AddExtension(Class, ClassExtension);
	}
}

bool UMDViewModelWidgetBlueprintExtension::HandleValidateGeneratedClass(UWidgetBlueprintGeneratedClass* Class)
{
	return Super::HandleValidateGeneratedClass(Class);
}

void UMDViewModelWidgetBlueprintExtension::HandleEndCompilation()
{
	Super::HandleEndCompilation();

	CompilerContext = nullptr;
}
