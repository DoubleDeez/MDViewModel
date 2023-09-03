#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"

#include "BlueprintActionDatabase.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Launch/Resources/Version.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"


void UMDViewModelWidgetBlueprintExtension::GetBPAndParentClassAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const
{
	for (const FMDViewModelEditorAssignment& Assignment : Assignments)
	{
		OutViewModelAssignments.Add(Assignment.Assignment, Assignment.Data);
	}

	UClass* SuperClass = GetWidgetBlueprint()->SkeletonGeneratedClass->GetSuperClass();
	if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(SuperClass))
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
		if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
		{
			OutViewModelAssignments.Append(Extension->GetAssignments());
		}
	}
}

void UMDViewModelWidgetBlueprintExtension::AddAssignment(FMDViewModelEditorAssignment&& Assignment)
{
	if (!Assignments.Contains(Assignment))
	{
		Assignments.Emplace(MoveTemp(Assignment));
		OnAssignmentsChanged.Broadcast();

		FBlueprintEditorUtils::MarkBlueprintAsModified(GetWidgetBlueprint());
		FBlueprintActionDatabase::Get().RefreshAssetActions(GetWidgetBlueprint());
	}
}

void UMDViewModelWidgetBlueprintExtension::UpdateAssignment(const FMDViewModelEditorAssignment& Assignment, FMDViewModelEditorAssignment&& UpdatedAssignment)
{
	const int32 AssignmentIndex = Assignments.IndexOfByKey(Assignment);
	if (AssignmentIndex != INDEX_NONE)
	{
		const FMDViewModelEditorAssignment OldAssignment = Assignments[AssignmentIndex];
		Assignments[AssignmentIndex] = UpdatedAssignment;

		if ((OldAssignment.Assignment.ViewModelName != UpdatedAssignment.Assignment.ViewModelName)
			|| (OldAssignment.Assignment.ViewModelClass != UpdatedAssignment.Assignment.ViewModelClass))
		{
			OnAssignmentChanged.Broadcast(OldAssignment.Assignment.ViewModelName, UpdatedAssignment.Assignment.ViewModelName,
				OldAssignment.Assignment.ViewModelClass, UpdatedAssignment.Assignment.ViewModelClass);
		}
		
		OnAssignmentsChanged.Broadcast();

		FBlueprintEditorUtils::MarkBlueprintAsModified(GetWidgetBlueprint());
		FBlueprintActionDatabase::Get().RefreshAssetActions(GetWidgetBlueprint());
	}
}

void UMDViewModelWidgetBlueprintExtension::RemoveAssignment(const FMDViewModelEditorAssignment& Assignment)
{
	if (Assignments.Remove(Assignment) > 0)
	{
		OnAssignmentsChanged.Broadcast();
		
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetWidgetBlueprint());
		FBlueprintActionDatabase::Get().RefreshAssetActions(GetWidgetBlueprint());
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

void UMDViewModelWidgetBlueprintExtension::HandleEndCompilation()
{
	Super::HandleEndCompilation();

	CompilerContext = nullptr;
}
