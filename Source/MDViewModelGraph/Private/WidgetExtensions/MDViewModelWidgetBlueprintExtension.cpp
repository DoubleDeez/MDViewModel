#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"


void UMDViewModelWidgetBlueprintExtension::AddAssignment(FMDViewModelEditorAssignment&& Assignment)
{
	if (!Assignments.Contains(Assignment))
	{
		Assignments.Emplace(MoveTemp(Assignment));
		OnAssignmentsChanged.Broadcast();

		FBlueprintEditorUtils::MarkBlueprintAsModified(GetWidgetBlueprint());
	}
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
		NewAssignment.Data.ViewModelSettings = UpdatedAssignment.Data.ViewModelSettings;

		Assignments.Insert(NewAssignment, AssignmentIndex);

		OnAssignmentsChanged.Broadcast();

		FBlueprintEditorUtils::MarkBlueprintAsModified(GetWidgetBlueprint());
	}
}

void UMDViewModelWidgetBlueprintExtension::RemoveAssignment(const FMDViewModelEditorAssignment& Assignment)
{
	if (Assignments.Remove(Assignment) > 0)
	{
		OnAssignmentsChanged.Broadcast();
		
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetWidgetBlueprint());
	}
}

bool UMDViewModelWidgetBlueprintExtension::DoesContainViewModelAssignment(TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	const bool bHasBPAssignment = Assignments.ContainsByPredicate([&](const FMDViewModelEditorAssignment& Assignment)
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

	if (UWidgetBlueprintGeneratedClass* SuperClass = Cast<UWidgetBlueprintGeneratedClass>(GetWidgetBlueprint()->GeneratedClass->GetSuperClass()))
	{
		if (UMDViewModelWidgetClassExtension* SuperExtension = SuperClass->GetExtension<UMDViewModelWidgetClassExtension>())
		{
			TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
			SuperExtension->SearchAssignments(ViewModelAssignments, ViewModelClass, ProviderTag, ViewModelName);

			return !ViewModelAssignments.IsEmpty();
		}
	}

	return false;
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
