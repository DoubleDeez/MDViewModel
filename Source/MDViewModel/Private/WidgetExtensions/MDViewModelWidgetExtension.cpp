#include "WidgetExtensions/MDViewModelWidgetExtension.h"

#include "MDViewModelModule.h"
#include "Blueprint/UserWidget.h"
#include "UObject/Package.h"
#include "ViewModel/MDViewModelBase.h"

void UMDViewModelWidgetExtension::Initialize()
{
	Super::Initialize();

	PopulateViewModels();
}

void UMDViewModelWidgetExtension::BeginDestroy()
{
	Super::BeginDestroy();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		CleanUpViewModels();
	}
}

UMDViewModelWidgetExtension* UMDViewModelWidgetExtension::GetOrCreate(UUserWidget* Widget)
{
	if (IsValid(Widget))
	{
		UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			return Extension;
		}

		return Widget->AddExtension<UMDViewModelWidgetExtension>();
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelWidgetExtension::SetViewModel(UMDViewModelBase* ViewModel, FName ViewModelName)
{
	if (IsValid(ViewModel))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModel->GetClass(), ViewModelName);
		if (ViewModelName != NAME_None)
		{
			const FMDViewModelInstanceKey Key = { ViewModelName, ViewModel->GetClass() };
			UMDViewModelBase* OldViewModel = ViewModels.FindRef(Key);
			ViewModels.FindOrAdd(Key) = ViewModel;

			BroadcastViewModelChanged(OldViewModel, ViewModel, ViewModel->GetClass(), ViewModelName);
		}
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelWidgetExtension::SetViewModelOfClass(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		UMDViewModelBase* ViewModel = NewObject<UMDViewModelBase>(GetTransientPackage(), ViewModelClass);
		if (IsValid(ViewModel))
		{
			ViewModel->InitializeViewModel();
			return SetViewModel(ViewModel, ViewModelName);
		}
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelWidgetExtension::GetViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName) const
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		if (ViewModelName != NAME_None)
		{
			const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
			return ViewModels.FindRef(Key);
		}
	}

	return nullptr;
}

void UMDViewModelWidgetExtension::OnProviderViewModelUpdated(TSubclassOf<UMDViewModelBase> ViewModelClass, FGameplayTag ProviderTag)
{
	UUserWidget* Widget = GetUserWidget();
	if (!IsValid(Widget))
	{
		return;
	}

	const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	ViewModelModule.SearchViewModelAssignments(Assignments, GetUserWidget()->GetClass(), ViewModelClass, ProviderTag);

	const TSharedPtr<FMDViewModelProviderBase> Provider = ViewModelModule.GetViewModelProvider(ProviderTag);
	if (!Provider.IsValid())
	{
		return;
	}

	for (const auto& Pair : Assignments)
	{
		Provider->AssignViewModel(*Widget, Pair.Key, Pair.Value);
	}
}

FDelegateHandle UMDViewModelWidgetExtension::ListenForChanges(FMDVMOnViewModelAssigned::FDelegate&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		return OnViewModelAssignedDelegates.FindOrAdd(Key).Add(MoveTemp(Delegate));
	}

	return {};
}

void UMDViewModelWidgetExtension::StopListeningForChanges(FDelegateHandle& Handle, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		if (FMDVMOnViewModelAssigned* Delegate = OnViewModelAssignedDelegates.Find(Key))
		{
			if (Delegate->Remove(Handle))
			{
				Handle.Reset();
			}
		}
	}
}

void UMDViewModelWidgetExtension::StopListeningForAllViewModelsChanges(const void* BoundObject)
{
	for (auto& Pair : OnViewModelAssignedDelegates)
	{
		Pair.Value.RemoveAll(BoundObject);
	}
}

void UMDViewModelWidgetExtension::BroadcastViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
{
	UUserWidget* OwnerWidget = GetUserWidget();
	if (IsValid(OldViewModel))
	{
		OldViewModel->OnUnsetFromWidget(OwnerWidget);
	}

	if (IsValid(NewViewModel))
	{
		NewViewModel->OnSetOnWidget(OwnerWidget);
	}

	const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
	if (const FMDVMOnViewModelAssigned* Delegate = OnViewModelAssignedDelegates.Find(Key))
	{
		Delegate->Broadcast(OldViewModel, NewViewModel);
	}
}

void UMDViewModelWidgetExtension::PopulateViewModels()
{
	UUserWidget* Widget = GetUserWidget();
	if (!IsValid(Widget))
	{
		return;
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	// TODO - Support DebugMode with editable debug view models
	if (Widget->IsPreviewTime())
	{
		return;
	}
#endif

	const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	ViewModelModule.GetViewModelAssignmentsForWidgetClass(Widget->GetClass(), Assignments);

	for (const auto& Pair : Assignments)
	{
		TSharedPtr<FMDViewModelProviderBase> Provider = ViewModelModule.GetViewModelProvider(Pair.Key.ProviderTag);
		if (ensureMsgf(Provider.IsValid(), TEXT("A View Model Provider with tag [%s] was not found"), *Pair.Key.ProviderTag.ToString()))
		{
			Provider->AssignViewModel(*Widget, Pair.Key, Pair.Value);
			Provider->OnViewModelUpdated.AddUObject(this, &UMDViewModelWidgetExtension::OnProviderViewModelUpdated, Pair.Key.ProviderTag);
		}
	}
}

void UMDViewModelWidgetExtension::CleanUpViewModels()
{
	// Broadcast out that we're null-ing out viewmodels
	{
		for (auto It = OnViewModelAssignedDelegates.CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = ViewModels.FindRef(It.Key());
			if (IsValid(ViewModel))
			{
				It.Value().Broadcast(ViewModel, nullptr);
			}
		}
	}

	// Viewmodels themselves
	{
		UUserWidget* OwnerWidget = GetUserWidget();
		for (auto It = ViewModels.CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = It.Value();
			if (IsValid(ViewModel))
			{
				ViewModel->OnUnsetFromWidget(OwnerWidget);
			}
		}

		ViewModels.Reset();
	}
}
