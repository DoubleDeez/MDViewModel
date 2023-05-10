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
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(UserWidget);
	ensure(Extension);

	if (TArray<QueuedListenerData>* QueuedListenerDatas = QueuedDelegates.Find(UserWidget))
	{
		for (QueuedListenerData& Data : *QueuedListenerDatas)
		{
			Extension->ListenForChanges(MoveTemp(Data.Delegate), Data.ViewModelClass, Data.ViewModelName);
		}

		QueuedDelegates.Remove(UserWidget);
	}
}

#if WITH_EDITOR
void UMDViewModelWidgetClassExtension::Construct(UUserWidget* UserWidget)
{
	Super::Construct(UserWidget);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	// Initialize isn't called in editor Debug mode so we force it here
	if (IsValid(UserWidget) && UserWidget->IsPreviewTime())
	{
		Initialize(UserWidget);
	}
#endif
}
#endif

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

void UMDViewModelWidgetClassExtension::QueueListenForChanges(UUserWidget* Widget, FMDVMOnViewModelAssigned::FDelegate&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(Widget))
	{
		if (UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>())
		{
			Extension->ListenForChanges(MoveTemp(Delegate), ViewModelClass, ViewModelName);
		}
		else
		{
			QueuedDelegates.FindOrAdd(Widget).Emplace(QueuedListenerData{ MoveTemp(Delegate), ViewModelClass, ViewModelName });
		}
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
		if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
		{
			Extension->GatherParentAssignments(SuperClass);
			Assignments.Append(Extension->Assignments);
		}
	}

	bHasGatheredParentAssignments = true;
}
