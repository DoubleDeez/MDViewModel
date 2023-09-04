#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Launch/Resources/Version.h"
#include "Logging/StructuredLog.h"
#include "Util/MDViewModelLog.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"

void UMDViewModelWidgetClassExtension::Initialize(UUserWidget* UserWidget)
{
	Super::Initialize(UserWidget);

	UE_LOGFMT(LogMDViewModel, Verbose, "Initializing View Model Extension for Widget [{WidgetName}]", UserWidget->GetPathName());

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

void UMDViewModelWidgetClassExtension::GetThisAndAncestorAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutAssignments) const
{
	// TODO - Consider whether the parent assignments should be copied into this extension at compile time
	OutAssignments = Assignments;

#if !WITH_EDITOR // Caching parent assignments is only allowed outside of editor
	if (!bHasGatheredParentAssignments)
#endif
	{
		ParentAssignments.Reset();
		
		UClass* SuperClass = CastChecked<UClass>(GetOuter())->GetSuperClass();
		if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(SuperClass))
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
			if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
			{
				// Any collisions will be overridden by the eldest BP
				Extension->GetThisAndAncestorAssignments(ParentAssignments);
			}
		}

		bHasGatheredParentAssignments = true;
	}

	OutAssignments.Append(ParentAssignments);
}

void UMDViewModelWidgetClassExtension::SearchAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	GetThisAndAncestorAssignments(OutViewModelAssignments);

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

void UMDViewModelWidgetClassExtension::GetViewModelClasses(TSet<TSubclassOf<UMDViewModelBase>>& OutViewModelClasses) const
{
	for (const auto& Pair : Assignments)
	{
		OutViewModelClasses.Add(Pair.Key.ViewModelClass);
	}
}

void UMDViewModelWidgetClassExtension::QueueListenForChanges(UUserWidget* Widget, FMDVMOnViewModelSet::FDelegate&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
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
