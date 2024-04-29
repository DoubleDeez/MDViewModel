#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

#include "Blueprint/UserWidget.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
#include "Logging/StructuredLog.h"
#endif
#include "Util/MDViewModelLog.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"

void UMDViewModelWidgetClassExtension::BeginDestroy()
{
	Super::BeginDestroy();

	// Work around issue where ~FInstancedStruct would crash when calling DestroyStruct on partially destroyed UserDefinedStructs
	Assignments.Empty();
}

void UMDViewModelWidgetClassExtension::Initialize(UUserWidget* UserWidget)
{
	Super::Initialize(UserWidget);

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
	UE_LOGFMT(LogMDViewModel, Verbose, "Initializing View Model Extension for Widget [{WidgetName}]", UserWidget->GetPathName());
#else
	UE_LOG(LogMDViewModel, Verbose, TEXT("Initializing View Model Extension for Widget [%s]"), *UserWidget->GetPathName());
#endif

	// ensure that we add the extension to the widget
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(UserWidget);
	ensure(Extension);

	if (TArray<QueuedListenerData>* QueuedListenerDatas = QueuedDelegates.Find(UserWidget))
	{
		for (QueuedListenerData& Data : *QueuedListenerDatas)
		{
			Extension->ListenForChanges(MoveTemp(Data.Delegate), Data.Assignment);
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

void UMDViewModelWidgetClassExtension::QueueListenForChanges(UUserWidget* Widget, FMDVMOnViewModelSet::FDelegate&& Delegate, const FMDViewModelAssignmentReference& Assignment)
{
	if (IsValid(Widget))
	{
		if (UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>())
		{
			Extension->ListenForChanges(MoveTemp(Delegate), Assignment);
		}
		else
		{
			QueuedDelegates.FindOrAdd(Widget).Emplace(QueuedListenerData{ MoveTemp(Delegate), Assignment });
		}
	}
}
