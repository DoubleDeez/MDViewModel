#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"

void UMDViewModelWidgetClassExtension::Initialize(UUserWidget* UserWidget)
{
	Super::Initialize(UserWidget);

	GatherParentAssignments(UserWidget->GetClass());

	// ensure that we add the extension to the widget
	const UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(UserWidget);
	ensure(Extension);
}

void UMDViewModelWidgetClassExtension::SetAssignments(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& InAssignments)
{
	Assignments = InAssignments;
}

void UMDViewModelWidgetClassExtension::SearchAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	for (const auto& Pair : Assignments)
	{
		if (ProviderTag.IsValid() && !ProviderTag.MatchesTagExact(Pair.Key.ProviderTag))
		{
			continue;
		}

		if (ViewModelName != NAME_None && ViewModelName != Pair.Key.ViewModelName)
		{
			continue;
		}

		if (ViewModelClass != nullptr && ViewModelClass != Pair.Key.ViewModelClass)
		{
			continue;
		}

		OutViewModelAssignments.Add(Pair.Key, Pair.Value);
	}
}

void UMDViewModelWidgetClassExtension::GetViewModelClasses(TSet<TSubclassOf<UMDViewModelBase>>& OutViewModelClasses) const
{
	for (const auto& Pair : Assignments)
	{
		OutViewModelClasses.Add(Pair.Key.ViewModelClass);
	}
}

void UMDViewModelWidgetClassExtension::GatherParentAssignments(TSubclassOf<UUserWidget> WidgetClass)
{
	if (bHasGatheredParentAssignments || WidgetClass == nullptr)
	{
		return;
	}

	UClass* SuperClass = WidgetClass->GetSuperClass();
	if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(SuperClass))
	{
		if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
		{
			// TODO - How to deal with conflicts when a child class adds design-time data to a super class's assignment
			Extension->GatherParentAssignments(SuperClass);
			Assignments.Append(Extension->Assignments);
		}
	}

	bHasGatheredParentAssignments = true;
}
